#include "graphics/EntryMenuScene.h"
#include <iostream>
#include <limits>

int main()
{
    using namespace horde::graphics;
    using namespace horde::gameplay::items;
    const auto content = MakeEntryMenuDescription();
    if (content.worldQuads.size() < 60 || !content.waterQuads.empty())
        return 1;
    EntryMenuSession menu;
    auto flame = IdentityHeldItemTransform(), light = flame;
    flame[13] = -.30f;
    light[13] = -.32f;
    menu.ConfigureSockets(flame, light, kEntryMenuLanternScale);
    for (int frame = 0; frame < 30; ++frame)
        menu.Advance(1.0 / 30);
    auto first = menu.Snapshot();
    if (first.tick != 60 || first.fire.worldFromFlame == first.fire.worldFromLight ||
        first.pendulum.worldFromBody == first.hinge)
        return 2;
    const auto expected = MultiplyHeldItemTransforms(
        first.pendulum.worldFromBody,
        HeldItemTransform{{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}});
    if (std::abs(first.fire.worldFromFlame[13] - MultiplyHeldItemTransforms(expected, flame)[13]) >
        1e-5f)
        return 3;
    menu.Pause(true);
    menu.Advance(20);
    menu.Advance(std::numeric_limits<double>::quiet_NaN());
    if (menu.Snapshot().tick != first.tick)
        return 4;
    menu.Pause(false);
    menu.ShowSidePage(true);
    menu.Advance(.25);
    menu.Advance(.25);
    if (std::abs(menu.Snapshot().camera.x + .60f) > 1e-5f)
        return 5;
    menu.SetReducedMotion(true);
    menu.ShowSidePage(false);
    menu.Advance(1.0 / 60);
    if (menu.Snapshot().camera.x != 0)
        return 6;
    menu.Play();
    menu.Advance(.10);
    if (!menu.ReadyToPlay() || menu.Snapshot().fade != 1)
        return 7;
    menu.Reset();
    if (menu.ReadyToPlay() || menu.Snapshot().tick != 0)
        return 8;
    menu.ShowSidePage(true);
    menu.Pause(true);
    menu.Reset();
    menu.Advance(1.0 / 60);
    std::string diagnostic;
    if (menu.Snapshot().tick != 1 || menu.Snapshot().camera.x != 0 ||
        !ValidateHeldItemSocketTransform(menu.Snapshot().fire.worldFromFlame, diagnostic))
        return 9;
    if (menu.Snapshot().pendulum.worldFromBody != menu.Snapshot().hinge)
        return 10;
    menu.SetReducedMotion(false);
    float maximumSway = 0;
    for (unsigned tick = 0; tick < 3600; ++tick)
    {
        menu.Advance(1.0 / 60);
        maximumSway = std::max(maximumSway, std::abs(menu.Snapshot().pendulum.strafeAngleRadians));
    }
    // Visible gentle motion, bounded independently of the physical pendulum's
    // much wider gameplay limits. Reduced motion must restore a static pivot.
    if (maximumSway < .01f || maximumSway > .12f)
    {
        std::cerr << "Entry sway outside review bounds: " << maximumSway << '\n';
        return 11;
    }
    menu.SetReducedMotion(true);
    menu.Advance(1.0 / 60);
    if (menu.Snapshot().pendulum.worldFromBody != menu.Snapshot().hinge)
        return 12;
    std::cout
        << "entry content, physical socket motion, pause, pan, reduced motion and Play fade pass\n";
}
