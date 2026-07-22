#include <iostream>
#include <string>

#include "ui/DiagnosticOverlay.h"

namespace
{

bool RequireContains(const std::string& text, const std::string& expected)
{
    if (text.find(expected) != std::string::npos)
    {
        return true;
    }
    std::cerr << "Developer overlay output is missing: " << expected << '\n';
    return false;
}

} // namespace

int main()
{
    horde::ui::DeveloperOverlaySnapshot snapshot;
    snapshot.buildIdentity = "0.1.2-alpha.1 DEBUG";
    snapshot.shaderIdentity = "0123456789ab";
    snapshot.gpuName = "Test RTX";
    snapshot.vulkanApi = "1.4.350";
    snapshot.rtMode = "RayTracingPipeline";
    snapshot.routeZone = "finale";
    snapshot.materialEncoding = "ASTC test route";
    snapshot.lanternPhase = "settled";
    snapshot.selectedEnemy = "lich";
    snapshot.encounterPhase = "charging";
    snapshot.internalWidth = 1080;
    snapshot.internalHeight = 2235;
    snapshot.presentationWidth = 1440;
    snapshot.presentationHeight = 2980;
    snapshot.blasCount = 8;
    snapshot.tlasCount = 1;
    snapshot.tlasInstanceCount = 18;
    snapshot.activeSkinnedEnemies = 1;
    snapshot.enemyHealth = 2;
    snapshot.renderScale = 0.75f;
    snapshot.fps = 83.815f;
    snapshot.frameTimeMs = 11.931f;
    snapshot.presented = true;

    const std::string text = horde::ui::BuildDeveloperOverlayText(snapshot);
    bool ok = true;
    ok &= RequireContains(text, "DEV  0.1.2-alpha.1 DEBUG  shader 0123456789ab");
    ok &= RequireContains(text, "RT RayTracingPipeline  |  presented YES  |  scale 75%");
    ok &= RequireContains(text, "FRAME 11.9 ms  |  83.8 FPS  |  1080x2235 -> 1440x2980");
    ok &= RequireContains(text, "SCENE finale  |  lantern settled  |  lich charging hp 2");
    ok &= RequireContains(text, "AS 8 BLAS / 1 TLAS / 18 inst  |  skinned 1");
    ok &= RequireContains(text, "MAT ASTC test route");

    snapshot.presented = false;
    snapshot.selectedEnemy = "none";
    snapshot.encounterPhase = "inactive";
    snapshot.enemyHealth = -1;
    const std::string inactiveText = horde::ui::BuildDeveloperOverlayText(snapshot);
    ok &= RequireContains(inactiveText, "presented NO");
    ok &= RequireContains(inactiveText, "none inactive");
    if (inactiveText.find(" hp ") != std::string::npos)
    {
        std::cerr << "Developer overlay printed health for an encounter without a health value.\n";
        ok = false;
    }
    return ok ? 0 : 1;
}
