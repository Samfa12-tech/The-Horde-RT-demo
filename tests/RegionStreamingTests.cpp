#include "world/streaming/RegionStreaming.h"
#include <iostream>
#include <limits>

using namespace horde::world::streaming;
namespace {
int failures=0;
void Check(bool value,const char* detail) {
    if(!value){++failures;std::cerr<<"FAIL: "<<detail<<'\n';}
}
std::vector<RegionDefinition> Catalog() {
    return {{1,{{-100,-10,-100},{100,10,100}},true,100,{}},
            {2,{{0,-2,-2},{10,4,4}},false,200,{1}},
            {3,{{30,-2,-2},{40,4,4}},false,160,{1}}};
}
struct Fixture {
    RegionStreaming stream;
    StreamingRules rules;
    std::string error;
    std::array<Contribution,3> contributions{{{1,false},{2,true},{3,true}}};
    Frame frame{1,0,{0,0,0},{0,0,0},{},contributions};
    Plan plan;
    void Configure() {auto d=Catalog();Check(stream.Configure(d,rules,1,error),"valid catalog");}
    Action Step(float x,double seconds) {
        frame.position[0]=x;frame.monotonicSeconds=seconds;
        Check(stream.Evaluate(frame,plan,error),"valid observation");
        return plan.count?plan.actions[0]:Action{ActionKind::AdvancePreparation,{}};
    }
    void Load(const Action& a,std::uint64_t bytes) {
        Check(a.kind==ActionKind::Prepare && a.token.region!=0,"bounded preparation requested");
        Check(stream.ObservePreparationAllocation(a.token,bytes) && stream.MarkPrepared(a.token,bytes),"actual prepared allocation recorded");
        Check(stream.MarkResident(a.token),"backend acknowledges adopted ready owner");
    }
};
}
int main() {
    {
        Fixture f;f.Configure();Check(f.stream.SeedResident(1,100),"coarse/shared seed");
        const auto first=f.Step(0,0);Check(first.token.region==2 && f.plan.count==1,"current area loads before distant optional detail");
        f.Load(first,180);
        Check(!f.stream.MarkResident(first.token),"duplicate adoption rejected");
        const auto next=f.Step(35,1);Check(next.token.region==3,"outbound destination demand");f.Load(next,150);
        f.Step(35,3.9);Check(f.plan.count==0,"retirement hysteresis retains old area");
        const auto retire=f.Step(35,4.1);Check(retire.kind==ActionKind::Retire && retire.token==first.token,"old detail retires automatically when certified unnecessary");
        Check(f.stream.MarkRetired(retire.token),"backend completes real retirement");
        const auto back=f.Step(0,5);Check(back.kind==ActionKind::Prepare && back.token.region==2 &&
            back.token.requestGeneration>first.token.requestGeneration,"return gets fresh preparation identity");
        Check(!f.stream.MarkPrepared(first.token,180),"old completed callback cannot resurrect returned region");
        f.Load(back,180);f.Step(0,6);
        Check(f.plan.residentDeviceBytes==430,"shared owner counted once across two detail owners");
    }
    {
        Fixture f;f.Configure();f.stream.SeedResident(1,100);f.stream.SeedResident(2,180);
        f.contributions[1].mayBeAbsent=false;f.Step(80,0);f.Step(80,20);
        Check(f.stream.States()[1].phase==Phase::Resident && f.plan.requiredUnavailable==0,
            "far unknown shadow/reflection contributor remains resident");
    }
    {
        Fixture f;f.rules.maxResidentDetailOwners=1;f.rules.retireDelaySeconds=0;f.Configure();
        f.stream.SeedResident(1,100);f.stream.SeedResident(2,180);
        const std::array<RegionId,1> next{{3}};f.frame.requiredForTraversal=next;
        const auto retirement=f.Step(25,0);
        Check(retirement.kind==ActionKind::Retire && retirement.token.region==2,
            "safe optional retention gives way to required destination when slot pool is full");
        const auto cancellation=f.Step(0,1);
        Check(cancellation.kind==ActionKind::CancelRetirement && cancellation.token==retirement.token,
            "immediate backtracking cancels retirement before backend destroys the owner");
        Check(f.stream.MarkRetirementCancelled(cancellation.token),"backend rollback acknowledgement retains owner");
        f.frame.requiredForTraversal={};Check(f.stream.States()[1].phase==Phase::Resident,"return did not silently clear live geometry");
    }
    {
        Fixture f;f.rules.workingSetDeviceBytes=250;f.Configure();f.stream.SeedResident(1,100);
        auto a=f.Step(0,0);Check(a.kind==ActionKind::Prepare && f.plan.requiredExceedsTarget,
            "required geometry is not suppressed by an insufficient measured target");
        f.Load(a,180);f.Step(20,1);
        Check(f.plan.preparationReservedDeviceBytes==0 && f.stream.States()[2].phase==Phase::Absent,
            "optional refinement does not exceed measured working-set target");
    }
    {
        Fixture f;f.Configure();f.stream.SeedResident(1,100);auto a=f.Step(0,0);
        Check(f.stream.ObservePreparationAllocation(a.token,170),"partial allocation ledger");
        Check(f.stream.Invalidate(2),"world invalidation requests cancellation");
        Check(!f.stream.MarkPrepared(a.token,180) && !f.stream.MarkResident(a.token),"stale readiness cannot publish");
        f.frame.worldGeneration=2;
        auto cancel=f.Step(0,0);Check(cancel.kind==ActionKind::CancelPreparation && cancel.token==a.token,
            "obsolete generation still has an identifiable owned cancellation");
        Check(f.stream.MarkPreparationCancelled(cancel.token),"real cancellation can release old-generation reservation");
        auto fresh=f.Step(0,1);Check(fresh.kind==ActionKind::Prepare && fresh.token.worldGeneration==2 &&
            fresh.token.requestGeneration>a.token.requestGeneration,"new world demand has a fresh token");
    }
    {
        Fixture f;f.Configure();f.stream.SeedResident(1,100);auto a=f.Step(0,0);
        f.stream.ObservePreparationAllocation(a.token,170);f.stream.MarkFailed(a.token,true);f.Step(0,1);
        Check(f.stream.States()[1].phase==Phase::Quarantined && f.plan.residentDeviceBytes==270 &&
            !f.stream.RetryFailedRegion(2) && !f.stream.MarkRetired(a.token),
            "unproved GPU work remains owned/accounted and cannot retry or retire as a normal page");
        Check(f.stream.MarkQuarantineReleased(a.token),"only explicit backend cleanup releases quarantine");
    }
    {
        Fixture f;auto d=Catalog();std::swap(d[0],d[1]);
        Check(f.stream.Configure(d,f.rules,1,f.error),"out-of-order dependency catalog admitted");
        auto a=f.Step(0,0);Check(a.token.region==1,"shared dependency loads first with one preparation slot");
        f.Load(a,100);auto child=f.Step(0,1);Check(child.token.region==2,"dependant preparation cannot deadlock behind ready shared owner");
    }
    {
        Fixture f;auto d=Catalog();d[0].dependencies={2};
        Check(!f.stream.Configure(d,f.rules,1,f.error),"dependency cycle fails closed");
        d=Catalog();d[1].id=1;Check(!f.stream.Configure(d,f.rules,1,f.error),"duplicate region identity rejected");
        d=Catalog();d[2].dependencies={999};Check(!f.stream.Configure(d,f.rules,1,f.error),"missing dependency rejected");
        d=Catalog();d[0].bounds.min[0]=std::numeric_limits<float>::quiet_NaN();
        Check(!f.stream.Configure(d,f.rules,1,f.error),"nonfinite topo bounds rejected");
    }
    {
        Fixture f;f.Configure();f.stream.SeedResident(1,100);
        f.frame.worldGeneration=0;Check(!f.stream.Evaluate(f.frame,f.plan,f.error),"stale observation rejected");
        f.frame.worldGeneration=1;f.frame.monotonicSeconds=-1;
        Check(!f.stream.Evaluate(f.frame,f.plan,f.error),"negative clock rejected");
        f.frame.monotonicSeconds=0;f.frame.position[0]=std::numeric_limits<float>::infinity();
        Check(!f.stream.Evaluate(f.frame,f.plan,f.error),"invalid position rejected");
        f.frame.position[0]=0;std::array<Contribution,2> duplicate{{{2,false},{2,true}}};f.frame.contributions=duplicate;
        Check(!f.stream.Evaluate(f.frame,f.plan,f.error),"conflicting duplicate contribution reports rejected");
    }
    {
        Fixture f;f.rules.maxResidentDetailOwners=1;f.Configure();
        f.stream.SeedResident(1,100);f.stream.SeedResident(2,180);
        const std::array<RegionId,1> next{{3}};f.frame.requiredForTraversal=next;
        const auto retirement=f.Step(15,0);
        Check(retirement.kind==ActionKind::Retire && retirement.token.region==2,
            "required destination can displace a near/predicted optional owner without waiting for hysteresis");
        f.Step(15,0.1);
        Check(!f.plan.count && f.plan.requiredBlockedByOwnerCapacity,
            "pending capacity retirement does not cancel itself because victim is still near");
        Check(f.stream.MarkRetired(retirement.token),"capacity victim really retired");
        const auto destination=f.Step(15,0.2);
        Check(destination.kind==ActionKind::Prepare && destination.token.region==3,
            "released capacity goes to required destination before optional victim reload");
        f.Load(destination,150);
        f.Step(15,1);Check(!f.plan.count,"optional detail cannot thrash required owner's full slot");
    }
    {
        Fixture f;auto d=Catalog();d[0].alwaysResident=false;
        d[0].bounds={{-80,-2,-2},{-70,4,4}};f.rules.workingSetDeviceBytes=50;
        Check(f.stream.Configure(d,f.rules,1,f.error),"optional shared dependency catalog");
        f.contributions[0].mayBeAbsent=true;
        auto parent=f.Step(-75,0);Check(parent.token.region==1,"current dependency starts preparing");
        const auto continuation=f.Step(15,1);
        Check(continuation.kind==ActionKind::AdvancePreparation && continuation.token==parent.token,
            "pressure cannot cancel distant dependency still needed by a near child");
    }
    {
        Fixture f;f.Configure();
        Check(!f.stream.SeedResident(2,180),"seed cannot bypass absent shared owner");
        Check(f.stream.SeedResident(1,0) && f.stream.SeedResident(2,0),
            "explicit observed zero-byte CPU-only owners are valid after dependencies are resident");
        f.Step(0,0);Check(!f.plan.costUnknown && !f.plan.residentDeviceBytes,
            "observed zero and unknown allocation are distinct");
    }
    {
        Fixture f;auto d=Catalog();d[1].preparationPeakDeviceBytes=0;
        Check(f.stream.Configure(d,f.rules,1,f.error),"unknown cost catalog admitted honestly");
        f.stream.SeedResident(1,100);const auto pending=f.Step(0,0);
        Check(pending.kind==ActionKind::Prepare && f.plan.costUnknown,"required unknown-cost load is flagged");
        f.Step(0,1);Check(f.plan.costUnknown,"pending unknown allocation is not presented as measured zero");
        Check(f.stream.MarkPrepared(pending.token,180),"actual cost replaces unknown reservation");
        f.Step(0,2);Check(!f.plan.costUnknown && f.plan.preparationReservedDeviceBytes==180,
            "prepared allocation is measured and remains owned until adoption");
    }
    if(!failures)std::cout<<"Region scheduler CPU ownership/budget/return contracts passed; no GPU/device readiness claim.\n";
    return failures?1:0;
}
