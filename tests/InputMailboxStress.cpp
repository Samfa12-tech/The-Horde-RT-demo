#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>

#include "gameplay/simulation/InputMailbox.h"

int main()
{
    using namespace horde::gameplay::simulation;
    constexpr std::uint64_t kPublications = 100000u;
    InputMailbox mailbox;
    std::atomic<bool> writerFinished{false};
    std::atomic<bool> coherent{true};

    std::thread writer([&mailbox, &writerFinished]()
    {
        for (std::uint64_t sequence = 1u; sequence <= kPublications; ++sequence)
        {
            const float value = static_cast<float>(sequence);
            InputSnapshot input;
            input.moveForward = value;
            input.moveStrafe = -value;
            input.yawRadians = value * 2.0f;
            input.pitchRadians = value * -3.0f;
            input.lanternStrength = value + 0.25f;
            input.paused = (sequence & 1u) != 0u;
            input.damageEnabled = !input.paused;
            input.commands.attack = sequence;
            input.commands.parry = sequence + 7u;
            input.commands.routeReset = sequence + 11u;
            input.commands.retry = sequence + 29u;
            mailbox.Publish(input);
        }
        writerFinished.store(true, std::memory_order_release);
    });

    std::thread reader([&mailbox, &writerFinished, &coherent]()
    {
        std::uint64_t latest = 0u;
        while (!writerFinished.load(std::memory_order_acquire) || latest < kPublications)
        {
            const PublishedInput published = mailbox.ConsumeLatest();
            latest = published.publicationSequence;
            const InputSnapshot& input = published.snapshot;
            if (latest == 0u)
            {
                continue;
            }
            const float value = static_cast<float>(latest);
            const bool expectedPaused = (latest & 1u) != 0u;
            const bool matches =
                input.moveForward == value &&
                input.moveStrafe == -value &&
                input.yawRadians == value * 2.0f &&
                input.pitchRadians == value * -3.0f &&
                input.lanternStrength == value + 0.25f &&
                input.paused == expectedPaused &&
                input.damageEnabled == !expectedPaused &&
                input.commands.attack == latest &&
                input.commands.parry == latest + 7u &&
                input.commands.routeReset == latest + 11u &&
                input.commands.retry == latest + 29u;
            if (!matches)
            {
                coherent.store(false, std::memory_order_release);
                return;
            }
        }
    });

    writer.join();
    reader.join();
    const PublishedInput finalPublication = mailbox.ConsumeLatest();
    if (!coherent.load(std::memory_order_acquire) ||
        finalPublication.publicationSequence != kPublications ||
        finalPublication.snapshot.commands.attack != kPublications ||
        finalPublication.snapshot.commands.parry != kPublications + 7u)
    {
        std::cerr << "Input mailbox stress failed: reader observed a mixed or missing publication.\n";
        return 1;
    }
    std::cout << "Input mailbox retained coherent POD snapshots across "
              << kPublications << " publications.\n";
    return 0;
}
