#include "audio/MenuAmbience.h"
#include <cmath>
#include <iostream>
#include <limits>

int main()
{
    using namespace horde::audio;
    MenuChainCreaks motion;
    for (std::uint64_t tick = 1; tick <= 3600; ++tick)
    {
        const float t = static_cast<float>(tick) / 60.f;
        motion.Step(tick, .04f * std::sin(t), .04f * std::cos(t), false);
    }
    if (motion.Serial() < 3 || motion.Serial() > 5) return 1;
    const auto serial = motion.Serial();
    // Reduced motion/Play cannot synthesize new chain cues.
    for (std::uint64_t tick = 3601; tick <= 7200; ++tick)
        motion.Step(tick, .04f * std::sin(static_cast<float>(tick)), .04f * std::cos(static_cast<float>(tick)), true);
    if (motion.Serial() != serial) return 2;
    motion.Step(8000, std::numeric_limits<float>::quiet_NaN(), 1.f, false);
    if (motion.Serial() != serial) return 3;
    motion.Reset();
    motion.Step(480, .04f, 1.f, false);
    motion.Step(481, .04f, -1.f, false);
    if (motion.Serial() != 1) return 4;
    motion.Step(482, .04f, 1.f, false);
    if (motion.Serial() != 1) return 5;
    motion.Step(1201, .04f, -1.f, false);
    if (motion.Serial() != 2) return 6;
    MenuCreakDelivery delivery;
    if (delivery.Observe(900, 2, true)) return 7; // Start establishes a baseline.
    if (delivery.Observe(900, 2, true)) return 8;
    if (!delivery.Observe(1800, 5, true)) return 9; // Catch-up produces at most one cue.
    if (delivery.Observe(1800, 5, true)) return 10;
    delivery.Suspend();
    if (delivery.Observe(2700, 8, true)) return 11; // No focus-return replay.
    if (!delivery.Observe(3600, 9, true)) return 12;
    if (delivery.Observe(1, 0, true)) return 13; // Rebuilt/cancelled menu.
    if (!delivery.Observe(900, 1, true)) return 14;
    if (kMenuFlameGain + kMenuRoomGain + kMenuChainGain >= 1.f) return 15;
    std::cout << "Bounded motion-linked chain cues and reset/focus/catch-up delivery pass\n";
}
