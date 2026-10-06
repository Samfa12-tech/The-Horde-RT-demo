#include "scene/assets/SkinnedMeshAsset.h"
#include <iostream>
#include <iomanip>
int main(int argc,char**argv){if(argc!=5)return 2;horde::scene::SkinnedMeshAsset a;horde::scene::SkinnedClipSet b{};for(auto&v:b.clips)v={"",false,false};b.clips[0]={argv[2],false,true};std::string d;if(!a.LoadClips(argv[1],b,d)){std::cerr<<d;return 1;}horde::scene::SkinnedNodeTransform t;if(!a.NodeTransform(horde::scene::SkinnedClip::Idle,std::stof(argv[4]),argv[3],t,d)){std::cerr<<d;return 1;}std::cout<<std::setprecision(10)<<'[';for(int i=0;i<16;i++){if(i)std::cout<<',';std::cout<<t[i];}std::cout<<"]\n";}
