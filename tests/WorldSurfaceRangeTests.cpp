#include "scene/RescueLidFinish.h"
#include "vulkan/raytracing/RtWorldSurfaceRange.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>

int main() {
    using namespace horde::vulkan::raytracing;
    bool ok=true;
    const auto check=[&](bool condition,const char* text) {
        if(!condition) {std::cerr<<"FAIL: "<<text<<'\n';ok=false;}
    };
    RtInstanceMetadata row{}; std::string diagnostic;
    check(SetWorldSurfaceRange(row,27,72,99,diagnostic) && row.primitiveBase==27 &&
        row.primitiveCount==72 && row.flags==0 && row.reserved1==1,
        "procedural subrange ending at buffer boundary is admitted without StaticPbr");
    const auto saved=row;
    check(!SetWorldSurfaceRange(row,27,73,99,diagnostic) &&
        std::memcmp(&row,&saved,sizeof(row))==0,"overflow fails without changing publication");
    check(!SetWorldSurfaceRange(row,27,0,99,diagnostic),"empty surface range fails");
    check(!SetWorldSurfaceRange(row,UINT32_MAX,1,UINT32_MAX,diagnostic),"overflow arithmetic fails");
    row.flags=static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr);
    const auto occupied=row;
    check(!SetWorldSurfaceRange(row,0,12,12,diagnostic) &&
        std::memcmp(&row,&occupied,sizeof(row))==0,"static geometry owner cannot be overwritten");
    const auto lid=horde::scene::kRescueBlockoutLid;
    const auto boxes=horde::scene::RescueLidFinishBoxes();
    check(boxes.size()==6,"lid has a bounded stone field, four rim bands and central panel");
    for(const auto& b:boxes)for(unsigned axis=0;axis<3;++axis)
        check(b.minimum[axis]>=lid.minimum[axis] && b.maximum[axis]<=lid.maximum[axis] &&
            b.maximum[axis]>b.minimum[axis] && b.materialCode==0,
            "closed relief remains within accepted collision/slide extent and stone family");
    check(boxes.front().minimum==lid.minimum &&
        boxes.front().maximum[0]==lid.maximum[0] && boxes.front().maximum[2]==lid.maximum[2] &&
        boxes.front().maximum[1]<lid.maximum[1],"base seals the full aperture beneath relief");
    return ok?0:1;
}
