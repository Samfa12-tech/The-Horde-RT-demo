#include "gameplay/campaign/CampaignSaveWorker.h"
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>

namespace cp=horde::gameplay::campaign;
namespace save=cp::async;
namespace io=cp::storage;
namespace fs=std::filesystem;
namespace {
int checks=0,failures=0;
void Check(bool value,const char* message){++checks;if(!value){++failures;std::cerr<<message<<'\n';}}
struct Directory {
    fs::path path=fs::temp_directory_path()/("horde-worker-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Directory(){if(!fs::create_directory(path))throw std::runtime_error("exclusive test directory failed");std::ofstream(path/"owned.txt")<<"fixture";}
    ~Directory(){std::error_code ec;if(path.filename().string().starts_with("horde-worker-test-") &&
        fs::weakly_canonical(path.parent_path(),ec)==fs::weakly_canonical(fs::temp_directory_path(),ec) && !ec &&
        fs::is_regular_file(path/"owned.txt"))fs::remove_all(path,ec);}
};
template<class F> bool Until(F f){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    do{if(f())return true;std::this_thread::yield();}while(std::chrono::steady_clock::now()<end);return false;}
void Baseline(save::Coordinator& owner,save::Worker& worker){const auto r=owner.BeginRead();
    Check(r && worker.Submit(*r),"real worker accepts read");
    Check(Until([&]{return owner.Poll();}),"real read returns through owner polling");
    const auto token=owner.State().preparedRead;
    Check(token && owner.UseReadAsWriteBaseline(*token),"owner explicitly accepts real file baseline");}
struct Latch:io::FaultInjector {
    std::mutex mutex;std::condition_variable cv;
    io::Operation operation=io::Operation::Read;bool armed=false,entered=false,released=false;
    std::atomic<unsigned> reads=0;
    void Arm(io::Operation op){std::lock_guard lock(mutex);operation=op;armed=true;entered=released=false;}
    bool Fail(io::Operation op,std::size_t) override {
        if(op==io::Operation::Read)++reads;
        std::unique_lock lock(mutex);if(!armed || op!=operation)return false;
        armed=false;entered=true;cv.notify_all();cv.wait(lock,[&]{return released;});return false;
    }
    bool Entered(){std::unique_lock lock(mutex);return cv.wait_for(lock,std::chrono::seconds(5),[&]{return entered;});}
    void Release(){std::lock_guard lock(mutex);released=true;cv.notify_all();}
};
}
int main(){
    Directory directory;Latch faults;
    {
        save::Worker worker(directory.path,&faults);
        for(unsigned slot=0;slot<3;++slot) for(auto kind:{cp::SaveKind::Manual,cp::SaveKind::Autosave}){
            save::Coordinator owner;const auto target=cp::SaveTarget(slot,kind);
            Check(target && owner.OpenSession(*target),"three slots have separate manual and latest auto identities");
            cp::Checkpoint c;c.vitality=1+slot;Check(owner.NoteCheckpoint(c),"logical checkpoint valid");Baseline(owner,worker);
            const auto write=owner.BeginWrite();Check(write && worker.Submit(*write),"real worker accepts prepared write");
            Check(Until([&]{return owner.Poll();}) && owner.State().currentCheckpointStored,"real disk publication acknowledges matching revision");
            io::StoredCheckpoint found;Check(io::Read(directory.path,cp::SaveBasename(*target),found).status==io::Status::Read &&
                found.checkpoint.vitality==c.vitality,"real selected slot file round trips");
            owner.Close();
        }
        Check(!cp::SaveTarget(3,cp::SaveKind::Manual) && !cp::SaveTarget(0,static_cast<cp::SaveKind>(99)) &&
            cp::SaveBasename(0).empty() && cp::SaveBasename(7).empty(),"closed target roster rejects invalid slot/kind");
        Check(Until([&]{return worker.Idle();}),"owned worker drains");
        save::Coordinator owner;owner.OpenSession(1);cp::Checkpoint newer;newer.vitality=3;
        owner.NoteCheckpoint(newer);Baseline(owner,worker);
        faults.Arm(io::Operation::FlushFile);auto write=owner.BeginWrite();Check(write && worker.Submit(*write),"preparation cancellation starts");
        Check(faults.Entered(),"worker blocks during off-gate file preparation");owner.Cancel();faults.Release();
        Check(Until([&]{return worker.Idle();}) && !owner.Poll(),"cancelled preparation cleans up without depositing success");
        io::StoredCheckpoint found;Check(io::Read(directory.path,cp::SaveBasename(1),found).status==io::Status::Read &&
            found.checkpoint.vitality==1,"revocation before gate preserves old file");
        Baseline(owner,worker);faults.Arm(io::Operation::AfterPublish);write=owner.BeginWrite();
        Check(write && worker.Submit(*write) && faults.Entered(),"worker reaches off-gate postpublication completion");
        owner.Close(); // Must not wait for the blocked cleanup/durability phase.
        Check(io::Read(directory.path,cp::SaveBasename(1),found).status==io::Status::Read && found.checkpoint.vitality==3,
            "already-authorized publication is visible while completion remains blocked");
        faults.Release();Check(Until([&]{return worker.Idle();}) && !owner.Poll() && owner.State().phase==save::Phase::Closed,
            "late finalized receipt cannot revive closed owner");
        save::Coordinator a,b,c;a.OpenSession(1);b.OpenSession(2);c.OpenSession(3);
        faults.Arm(io::Operation::Read);const auto ar=a.BeginRead(),br=b.BeginRead(),cr=c.BeginRead();
        Check(ar && worker.Submit(*ar) && faults.Entered(),"one IO request can be active");
        Check(br && worker.Submit(*br) && cr && !worker.Submit(*cr),"one waiting request bounds queue; excess rejected");
        const auto reads=faults.reads.load();a.Close();b.Close();c.Close();faults.Release();worker.Stop();
        Check(faults.reads==reads && !a.Poll() && !b.Poll(),"revocation plus shutdown drops queued work and joins owned thread");
    }
    for(const auto& file:fs::directory_iterator(directory.path))Check(file.path().filename().string().find(".pending-")==std::string::npos,
        "cancelled and completed writes leave no owned pending files");
    std::cout<<checks<<" campaign worker checks, "<<failures<<" failures\n";return failures?1:0;
}
