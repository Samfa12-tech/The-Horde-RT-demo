#pragma once
#include "gameplay/simulation/SimulationSnapshot.h"
#include <algorithm>
#include <cmath>

namespace horde::audio {
// Transient presentation only. One fixed-step owner publishes this multiplier
// for all beds; output workers never integrate a second envelope or save it.
class DialogueDucking {
public:
    static constexpr float kDuckedGain=.65f;
    static constexpr float kAttackSeconds=.20f,kHoldSeconds=.35f,kReleaseSeconds=.90f;
    void Step(bool speaking,float activeSeconds) noexcept {
        if(!std::isfinite(activeSeconds) || activeSeconds<=0) return;
        const float dt=std::min(activeSeconds,.05f);
        if(speaking) {
            quiet_=0;
            if(gain_==kDuckedGain) return;
            if(phase_!=Phase::Attack) Begin(Phase::Attack);
            Fade(dt,kDuckedGain,kAttackSeconds);
            return;
        }
        if(gain_==1.f) { Reset();return; }
        // Do not continue deepening an interrupted attack after speech stopped.
        // A following line starts from this gain without a unity-volume pulse.
        if(phase_==Phase::Attack) phase_=Phase::Hold;
        float releaseDelta=dt;
        if(quiet_<kHoldSeconds) {
            const float total=quiet_+dt;
            quiet_=std::min(total,kHoldSeconds);
            releaseDelta=std::max(0.f,total-kHoldSeconds);
        }
        if(quiet_<kHoldSeconds) return;
        if(phase_!=Phase::Release) Begin(Phase::Release);
        Fade(releaseDelta,1.f,kReleaseSeconds);
    }
    void Reset() noexcept { gain_=from_=1.f;quiet_=elapsed_=0;phase_=Phase::Idle; }
    float Gain() const noexcept { return gain_; }
private:
    enum class Phase { Idle,Attack,Hold,Release };
    void Begin(Phase phase) noexcept { phase_=phase;from_=gain_;elapsed_=0; }
    void Fade(float dt,float target,float duration) noexcept {
        elapsed_=std::min(duration,elapsed_+dt);
        const float t=elapsed_/duration,smooth=t*t*(3.f-2.f*t);
        gain_=elapsed_>=duration?target:std::clamp(from_+(target-from_)*smooth,kDuckedGain,1.f);
    }
    Phase phase_=Phase::Idle;
    float gain_=1.f,from_=1.f,quiet_=0,elapsed_=0;
};

inline float DialogueBedGain(const gameplay::simulation::SimulationSnapshot& s) noexcept {
    const float value=s.dialogueBedGain;
    if(!s.developmentRescueJourney || !s.playerAlive || !std::isfinite(value) ||
       value<DialogueDucking::kDuckedGain || value>1.f) return 1.f;
    return value;
}
} // namespace horde::audio
