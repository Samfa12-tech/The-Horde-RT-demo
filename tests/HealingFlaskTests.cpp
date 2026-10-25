#include "gameplay/healing/HealingFlask.h"
#include <array>
#include <iostream>
#include <limits>

namespace {
using namespace horde::gameplay::healing;
int checks=0,failures=0;
void Check(bool value,const char* message) {
    ++checks;if(!value){++failures;std::cerr<<message<<'\n';}
}
FlaskCompletion Advance(HealingFlask& flask,FlaskContext context,int ticks) {
    FlaskCompletion result;
    for(int tick=0;tick<ticks;++tick) {
        const auto step=flask.Step(1.0/60.0,context);
        if(step.completed) {Check(!result.completed,"one action cannot complete twice");result=step;}
    }
    return result;
}
}
int main() {
    HealingFlask flask;
    Check(flask.Snapshot().resources==FlaskResources{} &&
        flask.Snapshot().action==FlaskAction::Idle,"new game starts idle with two full charges");
    for(bool upgraded:{false,true}) for(unsigned charges=0;charges<=3;++charges) {
        const auto before=flask.Snapshot();
        const bool valid=charges<=(upgraded?3u:2u);
        Check(flask.Restore({upgraded,charges})==valid,"restore enforces capacity bounds");
        Check(valid?flask.Snapshot().resources==FlaskResources{upgraded,charges}:
            flask.Snapshot()==before,"exact restore never grants resources and rejection preserves state");
    }
    Check(!flask.Restore({true,std::numeric_limits<unsigned>::max()}),"overflow-size resource count rejects");
    FlaskContext context{2,true,true,false,false};
    flask.ResetNewGame();
    Check(flask.BeginDrink(context) && !flask.BeginDrink(context) && !flask.BeginRest(context),
        "duplicate or conflicting start cannot restart an active drink");
    Check(!Advance(flask,context,74).completed && flask.Snapshot().resources.charges==2,
        "no early heal or charge consumption before 1.25 seconds");
    const auto drink=flask.Step(1.0/60.0,context);
    Check(drink.completed && drink.action==FlaskAction::Drinking && drink.healHalfUnits==2 &&
        flask.Snapshot().resources.charges==1 && flask.Snapshot().action==FlaskAction::Idle,
        "drink commits one charge and one-heart effect on completion");
    Check(!Advance(flask,context,120).completed && flask.Snapshot().resources.charges==1,
        "continued stepping cannot replay a completed drink");
    for(int health=1;health<=6;++health) {
        flask.ResetNewGame();context.healthHalfUnits=health;
        Check(flask.BeginDrink(context)==(health<6),"full health rejects drinking without consuming");
        const auto completion=Advance(flask,context,75);
        Check(completion.completed==(health<6) && completion.healHalfUnits==(health<6?std::min(2,6-health):0),
            "one-heart heal clamps exactly, including missing half hearts");
    }
    context.healthHalfUnits=1;
    for(int reason=0;reason<4;++reason) for(bool rest:{false,true}) {
        flask.ResetNewGame();
        Check(rest?flask.BeginRest(context):flask.BeginDrink(context),"cancellation fixture begins normally");
        Advance(flask,context,rest?119:74);
        auto interrupted=context;
        if(reason==0) interrupted.cancelRequested=true;
        if(reason==1) interrupted.acceptedDamage=true;
        if(reason==2) {interrupted.drinkEligible=false;interrupted.restEligible=false;}
        if(reason==3) interrupted.healthHalfUnits=0;
        Check(!flask.Step(1.0/60.0,interrupted).completed &&
            flask.Snapshot().resources.charges==2 && flask.Snapshot().action==FlaskAction::Idle,
            "cancel/damage/prerequisite/death wins completion tie without charge use");
        Check(!Advance(flask,context,120).completed,"cancelled action cannot resume from a stale timer");
    }
    flask.Restore({false,0});
    Check(!flask.BeginDrink(context) && flask.BeginRest(context),"empty flask still permits explicitly eligible Rest");
    Check(!Advance(flask,context,119).completed,"Rest cannot complete before two gameplay seconds");
    const auto rest=flask.Step(1.0/60.0,context);
    Check(rest.completed && rest.action==FlaskAction::Resting && rest.healHalfUnits==5 &&
        flask.Snapshot().resources.charges==2,"Rest fully heals/refills without an enemy or reward effect");
    auto staleRest=context;staleRest.restEligible=false;
    Check(!flask.BeginRest(staleRest) && flask.Snapshot().action==FlaskAction::Idle,
        "rejected stale Rest cannot silently start drinking");
    flask.BeginDrink(context);Advance(flask,context,74);
    auto nowFull=context;nowFull.healthHalfUnits=6;
    const auto beforeFull=flask.Snapshot().resources;
    Check(!flask.Step(1.0/60.0,nowFull).completed && flask.Snapshot().resources==beforeFull,
        "becoming full before drink completion cancels without wasting a charge");
    context.healthHalfUnits=6;
    Check(flask.BeginRest(context),"full-health full-flask Rest remains explicitly repeatable");
    Check(Advance(flask,context,120).completed,"repeat Rest completes normally with zero heal needed");
    Check(flask.TryUpgrade() && flask.Snapshot().resources.Capacity()==3 &&
        flask.Snapshot().resources.charges==2 && !flask.TryUpgrade(),
        "one upgrade increases capacity only and cannot stack or refill");
    Check(flask.BeginRest(context),"upgraded full-health Rest starts");
    Check(Advance(flask,context,120).completed && flask.Snapshot().resources.charges==3,
        "the next Rest fills the newly available charge slot");
    context.healthHalfUnits=3;flask.ResetNewGame();flask.BeginDrink(context);Advance(flask,context,35);
    Check(!flask.TryUpgrade(),"upgrade cannot mutate resources during another action");
    const auto active=flask.Snapshot();
    Check(!flask.Restore({false,3}) && flask.Snapshot()==active,
        "invalid restore preserves both resources and the active timer");
    Check(flask.Restore({false,0}) && flask.Snapshot().action==FlaskAction::Idle &&
        flask.Snapshot().elapsedSeconds==0 && !Advance(flask,context,120).completed,
        "restoring exact resources drops in-progress use rather than replaying completion");
    flask.ResetNewGame();flask.BeginDrink(context);
    Check(!flask.Step(100,context).completed && flask.Snapshot().elapsedSeconds<=.05,
        "oversized owner update does not instantly heal or skip the bounded step");
    Check(!flask.Step(std::numeric_limits<double>::quiet_NaN(),context).completed &&
        flask.Snapshot().action==FlaskAction::Idle && flask.Snapshot().resources.charges==2,
        "invalid clock input cancels freely and never propagates NaN");
    for(int invalid:{-1,0,7,std::numeric_limits<int>::max()}) {
        context.healthHalfUnits=invalid;
        Check(!flask.BeginDrink(context) && !flask.BeginRest(context),"invalid or dead health cannot start recovery");
    }
    std::cout<<"healing flask checks="<<checks<<" failures="<<failures<<'\n';
    return failures?1:0;
}
