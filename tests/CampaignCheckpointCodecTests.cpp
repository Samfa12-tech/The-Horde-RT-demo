#include "gameplay/campaign/CampaignCheckpointCodec.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace cp=horde::gameplay::campaign;
namespace codec=cp::codec;
namespace dlg=horde::gameplay::dialogue;
namespace sim=horde::gameplay::simulation;
namespace tr=horde::gameplay::traversal;
namespace {
int checks=0,failures=0;
void Check(bool result,const char* message) {++checks;if(!result) {++failures;std::cerr<<message<<'\n';}}
std::uint32_t ReferenceCrc(std::span<const std::byte> bytes) {
    std::uint32_t crc=0xffffffffu;
    for(auto b:bytes) {
        crc^=std::to_integer<unsigned>(b);
        for(int bit=0;bit<8;++bit) crc=(crc&1)?(crc>>1)^0xedb88320u:crc>>1;
    }
    return ~crc;
}
void Put(std::vector<std::byte>& bytes,std::size_t offset,std::uint32_t value) {
    for(unsigned i=0;i<4;++i) bytes[offset+i]=std::byte((value>>(8*i))&255u);
}
void Rechecksum(std::vector<std::byte>& bytes) {Put(bytes,16,ReferenceCrc(std::span<const std::byte>(bytes).subspan(20)));}
std::vector<std::byte> FromHex(std::string_view hex) {
    const auto nibble=[](char c)->unsigned{return c<='9'?c-'0':c-'a'+10;};
    std::vector<std::byte> result;
    for(std::size_t i=0;i<hex.size();i+=2) result.push_back(std::byte((nibble(hex[i])<<4)|nibble(hex[i+1])));
    return result;
}
cp::Checkpoint Upper() {
    cp::Checkpoint c;c.id=cp::CheckpointId::SafeExit;c.zone=sim::WorldZoneId::TombExterior;
    c.position=tr::kExteriorLanding;c.firstAscentCompleted=true;
    c.dialogue.grateConsumed=c.dialogue.rescueConsumed=c.dialogue.reunionStarted=true;
    c.dialogue.waitingForRaise=true;return c;
}
cp::Checkpoint Endpoint() {
    auto c=Upper();c.contentRevision=2;c.id=cp::CheckpointId::ForestEndpoint;
    c.zone=sim::WorldZoneId::Lookout;
    const auto p=sim::kWorldRoutePoints.back();c.position={p.x,p.y,p.z};
    c.yaw=2.51f;c.pitch=-.15f;
    c.dialogue.waitingForRaise=false;c.dialogue.proofAccepted=true;
    c.dialogue.forestNightConsumed=c.dialogue.forestWaystoneConsumed=true;
    c.dialogue.forestClueConsumed=c.dialogue.forestVillageConsumed=true;
    return c;
}
std::vector<std::byte> Encode(const cp::Checkpoint& c) {
    std::vector<std::byte> bytes;codec::Error error=codec::Error::Corrupt;
    Check(codec::Encode(c,bytes,error) && error==codec::Error::None,"valid record encodes without residual error");
    return bytes;
}
void Reject(std::span<const std::byte> bytes,codec::Error expected) {
    auto output=Upper();output.schema=3;output.flask={true,3};output.vitality=2;output.yaw=.33f;output.swordStowed=true;
    const auto original=Encode(output);codec::Error error=codec::Error::None;
    Check(!codec::Decode(bytes,output,error) && error==expected,"invalid bytes reject with the expected typed error");
    Check(Encode(output)==original,"decode failure preserves every destination field");
}
void RoundTrip(const cp::Checkpoint& c) {
    const auto bytes=Encode(c);Check(bytes.size()==(c.schema==4?177u:(c.schema==3?168u:(c.schema==2?163u:162u))) && bytes.size()<=codec::kMaxFileBytes,"canonical format has a fixed bounded length");
    Check(bytes==Encode(c),"repeated encoding is deterministic");
    auto output=Upper();output.vitality=2;codec::Error error=codec::Error::Malformed;
    Check(codec::Decode(bytes,output,error) && error==codec::Error::None,"encoded checkpoint decodes successfully");
    Check(Encode(output)==bytes,"every logical field and float bit pattern round trips canonically");
}
}
int main() {
    for(auto seconds:{0ULL,1793314800ULL,253402300799ULL}) {
        auto dated=Upper();dated.schema=4;dated.flaskRecorded=true;dated.savedAtUnixSeconds=seconds;dated.halfHeart=true;dated.flask={true,2};RoundTrip(dated);
    }
    {auto bad=Upper();bad.schema=4;bad.savedAtUnixSeconds=253402300800ULL;
     std::vector<std::byte> out;codec::Error error{};Check(!codec::Encode(bad,out,error),"out-of-range timestamp rejected without writing");}
    static_assert(codec::kMaxFileBytes<=4096);
    // Independent literal format fixture pins magic, field order, LE floats,
    // zero-filled continuations, payload length and IEEE CRC32 0x01703382.
    const auto golden=FromHex(
        "48524443504b5400010000008e00000082337001010000000100000001000000"
        "00000000cdcc06c2333373bf333373c1db0fc9bf000000000300000003000000"
        "01010101010000006666e6bf333373c1db0fc9bf0ad7a3bd000080c0333373c1"
        "00000000000080c0333373c10000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "0000");
    const auto baseline=Encode(cp::Checkpoint{});
    Check(baseline==golden && golden.size()==162,"default checkpoint exactly matches golden bytes and checksum");
    Check(ReferenceCrc(std::span<const std::byte>(golden).subspan(20))==0x01703382u,"golden payload checksum is fixed independently");
    RoundTrip(cp::Checkpoint{});
    // Literal pre-flask schema2 pins the half-heart byte/CRC independently.
    const auto goldenHalf=FromHex(
        "48524443504b5400010000008f00000000eac598020000000100000001000000"
        "00000000cdcc06c2333373bf333373c1db0fc9bf000000000100000003000000"
        "01010101010000006666e6bf333373c1db0fc9bf0ad7a3bd000080c0333373c1"
        "00000000000080c0333373c10000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "000001");
    auto pinnedHalf=cp::Checkpoint{};pinnedHalf.schema=2;pinnedHalf.vitality=1;pinnedHalf.halfHeart=true;
    Check(Encode(pinnedHalf)==goldenHalf && goldenHalf.size()==163,
        "existing schema2 half-heart record keeps its exact golden bytes");
    for(const auto& oldBytes:{golden,goldenHalf}) {
        auto loaded=Endpoint();loaded.schema=3;loaded.flask={true,3};codec::Error error{};
        Check(codec::Decode(oldBytes,loaded,error) && loaded.flask==horde::gameplay::healing::FlaskResources{false,0} &&
              loaded.flask.Capacity()==2 && Encode(loaded)==oldBytes,
              "old schema1/2 decode empty base resources without rewriting health/progress bytes");
    }
    for(int health:{1,2,3}) for(bool half:{false,true}) for(bool upgraded:{false,true})
        for(unsigned charges=0;charges<=(upgraded?3u:2u);++charges)
            for(auto c:{cp::Checkpoint{},Upper(),Endpoint()}) {
                c.schema=3;c.vitality=health;c.halfHeart=half;c.flask={upgraded,charges};RoundTrip(c);
                const auto bytes=Encode(c);cp::Checkpoint loaded;codec::Error error{};
                Check(codec::Decode(bytes,loaded,error) && loaded.schema==3 &&
                      loaded.vitality==health && loaded.halfHeart==half && loaded.flask==c.flask,
                      "schema3 independently preserves exact resources and half health at every boundary");
            }
    {
        auto c=Endpoint();c.schema=3;c.vitality=1;c.halfHeart=true;c.flask={true,3};
        const auto bytes=Encode(c);
        // The historical142-byte prefix is followed by halfHeart, upgraded,
        // then a canonical LE32 charge count. No padding/timer is admitted.
        Check(bytes.size()==168 && bytes[162]==std::byte{1} && bytes[163]==std::byte{1} &&
              bytes[164]==std::byte{3} && bytes[165]==std::byte{0} &&
              bytes[166]==std::byte{0} && bytes[167]==std::byte{0},
              "schema3 extension has exact documented byte order and size");
        for(std::size_t offset:{162u,163u}) {
            auto bad=bytes;bad[offset]=std::byte{2};Rechecksum(bad);Reject(bad,codec::Error::Malformed);
        }
        for(auto invalidCount:{4u,0xffffffffu}) {
            auto bad=bytes;Put(bad,164,invalidCount);Rechecksum(bad);Reject(bad,codec::Error::InvalidCheckpoint);
        }
        auto base=bytes;base[163]=std::byte{0};Rechecksum(base);Reject(base,codec::Error::InvalidCheckpoint);
        for(std::uint32_t schema:{1u,2u}) {
            auto bad=bytes;Put(bad,20,schema);Rechecksum(bad);Reject(bad,codec::Error::Malformed);
        }
        auto future=bytes;Put(future,20,5);Rechecksum(future);Reject(future,codec::Error::UnsupportedSchema);
        for(std::uint32_t shortLength:{142u,143u,147u}) {
            auto bad=bytes;bad.resize(20+shortLength);Put(bad,12,shortLength);Rechecksum(bad);
            Reject(bad,codec::Error::Malformed);
        }
        auto extra=bytes;extra.push_back(std::byte{0});Put(extra,12,149);Rechecksum(extra);
        Reject(extra,codec::Error::Malformed);
        for(std::size_t length=0;length<bytes.size();++length)
            Reject(std::span<const std::byte>(bytes).first(length),codec::Error::Malformed);
    }
    for(std::uint32_t schema:{1u,2u}) for(bool upgraded:{false,true}) {
        auto invalid=cp::Checkpoint{};invalid.schema=schema;invalid.flask={upgraded,1};
        codec::Error error{};std::vector<std::byte> untouched{std::byte{66}};
        Check(!codec::Encode(invalid,untouched,error) && error==codec::Error::InvalidCheckpoint &&
              untouched==std::vector<std::byte>{std::byte{66}},
              "legacy encode cannot silently discard nonempty or upgraded resources");
    }

    for(int health:{1,2,3}) for(bool half:{false,true}) {
        for(auto c:{cp::Checkpoint{},Upper(),Endpoint()}) {
            c.schema=2;c.vitality=health;c.halfHeart=half;RoundTrip(c);
            const auto bytes=Encode(c);codec::Error error{};cp::Checkpoint loaded;
            Check(codec::Decode(bytes,loaded,error) && loaded.schema==2 &&
                  loaded.vitality==health && loaded.halfHeart==half,
                  "schema2 preserves every half-heart health and boundary exactly");
        }
    }
    {
        auto c=cp::Checkpoint{};c.schema=2;c.vitality=1;c.halfHeart=true;
        auto bytes=Encode(c);bytes.back()=std::byte{2};Rechecksum(bytes);
        Reject(bytes,codec::Error::Malformed);
        bytes=Encode(c);bytes.pop_back();Put(bytes,12,142);Rechecksum(bytes);
        Reject(bytes,codec::Error::Malformed);
        bytes=baseline;bytes.push_back(std::byte{0});Put(bytes,12,143);Rechecksum(bytes);
        Reject(bytes,codec::Error::Malformed);
        auto legacy=cp::Checkpoint{};legacy.halfHeart=true;codec::Error error{};
        std::vector<std::byte> sentinel{std::byte{55}};
        Check(!codec::Encode(legacy,sentinel,error) && error==codec::Error::InvalidCheckpoint &&
              sentinel==std::vector<std::byte>{std::byte{55}},
              "schema1 cannot silently discard half health and failure preserves output");
        cp::Checkpoint old;old.schema=2;old.halfHeart=true;
        Check(codec::Decode(golden,old,error) && !old.halfHeart && old.schema==1 &&
              Encode(old)==golden,"legacy decode clears stale fractions and keeps original bytes");
    }

    // The new logical content uses the same envelope/payload; historical lower
    // and summit bytes retain revision 1 and require no rewrite or migration.
    Check(cp::Checkpoint{}.contentRevision==1 && Upper().contentRevision==1,
        "old boundary content revisions remain unchanged");
    for(int health:{1,2,3}) for(bool sword:{false,true}) for(bool high:{false,true}) {
        auto endpoint=Endpoint();endpoint.vitality=health;
        endpoint.swordStowed=sword;endpoint.lanternHigh=high;RoundTrip(endpoint);
        auto returned=endpoint;returned.id=cp::CheckpointId::LowerClaimed;
        returned.zone=sim::WorldZoneId::Dungeon;returned.position=tr::kLowerLanding;
        returned.yaw=-1.57079632679f;returned.pitch=0;RoundTrip(returned);
        returned.id=cp::CheckpointId::SafeExit;returned.zone=sim::WorldZoneId::TombExterior;
        returned.position=tr::kExteriorLanding;returned.yaw=0;RoundTrip(returned);
    }
    {
        const auto encoded=Encode(Endpoint());
        Check(encoded.size()==golden.size() && encoded[8]==std::byte{1} && encoded[12]==std::byte{142},
            "forest content keeps format 1 and the exact bounded payload length");
        auto historical=Upper();codec::Error error{};
        Check(codec::Decode(golden,historical,error) && Encode(historical)==golden,
            "revision-1 golden still decodes and re-encodes byte-for-byte after endpoint support");
    }
    for(bool upper:{false,true}) for(int health:{1,2,3}) for(bool sword:{false,true}) for(bool high:{false,true}) {
        auto c=upper?Upper():cp::Checkpoint{};c.vitality=health;c.swordStowed=sword;c.lanternHigh=high;
        c.yaw=.43f;c.pitch=-.12f;c.torchDropX=-2.2f;c.torchDropZ=-15.4f;
        c.torchDropYaw=.8f;c.torchDropPitch=.1f;
        c.guards={cp::GuardPose{-4.2f,-15.1f,.2f},cp::GuardPose{-6.2f,-16.4f,-1.1f}};
        RoundTrip(c);
    }
    {
        auto c=cp::Checkpoint{};c.dialogue.rescueConsumed=true;c.dialogue.activeLine=dlg::Line::RescueFound;
        c.dialogue.continuations[0]=dlg::Line::RescueRope;c.dialogue.continuationCount=1;RoundTrip(c);
        c=Upper();c.dialogue.activeLine=dlg::Line::ReunionQuestion;RoundTrip(c);
        c.dialogue.activeLine=dlg::Line::ReunionHint;c.dialogue.reminderConsumed=true;RoundTrip(c);
        c=Upper();c.dialogue.waitingForRaise=false;c.dialogue.proofAccepted=true;
        c.dialogue.activeLine=dlg::Line::ReunionProof;c.dialogue.continuationCount=2;
        c.dialogue.continuations[0]=dlg::Line::ReunionFirstPiece;c.dialogue.continuations[1]=dlg::Line::ReunionDepart;RoundTrip(c);
        c.dialogue.activeLine=dlg::Line::None;c.dialogue.continuationCount=0;c.dialogue.continuations={};RoundTrip(c);
        c.dialogue.activeLine=dlg::Line::ForestNight;c.dialogue.forestNightConsumed=true;
        c.dialogue.continuationCount=2;c.dialogue.continuations[0]=dlg::Line::ForestWaystone;
        c.dialogue.continuations[1]=dlg::Line::ForestClue;RoundTrip(c);
        c.dialogue.activeLine=dlg::Line::ForestVillage;c.dialogue.continuationCount=0;c.dialogue.continuations={};
        c.dialogue.forestWaystoneConsumed=c.dialogue.forestClueConsumed=c.dialogue.forestVillageConsumed=true;RoundTrip(c);
        c=cp::Checkpoint{};c.dialogue.activeLine=dlg::Line::Grate;RoundTrip(c);
        c=cp::Checkpoint{};c.yaw=-0.0f;c.pitch=-0.0f;c.guards[0].yaw=-0.0f;RoundTrip(c);
        // Live guttering tracks a moving player beyond the trigger rectangle.
        c=cp::Checkpoint{};c.torchDropX=-3.1f;c.torchDropZ=-15.2f;RoundTrip(c);
        c=Upper();c.torchDropX=-3.1f;c.torchDropZ=-15.2f;RoundTrip(c);
        c=cp::Checkpoint{};c.torchDropX=-2.48f;c.torchDropZ=-16.4f;RoundTrip(c); // original schema-1 edge anchor
    }
    // Every possible truncation boundary fails without modifying output.
    for(std::size_t length=0;length<baseline.size();++length)
        Reject(std::span<const std::byte>(baseline).first(length),codec::Error::Malformed);
    for(std::size_t offset=0;offset<8;++offset) {
        auto bad=baseline;bad[offset]^=std::byte{1};Reject(bad,codec::Error::Malformed);
    }
    for(std::uint32_t version:{0u,2u,0xffffffffu}) {
        auto bad=baseline;Put(bad,8,version);Reject(bad,codec::Error::UnsupportedFormat);
    }
    for(std::uint32_t length:{0u,141u,143u,4076u}) {
        auto bad=baseline;Put(bad,12,length);Reject(bad,codec::Error::Malformed);
    }
    for(std::uint32_t length:{4077u,4096u,0xffffffffu}) {
        auto bad=baseline;Put(bad,12,length);Reject(bad,codec::Error::TooLarge);
    }
    auto extra=baseline;extra.push_back(std::byte{0});Reject(extra,codec::Error::Malformed);
    extra.resize(codec::kMaxFileBytes,std::byte{0});Reject(extra,codec::Error::Malformed);
    extra.push_back(std::byte{0});Reject(extra,codec::Error::TooLarge);
    for(std::size_t offset=16;offset<baseline.size();++offset) {
        auto bad=baseline;bad[offset]^=std::byte{1};Reject(bad,codec::Error::Corrupt);
    }
    // Corruption with a recomputed checksum still must pass the typed schema,
    // closed enum set, finite floats, strict bools and story invariants.
    for(std::uint32_t version:{0u,5u,0xffffffffu}) {
        auto bad=baseline;Put(bad,20,version);Rechecksum(bad);Reject(bad,codec::Error::UnsupportedSchema);
    }
    for(std::uint32_t version:{0u,3u,0xffffffffu}) {
        auto bad=baseline;
        Put(bad,24,version);Rechecksum(bad);Reject(bad,codec::Error::UnsupportedContent);
    }
    {
        auto bad=baseline;Put(bad,24,2);Rechecksum(bad);Reject(bad,codec::Error::InvalidCheckpoint);
        bad=Encode(Upper());Put(bad,24,2);Rechecksum(bad);Reject(bad,codec::Error::InvalidCheckpoint);
        bad=Encode(Endpoint());Put(bad,24,1);Rechecksum(bad);Reject(bad,codec::Error::InvalidCheckpoint);
        bad=Encode(Endpoint());Put(bad,28,4);Rechecksum(bad);Reject(bad,codec::Error::InvalidCheckpoint);
        bad=Encode(Endpoint());Put(bad,32,static_cast<std::uint32_t>(sim::WorldZoneId::TombExterior));
        Rechecksum(bad);Reject(bad,codec::Error::InvalidCheckpoint);
    }
    constexpr std::array<std::size_t,18> boolOffsets{44,45,46,47,48,49,50,51,132,133,134,135,136,137,138,139,140,141};
    for(auto offset:boolOffsets) for(unsigned value:{2u,255u}) {
        auto bad=baseline;bad[20+offset]=std::byte(value);Rechecksum(bad);Reject(bad,codec::Error::Malformed);
    }
    for(auto offset:{8u,12u,92u,100u,128u}) {
        auto bad=baseline;Put(bad,20+offset,99);Rechecksum(bad);Reject(bad,codec::Error::InvalidCheckpoint);
    }
    for(auto offset:{16u,20u,24u,28u,32u,52u,56u,60u,64u,68u,72u,76u,80u,84u,88u})
        for(std::uint32_t bits:{0x7fc00000u,0x7f800000u,0xff800000u}) {
            auto bad=baseline;Put(bad,20+offset,bits);Rechecksum(bad);Reject(bad,codec::Error::InvalidCheckpoint);
        }
    for(auto offset:{36u,40u}) {
        auto bad=baseline;Put(bad,20+offset,0);Rechecksum(bad);Reject(bad,codec::Error::InvalidCheckpoint);
    }
    for(auto offset:{44u,45u,46u,47u,48u}) {
        auto bad=baseline;bad[20+offset]=std::byte{0};Rechecksum(bad);Reject(bad,codec::Error::InvalidCheckpoint);
    }
    auto bad=baseline;Put(bad,116,9);Rechecksum(bad);Reject(bad,codec::Error::InvalidCheckpoint); // queue count
    auto queue=Upper();queue.dialogue.waitingForRaise=false;queue.dialogue.proofAccepted=true;
    queue.dialogue.activeLine=dlg::Line::ReunionProof;queue.dialogue.continuationCount=2;
    queue.dialogue.continuations[0]=dlg::Line::ReunionFirstPiece;queue.dialogue.continuations[1]=dlg::Line::ReunionDepart;
    bad=Encode(queue);Put(bad,120,static_cast<std::uint32_t>(dlg::Line::ReunionDepart));
    Put(bad,124,static_cast<std::uint32_t>(dlg::Line::ReunionFirstPiece));Rechecksum(bad);Reject(bad,codec::Error::InvalidCheckpoint);
    bad=baseline;bad[20+136]=std::byte{1};Rechecksum(bad);Reject(bad,codec::Error::InvalidCheckpoint); // proof without reunion
    const auto invalidEncode=[&](const cp::Checkpoint& c,codec::Error expected) {
        std::vector<std::byte> output{std::byte{17},std::byte{83},std::byte{255}};const auto original=output;
        codec::Error error=codec::Error::None;
        Check(!codec::Encode(c,output,error) && error==expected && output==original,
            "invalid encode leaves the caller's complete byte output untouched");
    };
    auto invalid=cp::Checkpoint{};invalid.schema=5;invalidEncode(invalid,codec::Error::UnsupportedSchema);
    invalid=cp::Checkpoint{};invalid.contentRevision=3;invalidEncode(invalid,codec::Error::UnsupportedContent);
    invalid=cp::Checkpoint{};invalid.contentRevision=2;invalidEncode(invalid,codec::Error::InvalidCheckpoint);
    invalid=Upper();invalid.contentRevision=2;invalidEncode(invalid,codec::Error::InvalidCheckpoint);
    invalid=Endpoint();invalid.contentRevision=1;invalidEncode(invalid,codec::Error::InvalidCheckpoint);
    for(unsigned missing=0;missing<7;++missing) {
        invalid=Endpoint();
        if(missing==0)invalid.firstAscentCompleted=false;
        if(missing==1)invalid.dialogue.proofAccepted=false;
        if(missing==2)invalid.dialogue.forestNightConsumed=false;
        if(missing==3)invalid.dialogue.forestWaystoneConsumed=false;
        if(missing==4)invalid.dialogue.forestClueConsumed=false;
        if(missing==5)invalid.dialogue.forestVillageConsumed=false;
        if(missing==6)invalid.dialogue.activeLine=dlg::Line::ForestVillage;
        invalidEncode(invalid,codec::Error::InvalidCheckpoint);
    }
    invalid=Endpoint();invalid.dialogue.continuationCount=1;invalid.dialogue.continuations[0]=dlg::Line::ForestVillage;
    invalidEncode(invalid,codec::Error::InvalidCheckpoint);
    invalid=cp::Checkpoint{};invalid.vitality=0;invalidEncode(invalid,codec::Error::InvalidCheckpoint);
    invalid=queue;invalid.dialogue.continuationCount=9;invalidEncode(invalid,codec::Error::InvalidCheckpoint);
    std::cout<<checks<<" campaign codec checks, "<<failures<<" failures\n";
    return failures?1:0;
}
