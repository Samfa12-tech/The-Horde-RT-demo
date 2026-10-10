#include "platform/windows/WindowsVitalityHudLayout.h"
#include <iostream>
using namespace horde::platform::windows;
int main() {
    int failures=0;
    const auto check=[&](bool ok,const char* why){if(!ok){++failures;std::cerr<<why<<'\n';}};
    check(VitalityHudColumns(94,23,5,8)==2,
        "125-percent legacy sizing reproduction no longer describes the clipping defect");
    for(int dpi:{96,120,144,168,192,216,240,288})
        for(int count:{0,1,2,3,4,8}) for(int width:{80,160,320,1232}) {
            const auto layout=LayoutWindowsVitalityHud(width,count,dpi);
            const int paintedColumns=VitalityHudColumns(layout.width,layout.heartWidth,layout.gap,layout.insetX);
            check(paintedColumns==layout.columns,"paint and layout disagree about columns");
            for(int heart=0;heart<count;++heart) {
                const int x=layout.insetX+(heart%paintedColumns)*(layout.heartWidth+layout.gap);
                const int y=layout.insetY+(heart/paintedColumns)*(layout.heartHeight+layout.gap);
                check(x>=0&&x+layout.heartWidth<=layout.width-layout.insetX,
                    "heart exceeds horizontal paint bounds");
                check(y>=0&&y+layout.heartHeight<=layout.height-layout.insetY,
                    "heart is wrapped into a clipped row");
            }
        }
    const auto owner=LayoutWindowsVitalityHud(1232,3,120);
    check(owner.columns==3&&owner.rows==1&&owner.width==95,"three hearts must fit at owner-like 125-percent DPI");
    std::cout<<"Vitality layout: 192 DPI/count/width combinations, failures="<<failures<<'\n';
    return failures?1:0;
}
