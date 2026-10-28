#include "platform/windows/WindowsAcceptanceProfile.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace fs=std::filesystem;
using Profile=horde::platform::windows::WindowsAcceptanceProfile;
namespace {
int checks=0,failures=0;
void Check(bool value,const char* message){++checks;if(!value){++failures;std::cerr<<message<<'\n';}}
std::string Read(const fs::path& path){std::ifstream file(path,std::ios::binary);return {std::istreambuf_iterator<char>(file),{}};}
bool Open(Profile& profile,const fs::path& reports,const fs::path& package,const fs::path& root,bool enabled=true){
    const auto value=root.wstring();const std::wstring_view args[]={L"--acceptance-root",value};
    return profile.Configure(args,enabled,reports,package);
}
}
int main(){
    const auto temp=fs::temp_directory_path();
    const auto base=temp/("horde-acceptance-profile-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if(!fs::create_directory(base))return 2;
    std::ofstream(base/"fixture.owner")<<"disposable";
    const auto reports=base/"reports",package=reports/"horde-windows-acceptance-test",root=package/"state";
    fs::create_directories(package);
    const auto owner=base/"owner-careers";fs::create_directory(owner);std::ofstream(owner/"slot-1-manual.hrd")<<"owner sentinel";
    {
        Profile ordinary;Check(ordinary.Configure({},false,{},{} )&&!ordinary.Active(),"ordinary build does not require or activate test state");
        Profile shipping;Check(!Open(shipping,reports,package,root,false)&&!fs::exists(root),"ordinary build rejects acceptance flag before state access");
        Profile missing;Check(!missing.Configure({},true,reports,base),"development acceptance outside its dedicated package cannot fall back to owner state");
        Profile outside;Check(!Open(outside,reports,package,owner)&&Read(owner/"slot-1-manual.hrd")=="owner sentinel","owner-career path rejected without mutation");
        Profile relative;Check(!Open(relative,reports,package,L"state"),"relative root rejected");
        Profile traversal;Check(!Open(traversal,reports,package,package/L".."/package.filename()/L"state"),"parent traversal rejected");
        Profile extra;const auto value=root.wstring();const std::wstring_view args[]={L"--acceptance-root",value,L"--campaign-validate-hidden"};
        Check(!extra.Configure(args,true,reports,package),"acceptance cannot combine capture or automated journey flags");
    }
    {
        Profile first;Check(Open(first,reports,package,root)&&first.Active(),"fresh dedicated root admitted");
        Check(first.CampaignDirectory()==root/"campaign-v1"&&first.SettingsPath()==root/"settings.ini","all mutable game state has isolated paths");
        std::ofstream(first.CampaignDirectory()/"slot-1-manual.hrd")<<"earned test record";
        std::ofstream(first.SettingsPath())<<"[audio]\nsfxVolume=61\n";
        Profile concurrent;Check(!Open(concurrent,reports,package,root)&&!concurrent.Active(),"exclusive process lease prevents concurrent acceptance owners");
    }
    {
        Profile restored;Check(restored.Configure({},true,reports,package),"no-argument desktop launch reopens only the dedicated package state after process lease release");
        Check(Read(restored.CampaignDirectory()/"slot-1-manual.hrd")=="earned test record"&&Read(restored.SettingsPath()).find("61")!=std::string::npos,"relaunch preserves exact isolated save and settings");
    }
    std::ofstream(root/"profile.owner",std::ios::trunc)<<"not our profile";
    {Profile corrupt;Check(!Open(corrupt,reports,package,root)&&!corrupt.Active(),"incorrect ownership marker fails closed");}
    const auto linkedPackage=reports/"horde-windows-acceptance-linked";fs::create_directory(linkedPackage);
    const auto linkedState=linkedPackage/"state";
    {Profile owned;Check(Open(owned,reports,linkedPackage,linkedState),"valid profile for hardlink rejection witness");}
    std::error_code ec;fs::create_hard_link(owner/"slot-1-manual.hrd",linkedState/"settings.ini",ec);
    Check(!ec,"disposable hardlink fixture created");
    {Profile linked;Check(!Open(linked,reports,linkedPackage,linkedState)&&!linked.Active(),"hardlinked test state cannot target an owner file");}
    const auto junctionPackage=reports/"horde-windows-acceptance-symlink";fs::create_directory(junctionPackage);
    if(CreateSymbolicLinkW((junctionPackage/"state").c_str(),owner.c_str(),SYMBOLIC_LINK_FLAG_DIRECTORY|0x2)){
        Profile linked;Check(!Open(linked,reports,junctionPackage,junctionPackage/"state"),"linked root rejected");
    }else std::cout<<"Unprivileged directory symlink fixture unavailable; linked-root runtime test NOT_RUN\n";
    Check(Read(owner/"slot-1-manual.hrd")=="owner sentinel"&&std::distance(fs::directory_iterator(owner),fs::directory_iterator{})==1,"owner sentinel and exact file roster preserved across all rejected and successful paths");
    // Only this exclusive fixture root, with its own marker and checked parent, is removed.
    if(base.parent_path()==temp&&base.filename().wstring().starts_with(L"horde-acceptance-profile-test-")&&Read(base/"fixture.owner")=="disposable")fs::remove_all(base,ec);
    std::cout<<checks<<" acceptance profile checks, "<<failures<<" failures\n";
    return failures?1:0;
}
