#pragma once
#include <algorithm>
#include <cmath>
namespace horde::gameplay::dialogue {
enum class SubtitlePosition : int { Auto,Top,Bottom };
struct SubtitleRequest {
    int width=1280,height=720,margin=18,topReserved=68,bottomReserved=100;
    int fontPixels=22,lines=1; bool touch=false;
    SubtitlePosition position=SubtitlePosition::Auto;
};
struct SubtitleLayout { int x=0,y=0,width=0,height=0;bool fits=false;
    SubtitlePosition region=SubtitlePosition::Bottom; };
inline SubtitleLayout LayoutSubtitle(const SubtitleRequest& r) {
    SubtitleLayout out;
    out.region=r.position==SubtitlePosition::Auto?(r.touch?SubtitlePosition::Top:SubtitlePosition::Bottom):r.position;
    const int margin=std::max(8,r.margin),available=std::max(0,r.height-r.topReserved-r.bottomReserved-2*margin);
    out.width=std::max(0,std::min(900,r.width-2*margin));out.x=(r.width-out.width)/2;
    out.height=std::max(1,r.lines)*(std::max(12,r.fontPixels)+7)+20;
    out.fits=out.width>=120&&out.height<=available;
    out.y=out.region==SubtitlePosition::Top?r.topReserved+margin:
        r.height-r.bottomReserved-margin-out.height;
    // Never move into reserved controls to make an impossible setting appear valid.
    return out;
}
} // namespace horde::gameplay::dialogue
