#include "audio/MusicPlaybackSession.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace horde::audio
{

bool MusicPlaybackInbox::Publish(
    const gameplay::simulation::SimulationSnapshot& snapshot,
    const std::span<const gameplay::simulation::GameplayEvent> events,
    const std::uint64_t lifetimeToken, const std::uint64_t resetToken,
    const bool externallySuspended)
{
    const std::lock_guard lock(mutex_);
    const bool newLifetime = !pending_.available || pending_.lifetimeToken != lifetimeToken;
    const bool restart = newLifetime || resetToken_ != resetToken ||
        pending_.snapshot.retryGeneration != snapshot.retryGeneration ||
        pending_.snapshot.lastConsumedRouteResetSequence != snapshot.lastConsumedRouteResetSequence;
    if (!restart && snapshot.tickIndex < pending_.snapshot.tickIndex)
    {
        return false; // An old publication cannot roll back the live phase.
    }
    if (restart)
    {
        ++pending_.restartEpoch;
        pending_.eventCount = 0u;
    }
    if (newLifetime) lastEventSequence_ = 0u;
    resetToken_ = resetToken;
    pending_.snapshot = snapshot;
    pending_.lifetimeToken = lifetimeToken;
    pending_.externallySuspended = externallySuspended;
    pending_.available = true;
    for (const auto& event : events)
    {
        if (event.sequence == 0u || event.sequence <= lastEventSequence_) continue;
        lastEventSequence_ = event.sequence;
        if (pending_.eventCount == pending_.events.size())
        {
            ++pending_.overflowCount;
            continue;
        }
        pending_.events[pending_.eventCount++] = event;
    }
    return true;
}

MusicPlaybackInput MusicPlaybackInbox::Take()
{
    const std::lock_guard lock(mutex_);
    MusicPlaybackInput result = pending_;
    pending_.eventCount = 0u;
    return result;
}

MusicPlaybackSession::MusicPlaybackSession(const std::span<const MusicPcmClip> clips) noexcept
    : stream_(clips)
{
}

MusicPcmStatus MusicPlaybackSession::Observe(const MusicPlaybackInput& input) noexcept
{
    if (!input.available || input.eventCount > input.events.size() || input.overflowCount != 0u ||
        input.snapshot.eventQueueOverflowCount != 0u)
    {
        return MusicPcmStatus::InvalidSelection;
    }
    if (!epochInitialized_ || restartEpoch_ != input.restartEpoch)
    {
        director_.Reset();
        stream_.Reset();
        generatedFrames_ = 0u;
        restartEpoch_ = input.restartEpoch;
        epochInitialized_ = true;
    }
    selection_ = director_.Update(input.snapshot,
        std::span(input.events).first(input.eventCount),
        static_cast<double>(generatedFrames_) / kMusicPcmSampleRate,
        input.lifetimeToken, input.externallySuspended);
    return stream_.SetSelection(selection_);
}

MusicPcmStatus MusicPlaybackSession::Render(
    const MusicPlaybackInput& input, const std::span<float> output) noexcept
{
    std::fill(output.begin(), output.end(), 0.0f);
    if ((output.size() & 1u) != 0u) return MusicPcmStatus::InvalidOutput;
    const std::size_t totalFrames = output.size() / 2u;
    if (totalFrames > std::numeric_limits<std::uint64_t>::max() - generatedFrames_)
        return MusicPcmStatus::InvalidOutput;
    auto status = Observe(input);
    if (status != MusicPcmStatus::Ok) return status;
    std::size_t offset = 0u;
    while (offset < totalFrames)
    {
        std::size_t count = totalFrames - offset;
        if (selection_.cue == MusicCue::C || selection_.cue == MusicCue::G)
        {
            const double bodyFrames = kMusicPcmAssets[static_cast<std::size_t>(selection_.cue)].bodyFrames;
            // Only sub-sample roundoff is removed; a whole remaining frame must
            // still render before a one-shot completes. Director uses the same
            // tiny boundary epsilon, far below one 48 kHz sample.
            const double remaining = std::ceil(bodyFrames -
                selection_.positionSeconds * kMusicPcmSampleRate - 0.00001);
            if (remaining <= 0.0) return MusicPcmStatus::InvalidSelection;
            count = std::min(count, static_cast<std::size_t>(remaining));
        }
        status = stream_.Render(output.subspan(offset * 2u, count * 2u));
        if (status != MusicPcmStatus::Ok) return status;
        generatedFrames_ += count;
        offset += count;
        // Observe at the exact body boundary, even when it is the end of this
        // buffer. Repeated event sequences are already deduplicated by Director.
        status = Observe(input);
        if (status != MusicPcmStatus::Ok) return status;
    }
    return status;
}

} // namespace horde::audio
