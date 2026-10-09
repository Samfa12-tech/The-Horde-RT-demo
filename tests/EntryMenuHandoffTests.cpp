#include "graphics/EntryMenuHandoff.h"
#include <iostream>

int main()
{
    using namespace horde::graphics;
    EntryMenuHandoff state;
    EntryMenuControls controls;
    controls.enabled = true;
    if (!state.Observe(controls, 7) || state.DesiredScene(false) != GraphicsScene::EntryMenu)
        return 1; // Generation zero admits cold startup before surface acceptance.
    controls.generation = 6;
    controls.play = true;
    if (state.Observe(controls, 7) || state.PlayRequested()) return 2;
    controls.generation = 7;
    state.Observe(controls, 7);
    if (!state.PlayRequested() || state.Phase() != 1) return 3;
    state.Presented(GraphicsScene::EntryMenu, false);
    if (state.DesiredScene(false) != GraphicsScene::EntryMenu) return 4;
    // Preview presentation cannot finish an Entry fade or acknowledge Play.
    state.Presented(GraphicsScene::Preview, true);
    if (state.DesiredScene(true) != GraphicsScene::Preview || state.Phase() != 1) return 5;
    state.Presented(GraphicsScene::EntryMenu, true);
    if (state.DesiredScene(false) != GraphicsScene::Showcase || state.Phase() == 3) return 6;
    state.BeginLoad();
    state.Presented(GraphicsScene::Showcase, false);
    if (state.Phase() != 2) return 7;
    state.EndLoad();
    if (state.Phase() != 2) return 13;
    state.Presented(GraphicsScene::Showcase, false);
    if (state.Phase() != 3) return 8;
    // Cancellation on interruption invalidates a previously presented tomb.
    controls.play = false;
    ++controls.resetSerial;
    if (!state.Observe(controls, 7) || state.Phase() != 0 ||
        state.DesiredScene(false) != GraphicsScene::EntryMenu) return 9;
    controls.play = true;
    state.Observe(controls, 7);
    state.Presented(GraphicsScene::EntryMenu, true);
    state.BeginLoad();
    state.FailLoad();
    state.Presented(GraphicsScene::Showcase, false);
    if (state.Phase() != 4 || state.PlayRequested() ||
        state.DesiredScene(false) != GraphicsScene::EntryMenu) return 10;
    controls.play = false;
    ++controls.resetSerial;
    state.Observe(controls, 7);
    controls.play = true;
    state.Observe(controls, 7);
    if (state.Phase() != 1 || !state.PlayRequested()) return 11;
    controls.enabled = false;
    controls.play = false;
    ++controls.resetSerial;
    state.Observe(controls, 7);
    if (state.DesiredScene(false) != GraphicsScene::Showcase || state.Phase() != 0) return 12;

    // Back can cancel after the black Entry frame committed the Showcase
    // handoff. A failed return to Entry must latch on the restored Showcase
    // scene instead of retrying the failed load every owner iteration.
    EntryMenuHandoff failedReturn;
    EntryMenuControls returnControls;
    returnControls.enabled = true;
    returnControls.generation = 7;
    if (!failedReturn.Observe(returnControls, 7)) return 14;
    returnControls.play = true;
    failedReturn.Observe(returnControls, 7);
    failedReturn.Presented(GraphicsScene::EntryMenu, true);
    if (failedReturn.DesiredScene(false) != GraphicsScene::Showcase) return 15;

    // Model an already-started scene switch completing after Back resets the
    // handoff: the owner has Showcase, but Entry is desired again.
    failedReturn.BeginLoad();
    returnControls.play = false;
    ++returnControls.resetSerial;
    failedReturn.Observe(returnControls, 7);
    failedReturn.EndLoad();
    failedReturn.Presented(GraphicsScene::Showcase, false);
    if (failedReturn.DesiredScene(false) != GraphicsScene::EntryMenu ||
        failedReturn.Phase() != 0) return 16;

    failedReturn.BeginLoad();
    failedReturn.FailLoad(GraphicsScene::Showcase);
    for (int i = 0; i < 3; ++i)
    {
        failedReturn.Observe(returnControls, 7);
        failedReturn.Presented(GraphicsScene::Showcase, false);
        if (failedReturn.DesiredScene(false) != GraphicsScene::Showcase ||
            failedReturn.Phase() != 4 || failedReturn.PlayRequested()) return 17;
    }

    // A genuine rising Play request during phase 4 advances resetSerial. The
    // failed fallback clears, but old Showcase presentation cannot complete
    // the new attempt; Entry must present black before Showcase can be ACKed.
    returnControls.play = true;
    ++returnControls.resetSerial;
    if (!failedReturn.Observe(returnControls, 7) ||
        failedReturn.DesiredScene(false) != GraphicsScene::EntryMenu ||
        failedReturn.Phase() != 1 || !failedReturn.PlayRequested()) return 18;
    failedReturn.Presented(GraphicsScene::Showcase, false);
    if (failedReturn.DesiredScene(false) != GraphicsScene::EntryMenu ||
        failedReturn.Phase() != 1) return 19;
    failedReturn.BeginLoad();
    failedReturn.EndLoad();
    failedReturn.Presented(GraphicsScene::EntryMenu, false);
    if (failedReturn.DesiredScene(false) != GraphicsScene::EntryMenu) return 20;
    failedReturn.Presented(GraphicsScene::EntryMenu, true);
    if (failedReturn.DesiredScene(false) != GraphicsScene::Showcase ||
        failedReturn.Phase() == 3) return 21;
    failedReturn.BeginLoad();
    failedReturn.EndLoad();
    failedReturn.Presented(GraphicsScene::Showcase, false);
    if (failedReturn.Phase() != 3) return 22;
    std::cout << "Entry generation, two-present Play handoff, loading, cancellation and retry pass\n";
}
