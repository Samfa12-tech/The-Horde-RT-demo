#include "gameplay/DevelopmentCheckpoints.h"
#include "gameplay/ShowcaseCheckpoints.h"
#include "vulkan/raytracing/DevelopmentStaticAssetPolicy.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace
{

int failures = 0;

void Check(bool condition, std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

} // namespace

int main()
{
    using namespace horde::gameplay;
    using namespace horde::vulkan::raytracing;

    Check(kShowcaseCheckpoints.size() == 13u,
          "the existing release checkpoint array remains exactly 13 entries");
    Check(FindShowcaseCheckpoint("pbr-sword-closeup") == nullptr,
          "development proof does not enter the release checkpoint lookup");
    Check(FindShowcaseCheckpoint("pbr-torch-fire") == nullptr,
          "torch development proof does not enter the release checkpoint lookup");
    Check(FindShowcaseCheckpoint("player-body-grips") == nullptr,
          "player-body proof does not enter the release checkpoint lookup");
    Check(kDevelopmentCheckpoints.size() == 6u,
          "six isolated render-development checkpoints are exposed");
    const DevelopmentCheckpoint* checkpoint = FindDevelopmentCheckpoint("pbr-sword-closeup");
    Check(checkpoint != nullptr && checkpoint->id == 100 && checkpoint->baseShowcaseCheckpointId == 0 &&
              checkpoint->name == std::string_view("pbr-sword-closeup") &&
              checkpoint->cameraX == 0.0f && checkpoint->cameraZ == 1.85f,
          "development close-up has a stable identity and imports the opening gameplay state");
    const DevelopmentCheckpoint* torch = FindDevelopmentCheckpoint("pbr-torch-fire");
    Check(torch != nullptr && torch->id == 101 && torch->baseShowcaseCheckpointId == 0 &&
              torch->cameraX == 0.0f && torch->cameraZ == 1.85f,
          "production torch/fire close-up has a stable isolated identity");
    const DevelopmentCheckpoint* player = FindDevelopmentCheckpoint("player-body-grips");
    Check(player != nullptr && player->id == 102 && player->baseShowcaseCheckpointId == 0 &&
              player->cameraX == 0.0f && player->cameraZ == 1.85f &&
              player->pitch < -0.28f,
          "player body/grips proof has a stable downward-view identity");
    const DevelopmentCheckpoint* fallback = FindDevelopmentCheckpoint("player-fallback-grips");
    Check(fallback != nullptr && fallback->baseShowcaseCheckpointId == 0 &&
              fallback->pitch == player->pitch,
          "procedural player A/B checkpoint uses the exact skinned grip view");
    Check(FindDevelopmentCheckpoint(102) == player && FindDevelopmentCheckpoint(105) == fallback,
          "Android debug automation resolves bounded development checkpoints by ID");
    Check(FindDevelopmentCheckpoint("opening") == nullptr,
          "release checkpoint names cannot resolve through the development lookup");

    const std::filesystem::path sourceRoot{"C:/source/horde"};
    const auto enabled = ResolveDevelopmentStaticAssetDirectory(true, true, sourceRoot);
    Check(enabled == sourceRoot / "assets/models/weapons/meshy/runtime-development",
          "Debug development capture resolves only the audited source-tree derivative");
    Check(ResolveDevelopmentStaticAssetDirectory(true, false, sourceRoot).empty(),
          "ordinary Debug gameplay does not silently activate source-tree assets");
    Check(ResolveDevelopmentStaticAssetDirectory(false, true, sourceRoot).empty(),
          "packaged Release disables source-tree fallback even when a path is supplied");
    Check(UseGenericStaticAssetForCheckpoint("pbr-sword-closeup"),
          "the explicit development checkpoint activates generic static metadata");
    Check(UseGenericStaticAssetForCheckpoint("pbr-torch-fire"),
          "the production torch/fire checkpoint activates the generic static slot");
    Check(!UseGenericStaticAssetForCheckpoint("lantern-drop"),
          "the historical external checkpoint literal does not activate the generic proof");

    if (failures != 0)
    {
        std::cerr << failures << " development static asset assertion(s) failed\n";
        return 1;
    }
    std::cout << "Development static asset policy passed\n";
    return 0;
}
