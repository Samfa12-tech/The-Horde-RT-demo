#include "gameplay/campaign/CampaignCheckpointCodec.h"
#include <array>
#include <bit>
#include <limits>
#include <utility>

namespace horde::gameplay::campaign::codec {
namespace {
static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559);
constexpr std::array<std::byte,8> kMagic{std::byte{'H'},std::byte{'R'},std::byte{'D'},
    std::byte{'C'},std::byte{'P'},std::byte{'K'},std::byte{'T'},std::byte{0}};
constexpr std::uint32_t kFormat=1,kLegacyPayloadBytes=142,kHalfHeartPayloadBytes=143,
    kFlaskPayloadBytes=148,kDatedPayloadBytes=157;
constexpr std::size_t kHeaderBytes=20;
static_assert(kHeaderBytes+kDatedPayloadBytes<=kMaxFileBytes);
constexpr std::uint32_t PayloadBytes(std::uint32_t schema) {
    return schema==4?kDatedPayloadBytes:(schema==3?kFlaskPayloadBytes:(schema==2?kHalfHeartPayloadBytes:kLegacyPayloadBytes));
}
std::uint32_t Crc32(std::span<const std::byte> bytes) {
    std::uint32_t crc=0xffffffffu;
    for(auto b:bytes) {
        crc^=std::to_integer<std::uint32_t>(b);
        for(unsigned i=0;i<8;++i) crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);
    }
    return crc^0xffffffffu;
}
void U32(std::vector<std::byte>& bytes,std::uint32_t value) {
    for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<std::byte>((value>>(8*i))&255u));
}
void Float(std::vector<std::byte>& bytes,float value) {U32(bytes,std::bit_cast<std::uint32_t>(value));}
void Bool(std::vector<std::byte>& bytes,bool value) {bytes.push_back(value?std::byte{1}:std::byte{0});}
class Reader {
public:
    explicit Reader(std::span<const std::byte> bytes):bytes_(bytes) {}
    std::uint32_t U32() {
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::to_integer<std::uint32_t>(bytes_[offset_++])<<(8*i);
        return value;
    }
    float Float() {return std::bit_cast<float>(U32());}
    bool Bool() {
        const auto value=std::to_integer<unsigned>(bytes_[offset_++]);
        if(value>1) valid_=false;
        return value==1;
    }
    bool Valid() const {return valid_;}
private:
    std::span<const std::byte> bytes_;std::size_t offset_=0;bool valid_=true;
};
Error VersionError(const Checkpoint& c) {
    if(c.schema!=1 && c.schema!=2 && c.schema!=3 && c.schema!=4) return Error::UnsupportedSchema;
    if(c.contentRevision!=1 && c.contentRevision!=2) return Error::UnsupportedContent;
    return Error::None;
}
}
bool Encode(const Checkpoint& c,std::vector<std::byte>& out,Error& error) {
    error=VersionError(c);
    if(error!=Error::None) return false;
    if(!Validate(c)) {error=Error::InvalidCheckpoint;return false;}
    const auto payloadBytes=PayloadBytes(c.schema);
    std::vector<std::byte> payload;payload.reserve(payloadBytes);
    U32(payload,c.schema);U32(payload,c.contentRevision);
    U32(payload,static_cast<std::uint32_t>(c.id));U32(payload,static_cast<std::uint32_t>(c.zone));
    Float(payload,c.position.x);Float(payload,c.position.y);Float(payload,c.position.z);
    Float(payload,c.yaw);Float(payload,c.pitch);
    U32(payload,std::bit_cast<std::uint32_t>(c.vitality));U32(payload,c.defeatedGuards);
    Bool(payload,c.keeperDefeated);Bool(payload,c.lanternClaimed);Bool(payload,c.lidOpen);
    Bool(payload,c.ropeReady);Bool(payload,c.night);Bool(payload,c.firstAscentCompleted);
    Bool(payload,c.swordStowed);Bool(payload,c.lanternHigh);
    Float(payload,c.torchDropX);Float(payload,c.torchDropZ);Float(payload,c.torchDropYaw);Float(payload,c.torchDropPitch);
    for(const auto& g:c.guards) {Float(payload,g.x);Float(payload,g.z);Float(payload,g.yaw);}
    const auto& p=c.dialogue;
    U32(payload,static_cast<std::uint32_t>(p.activeLine));
    U32(payload,static_cast<std::uint32_t>(p.continuationCount));
    for(auto line:p.continuations) U32(payload,static_cast<std::uint32_t>(line));
    Bool(payload,p.grateConsumed);Bool(payload,p.rescueConsumed);Bool(payload,p.reunionStarted);
    Bool(payload,p.waitingForRaise);Bool(payload,p.proofAccepted);Bool(payload,p.reminderConsumed);
    Bool(payload,p.forestNightConsumed);Bool(payload,p.forestWaystoneConsumed);
    Bool(payload,p.forestClueConsumed);Bool(payload,p.forestVillageConsumed);
    if(c.schema>=2) Bool(payload,c.halfHeart);
    if(c.schema>=3) {Bool(payload,c.flask.upgraded);U32(payload,c.flask.charges);}
    if(c.schema>=4) {U32(payload,static_cast<std::uint32_t>(c.savedAtUnixSeconds));U32(payload,static_cast<std::uint32_t>(c.savedAtUnixSeconds>>32));Bool(payload,c.flaskRecorded);}
    std::vector<std::byte> encoded;encoded.reserve(kHeaderBytes+payloadBytes);
    encoded.insert(encoded.end(),kMagic.begin(),kMagic.end());
    U32(encoded,kFormat);U32(encoded,payloadBytes);U32(encoded,Crc32(payload));
    encoded.insert(encoded.end(),payload.begin(),payload.end());
    out=std::move(encoded);error=Error::None;return true;
}
bool Decode(std::span<const std::byte> bytes,Checkpoint& out,Error& error) {
    if(bytes.size()>kMaxFileBytes) {error=Error::TooLarge;return false;}
    if(bytes.size()<kHeaderBytes) {error=Error::Malformed;return false;}
    for(std::size_t i=0;i<kMagic.size();++i)
        if(bytes[i]!=kMagic[i]) {error=Error::Malformed;return false;}
    Reader header(bytes.subspan(8,kHeaderBytes-8));
    if(header.U32()!=kFormat) {error=Error::UnsupportedFormat;return false;}
    const auto length=header.U32(),crc=header.U32();
    if(length>kMaxFileBytes-kHeaderBytes) {error=Error::TooLarge;return false;}
    if((length!=kLegacyPayloadBytes && length!=kHalfHeartPayloadBytes &&
        length!=kFlaskPayloadBytes && length!=kDatedPayloadBytes) ||
       bytes.size()!=kHeaderBytes+length) {error=Error::Malformed;return false;}
    const auto payload=bytes.subspan(kHeaderBytes);
    if(Crc32(payload)!=crc) {error=Error::Corrupt;return false;}
    Reader r(payload);Checkpoint c;
    c.schema=r.U32();c.contentRevision=r.U32();
    error=VersionError(c);if(error!=Error::None) return false;
    if(length!=PayloadBytes(c.schema)) {error=Error::Malformed;return false;}
    c.id=static_cast<CheckpointId>(r.U32());c.zone=static_cast<simulation::WorldZoneId>(r.U32());
    c.position={r.Float(),r.Float(),r.Float()};c.yaw=r.Float();c.pitch=r.Float();
    c.vitality=std::bit_cast<std::int32_t>(r.U32());c.defeatedGuards=r.U32();
    c.keeperDefeated=r.Bool();c.lanternClaimed=r.Bool();c.lidOpen=r.Bool();
    c.ropeReady=r.Bool();c.night=r.Bool();c.firstAscentCompleted=r.Bool();
    c.swordStowed=r.Bool();c.lanternHigh=r.Bool();
    c.torchDropX=r.Float();c.torchDropZ=r.Float();c.torchDropYaw=r.Float();c.torchDropPitch=r.Float();
    for(auto& g:c.guards) {g.x=r.Float();g.z=r.Float();g.yaw=r.Float();}
    auto& p=c.dialogue;p.activeLine=static_cast<dialogue::Line>(r.U32());p.continuationCount=r.U32();
    for(auto& line:p.continuations) line=static_cast<dialogue::Line>(r.U32());
    p.grateConsumed=r.Bool();p.rescueConsumed=r.Bool();p.reunionStarted=r.Bool();
    p.waitingForRaise=r.Bool();p.proofAccepted=r.Bool();p.reminderConsumed=r.Bool();
    p.forestNightConsumed=r.Bool();p.forestWaystoneConsumed=r.Bool();
    p.forestClueConsumed=r.Bool();p.forestVillageConsumed=r.Bool();
    if(c.schema>=2) c.halfHeart=r.Bool();
    if(c.schema>=3) {c.flask.upgraded=r.Bool();c.flask.charges=r.U32();}
    if(c.schema>=4) {const auto low=r.U32();c.savedAtUnixSeconds=low|(static_cast<std::uint64_t>(r.U32())<<32);c.flaskRecorded=r.Bool();}
    if(!r.Valid()) {error=Error::Malformed;return false;}
    if(!Validate(c)) {error=Error::InvalidCheckpoint;return false;}
    out=std::move(c);error=Error::None;return true;
}
} // namespace horde::gameplay::campaign::codec
