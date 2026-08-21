#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <latch>
#include <thread>

#include "gameplay/simulation/InputMailbox.h"

namespace
{

using horde::gameplay::simulation::InputSnapshot;
using horde::gameplay::simulation::PublishedInput;

constexpr std::uint64_t kPublications = 125000u;
constexpr std::size_t kReaderCount = 4u;
constexpr std::chrono::seconds kCompletionDeadline{30};

InputSnapshot MakeSnapshot(std::uint64_t sequence)
{
    const float value = static_cast<float>(sequence);
    InputSnapshot input;
    input.moveForward = value;
    input.moveStrafe = -value;
    input.yawRadians = value * 2.0f;
    input.pitchRadians = value * -3.0f;
    input.lanternStrength = value + 0.25f;
    input.authoritativePlayerX = value + 0.5f;
    input.authoritativePlayerZ = value - 0.5f;
    input.paused = (sequence & 1u) != 0u;
    input.damageEnabled = !input.paused;
    input.hasAuthoritativePlayerPose = (sequence & 2u) != 0u;
    input.commands.attack = sequence;
    input.commands.parry = sequence + 7u;
    input.commands.routeReset = sequence + 11u;
    input.commands.retry = sequence + 29u;
    return input;
}

bool MatchesSequence(const PublishedInput& published)
{
    const std::uint64_t sequence = published.publicationSequence;
    if (sequence == 0u)
    {
        return true;
    }

    const InputSnapshot expected = MakeSnapshot(sequence);
    const InputSnapshot& actual = published.snapshot;
    return actual.moveForward == expected.moveForward &&
           actual.moveStrafe == expected.moveStrafe &&
           actual.yawRadians == expected.yawRadians &&
           actual.pitchRadians == expected.pitchRadians &&
           actual.lanternStrength == expected.lanternStrength &&
           actual.authoritativePlayerX == expected.authoritativePlayerX &&
           actual.authoritativePlayerZ == expected.authoritativePlayerZ &&
           actual.paused == expected.paused &&
           actual.damageEnabled == expected.damageEnabled &&
           actual.hasAuthoritativePlayerPose == expected.hasAuthoritativePlayerPose &&
           actual.commands.attack == expected.commands.attack &&
           actual.commands.parry == expected.commands.parry &&
           actual.commands.routeReset == expected.commands.routeReset &&
           actual.commands.retry == expected.commands.retry;
}

} // namespace

int main()
{
    using namespace horde::gameplay::simulation;
    using Clock = std::chrono::steady_clock;

    InputMailbox mailbox;
    std::atomic<bool> writerFinished{false};
    std::atomic<bool> coherent{true};
    std::atomic<bool> monotonic{true};
    std::atomic<bool> timedOut{false};
    std::atomic<bool> workCompleted{false};
    std::array<std::atomic<std::uint64_t>, kReaderCount> finalObserved{};
    std::latch startGate(static_cast<std::ptrdiff_t>(kReaderCount + 1u));
    const Clock::time_point deadline = Clock::now() + kCompletionDeadline;
    std::thread watchdog([&]()
    {
        while (!workCompleted.load(std::memory_order_acquire))
        {
            if (Clock::now() >= deadline)
            {
                std::cerr << "Input mailbox stress timed out before all threads completed.\n";
                std::_Exit(1);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
    });

    std::thread writer([&]()
    {
        startGate.count_down();
        startGate.wait();

        for (std::uint64_t sequence = 1u; sequence <= kPublications; ++sequence)
        {
            if (!coherent.load(std::memory_order_acquire) ||
                !monotonic.load(std::memory_order_acquire) ||
                Clock::now() >= deadline)
            {
                timedOut.store(Clock::now() >= deadline, std::memory_order_release);
                break;
            }
            mailbox.Publish(MakeSnapshot(sequence));
        }
        writerFinished.store(true, std::memory_order_release);
    });

    std::array<std::thread, kReaderCount> readers;
    for (std::size_t readerIndex = 0u; readerIndex < readers.size(); ++readerIndex)
    {
        readers[readerIndex] = std::thread([&, readerIndex]()
        {
            startGate.count_down();
            startGate.wait();

            std::uint64_t latest = 0u;
            for (;;)
            {
                if (writerFinished.load(std::memory_order_acquire) && latest >= kPublications)
                {
                    finalObserved[readerIndex].store(latest, std::memory_order_release);
                    return;
                }
                if (!coherent.load(std::memory_order_acquire) ||
                    !monotonic.load(std::memory_order_acquire) ||
                    Clock::now() >= deadline)
                {
                    timedOut.store(Clock::now() >= deadline, std::memory_order_release);
                    finalObserved[readerIndex].store(latest, std::memory_order_release);
                    return;
                }

                const PublishedInput published = mailbox.ConsumeLatest();
                if (published.publicationSequence < latest)
                {
                    monotonic.store(false, std::memory_order_release);
                    finalObserved[readerIndex].store(latest, std::memory_order_release);
                    return;
                }
                latest = published.publicationSequence;
                if (!MatchesSequence(published))
                {
                    coherent.store(false, std::memory_order_release);
                    finalObserved[readerIndex].store(latest, std::memory_order_release);
                    return;
                }
            }
        });
    }

    writer.join();
    for (std::thread& reader : readers)
    {
        reader.join();
    }
    workCompleted.store(true, std::memory_order_release);
    watchdog.join();

    const PublishedInput finalPublication = mailbox.ConsumeLatest();
    bool everyReaderSawFinal = true;
    for (const std::atomic<std::uint64_t>& observed : finalObserved)
    {
        everyReaderSawFinal = everyReaderSawFinal &&
                              observed.load(std::memory_order_acquire) == kPublications;
    }

    if (!coherent.load(std::memory_order_acquire) ||
        !monotonic.load(std::memory_order_acquire) ||
        timedOut.load(std::memory_order_acquire) ||
        !everyReaderSawFinal ||
        finalPublication.publicationSequence != kPublications ||
        !MatchesSequence(finalPublication))
    {
        std::cerr << "Input mailbox stress failed: readers did not observe coherent, monotonic final publications.\n";
        return 1;
    }

    std::cout << "Input mailbox retained coherent POD snapshots across "
              << kPublications << " publications for " << kReaderCount
              << " concurrent readers.\n";
    return 0;
}
