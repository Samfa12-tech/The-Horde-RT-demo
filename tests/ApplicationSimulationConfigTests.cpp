#include "gameplay/simulation/PrivateRevivalApplication.h"
#include <iostream>
using namespace horde::gameplay::simulation;
namespace cp=horde::gameplay::campaign;
int main() {
    bool passed=true;const bool admitted=HORDE_RT_REVIVAL_ADMITTED!=0;
    const bool flaskAdmitted=HORDE_RT_FLASK_ADMITTED!=0;
    auto check=[&](bool value,const char* message){if(!value){passed=false;std::cerr<<message<<'\n';}};
    const auto config=PrivateRevivalApplicationConfig();
    check(config.skeletonRevival1_7==admitted && config.naturalPracticeRevival1_7==admitted,"application flags respect private/public admission");
    check(config.healingFlask1_7==flaskAdmitted && config.flaskUtilityCandidate==flaskAdmitted,
          "flask application admission is explicit and shared");
    auto game=CreateApplicationGameSimulation();
    auto readiness=[&]{return game.ConfigureSkeletonRevivalPresentation(1,89.f/30.f,1.5f,true,true);};
    check(readiness()==admitted,"startup uses application admission");
    for(int newGame=0;newGame<2;++newGame) {
        game=CreateApplicationGameSimulation();game.StartLiveDevelopmentRescueJourney(true);
        check(readiness()==admitted,"first/second New Game retains admitted configuration");
        check(game.Snapshot().healingFlaskEnabled==flaskAdmitted &&
              game.Snapshot().flaskUtilityCandidate==flaskAdmitted,"first/second New Game retains native flask ownership");
        game.RetryEncounter();check(readiness()==admitted,"Retry preserves application configuration");
        game.ResetRoute();check(readiness()==admitted,"route reset preserves application configuration");
    }
    cp::Checkpoint c;c.dialogue.grateConsumed=c.dialogue.rescueConsumed=true;const float y=c.position.y;
    const std::array<TriangleSupportSurface::Triangle,2> flat{{
        {{{{-100,y,-100},{-100,y,100},{100,y,100}}}},
        {{{{-100,y,-100},{100,y,100},{100,y,-100}}}}
    }};
    std::string error;const auto terrain=TriangleSupportSurface::Build(flat,error);
    const auto token=game.BeginCampaignRestore(c);
    check(token && game.CommitCampaignRestore(*token,{token->worldGeneration,c.zone},terrain,{}),"normal load commits through typed world/support readiness");
    check(readiness()==admitted,"committed load preserves application admission");
    return passed?0:1;
}
