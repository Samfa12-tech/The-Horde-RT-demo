#include "vulkan/raytracing/DevelopmentWorldSceneAdapter.h"
#include "scene/ChapterTerrainSurface.h"
#include "scene/assets/AssetManifest.h"
#include <filesystem>
#include <iostream>
#include <memory>
#include <limits>
#include <vector>

namespace sim=horde::gameplay::simulation;
namespace cp=horde::gameplay::campaign;
namespace tr=horde::gameplay::traversal;
using horde::vulkan::raytracing::PublishDevelopmentWorldReadiness;
namespace {
int checks=0,failures=0;
void Check(bool value,const char* message){++checks;if(!value){++failures;std::cerr<<"FAIL: "<<message<<'\n';}}
// Explicit CPU-only scene protocol. No Vulkan resources or presentation are
// constructed/claimed here. Production still calls the exact same adapter
// using PresentableTinyRtScene's real buffer/BLAS/TLAS/contributor proof.
struct SceneFixture {
    std::array<sim::ZoneReadiness,4> responses{};
    std::shared_ptr<const sim::TriangleSupportSurface> terrain;
    mutable std::vector<sim::WorldZoneToken> queried;
    sim::GameSimulation* invalidateOnQuery=nullptr;
    mutable bool invalidated=false;
    horde::gameplay::healing::FlaskUpgradeReadiness flask{1,std::nullopt};
    auto FlaskUpgradeReadiness()const{return flask;}
    sim::ZoneReadiness WorldZoneReadiness(sim::WorldZoneToken token)const {
        queried.push_back(token);
        if(invalidateOnQuery && !invalidated){invalidated=true;invalidateOnQuery->InvalidateWorldZoneReadiness();}
        return responses[static_cast<std::size_t>(token.zone)];
    }
    auto ChapterTerrainSurface()const{return terrain;}
};
std::unique_ptr<sim::GameSimulation> RestoreUpper(const std::shared_ptr<const sim::TriangleSupportSurface>& terrain,bool utility=false){
    auto config=sim::ProductionGameSimulationConfig();config.healingFlask1_7=config.flaskUtilityCandidate=utility;
    auto game=std::make_unique<sim::GameSimulation>(config);
    cp::Checkpoint checkpoint;checkpoint.id=cp::CheckpointId::SafeExit;
    checkpoint.zone=sim::WorldZoneId::TombExterior;checkpoint.position=tr::kExteriorLanding;
    checkpoint.firstAscentCompleted=true;checkpoint.dialogue.grateConsumed=checkpoint.dialogue.rescueConsumed=true;
    checkpoint.dialogue.reunionStarted=checkpoint.dialogue.waitingForRaise=true;
    const auto token=game->BeginCampaignRestore(checkpoint);
    Check(token && game->CommitCampaignRestore(*token,{token->worldGeneration,checkpoint.zone},terrain,{}),
          "logical upper restore consumes actual CPU terrain fixture");
    return game;
}
}
int main(){
    const auto directory=std::filesystem::path(HORDE_RT_SOURCE_DIR)/"assets/models/world/runtime/chapter-terrain-v1";
    horde::scene::assets::AssetManifest manifest;horde::scene::assets::StaticMeshAsset mesh;std::string diagnostic;
    if(!horde::scene::assets::AssetManifest::Load(directory/"asset.manifest.json",manifest,diagnostic) ||
       !horde::scene::assets::StaticMeshAsset::Load(directory/"chapter-terrain-lod0.runtime.glb",manifest,mesh,diagnostic)){
        std::cerr<<diagnostic;return 1;}
    const auto terrain=horde::scene::PrepareChapterTerrainSurface(mesh,diagnostic);
    if(!terrain){std::cerr<<diagnostic;return 1;}
    {
        auto game=RestoreUpper(terrain);
        Check(game->Snapshot().worldRoute.readiness[0]==sim::ZoneReadiness::Unprepared &&
              game->Snapshot().rescuePrompt==tr::RescuePrompt::Preparing,
              "restored upper side starts with unprepared dungeon return destination");
        SceneFixture scene;scene.terrain=terrain;scene.responses.fill(sim::ZoneReadiness::Preparing);
        PublishDevelopmentWorldReadiness(*game,scene);
        Check(scene.queried.size()==3 && scene.queried[0].zone==sim::WorldZoneId::Dungeon &&
              game->Snapshot().worldRoute.readiness[0]==sim::ZoneReadiness::Preparing &&
              game->Snapshot().rescuePrompt==tr::RescuePrompt::Preparing,
              "adapter queries zone zero and preserves Preparing when renderer is preparing");
        scene.responses[0]=sim::ZoneReadiness::Failed;
        PublishDevelopmentWorldReadiness(*game,scene);
        Check(game->Snapshot().worldRoute.readiness[0]!=sim::ZoneReadiness::Ready &&
              game->Snapshot().rescuePrompt==tr::RescuePrompt::Preparing,
              "failed scene response cannot manufacture resident dungeon readiness");
        scene.responses.fill(sim::ZoneReadiness::Ready);
        PublishDevelopmentWorldReadiness(*game,scene);
        for(const auto state:game->Snapshot().worldRoute.readiness)
            Check(state==sim::ZoneReadiness::Ready,"every resident zone can be admitted from current scene readiness");
        Check(game->Snapshot().rescuePrompt==tr::RescuePrompt::Descend,
              "actual adapter publication changes restored upper prompt to Descend");
        const auto queries=scene.queried.size();PublishDevelopmentWorldReadiness(*game,scene);
        Check(scene.queried.size()==queries,"already-ready zones are not queried or regressed");
        sim::InputSnapshot input;input.damageEnabled=false;
        input.yawRadians=game->Snapshot().playerYawRadians;input.pitchRadians=game->Snapshot().playerPitchRadians;
        game->StepFixed(input); // A fresh neutral direct-host publication releases restore input floor.
        ++input.commands.interact;game->StepFixed(input);
        Check(game->Snapshot().rescue.equipmentStowed,"ordinary contextual input commits real descent without direct dungeon publication");
        for(int tick=0;tick<1800 && game->Snapshot().rescue.equipmentStowed;++tick)game->StepFixed(input);
        Check(!game->Snapshot().rescue.exteriorSide && !game->Snapshot().rescue.equipmentStowed &&
              game->Snapshot().rescue.sawDescent && game->Snapshot().rescue.sawLanding,
              "renderer-admitted return route reaches the real lower landing through native motor");
    }
    {
        auto game=RestoreUpper(terrain);
        SceneFixture scene;scene.responses.fill(sim::ZoneReadiness::Ready); // Deliberately missing terrain.
        PublishDevelopmentWorldReadiness(*game,scene);
        Check(game->Snapshot().worldRoute.readiness[0]==sim::ZoneReadiness::Ready,
              "resident dungeon uses generic readiness and does not require a terrain publication");
        Check(game->Snapshot().worldRoute.readiness[2]==sim::ZoneReadiness::Preparing &&
              game->Snapshot().worldRoute.readiness[3]==sim::ZoneReadiness::Preparing,
              "rescue exterior Ready responses still require the scene's authored terrain");
    }
    {
        auto game=RestoreUpper(terrain);const auto before=game->Snapshot().worldRoute.generation;
        SceneFixture scene;scene.terrain=terrain;scene.responses.fill(sim::ZoneReadiness::Ready);scene.invalidateOnQuery=game.get();
        PublishDevelopmentWorldReadiness(*game,scene);
        Check(game->Snapshot().worldRoute.generation>before,"fixture invalidates resources while old captured readiness responses are outstanding");
        for(std::size_t zone=1;zone<sim::kWorldZones.size();++zone)
            Check(game->Snapshot().worldRoute.readiness[zone]==sim::ZoneReadiness::Unprepared,
                  "stale adapter responses cannot prepare or ready replacement exterior generation");
        for(const auto token:scene.queried)Check(token.generation==before,"one adapter pass never relabels old scene responses with a newer generation");
    }
    {
        auto game=RestoreUpper(terrain);
        horde::vulkan::raytracing::PresentableTinyRtScene notInitialised;
        PublishDevelopmentWorldReadiness(*game,notInitialised);
        Check(game->Snapshot().worldRoute.readiness[0]!=sim::ZoneReadiness::Ready,
              "real uninitialized renderer cannot certify dungeon readiness");
    }
    {
        auto game=RestoreUpper(terrain,true);SceneFixture scene;scene.terrain=terrain;scene.responses.fill(sim::ZoneReadiness::Ready);
        horde::gameplay::healing::FlaskUpgradeSite site;const auto point=sim::kWorldRoutePoints[6];
        site.supportPoint={point.x,terrain->Query(point.x,point.z).worldY,point.z};
        site.worldFromItem[12]=point.x;site.worldFromItem[13]=site.supportPoint[1]+.075f;site.worldFromItem[14]=point.z;
        scene.flask={7,site};PublishDevelopmentWorldReadiness(*game,scene);
        Check(game->Snapshot().flaskUpgrade.site.has_value(),"current CPU scene receipt publishes an item without awarding entitlement");
        const auto queries=scene.queried.size();scene.flask={8,std::nullopt};PublishDevelopmentWorldReadiness(*game,scene);
        Check(scene.queried.size()==queries && !game->Snapshot().flaskUpgrade.site,
              "already-ready zones still receive current pickup resource withdrawal");
        scene.flask={7,site};PublishDevelopmentWorldReadiness(*game,scene);
        Check(!game->Snapshot().flaskUpgrade.site,"old resource response cannot resurrect withdrawn pickup");
        scene.flask={8,site};PublishDevelopmentWorldReadiness(*game,scene);
        Check(game->Snapshot().flaskUpgrade.site==site && !game->Snapshot().healingFlask.resources.upgraded,
              "current resource receipt restores presentation only");
        auto bad=site;bad.worldFromItem[12]=std::numeric_limits<float>::quiet_NaN();
        const auto token=sim::WorldZoneToken{game->Snapshot().worldRoute.generation,sim::WorldZoneId::Lookout};
        Check(!game->PublishFlaskUpgradeReadiness(token,{9,bad}) && game->Snapshot().flaskUpgrade.site==site,
              "invalid receipt cannot overwrite a coherent current pickup");
        game->InvalidateWorldZoneReadiness();scene.invalidateOnQuery=game.get();
        PublishDevelopmentWorldReadiness(*game,scene);
        Check(!game->Snapshot().flaskUpgrade.site,"same captured adapter token rejects pickup reply after mid-pass world invalidation");
        horde::vulkan::raytracing::PresentableTinyRtScene notInitialised;
        Check(!notInitialised.FlaskUpgradeReadiness().site,"real uninitialized native scene cannot manufacture item readiness");
    }
    std::cout<<checks<<" development world scene adapter checks, "<<failures<<" failures; CPU protocol/native motor only, no GPU presentation claim\n";
    return failures?1:0;
}
