#include "ui/DiagnosticOverlay.h"

#include <cmath>
#include <iomanip>
#include <sstream>

#include "vulkan/RtCapabilityReport.h"

namespace horde::ui
{

std::string BuildDiagnosticOverlayText(const vulkan::DeviceCapabilities& capabilities)
{
    return vulkan::BuildCapabilityTextReport(capabilities);
}

std::string BuildUnsupportedDeviceText(const vulkan::DeviceCapabilities& capabilities)
{
    std::ostringstream out;
    out << "Unsupported Vulkan RT device\n";
    out << "This project is RT or nothing. No fake fallback will be used.\n\n";
    out << vulkan::BuildCapabilityTextReport(capabilities);
    return out.str();
}

std::string BuildDeveloperOverlayText(const DeveloperOverlaySnapshot& snapshot)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(1);
    out << "DEV  " << snapshot.buildIdentity << "  shader " << snapshot.shaderIdentity << '\n';
    out << snapshot.gpuName << "  |  Vulkan " << snapshot.vulkanApi << '\n';
    out << "RT " << snapshot.rtMode << "  |  presented " << (snapshot.presented ? "YES" : "NO")
        << "  |  scale " << std::lround(snapshot.renderScale * 100.0f) << "%\n";
    out << "FRAME " << snapshot.frameTimeMs << " ms  |  " << snapshot.fps << " FPS  |  "
        << snapshot.internalWidth << 'x' << snapshot.internalHeight << " -> "
        << snapshot.presentationWidth << 'x' << snapshot.presentationHeight << '\n';
    out << "SCENE " << snapshot.routeZone << "  |  lantern " << snapshot.lanternPhase
        << "  |  " << snapshot.selectedEnemy << ' ' << snapshot.encounterPhase;
    if (snapshot.enemyHealth >= 0)
    {
        out << " hp " << snapshot.enemyHealth;
    }
    out << '\n';
    out << "AS " << snapshot.blasCount << " BLAS / " << snapshot.tlasCount << " TLAS / "
        << snapshot.tlasInstanceCount << " inst  |  skinned " << snapshot.activeSkinnedEnemies << '\n';
    out << "MAT " << snapshot.materialEncoding;
    return out.str();
}

} // namespace horde::ui
