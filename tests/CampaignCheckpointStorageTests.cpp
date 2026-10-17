#include "gameplay/campaign/CampaignCheckpointStorage.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <thread>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace cp=horde::gameplay::campaign;
namespace codec=cp::codec;
namespace store=cp::storage;
namespace fs=std::filesystem;
namespace {
int checks=0,failures=0;
void Check(bool result,const char* message) {++checks;if(!result){++failures;std::cerr<<message<<'\n';}}
std::vector<std::byte> Encode(const cp::Checkpoint& c) {
    std::vector<std::byte> bytes;codec::Error error{};
    Check(codec::Encode(c,bytes,error),"fixture logical checkpoint encodes");return bytes;
}
std::vector<std::byte> Bytes(const fs::path& path) {
    std::ifstream input(path,std::ios::binary);
    const std::vector<char> bytes{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
    std::vector<std::byte> out;for(unsigned char b:bytes) out.push_back(std::byte{b});return out;
}
void Raw(const fs::path& path,const std::vector<std::byte>& bytes) {
    std::ofstream output(path,std::ios::binary|std::ios::trunc);
    output.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    output.close();Check(bool(output),"disposable malformed fixture writes completely");
}
struct TempDirectory {
    fs::path path;
    TempDirectory() {
        const auto suffix=std::chrono::steady_clock::now().time_since_epoch().count();
        path=fs::temp_directory_path()/("horde-campaign-storage-test-"+std::to_string(suffix));
        if(!fs::create_directory(path)) throw std::runtime_error("exclusive disposable directory creation failed");
        std::ofstream(path/"test-owner.txt")<<"disposable campaign storage fixture";
    }
    ~TempDirectory() {
        // Only our exclusively created direct temp child may be recursively removed.
        std::error_code error;
        if(path.filename().string().starts_with("horde-campaign-storage-test-") &&
           fs::weakly_canonical(path.parent_path(),error)==fs::weakly_canonical(fs::temp_directory_path(),error) &&
           !error && fs::is_regular_file(path/"test-owner.txt")) fs::remove_all(path,error);
    }
};
bool Same(const store::StoredCheckpoint& a,const store::StoredCheckpoint& b) {
    return a.target==b.target && a.encoded==b.encoded && Encode(a.checkpoint)==Encode(b.checkpoint);
}
store::StoredCheckpoint Read(const fs::path& root,std::string_view name) {
    store::StoredCheckpoint saved;
    Check(store::Read(root,name,saved).status==store::Status::Read,"fresh adapter reads a complete typed record");
    return saved;
}
struct Fault:store::FaultInjector {
    store::Operation operation=store::Operation::Write;
    std::size_t after=0,limit=std::numeric_limits<std::size_t>::max();
    bool active=true,crash=false;
    bool Fail(store::Operation op,std::size_t done) override {
        if(!active || op!=operation || done<after) return false;
        if(crash) std::_Exit(73);return true;
    }
    std::size_t WriteLimit(std::size_t requested) override {return std::min(requested,limit);}
};
std::vector<fs::path> Pending(const fs::path& root) {
    std::vector<fs::path> out;
    for(const auto& entry:fs::directory_iterator(root)) if(entry.path().filename().string().find(".pending-")!=std::string::npos)
        out.push_back(entry.path());
    std::sort(out.begin(),out.end());return out;
}
bool CrashChild(const fs::path& exe,const fs::path& root,int stage) {
#ifdef _WIN32
    std::wstring command=L"\""+exe.wstring()+L"\" --owned-crash-child \""+root.wstring()+L"\" "+std::to_wstring(stage);
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
    if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process)) return false;
    const auto wait=WaitForSingleObject(process.hProcess,15000);DWORD exit=0;
    if(wait!=WAIT_OBJECT_0) {
        std::cerr<<"Owned child pending: pid="<<process.dwProcessId<<" disposable root retained="<<root.string()<<'\n';
        // Preserve evidence and stop all root mutation/cleanup if our own child
        // has an uncertain outcome. Do not kill or touch any owner process.
        std::_Exit(79);
    }
    const bool okay=wait==WAIT_OBJECT_0 && GetExitCodeProcess(process.hProcess,&exit) && exit==73;
    CloseHandle(process.hThread);CloseHandle(process.hProcess);return okay;
#else
    const auto process=fork();
    if(process<0) return false;
    if(process==0) {const auto number=std::to_string(stage);execl(exe.c_str(),exe.c_str(),"--owned-crash-child",root.c_str(),number.c_str(),nullptr);std::_Exit(74);}
    int status=0;return waitpid(process,&status,0)==process && WIFEXITED(status) && WEXITSTATUS(status)==73;
#endif
}
int Child(const fs::path& root,int stage) {
    std::error_code error;
    if(!root.filename().string().starts_with("horde-campaign-storage-test-") ||
       fs::weakly_canonical(root.parent_path(),error)!=fs::weakly_canonical(fs::temp_directory_path(),error) ||
       error || !fs::is_regular_file(root/"test-owner.txt") || stage<0 || stage>3) return 75;
    store::StoredCheckpoint old;
    if(store::Read(root,"crash.sav",old).status!=store::Status::Read) return 76;
    auto candidate=old.checkpoint;candidate.vitality=1;
    Fault fault;fault.crash=true;fault.limit=7;
    fault.operation=std::array{store::Operation::Write,store::Operation::FlushFile,store::Operation::Publish,store::Operation::AfterPublish}[stage];
    fault.after=stage==0?7:0;
    store::Write(root,"crash.sav",candidate,old,&fault);return 77;
}
void Put(std::vector<std::byte>& bytes,std::size_t offset,std::uint32_t value) {
    for(unsigned i=0;i<4;++i) bytes[offset+i]=std::byte((value>>(8*i))&255);
}
void Rechecksum(std::vector<std::byte>& bytes) {
    std::uint32_t crc=0xffffffffu;
    for(std::size_t i=20;i<bytes.size();++i) {crc^=std::to_integer<unsigned>(bytes[i]);for(int bit=0;bit<8;++bit)crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);}
    Put(bytes,16,crc^0xffffffffu);
}
}
int main(int argc,char** argv) {
    if(argc==4 && std::string_view(argv[1])=="--owned-crash-child") return Child(fs::path(argv[2]),std::atoi(argv[3]));
    if(argc!=1) return 78;
    TempDirectory temporary;const auto root=temporary.path;const auto exe=fs::absolute(argv[0]);
    cp::Checkpoint initial,changed;changed.vitality=2;changed.swordStowed=true;
    const auto initialBytes=Encode(initial),changedBytes=Encode(changed);
    store::StoredCheckpoint untouched;untouched.target="sentinel";untouched.encoded={std::byte{90}};
    const auto sentinel=untouched;
    Check(store::Read(root,"missing.sav",untouched).status==store::Status::NotFound && Same(untouched,sentinel),
        "missing read leaves every output field untouched");
    for(const auto* name:{"",".","..","../outside","a/b","a\\b","C:save","NUL.sav","CON","COM1.dat","slot.","a b"}) {
        Check(store::Write(root,name,initial).status==store::Status::InvalidPath,"unsafe or device basename rejects before writing");
        Check(store::Read(root,name,untouched).status==store::Status::InvalidPath && Same(untouched,sentinel),"invalid read path leaves output untouched");
    }
    Check(store::Write(root/"missing-directory","slot.sav",initial).status==store::Status::InvalidPath && !fs::exists(root/"missing-directory"),
        "adapter does not create missing directories or choose a save location");
    fs::create_directory(root/".blocked-lock.sav.lock");
    Check(store::Write(root,"blocked-lock.sav",initial).status==store::Status::OpenFailed &&
        !fs::exists(root/"blocked-lock.sav") && fs::is_directory(root/".blocked-lock.sav.lock"),
        "inaccessible lock is an open failure rather than retryable contention, and remains untouched");
    const auto first=store::Write(root,"slot-a.sav",initial);
    Check(first.status==store::Status::Committed,"create-only storage publishes complete checkpoint");
    if(first.status!=store::Status::Committed) {
        std::cerr<<"initial publication status="<<static_cast<int>(first.status)<<" native="<<first.systemError<<'\n';return 1;
    }
    auto a=Read(root,"slot-a.sav");
    Check(a.encoded==initialBytes,"file bytes exactly match canonical codec");
    Check(store::Write(root,"slot-a.sav",changed).status==store::Status::Conflict && Bytes(root/"slot-a.sav")==initialBytes,
        "create-only mode never overwrites an existing valid record");
    const auto noChange=store::Write(root,"slot-a.sav",initial,a);
    Check(noChange.status==store::Status::AlreadyCurrent && noChange.publication==store::Publication::Unchanged &&
        noChange.durability==store::Durability::None,"identical expected candidate is a no-op without a new persistence claim");
    const auto committed=store::Write(root,"slot-a.sav",changed,a);
    Check(committed.status==store::Status::Committed && committed.publication==store::Publication::Published,"exact expected valid record can update");
#ifdef _WIN32
    Check(committed.durability==store::Durability::FileFlushed,"Windows confirms file flush without inventing directory/power-loss durability");
#else
    Check(committed.durability==store::Durability::FileAndDirectoryFlushed,"POSIX commit flushes file and containing directory");
#endif
    Check(Read(root,"slot-a.sav").encoded==changedBytes,"updated checkpoint round trips through a fresh read");
    Check(store::Write(root,"slot-a.sav",initial,a).status==store::Status::Conflict && Bytes(root/"slot-a.sav")==changedBytes,
        "stale expected content cannot roll back a newer record");
    for(const auto* name:{"slot-b.sav","slot-c.sav"}) {
        Check(store::Write(root,name,initial).status==store::Status::Committed,"independent disposable slot fixture creates");
        Check(store::Write(root,name,changed,a).status==store::Status::InvalidExpectation && Bytes(root/name)==initialBytes,
            "expected receipt from another target cannot overwrite this record");
    }
    auto invalid=initial;invalid.vitality=0;
    Check(store::Write(root,"never-created.sav",invalid).status==store::Status::InvalidCandidate && !fs::exists(root/"never-created.sav"),
        "invalid candidate does not create a target");
    Check(store::Write(root,"slot-a.sav",invalid,Read(root,"slot-a.sav")).status==store::Status::InvalidCandidate &&
        Bytes(root/"slot-a.sav")==changedBytes,"invalid candidate cannot overwrite a valid file");
    auto badExpected=a;badExpected.encoded[0]^=std::byte{1};
    Check(store::Write(root,"slot-a.sav",initial,badExpected).status==store::Status::InvalidExpectation,
        "malformed expected bytes are not overwrite permission");
    // Every partial file and all future version classes remain byte-identical.
    for(std::size_t length=0;length<initialBytes.size();++length) {
        const std::vector<std::byte> partial(initialBytes.begin(),initialBytes.begin()+length);Raw(root/"broken.sav",partial);
        const auto result=store::Read(root,"broken.sav",untouched);
        Check(result.status==store::Status::InvalidExisting && Same(untouched,sentinel),"truncated read leaves logical output untouched");
        Check(store::Write(root,"broken.sav",changed).status==store::Status::InvalidExisting && Bytes(root/"broken.sav")==partial,
            "partial existing save is never migrated, overwritten or repaired automatically");
    }
    for(std::size_t offset:{8u,20u,24u}) {
        auto future=initialBytes;Put(future,offset,2);if(offset>=20)Rechecksum(future);Raw(root/"future.sav",future);
        const auto result=store::Read(root,"future.sav",untouched);
        Check(result.status==store::Status::InvalidExisting && result.codecError!=codec::Error::None && Same(untouched,sentinel),
            "unsupported format/schema/content returns typed failure without output mutation");
        Check(store::Write(root,"future.sav",changed).status==store::Status::InvalidExisting && Bytes(root/"future.sav")==future,
            "unsupported existing versions are preserved without migration");
    }
    auto corrupt=initialBytes;corrupt.back()^=std::byte{1};Raw(root/"corrupt.sav",corrupt);
    Check(store::Write(root,"corrupt.sav",changed).status==store::Status::InvalidExisting && Bytes(root/"corrupt.sav")==corrupt,
        "corrupt checksum target stays untouched");
    Raw(root/"oversize.sav",std::vector<std::byte>(codec::kMaxFileBytes+1));
    Check(store::Read(root,"oversize.sav",untouched).status==store::Status::InvalidExisting && Same(untouched,sentinel),"oversized input is bounded before allocation/decode");
    fs::create_directory(root/"directory.sav");
    Check(store::Write(root,"directory.sav",changed).status!=store::Status::Committed && fs::is_directory(root/"directory.sav"),"nonregular target is never replaced");
    std::error_code symlinkError;fs::create_symlink(root/"slot-c.sav",root/"alias.sav",symlinkError);
#ifndef _WIN32
    Check(!symlinkError,"POSIX disposable symlink fixture creates");
#endif
    if(!symlinkError) {
        Check(store::Write(root,"alias.sav",changed).status!=store::Status::Committed && Bytes(root/"slot-c.sav")==initialBytes,
            "symlink/reparse target cannot modify its referent");
    } else std::cout<<"Windows symlink fixture unavailable; nonregular rejection tested; reparse runtime fixture NOT_RUN\n";
    const auto expected=Read(root,"slot-b.sav");
    for(const auto operation:{store::Operation::Read,store::Operation::OpenTemporary,store::Operation::Write,
        store::Operation::FlushFile,store::Operation::CloseFile,store::Operation::Publish}) {
        Fault fault;fault.operation=operation;
        const auto result=store::Write(root,"slot-b.sav",changed,expected,&fault);
        Check(result.status!=store::Status::Committed && result.publication==store::Publication::Unchanged &&
            Bytes(root/"slot-b.sav")==initialBytes,"injected prepublication IO failure preserves exact previous bytes");
        Check(Pending(root).empty(),"ordinary failed attempt cleans only its own temporary file");
    }
    for(std::size_t prefix=1;prefix<initialBytes.size();++prefix) {
        Fault fault;fault.limit=1;fault.after=prefix;
        const auto result=store::Write(root,"slot-b.sav",changed,expected,&fault);
        Check(result.status==store::Status::WriteFailed && result.publication==store::Publication::Unchanged &&
            Bytes(root/"slot-b.sav")==initialBytes && Pending(root).empty(),"every partial-write boundary preserves old record");
    }
    Fault shortWrites;shortWrites.active=false;shortWrites.limit=1;
    Check(store::Write(root,"slot-b.sav",changed,expected,&shortWrites).status==store::Status::Committed &&
        Read(root,"slot-b.sav").encoded==changedBytes,"short successful writes are retried to complete canonical bytes");
    auto b=Read(root,"slot-b.sav");Fault after;after.operation=store::Operation::AfterPublish;
    const auto uncertain=store::Write(root,"slot-b.sav",initial,b,&after);
    Check(uncertain.status==store::Status::PublishedDurabilityUnknown && uncertain.publication==store::Publication::Published &&
        uncertain.durability==store::Durability::None && Read(root,"slot-b.sav").encoded==initialBytes,
        "postpublication failure reports new visibility and uncertain durability instead of false old-file preservation");
    struct Reentry:store::FaultInjector {
        fs::path root;store::StoredCheckpoint expected;cp::Checkpoint candidate;bool invoked=false;
        bool Fail(store::Operation operation,std::size_t) override {
            if(operation==store::Operation::Publish && !invoked) {
                invoked=true;const auto nested=store::Write(root,"slot-c.sav",candidate,expected);
                Check(nested.status==store::Status::Busy && nested.publication==store::Publication::Unchanged,
                    "a second cooperating writer cannot enter a pending update");
            }
            return false;
        }
    } reentry;
    reentry.root=root;reentry.expected=Read(root,"slot-c.sav");reentry.candidate=changed;
    Check(store::Write(root,"slot-c.sav",changed,reentry.expected,&reentry).status==store::Status::Committed && reentry.invoked,
        "nonblocking lock releases after complete writer transaction");
#ifdef _WIN32
    auto current=Read(root,"slot-c.sav");
    const auto denyDelete=CreateFileW((root/"slot-c.sav").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    Check(denyDelete!=INVALID_HANDLE_VALUE,"real Windows sharing-conflict fixture opens");
    const auto denied=store::Write(root,"slot-c.sav",initial,current);
    Check(denied.status==store::Status::PublishFailed && denied.publication==store::Publication::Unchanged &&
        Bytes(root/"slot-c.sav")==changedBytes,"real OS replacement denial preserves previous bytes");
    if(denyDelete!=INVALID_HANDLE_VALUE)CloseHandle(denyDelete);
    Check(SetFileAttributesW((root/"slot-c.sav").c_str(),FILE_ATTRIBUTE_READONLY)!=FALSE,"owned read-only failure fixture sets its attribute");
    const auto readOnly=store::Write(root,"slot-c.sav",initial,current);
    Check(readOnly.status==store::Status::PublishFailed && readOnly.publication==store::Publication::Unchanged &&
        Bytes(root/"slot-c.sav")==changedBytes,"real OS read-only replacement denial preserves previous bytes");
    Check(SetFileAttributesW((root/"slot-c.sav").c_str(),FILE_ATTRIBUTE_NORMAL)!=FALSE,"owned read-only fixture restores its attribute");
#endif
    // Each child exits itself abruptly at a declared seam; no owner process is killed.
    for(int stage=0;stage<4;++stage) {
        const auto path=root/"crash.sav";Raw(path,initialBytes);
        Check(CrashChild(exe,root,stage),"owned child terminates at the requested storage boundary");
        Check(Bytes(path)==(stage==3?Encode(cp::Checkpoint{.vitality=1}):initialBytes),
            "process interruption leaves old target before publication and a complete new target afterward");
        Check(store::Read(root,"crash.sav",untouched).status==store::Status::Read,"fresh reader ignores abandoned temporary writes");
    }
    const auto abandoned=Pending(root);std::vector<std::vector<std::byte>> abandonedBytes;
    for(const auto& path:abandoned)abandonedBytes.push_back(Bytes(path));
    Check(abandoned.size()>=3,"abrupt prepublication exits retain their own disposable orphans");
    auto crash=Read(root,"crash.sav");
    Check(store::Write(root,"crash.sav",changed,crash).status==store::Status::Committed,"later normal update succeeds without orphan sweeping");
    for(std::size_t i=0;i<abandoned.size();++i)Check(Bytes(abandoned[i])==abandonedBytes[i],"later updates never delete or modify abandoned files");
    Check(store::Write(root,"concurrent.sav",initial).status==store::Status::Committed,"atomic reader fixture initializes");
    std::atomic<bool> finished=false;std::atomic<unsigned> reads=0,readErrors=0;
    std::thread reader([&]{while(!finished.load()) {store::StoredCheckpoint observed;const auto result=store::Read(root,"concurrent.sav",observed);
        if(result.status!=store::Status::Read || (observed.encoded!=initialBytes && observed.encoded!=changedBytes))++readErrors;
        ++reads;std::this_thread::yield();}});
    for(int i=0;i<30;++i) {auto old=Read(root,"concurrent.sav");
        const auto result=store::Write(root,"concurrent.sav",old.encoded==initialBytes?changed:initial,old);
        if(result.status!=store::Status::Committed) std::cerr<<"concurrent writer status="<<static_cast<int>(result.status)<<" native="<<result.systemError<<'\n';
        Check(result.status==store::Status::Committed,"bounded alternating writer publishes under an active reader");}
    finished=true;reader.join();Check(reads>0 && readErrors==0,"concurrent readers see complete old or new records, never partial bytes");
    std::cout<<checks<<" campaign storage checks, "<<failures<<" failures; concurrent reads="<<reads.load()<<'\n';
    return failures?1:0;
}
