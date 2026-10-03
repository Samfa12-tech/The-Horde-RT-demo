#include "graphics/GraphicsPreviewPerformance.h"
#include <cmath>
#include <limits>
int main() {
    using namespace horde::graphics;
    GraphicsPreviewPerformance p;
    p.BeginScope(4);
    if (p.Snapshot().meanGpuMilliseconds || p.Snapshot().trackedDeviceLocalBytes) return 1;
    if (p.RecordFrame(0.0, 1, {}, true, false)) return 2;
    if (p.RecordFrame(0.1, -1, {}, true, false)) return 3;
    p.SetTrackedAllocations(1000, 500); // Keep independent overlapping classifications.
    p.RecordFrame(1.0/30.0, 10, 6.0, true, false);
    p.RecordFrame(1.0/30.0, 12, {}, false, false);
    p.RecordFrame(0.5, 490, {}, true, true); // Real transition stall stays in throughput and graph.
    auto s = p.Snapshot();
    if (s.scopeEpoch != 4 || s.sampleCount != 3 || s.successfulPresentCount != 2 || s.transitionCount != 1) return 4;
    if (std::abs(s.successfulPresentsPerSecond - 2.0/(0.5+2.0/30.0)) > 1e-9) return 5;
    if (std::abs(s.meanLoopMilliseconds - 1000.0/30.0) > 1e-9 || s.meanCpuRenderMilliseconds != 11) return 6;
    if (s.meanGpuMilliseconds != 6 || s.trackedDeviceLocalBytes != 1000 || s.trackedHostVisibleBytes != 500) return 7;
    for (unsigned i=0;i<140;++i) p.RecordFrame(0.02, i, std::numeric_limits<double>::quiet_NaN(), true, false);
    s = p.Snapshot();
    if (s.sampleCount != 128 || s.samples[0].cpuRenderMilliseconds != 12 || s.samples[127].cpuRenderMilliseconds != 139) return 8;
    if (s.meanGpuMilliseconds || std::abs(s.successfulPresentsPerSecond - 50.0)>1e-9) return 9;
    p.BeginScope(5);
    s = p.Snapshot();
    if (s.scopeEpoch != 5 || s.sampleCount || s.meanGpuMilliseconds || s.trackedDeviceLocalBytes) return 10;
    return 0;
}
