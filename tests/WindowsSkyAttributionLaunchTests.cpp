#include "platform/windows/WindowsSkyAttributionLaunch.h"
#include <iostream>
#include <vector>
using namespace horde::platform::windows;
int main()
{
    unsigned failures=0,checks=0;
    const auto check=[&](bool value,const char* reason) {
        ++checks;if(!value){++failures;std::cerr<<"FAIL: "<<reason<<'\n';}
    };
    const std::vector<std::wstring_view> valid{L"--development-rescue-journey",L"--capture-showcase",L"fresh",
        L"--development-checkpoint",L"chapter-landing-north",L"--capture-sky-attribution",L"directional"};
    auto result=ParseSkyAttributionLaunch(valid,true);
    check(result.requested && result.error.empty() && result.mode==SkyAttributionMode::DirectionalOnly,
          "explicit frozen Debug chapter capture must opt into directional-only lighting");
    auto off=valid;off.back()=L"off";
    result=ParseSkyAttributionLaunch(off,true);
    check(result.requested && result.error.empty() && result.mode==SkyAttributionMode::Disabled,
          "sky-off attribution must retain an explicit identity");
    check(!ParseSkyAttributionLaunch({},true).requested,"ordinary launch must retain production lighting");
    check(!ParseSkyAttributionLaunch(valid,false).error.empty(),"Release must reject diagnostic light mutation");
    for(unsigned change=0;change<7;++change) {
        auto bad=valid;
        if(change==0) bad.erase(bad.begin());
        if(change==1) bad[1]=L"--other";
        if(change==2) bad[3]=L"--other";
        if(change==3) bad[4]=L"opening";
        if(change==4) bad.back()=L"lower";
        if(change==5) bad.pop_back();
        if(change==6) {bad.push_back(L"--capture-sky-attribution");bad.push_back(L"off");}
        check(!ParseSkyAttributionLaunch(bad,true).error.empty(),
              "missing authority, missing values or duplicate/unknown modes must reject");
    }
    auto moving=valid;moving.push_back(L"--validate-chapter-roundtrip");
    check(!ParseSkyAttributionLaunch(moving,true).error.empty(),"attribution may not mutate a moving route");
    auto performance=valid;performance.push_back(L"--benchmark-showcase");
    check(!ParseSkyAttributionLaunch(performance,true).error.empty(),"attribution may not taint a benchmark");
    std::cout<<checks<<" sky-attribution launch checks, "<<failures<<" failures\n";
    return failures?1:0;
}
