#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace horde::gameplay::simulation {
// Continuous intent complements immediate attack edges; it never generates a cut.
class AttackHoldToSheath {
public:
    static constexpr float kHoldSeconds=.75f;
    void Reset(){seconds_=0;source_=0;blocked_=consumed_=false;}
    void Cancel(){seconds_=0;blocked_=true;}
    bool Step(bool held,std::uint8_t source,bool eligible,bool safe,float seconds) {
        if(source!=source_) {
            if(source_!=0)Cancel();
            source_=source;seconds_=0;
        }
        if(!eligible){Cancel();return false;}
        if(!held){seconds_=0;blocked_=consumed_=false;return false;}
        if(blocked_ || consumed_ || source==0 || !std::isfinite(seconds) || seconds<=0)return false;
        seconds_=std::min(kHoldSeconds,seconds_+std::min(seconds,.05f));
        if(seconds_+.000001f<kHoldSeconds || !safe)return false;
        consumed_=true;seconds_=0;return true;
    }
    float Progress() const{return seconds_/kHoldSeconds;}
private:
    float seconds_=0;std::uint8_t source_=0;bool blocked_=false,consumed_=false;
};
}
