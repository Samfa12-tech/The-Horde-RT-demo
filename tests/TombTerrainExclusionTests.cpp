#include "scene/DevelopmentWorldGeometry.h"
#include "scene/OccupiedTombVolume.h"
#include <iostream>
#include <limits>
int main() {
    int failures=0,checks=0;
    const auto check=[&](bool pass,const char* message){++checks;if(!pass){++failures;std::cerr<<message<<'\n';}};
    const auto terrain=horde::scene::PrepareDevelopmentWorldGeometry(false,true);
    for(std::size_t room=0;room<horde::gameplay::kShowcaseWalkableRects.size();++room) {
        std::size_t hits=0;
        for(std::size_t i=0;i<terrain.triangles.size();++i)
            if(horde::scene::ExteriorTriangleEntersOccupiedTomb(terrain.triangles[i].points,
                horde::gameplay::kShowcaseWalkableRects[room])) {
                ++hits;std::cerr<<"occupied room="<<room<<" terrain triangle="<<i<<'\n';
                for(const auto& p:terrain.triangles[i].points)
                    std::cerr<<'('<<p[0]<<','<<p[1]<<','<<p[2]<<") ";
                std::cerr<<'\n';
            }
        check(hits==0,"actual exterior triangles cross an occupied tomb room or passage");
    }
    const auto gallery=horde::gameplay::kShowcaseWalkableRects[6];
    std::array<horde::scene::TerrainPoint,3> thin{{{-40.f,0.f,-15.2f},{10.f,0.f,-15.2f},{10.f,.1f,-15.1f}}};
    check(horde::scene::ExteriorTriangleEntersOccupiedTomb(thin,gallery),
        "thin crossing triangle with all vertices outside evades exclusion");
    std::vector<std::array<horde::scene::TerrainPoint,3>> clipped;
    check(horde::scene::ExteriorTriangleOutsideTomb(thin,clipped)&&!clipped.empty(),
        "bounded subtraction loses all valid sides of a crossing triangle");
    bool outside=true;
    for(const auto& fragment:clipped)
        outside&=!horde::scene::ExteriorTriangleEntersOccupiedTomb(fragment);
    check(outside,"a retained fragment still enters the occupied interior");
    const std::array<horde::scene::TerrainPoint,3> high{{{-28.f,2.05f,-16.f},{-20.f,2.05f,-16.f},{-20.f,2.05f,-14.f}}};
    check(horde::scene::ExteriorTriangleOutsideTomb(high,clipped)&&clipped.size()==1&&clipped[0]==high,
        "unaffected exterior triangle changes during subtraction");
    float roof=0;
    check(horde::scene::RetainedTombRoofAt(-24.f,-15.f,roof)&&
          std::abs(roof-horde::scene::kShowcaseRouteCeilingWorldY)<1e-6f,
          "support join does not use the actual retained gallery roof");
    check(!horde::scene::RetainedTombRoofAt(-33.7f,-15.2f,roof),
          "support join closes the rescue aperture");
    check(!horde::scene::ExteriorTriangleEntersOccupiedTomb(
        {{{-28.f,2.05f,-16.f},{-20.f,2.05f,-16.f},{-20.f,2.05f,-14.f}}},gallery),
        "valid terrain above the roof is rejected");
    check(!horde::scene::ExteriorTriangleEntersOccupiedTomb(
        {{{-28.f,-5.f,-16.f},{-20.f,-5.f,-16.f},{-20.f,-5.f,-14.f}}},gallery),
        "valid deep underside is rejected");
    check(!horde::scene::ExteriorTriangleEntersOccupiedTomb(
        {{{-28.f,0.f,gallery.minZ},{-20.f,0.f,gallery.minZ},{-20.f,1.f,gallery.minZ}}},gallery),
        "an adjoining wall outside the occupied inset is rejected");
    thin[0][0]=std::numeric_limits<float>::quiet_NaN();
    check(horde::scene::ExteriorTriangleEntersOccupiedTomb(thin,gallery),
        "nonfinite geometry can evade admission rejection");
    const auto previous=clipped;
    check(!horde::scene::ExteriorTriangleOutsideTomb(thin,clipped)&&clipped==previous,
        "invalid subtraction publishes or corrupts an output");
    check(terrain.valid,"repaired geometry is invalid");
    std::cout<<"Tomb terrain exclusion checks="<<checks<<" failures="<<failures<<" triangles="<<terrain.triangles.size()<<'\n';
    return failures?1:0;
}
