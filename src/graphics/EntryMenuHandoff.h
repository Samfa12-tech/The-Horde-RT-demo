#pragma once

#include "graphics/GraphicsSettings.h"
#include <cstdint>
#include <limits>

namespace horde::graphics
{
struct EntryMenuControls
{
    bool enabled = false, sidePage = false, reducedMotion = false, play = false;
    std::uint64_t generation = 0u, resetSerial = 0u;
};

inline bool CurrentEntryMenuControls(const EntryMenuControls& controls,
                                    std::uint64_t generation) noexcept
{
    return generation != 0u && (controls.generation == 0u || controls.generation == generation);
}

// Render-owner authority. A completed fade alone cannot release gameplay: the
// black Entry frame and the replacement Showcase frame must both present.
class EntryMenuHandoff
{
public:
    bool Observe(const EntryMenuControls& controls, std::uint64_t generation) noexcept
    {
        if (!CurrentEntryMenuControls(controls, generation)) return false;
        enabled_ = controls.enabled;
        play_ = controls.play;
        if (resetSerial_ == controls.resetSerial) return false;
        resetSerial_ = controls.resetSerial;
        handedOff_ = failed_ = loading_ = awaitingPresentation_ = showcasePresented_ = false;
        failureFallback_ = GraphicsScene::EntryMenu;
        return true;
    }
    GraphicsScene DesiredScene(bool previewEnabled) const noexcept
    {
        if (previewEnabled) return GraphicsScene::Preview;
        if (failed_) return failureFallback_;
        return
            enabled_ && !handedOff_ ? GraphicsScene::EntryMenu : GraphicsScene::Showcase;
    }
    void BeginLoad() noexcept
    {
        loading_ = true;
        awaitingPresentation_ = showcasePresented_ = false;
    }
    void EndLoad() noexcept
    {
        loading_ = false;
        awaitingPresentation_ = true; // Resource readiness still needs a current output frame.
    }
    void FailLoad(const GraphicsScene restoredScene = GraphicsScene::EntryMenu) noexcept
    {
        loading_ = awaitingPresentation_ = false;
        failed_ = true;
        handedOff_ = showcasePresented_ = false;
        failureFallback_ = restoredScene;
    }
    void Presented(GraphicsScene scene, bool blackEntry) noexcept
    {
        if (failed_ || loading_) return;
        awaitingPresentation_ = false;
        if (!enabled_) return;
        if (scene == GraphicsScene::EntryMenu && play_ && blackEntry) handedOff_ = true;
        if (scene == GraphicsScene::Showcase && handedOff_) showcasePresented_ = true;
    }
    int Phase() const noexcept
    {
        if (failed_) return 4;
        if (loading_ || awaitingPresentation_) return 2;
        if (showcasePresented_) return 3;
        return enabled_ && play_ ? 1 : 0;
    }
    bool Failed() const noexcept { return failed_; }
    bool PlayRequested() const noexcept { return enabled_ && play_ && !failed_; }
    std::uint64_t ResetSerial() const noexcept { return resetSerial_; }
private:
    std::uint64_t resetSerial_ = std::numeric_limits<std::uint64_t>::max();
    GraphicsScene failureFallback_ = GraphicsScene::EntryMenu;
    bool enabled_ = false, play_ = false, handedOff_ = false;
    bool failed_ = false, loading_ = false, awaitingPresentation_ = false, showcasePresented_ = false;
};
} // namespace horde::graphics
