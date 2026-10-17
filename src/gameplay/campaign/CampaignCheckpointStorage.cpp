#include "gameplay/campaign/CampaignCheckpointStorage.h"
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <limits>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace horde::gameplay::campaign::storage {
namespace {
#ifdef _WIN32
using Handle = HANDLE;
const Handle Invalid = INVALID_HANDLE_VALUE;
std::uint32_t Error() { return GetLastError(); }
void Close(Handle h) { if(h!=Invalid) CloseHandle(h); }
#else
using Handle = int;
constexpr Handle Invalid = -1;
std::uint32_t Error() { return static_cast<std::uint32_t>(errno); }
void Close(Handle h) { if(h!=Invalid) close(h); }
#endif
struct Owned {
    Handle value = Invalid;
    explicit Owned(Handle h=Invalid):value(h){}
    ~Owned(){Close(value);}
    Owned(const Owned&)=delete;
    Owned& operator=(const Owned&)=delete;
};
Result Failure(Status status, std::uint32_t error=0, codec::Error codecError=codec::Error::None) {
    return {status,Publication::Unchanged,Durability::None,codecError,error};
}
bool Fail(FaultInjector* faults,Operation op,std::size_t bytes=0) { return faults && faults->Fail(op,bytes); }
bool ValidName(std::string_view name) {
    if(name.empty() || name.size()>64 || name.front()=='.' || name.back()=='.') return false;
    std::string base;
    for(char c:name) {
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='-'||c=='_')) return false;
        if(base.find('.')==std::string::npos) base+=c>='a'&&c<='z'?char(c-'a'+'A'):c;
    }
    base=base.substr(0,base.find('.'));
    return base!="CON" && base!="PRN" && base!="AUX" && base!="NUL" &&
        !(base.size()==4 && (base.substr(0,3)=="COM"||base.substr(0,3)=="LPT") && base[3]>='0' && base[3]<='9');
}
bool Target(const std::filesystem::path& directory,std::string_view name,std::filesystem::path& target) {
    if(!ValidName(name) || directory.empty()) return false;
    std::error_code ec;
    const auto absolute=std::filesystem::absolute(directory,ec);
    if(ec) return false;
    target=absolute.lexically_normal()/std::string(name);
    return true;
}
Handle OpenDirectory(const std::filesystem::path& directory) {
#ifdef _WIN32
    const auto h=CreateFileW(directory.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,
        nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(h==Invalid) return Invalid;
    BY_HANDLE_FILE_INFORMATION info{};
    if(!GetFileInformationByHandle(h,&info) || !(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
       (info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)) { Close(h);SetLastError(ERROR_INVALID_PARAMETER);return Invalid; }
    return h;
#else
    return open(directory.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
#endif
}
Handle OpenRead(Handle directory,const std::filesystem::path& target) {
#ifdef _WIN32
    (void)directory;
    return CreateFileW(target.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
#else
    return openat(directory,target.filename().c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
#endif
}
bool Missing(std::uint32_t error) {
#ifdef _WIN32
    return error==ERROR_FILE_NOT_FOUND;
#else
    return error==ENOENT;
#endif
}
bool LockBusy(std::uint32_t error) {
#ifdef _WIN32
    return error==ERROR_LOCK_VIOLATION || error==ERROR_SHARING_VIOLATION;
#else
    return error==EWOULDBLOCK || error==EAGAIN;
#endif
}
bool RegularSize(Handle h,std::size_t& size) {
#ifdef _WIN32
    BY_HANDLE_FILE_INFORMATION info{};
    if(!GetFileInformationByHandle(h,&info) || GetFileType(h)!=FILE_TYPE_DISK ||
       (info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))) return false;
    const auto length=(std::uint64_t(info.nFileSizeHigh)<<32)|info.nFileSizeLow;
#else
    struct stat info{};
    if(fstat(h,&info)!=0 || !S_ISREG(info.st_mode) || info.st_size<0) return false;
    const auto length=static_cast<std::uint64_t>(info.st_size);
#endif
    if(length>codec::kMaxFileBytes) return false;
    size=static_cast<std::size_t>(length);return true;
}
Result ReadAt(Handle directory,const std::filesystem::path& target,StoredCheckpoint& output,FaultInjector* faults) {
    if(Fail(faults,Operation::Read)) return Failure(Status::ReadFailed);
    Owned file(OpenRead(directory,target));
    if(file.value==Invalid) {const auto error=Error();return Failure(Missing(error)?Status::NotFound:Status::ReadFailed,error);}
    std::size_t length=0;
    if(!RegularSize(file.value,length)) return Failure(Status::InvalidExisting);
    StoredCheckpoint next;next.target=target;next.encoded.resize(length);
    std::size_t completed=0;
    while(completed<length) {
#ifdef _WIN32
        DWORD count=0;
        if(!ReadFile(file.value,next.encoded.data()+completed,static_cast<DWORD>(length-completed),&count,nullptr))
            return Failure(Status::ReadFailed,Error());
#else
        const auto count=read(file.value,next.encoded.data()+completed,length-completed);
        if(count<0 && errno==EINTR) continue;
        if(count<0) return Failure(Status::ReadFailed,Error());
#endif
        if(count==0) return Failure(Status::ReadFailed);
        completed+=static_cast<std::size_t>(count);
    }
    // Reject growth/racing in-place writes rather than accepting a valid prefix.
    std::byte extra{};
#ifdef _WIN32
    DWORD count=0;
    if(!ReadFile(file.value,&extra,1,&count,nullptr) || count!=0) return Failure(Status::ReadFailed,Error());
#else
    ssize_t count;
    do {count=read(file.value,&extra,1);} while(count<0 && errno==EINTR);
    if(count!=0) return Failure(Status::ReadFailed,count<0?Error():0);
#endif
    codec::Error error=codec::Error::None;
    if(!codec::Decode(next.encoded,next.checkpoint,error)) return Failure(Status::InvalidExisting,0,error);
    output=std::move(next);return {Status::Read};
}
Handle Lock(Handle directory,const std::filesystem::path& target) {
    const auto path=target.parent_path()/("."+target.filename().string()+".lock");
#ifdef _WIN32
    (void)directory;
    const auto h=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
        nullptr,OPEN_ALWAYS,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(h==Invalid) return Invalid;
    std::size_t size=0;OVERLAPPED overlap{};
    if(!RegularSize(h,size)) {Close(h);SetLastError(ERROR_INVALID_PARAMETER);return Invalid;}
    if(!LockFileEx(h,LOCKFILE_EXCLUSIVE_LOCK|LOCKFILE_FAIL_IMMEDIATELY,0,1,0,&overlap)) {
        const auto error=Error();Close(h);SetLastError(error);return Invalid;
    }
#else
    const auto h=openat(directory,path.filename().c_str(),O_RDWR|O_CREAT|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC,0600);
    if(h==Invalid) return Invalid;
    std::size_t size=0;
    if(!RegularSize(h,size)) {Close(h);errno=EINVAL;return Invalid;}
    if(flock(h,LOCK_EX|LOCK_NB)!=0) {const auto error=errno;Close(h);errno=error;return Invalid;}
#endif
    return h;
}
bool SameExpected(const Result& read,const StoredCheckpoint& current,const std::optional<StoredCheckpoint>& expected) {
    return expected ? read.status==Status::Read && current.encoded==expected->encoded : read.status==Status::NotFound;
}
std::filesystem::path Temporary(const std::filesystem::path& target) {
    static std::atomic<std::uint64_t> sequence{0};
#ifdef _WIN32
    const auto process=GetCurrentProcessId();
#else
    const auto process=getpid();
#endif
    const auto clock=std::chrono::steady_clock::now().time_since_epoch().count();
    return target.parent_path()/("."+target.filename().string()+".pending-"+std::to_string(process)+"-"+
        std::to_string(clock)+"-"+std::to_string(++sequence));
}
Handle CreateTemporary(Handle directory,const std::filesystem::path& target) {
#ifdef _WIN32
    (void)directory;
    return CreateFileW(target.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
#else
    return openat(directory,target.filename().c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
#endif
}
void RemoveTemporary(Handle directory,const std::filesystem::path& path) {
#ifdef _WIN32
    (void)directory;DeleteFileW(path.c_str());
#else
    unlinkat(directory,path.filename().c_str(),0);
#endif
}
struct PendingFile {
    Handle directory;std::filesystem::path path;Owned file;
    PendingFile(Handle dir,std::filesystem::path p,Handle h):directory(dir),path(std::move(p)),file(h){}
    ~PendingFile(){Close(file.value);file.value=Invalid;RemoveTemporary(directory,path);}
};
bool Publish(Handle directory,const std::filesystem::path& temp,const std::filesystem::path& target,bool replace) {
#ifdef _WIN32
    // Windows10+ same-volume name cutover. POSIX semantics retain old reader
    // handles while subsequent opens see the new file; no copy/delete fallback.
    Owned file(CreateFileW(temp.c_str(),DELETE|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
    if(file.value==Invalid) return false;
    (void)directory;
    const auto name=target.wstring();
    const auto size=offsetof(FILE_RENAME_INFO,FileName)+(name.size()+1)*sizeof(wchar_t);
    std::vector<std::byte> buffer(size);
    auto* rename=reinterpret_cast<FILE_RENAME_INFO*>(buffer.data());
    rename->Flags=replace?(FILE_RENAME_FLAG_REPLACE_IF_EXISTS|FILE_RENAME_FLAG_POSIX_SEMANTICS):0;
    rename->RootDirectory=nullptr;rename->FileNameLength=static_cast<DWORD>(name.size()*sizeof(wchar_t));
    std::memcpy(rename->FileName,name.c_str(),(name.size()+1)*sizeof(wchar_t));
    const bool success=SetFileInformationByHandle(file.value,FileRenameInfoEx,rename,static_cast<DWORD>(size))!=FALSE;
    const auto error=Error();Close(file.value);file.value=Invalid;SetLastError(error);return success;
#else
    if(replace) return renameat(directory,temp.filename().c_str(),directory,target.filename().c_str())==0;
    // linkat is a same-filesystem, no-clobber publication even on API24 Android;
    // a raced create cannot be overwritten. The owned temp link is removed next.
    if(linkat(directory,temp.filename().c_str(),directory,target.filename().c_str(),0)!=0) return false;
    return true;
#endif
}
} // namespace

Result Read(const std::filesystem::path& directory,std::string_view name,StoredCheckpoint& output,FaultInjector* faults) {
    std::filesystem::path target;
    if(!Target(directory,name,target)) return Failure(Status::InvalidPath);
    Owned dir(OpenDirectory(target.parent_path()));
    if(dir.value==Invalid) return Failure(Status::InvalidPath,Error());
    return ReadAt(dir.value,target,output,faults);
}

Result Write(const std::filesystem::path& directory,std::string_view name,const Checkpoint& candidate,
             const std::optional<StoredCheckpoint>& expected,FaultInjector* faults) {
    std::vector<std::byte> encoded;codec::Error codecError=codec::Error::None;
    if(!codec::Encode(candidate,encoded,codecError)) return Failure(Status::InvalidCandidate,0,codecError);
    std::filesystem::path target;
    if(!Target(directory,name,target)) return Failure(Status::InvalidPath);
    if(expected) {
        Checkpoint checked;
        if(expected->target!=target || !codec::Decode(expected->encoded,checked,codecError))
            return Failure(Status::InvalidExpectation,0,codecError);
    }
    Owned dir(OpenDirectory(target.parent_path()));
    if(dir.value==Invalid) return Failure(Status::InvalidPath,Error());
    Owned lock(Lock(dir.value,target));
    if(lock.value==Invalid) {const auto error=Error();return Failure(LockBusy(error)?Status::Busy:Status::OpenFailed,error);}
    StoredCheckpoint current;auto read=ReadAt(dir.value,target,current,faults);
    if(read.status!=Status::Read && read.status!=Status::NotFound) return read;
    if(!SameExpected(read,current,expected)) return Failure(Status::Conflict);
    if(expected && encoded==expected->encoded)
        return {Status::AlreadyCurrent}; // no write, no new durability claim
    if(Fail(faults,Operation::OpenTemporary)) return Failure(Status::OpenFailed);
    const auto temp=Temporary(target);const auto handle=CreateTemporary(dir.value,temp);
    if(handle==Invalid) return Failure(Status::OpenFailed,Error());
    PendingFile pending(dir.value,temp,handle);
    std::size_t completed=0;
    while(completed<encoded.size()) {
        if(Fail(faults,Operation::Write,completed)) return Failure(Status::WriteFailed);
        auto requested=encoded.size()-completed;
        if(faults) requested=std::min(requested,faults->WriteLimit(requested));
        if(requested==0) return Failure(Status::WriteFailed);
#ifdef _WIN32
        DWORD count=0;
        if(!WriteFile(handle,encoded.data()+completed,static_cast<DWORD>(requested),&count,nullptr))
            return Failure(Status::WriteFailed,Error());
#else
        const auto count=write(handle,encoded.data()+completed,requested);
        if(count<0 && errno==EINTR) continue;
        if(count<0) return Failure(Status::WriteFailed,Error());
#endif
        if(count==0) return Failure(Status::WriteFailed);
        completed+=static_cast<std::size_t>(count);
    }
    if(Fail(faults,Operation::FlushFile)) return Failure(Status::FlushFailed);
#ifdef _WIN32
    if(!FlushFileBuffers(handle)) return Failure(Status::FlushFailed,Error());
#else
    int flush;
    do {flush=fsync(handle);} while(flush!=0 && errno==EINTR);
    if(flush!=0) return Failure(Status::FlushFailed,Error());
#endif
    if(Fail(faults,Operation::CloseFile)) return Failure(Status::CloseFailed);
    pending.file.value=Invalid;
#ifdef _WIN32
    if(!CloseHandle(handle)) return Failure(Status::CloseFailed,Error());
#else
    if(close(handle)!=0) return Failure(Status::CloseFailed,Error());
#endif
    // Recheck after preparation. The lock serializes participating writers;
    // hostile/uncoordinated directory mutation is outside this adapter contract.
    StoredCheckpoint latest;read=ReadAt(dir.value,target,latest,faults);
    if(read.status!=Status::Read && read.status!=Status::NotFound) return read;
    if(!SameExpected(read,latest,expected)) return Failure(Status::Conflict);
    if(Fail(faults,Operation::Publish)) return Failure(Status::PublishFailed);
    if(!Publish(dir.value,temp,target,expected.has_value())) {
        const auto nativeError=Error();StoredCheckpoint observed;
        const auto observedRead=ReadAt(dir.value,target,observed,nullptr);
        if(observedRead.status==Status::Read && observed.encoded==encoded)
            return {Status::PublishedDurabilityUnknown,Publication::Published,Durability::None,codec::Error::None,nativeError};
        if(SameExpected(observedRead,observed,expected)) return Failure(Status::PublishFailed,nativeError);
        return {Status::PublishOutcomeUnknown,Publication::Unknown,Durability::None,codec::Error::None,nativeError};
    }
    // On POSIX create-only publication temporarily owns two links. Both names
    // refer to complete flushed bytes; readers may safely open the final name.
    RemoveTemporary(dir.value,temp);
    if(Fail(faults,Operation::AfterPublish))
        return {Status::PublishedDurabilityUnknown,Publication::Published};
#ifdef _WIN32
    return {Status::Committed,Publication::Published,Durability::FileFlushed};
#else
    int flushDirectory;
    do {flushDirectory=fsync(dir.value);} while(flushDirectory!=0 && errno==EINTR);
    if(flushDirectory!=0)
        return {Status::PublishedDurabilityUnknown,Publication::Published,Durability::None,codec::Error::None,Error()};
    return {Status::Committed,Publication::Published,Durability::FileAndDirectoryFlushed};
#endif
}
} // namespace horde::gameplay::campaign::storage
