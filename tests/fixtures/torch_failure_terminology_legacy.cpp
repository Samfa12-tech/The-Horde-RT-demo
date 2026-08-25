#include "gameplay/ShowcaseGameplay.h"
#include "gameplay/simulation/InputSnapshot.h"
#include "gameplay/simulation/SimulationSnapshot.h"

int main()
{
    horde::gameplay::LanternSequence sequence;
    horde::gameplay::LanternSnapshot snapshot = sequence.Snapshot();
    snapshot.phase = horde::gameplay::LanternPhase::Held;

    horde::gameplay::simulation::InputSnapshot input;
    input.lanternStrength = 1.8f;
    horde::gameplay::simulation::SimulationSnapshot output;
    output.lanternStrength = input.lanternStrength;
    output.lantern = snapshot;
    return output.lantern.phase == horde::gameplay::LanternPhase::Held ? 0 : 1;
}
