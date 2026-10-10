#include "scene/TombWallRecess.h"
#include "scene/TombDressingContact.h"
#include <cmath>
#include <iostream>

int main() {
    using namespace horde::scene;
    bool ok = true;
    const auto check = [&](bool value, const char* label) { if (!value) {ok = false; std::cerr << label << '\n';} };
    const auto contains = [](const auto& panels, float z, float y) {
        for (const auto& p : panels)
            if (z > p.minimumZ && z < p.maximumZ && y > p.minimumY && y < p.maximumY) return true;
        return false;
    };
    for (const bool dressing : {false, true}) {
        const auto visible = TombWallPanels(-1.85f, -6.4f, 3.4f, -.95f, 1.35f, dressing);
        const auto backing = TombWallPanels(-1.85f, -6.47f, 3.4f, -1.02f, 1.42f, dressing);
        check(contains(visible, -.7f, .2f) == !dressing && contains(backing, -.7f, .2f) == !dressing,
              "visible wall and hidden shell share the fitted first niche aperture");
        check(contains(visible, -2.6f, .2f) == !dressing && contains(backing, -2.6f, .2f) == !dressing,
              "visible wall and hidden shell share the fitted second niche aperture");
        check(contains(visible, -1.25f, .2f) && contains(visible, -.7f, -.8f) && contains(visible, -.7f, 1.1f),
              "portal margin, lower masonry and lintel stay sealed around the smaller opening");
        check(contains(visible, -.7f, .75f) && contains(backing, -.7f, .75f),
              "wall above the reduced niche remains a solid stone bridge");
        for (const auto& panel : visible)
            check(std::isfinite(panel.minimumZ) && panel.maximumZ > panel.minimumZ && panel.maximumY > panel.minimumY,
                  "all retained wall panels have finite nonzero extent");
    }
    const auto outer = TombWallPanels(6.f, -15.2f, -10.f, -.95f, 1.35f, true);
    check(!contains(outer, -12.4f, .2f) && contains(outer, -14.f, .2f) && contains(outer, -10.5f, .2f) &&
          contains(outer, -12.4f, .75f),
          "only selected outer wall aperture opens; neighboring route walls remain");
    const auto untouched = TombWallPanels(1.85f, -6.4f, 3.4f, -.95f, 1.35f, true);
    check(untouched.size() == 1 && contains(untouched, -.7f, .2f), "right grate wall is untouched");
    check(!TombDressingMovementClear(-36.f, -17.9f, -36.45f, -17.9f),
          "visible broken urn blocks a capsule sweep, not just its endpoint");
    check(!TombDressingMovementClear(-31.8f, -17.95f, -30.3f, -17.95f),
          "long movement cannot tunnel through offering bowl footprint");
    check(TombDressingMovementClear(-34.f, -16.f, -33.f, -16.f) &&
          TombDressingMovementClear(-1.3f, -1.f, -.3f, -1.f),
          "Keeper combat and opening navigation lanes stay clear");
    return ok ? 0 : 1;
}
