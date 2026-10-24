#include "vulkan/raytracing/PresentableTinyRtScene.h"
#include "vulkan/raytracing/TlasInstanceRefresh.h"
#include "scene/CorridorStaticDetail.h"
#include "scene/CorridorStaticDetailFaces.h"
#include "scene/ChapterRegionalWater.h"
#include "gameplay/simulation/GameSimulation.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <stdexcept>
#include <utility>

namespace horde::vulkan::raytracing {
struct PresentableTinyRtSceneStaticCutoverTestAccess {
    using Scene = PresentableTinyRtScene;
    // The fixture keeps many independent renderer owners in this Debug function.
    // Heap-backed local RAII avoids MSVC stack-frame exhaustion; ownership,
    // destruction order and all move/retirement assertions remain unchanged.
    template<class T> static T Handle(std::uintptr_t id) {
        if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(id);
        else return static_cast<T>(id);
    }
    static Scene::Buffer Buffer(std::uintptr_t id, void* map=nullptr) {
        Scene::Buffer b{}; b.buffer=Handle<VkBuffer>(id); b.memory=Handle<VkDeviceMemory>(id+1000);
        b.address=id*4096; b.size=b.allocationSize=256; b.mappedWriteData=map; return b;
    }
    static Scene::AccelerationStructure As(std::uintptr_t id) {
        Scene::AccelerationStructure a{}; a.handle=Handle<VkAccelerationStructureKHR>(id);
        a.address=id*4096; a.backing=Buffer(id); return a;
    }
    struct Proof {
        Scene* scene=nullptr; bool succeed=true, stale=false, descriptorFailure=false, descriptorThrow=false, poseChange=false;
        unsigned proof=0, publish=0, retire=0, record=0;
        bool recordFailure=false, recordThrow=false, omitDefinitions=false, wrongWorld=false, resetDuringRecord=false;
        bool recordingCorrect=false;
        bool publicationCorrect=false, retirementCorrect=false;
        bool sharedVertices=false;
    };
    static void Seed(Scene& s, int& oldMap, int& newMap) {
        s.ready_=true; s.staticGeometryGeneration_=41;
        s.staticVertexBuffer_=Buffer(1); s.staticIndexBuffer_=Buffer(2);
        s.staticGeometryTransformBuffer_=Buffer(3); s.instanceMetadataBuffer_=Buffer(4,&oldMap);
        s.primitiveMetadataBuffer_=Buffer(5); s.instanceBuffer_=Buffer(6,&oldMap);
        s.collapseBlas_=As(7); s.tlas_=As(8); s.tlasUpdateScratch_=Buffer(9);
        s.worldPlayerVertexBuffer_=Buffer(10); s.viewmodelVertexBuffer_=Buffer(11);
        s.chapterActorPoseGeneration_=17; s.chapterActorPoseSeconds_=2.0f;
        s.thirdActorIsCompanion_=true; s.waterDropletGeometryVisible_=false;
        s.preparedStaticGeometry_=std::make_unique<Scene::PreparedStaticGeometry>();
        auto& c=*s.preparedStaticGeometry_; c.sourceGeneration=41; c.ready=true;
        c.vertices=Buffer(101); c.indices=Buffer(102); c.transforms=Buffer(103);
        c.instances=Buffer(104,&newMap); c.primitives=Buffer(105); c.tlasInstances=Buffer(106,&newMap);
        c.collapse=As(107); c.tlas=As(108); c.tlasScratch=Buffer(109);
        c.definitions[Scene::kCollapseInstanceIndex].accelerationStructureReference=c.collapse.address;
        c.definitions[0].mask=0x41; c.definitions[0].instanceCustomIndex=21;
        s.tlasBuiltInstances_ = c.definitions;
        s.tlasBuiltInstances_[Scene::kCollapseInstanceIndex].accelerationStructureReference=s.collapseBlas_.address;
        s.tlasInstanceDefinitionsValid_=true;
    }
    static Scene::StaticCutoverApi Api(Proof& p) {
        return {&p,
            [](void* u,std::string& error) {
                auto& p=*static_cast<Proof*>(u); ++p.proof;
                if(p.stale) ++p.scene->staticGeometryGeneration_;
                if(p.poseChange) p.scene->tlasBuiltInstances_[0].transform.matrix[0][3]+=1.0f;
                if(!p.succeed) error="Injected incomplete retirement proof";
                return p.succeed;
            },
            [](void* u,Scene& s,std::string& error) {
                auto& p=*static_cast<Proof*>(u); ++p.publish;
                p.publicationCorrect = p.proof==1 && p.retire==0 && s.staticGeometryGeneration_==41 &&
                    s.staticVertexBuffer_.buffer==Handle<VkBuffer>(p.sharedVertices?1:101) && s.tlas_.handle==Handle<VkAccelerationStructureKHR>(108);
                if(p.descriptorThrow) throw std::runtime_error("Injected descriptor preparation exception");
                if(p.descriptorFailure) error="Injected descriptor contract rejection before publication";
                return !p.descriptorFailure;
            },
            [](void* u,Scene& s,Scene::PreparedStaticGeometry& c) noexcept {
                auto& p=*static_cast<Proof*>(u); ++p.retire;
                p.retirementCorrect = p.publish==1 && s.staticGeometryGeneration_==42 &&
                    s.collapseBlas_.address==107*4096 && s.tlasBuiltInstances_[Scene::kCollapseInstanceIndex].accelerationStructureReference==107*4096 &&
                    s.tlasBuiltInstances_[0].instanceCustomIndex==21 && s.tlasInstanceDefinitionsValid_ &&
                    (s.tlasPendingDefinitionsValid_ == (p.record==1)) &&
                    (!p.record || s.tlasPendingInstances_[Scene::kCollapseInstanceIndex].accelerationStructureReference==107*4096) &&
                    s.lastInstanceMasks_[0]==0x41 && s.chapterActorPoseGeneration_==UINT64_MAX && s.chapterActorPoseSeconds_<0 &&
                    s.chapterActorNeedsRefit_ && s.waterDropletGeometryVisible_ &&
                    s.worldPlayerVertexBuffer_.buffer==Handle<VkBuffer>(10) && s.viewmodelVertexBuffer_.buffer==Handle<VkBuffer>(11) && c.vertices.buffer==Handle<VkBuffer>(p.sharedVertices?0:1) &&
                    c.instances.buffer==Handle<VkBuffer>(4) && c.tlasInstances.buffer==Handle<VkBuffer>(6) &&
                    c.collapse.handle==Handle<VkAccelerationStructureKHR>(7) && c.tlas.handle==Handle<VkAccelerationStructureKHR>(8) &&
                    c.tlasScratch.buffer==Handle<VkBuffer>(9);
                // These are synthetic handles, never actual Vulkan allocations.
                c = {};
            }};
    }
    static bool Check(bool condition,const char* message) {
        if(!condition) std::cerr<<"FAIL: "<<message<<'\n'; return condition;
    }
    static Scene::StaticCutoverApi FrameApi(Proof& p) {
        auto api=Api(p);
        api.recordCurrentFrame=[](void* u,Scene& s,std::string& error) {
            auto& p=*static_cast<Proof*>(u);++p.record;
            p.recordingCorrect=p.proof==1 && p.publish==1 && p.retire==0 &&
                s.staticGeometryGeneration_==41 && s.staticVertexBuffer_.buffer==Handle<VkBuffer>(p.sharedVertices?1:101) &&
                s.chapterActorNeedsRefit_ && s.tlasBuiltInstances_[Scene::kCollapseInstanceIndex].accelerationStructureReference==107*4096;
            if(p.recordThrow)throw std::runtime_error("Injected current-frame record exception");
            if(p.recordFailure){error="Injected current-frame record failure";return false;}
            auto current=s.tlasBuiltInstances_;
            current[20].transform.matrix[0][3]=9.f;
            current[21].transform.matrix[1][3]=8.f;
            current[2].transform.matrix[2][3]=7.f;
            current[2].mask=0x01;
            if(p.wrongWorld)current[Scene::kCollapseInstanceIndex].accelerationStructureReference=7*4096;
            if(!p.omitDefinitions)s.StageTlasSubmissionDefinitions(current);
            if(p.resetDuringRecord)++s.staticGeometryGeneration_;
            return true;
        };
        return api;
    }
    static int KeeperVertexLifetimeChecks() {
        int failures=0; std::string error;
        struct Calls {
            Scene* scene=nullptr;
            unsigned proof=0, prepare=0, bind=0, destroy=0;
            bool proofOk=true, prepareOk=true, stale=false, incomplete=false, proofThrows=false, prepareThrows=false;
            VkBuffer bound=VK_NULL_HANDLE;
            bool retirementOrdered=true;
        };
        const auto frame=[] {
            RtSceneFrameInputs f{};
            f.developmentRescueJourney=true;
            f.lich.phase=horde::gameplay::LichPhase::Dead;
            f.lich.deathAnimationComplete=true;f.lich.deathDissolveProgress=1.f;
            f.chapterCompanion.grounded=true;f.chapterCompanion.generation=17;
            return f;
        }();
        const auto seed=[](Scene& s) {
            s.ready_=true;s.chapterActorReady_=true;s.staticGeometryGeneration_=41;
            s.thirdActorIsCompanion_=s.thirdActorSubmittedCompanion_=true;
            s.chapterActorPoseGeneration_=17;
            s.staticVertexBuffer_=Buffer(1);
            s.pipelineBundle_.descriptorSet=Handle<VkDescriptorSet>(19);
            s.characterSlot_.LichGpu().vertices=Buffer(2);
            s.characterSlot_.LichGpu().vertices.mappedWriteData=reinterpret_cast<void*>(1);
            s.characterSlot_.LichGpu().vertices.memoryPropertyFlags=
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            s.characterSlot_.LichGpu().accelerationStructure=As(3);
            s.characterSlot_.LichGpu().updateScratch=Buffer(4);
        };
        const auto api=[](Calls& c) {
            Scene::ThirdActorVertexLifetimeApi a{};a.user=&c;
            a.retireReferences=[](void* u,std::string& error) {
                auto& c=*static_cast<Calls*>(u);++c.proof;
                if(c.proofThrows)throw std::runtime_error("Injected proof exception");
                if(c.stale)++c.scene->staticGeometryGeneration_;
                if(!c.proofOk)error="Injected unproved GPU completion";
                return c.proofOk;
            };
            a.prepareKeeper=[](void* u,Scene::Buffer& b,std::string& error) {
                auto& c=*static_cast<Calls*>(u);++c.prepare;
                b=Buffer(7);b.mappedWriteData=reinterpret_cast<void*>(1);
                if(c.prepareThrows)throw std::runtime_error("Injected prepare exception");
                if(c.incomplete)b.address=0;
                if(!c.prepareOk)error="Injected allocation/map/upload failure";
                return c.prepareOk;
            };
            a.bindVertices=[](void* u,const VkDescriptorBufferInfo& info) noexcept {
                auto& c=*static_cast<Calls*>(u);++c.bind;c.bound=info.buffer;
                c.retirementOrdered &= c.proof==1 && info.offset==0 && info.range>0 && c.destroy==0;
            };
            a.destroyVertices=[](void* u,Scene::Buffer& b) noexcept {
                auto& c=*static_cast<Calls*>(u);++c.destroy;
                if(b.buffer==Handle<VkBuffer>(2))
                    c.retirementOrdered &= c.bound==Handle<VkBuffer>(1) && c.proof==1;
                b={};
            };
            return a;
        };
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);Calls c{&s};auto a=api(c);
            const auto before=s.ResourceInventory();
            failures+=!Check(s.PrepareThirdActorVertexLifetimeWithApi(frame,a,error) &&
                s.characterSlot_.LichGpu().vertices.Empty() && c.retirementOrdered &&
                c.proof==1 && c.bind==1 && c.destroy==1 && c.prepare==0,
                "completed Kit adoption rebinds retained storage before retiring exactly one Keeper buffer");
            const auto after=s.ResourceInventory();
            failures+=!Check(after.bufferCount+1==before.bufferCount &&
                after.memoryAllocationCount+1==before.memoryAllocationCount &&
                after.hostVisibleBytes+256==before.hostVisibleBytes &&
                after.bottomLevelAccelerationStructureCount==before.bottomLevelAccelerationStructureCount &&
                s.KeeperVertexLifetime().retiredAllocationBytes==256 &&
                s.KeeperVertexLifetime().retirementCount==1 &&
                s.characterSlot_.LichGpu().accelerationStructure.handle==Handle<VkAccelerationStructureKHR>(3) &&
                s.characterSlot_.LichGpu().updateScratch.buffer==Handle<VkBuffer>(4),
                "inventory saving is exact and shared third BLAS/scratch remain unchanged");
            failures+=!Check(s.PrepareThirdActorVertexLifetimeWithApi(frame,a,error) &&
                c.proof==1 && c.destroy==1,
                "repeated Kit frames never retire twice");
            auto back=frame;back.chapterCompanion.grounded=false;
            c={&s};a=api(c);
            failures+=!Check(s.PrepareThirdActorVertexLifetimeWithApi(back,a,error) &&
                s.characterSlot_.LichGpu().vertices.buffer==Handle<VkBuffer>(7) &&
                c.prepare==1 && c.proof==1 && c.bind==1 && c.destroy==0 &&
                !s.KeeperVertexLifetime().retired && s.KeeperVertexLifetime().restorationCount==1 &&
                s.thirdActorIsCompanion_,
                "grounded loss prepares valid Keeper storage without publishing a partial actor switch");
            s.thirdActorIsCompanion_=false;s.thirdActorSubmittedCompanion_=false;
            c={&s};a=api(c);
            failures+=!Check(s.PrepareThirdActorVertexLifetimeWithApi(back,a,error) &&
                c.proof==0 && c.bind==0 && c.prepare==0,
                "ordinary Keeper frames perform no allocation or retirement");
        }
        for(int edge=0;edge<7;++edge) {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);Calls c{&s};auto a=api(c);
            if(edge==0)c.proofOk=false;
            if(edge==1)c.stale=true;
            if(edge==2)s.thirdActorSubmittedCompanion_=false;
            if(edge==3)s.thirdActorNeedsBuild_=true;
            if(edge==4)s.chapterActorPoseGeneration_=18;
            if(edge==5)a.retireReferences=nullptr;
            if(edge==6)c.proofThrows=true;
            const bool noTransition=edge>=2 && edge<=4;
            failures+=!Check(s.PrepareThirdActorVertexLifetimeWithApi(frame,a,error)==noTransition &&
                !s.characterSlot_.LichGpu().vertices.Empty() && c.bind==0 && c.destroy==0,
                "unsubmitted, stale or unproved adoption retains the Keeper allocation");
        }
        for(int edge=0;edge<7;++edge) {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);s.characterSlot_.LichGpu().vertices={};
            auto back=frame;back.developmentRescueJourney=false;
            Calls c{&s};auto a=api(c);
            if(edge==0)c.prepareOk=false;
            if(edge==1)c.incomplete=true;
            if(edge==2)c.proofOk=false;
            if(edge==3)c.stale=true;
            if(edge==4)a.prepareKeeper=nullptr;
            if(edge==5)c.prepareThrows=true;
            if(edge==6)c.proofThrows=true;
            failures+=!Check(!s.PrepareThirdActorVertexLifetimeWithApi(back,a,error) &&
                s.characterSlot_.LichGpu().vertices.Empty() && s.thirdActorIsCompanion_ &&
                c.bind==0 && c.destroy==(edge==4?0u:1u),
                "failed/reset restore retains Kit owner and destroys only the unsubmitted candidate");
            c={&s};a=api(c);
            failures+=!Check(s.PrepareThirdActorVertexLifetimeWithApi(back,a,error) &&
                s.characterSlot_.LichGpu().vertices.buffer==Handle<VkBuffer>(7) &&
                c.bind==1 && c.destroy==0,
                "failed Keeper storage preparation remains retryable");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);Calls c{&s};auto a=api(c);
            failures+=!Check(s.PrepareThirdActorVertexLifetimeWithApi(frame,a,error),
                "retired actor owner may move only after completed-work retirement");
            auto movedOwner=std::make_unique<Scene>(std::move(s));auto& moved=*movedOwner;c={&moved};a=api(c);
            auto back=frame;back.chapterCompanion.grounded=false;
            failures+=!Check(moved.KeeperVertexLifetime().retired &&
                !s.KeeperVertexLifetime().retired && s.KeeperVertexLifetime().retirementCount==0 &&
                moved.PrepareThirdActorVertexLifetimeWithApi(back,a,error) &&
                moved.KeeperVertexLifetime().restorationCount==1,
                "move transfers retirement state and reset restores only the new owner");
            moved.Destroy();
            failures+=!Check(!moved.KeeperVertexLifetime().retired &&
                moved.KeeperVertexLifetime().retirementCount==0,
                "reconstruction resets actor lifetime evidence without stale ownership");
        }
        std::cout << "Keeper vertex lifetime checks: " << (failures?"FAIL":"PASS") << '\n';
        return failures;
    }
    static int KeeperTextureLifetimeChecks() {
        int failures=0;std::string error;
        const auto texture=[](unsigned id) {
            Scene::TextureArray t{};t.image=Handle<VkImage>(id);t.memory=Handle<VkDeviceMemory>(id);
            t.view=Handle<VkImageView>(id);t.allocationSize=256;t.mipLevels=1;
            t.memoryPropertyFlags=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;return t;
        };
        RtSceneFrameInputs frame{};frame.developmentRescueJourney=true;
        frame.lich.phase=horde::gameplay::LichPhase::Dead;frame.lich.deathAnimationComplete=true;
        frame.lich.deathDissolveProgress=1;frame.chapterCompanion.grounded=true;frame.chapterCompanion.generation=17;
        const auto seed=[&](Scene& s) {
            s.ready_=s.chapterActorReady_=s.genericStaticAssetEnabled_=true;
            s.thirdActorIsCompanion_=s.thirdActorSubmittedCompanion_=true;s.chapterActorPoseGeneration_=17;
            s.staticGeometryGeneration_=41;s.pipelineBundle_.descriptorSet=Handle<VkDescriptorSet>(19);
            s.materialSampler_=Handle<VkSampler>(1);s.staticBaseColor_=texture(10);s.staticEmissive_=texture(11);
            s.lichBaseColor_=texture(20);s.lichEmissive_=texture(21);
        };
        struct Calls {
            Scene* scene;unsigned proof=0,prepare=0,bind=0,destroy=0;
            bool proofOk=true,prepareOk=true,partial=false,stale=false,throws=false,unproved=false,pendingThrow=false;
            std::array<VkImageView,2> bound{};bool ordered=true;
        };
        const auto api=[&](Calls& c) {
            Scene::ThirdActorTextureLifetimeApi a{};a.user=&c;
            a.retireReferences=[](void* u,std::string&) {
                auto& c=*static_cast<Calls*>(u);++c.proof;
                if(c.stale)++c.scene->staticGeometryGeneration_;
                return c.proofOk;
            };
            a.prepareKeeper=+[](void* u,Scene::TextureArray& base,Scene::TextureArray& emission,std::string&) {
                auto& c=*static_cast<Calls*>(u);++c.prepare;
                base.image=Handle<VkImage>(30);base.memory=Handle<VkDeviceMemory>(30);base.view=Handle<VkImageView>(30);
                base.allocationSize=256;base.mipLevels=1;base.memoryPropertyFlags=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                if(!c.partial) {emission=base;emission.image=Handle<VkImage>(31);emission.memory=Handle<VkDeviceMemory>(31);emission.view=Handle<VkImageView>(31);}
                if(c.unproved || c.pendingThrow) {
                    c.scene->staticGeometryCompletionUnproved_=c.unproved;
                    c.scene->keeperTextureUploadRecovery_.staging=Buffer(32);
                    c.scene->keeperTextureUploadRecovery_.staging.memoryPropertyFlags=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                    c.scene->keeperTextureUploadRecovery_.work.command=Handle<VkCommandBuffer>(33);
                    c.scene->keeperTextureUploadRecovery_.work.fence=Handle<VkFence>(34);
                    c.scene->keeperTextureUploadRecovery_.work.submitted=true;
                    c.scene->keeperTextureUploadRecovery_.work.completionUnproved=true;
                    if(c.pendingThrow)throw std::runtime_error("Exception after submitted texture upload");
                    return false;
                }
                if(c.throws)throw std::runtime_error("Injected texture preparation exception");
                return c.prepareOk;
            };
            a.bindImages=[](void* u,const std::array<VkDescriptorImageInfo,2>& images) noexcept {
                auto& c=*static_cast<Calls*>(u);++c.bind;
                c.bound={images[0].imageView,images[1].imageView};
                c.ordered &= c.proof>0 && images[0].sampler && images[1].sampler &&
                    images[0].imageLayout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
                    images[1].imageLayout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            };
            a.destroyImage=[](void* u,Scene::TextureArray& t) noexcept {
                auto& c=*static_cast<Calls*>(u);++c.destroy;
                if(t.view==Handle<VkImageView>(20)||t.view==Handle<VkImageView>(21))
                    c.ordered &= c.bind==1 && c.bound[0]==Handle<VkImageView>(10) && c.bound[1]==Handle<VkImageView>(11);
                t={};
            };return a;
        };
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);Calls c{&s};auto a=api(c);const auto before=s.ResourceInventory();
            failures+=!Check(s.PrepareThirdActorTextureLifetimeWithApi(frame,a,error) && c.ordered &&
                c.proof==1 && c.prepare==0 && c.bind==1 && c.destroy==2 && s.KeeperTextureLifetime().retired &&
                !s.lichBaseColor_.image && !s.lichEmissive_.image && s.KeeperTextureLifetime().retiredDeviceLocalBytes==512 &&
                s.KeeperTextureDescriptors()[0].imageView==Handle<VkImageView>(10) &&
                s.KeeperTextureDescriptors()[1].imageView==Handle<VkImageView>(11) &&
                s.ResourceInventory().deviceLocalBytes+512==before.deviceLocalBytes,
                "completed Kit adoption aliases both descriptors before retiring both exact GPU texture owners");
            failures+=!Check(s.PrepareThirdActorTextureLifetimeWithApi(frame,a,error) && c.bind==1 && c.destroy==2,
                "repeat exterior traversal must not retire Keeper textures twice");
            auto reset=frame;reset.lich.phase=horde::gameplay::LichPhase::Dormant;reset.chapterCompanion.grounded=false;
            failures+=!Check(s.PrepareThirdActorTextureLifetimeWithApi(reset,a,error) && c.prepare==1 && c.bind==2 &&
                s.lichBaseColor_.view==Handle<VkImageView>(30) && s.lichEmissive_.view==Handle<VkImageView>(31) &&
                !s.KeeperTextureLifetime().retired && s.KeeperTextureLifetime().restorationCount==1 &&
                s.KeeperTextureDescriptors()[0].imageView==Handle<VkImageView>(30) &&
                s.KeeperTextureDescriptors()[1].imageView==Handle<VkImageView>(31) &&
                s.ResourceInventory().deviceLocalBytes==before.deviceLocalBytes,
                "reset restores the complete exact pair before either Keeper texture can be sampled");
            s.Destroy();
        }
        for(unsigned fault=0;fault<9;++fault) {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);Calls c{&s};auto a=api(c);
            if(fault==0)c.proofOk=false;if(fault==1)c.stale=true;
            if(fault==2)s.thirdActorSubmittedCompanion_=false;
            if(fault==3)s.thirdActorNeedsBuild_=true;
            if(fault==4)s.staticEmissive_.view=VK_NULL_HANDLE;
            if(fault==5)a.retireReferences=nullptr;
            if(fault==6)a.bindImages=nullptr;
            if(fault==7)a.destroyImage=nullptr;
            if(fault==8)s.materialSampler_=VK_NULL_HANDLE;
            const bool noTransition=fault==2||fault==3;
            failures+=!Check(s.PrepareThirdActorTextureLifetimeWithApi(frame,a,error)==noTransition &&
                s.lichBaseColor_.image==Handle<VkImage>(20) && s.lichEmissive_.image==Handle<VkImage>(21) &&
                c.bind==0 && c.destroy==0,"failed proof, stale generation, unsubmitted BUILD or missing alias cannot retire textures");
            s.Destroy();
        }
        for(unsigned fault=0;fault<3;++fault) {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);s.lichBaseColor_={};s.lichEmissive_={};s.keeperTextureLifetime_.retired=true;
            Calls c{&s};auto a=api(c);c.prepareOk=fault!=0;c.partial=fault==1;c.throws=fault==2;
            auto reset=frame;reset.chapterCompanion.grounded=false;
            failures+=!Check(!s.PrepareThirdActorTextureLifetimeWithApi(reset,a,error) && c.bind==0 &&
                s.KeeperTextureLifetime().retired && !s.lichBaseColor_.image && !s.lichEmissive_.image &&
                c.destroy==(fault==1?1u:2u),"partial, failed or throwing texture restore keeps both live aliases and cleans only temporary owners");
            s.Destroy();
        }
        for(unsigned fault=0;fault<2;++fault) {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);s.lichBaseColor_={};s.lichEmissive_={};s.keeperTextureLifetime_.retired=true;
            s.keeperTextureDirectory_="exact/keeper/source";Calls c{&s};auto a=api(c);
            c.unproved=fault==0;c.pendingThrow=fault==1;
            auto reset=frame;reset.chapterCompanion.grounded=false;
            failures+=!Check(!s.PrepareThirdActorTextureLifetimeWithApi(reset,a,error) && !s.ready_ &&
                c.bind==0 && c.destroy==0 && s.keeperTextureRestoreQuarantine_[0].image==Handle<VkImage>(30) &&
                s.keeperTextureRestoreQuarantine_[1].image==Handle<VkImage>(31) &&
                s.ResourceInventory().deviceLocalBytes==1024 && s.ResourceInventory().hostVisibleBytes==256,
                "unproved texture upload retains and counts both images, staging and the owning command/fence");
            auto movedOwner=std::make_unique<Scene>(std::move(s));auto& moved=*movedOwner;
            failures+=!Check(moved.keeperTextureRestoreQuarantine_[0].image==Handle<VkImage>(30) &&
                moved.keeperTextureUploadRecovery_.work.command==Handle<VkCommandBuffer>(33) &&
                moved.keeperTextureUploadRecovery_.staging.buffer==Handle<VkBuffer>(32) &&
                moved.KeeperTextureLifetime().retired && moved.keeperTextureDirectory_=="exact/keeper/source" &&
                !s.keeperTextureRestoreQuarantine_[0].image && !s.keeperTextureUploadRecovery_.work.command &&
                !s.keeperTextureUploadRecovery_.staging.buffer && !s.KeeperTextureLifetime().retired,
                "move transfers the quarantine and exact-source identity without duplicating GPU owners");
            moved.Destroy();
            failures+=!Check(!moved.keeperTextureRestoreQuarantine_[0].image &&
                !moved.keeperTextureUploadRecovery_.work.command && !moved.keeperTextureUploadRecovery_.staging.buffer &&
                !moved.KeeperTextureLifetime().retired && moved.keeperTextureDirectory_.empty(),
                "idle teardown clears pending texture ownership and retirement evidence");
        }
        return failures;
    }
    static int ForestRetentionChecks() {

        // Synthetic CPU policy/packing fixtures only. No imported asset, RT
        // allocation, opaque-face verification or GPU presentation is claimed.
        using namespace horde::scene;using namespace horde::scene::assets;
        int failures=0;std::string error;
        const auto identity=horde::gameplay::items::IdentityHeldItemTransform();
        auto prepared=std::make_shared<PreparedForestDressing>();
        constexpr std::array<const char*,4> names{{"ForestBark","ForestPine","ForestMoss","ForestAlder"}};
        for(unsigned f=0;f<4u;++f) {
            auto& material=prepared->materialFamilies[f];material.name=names[f];
            material.textureGroup=f;material.deferTextureAllocation=true;material.baseColorTexture=0;
            material.metallicFactor=0;material.normalTexture=material.ormTexture=f==0?0:-1;
        }
        for(unsigned m=0;m<2u;++m) {
            auto& master=prepared->masters[m];master.nodeTransforms.push_back({"synthetic-local",identity});
            master.bounds={{{-.1f,0.f,-.1f}},{{.1f,1.f,.1f}}};
            prepared->primitiveFamilies[m]={{0u,static_cast<std::uint8_t>(m==0?1u:3u),2u}};
            const std::uint32_t total=m==0?15000u:15306u;
            for(unsigned p=0;p<3u;++p) {
                master.materials.push_back(prepared->materialFamilies[prepared->primitiveFamilies[m][p]]);
                for(unsigned v=0;v<3u;++v) {
                    StaticRtVertex vertex{};vertex.position={{v==0?-.1f:v==1?.1f:0.f,v==2?1.f:0.f,v==0?-.1f:v==1?.1f:0.f,1.f}};
                    vertex.normal={{0,0,1,0}};vertex.tangent={{1,0,0,1}};master.vertices.push_back(vertex);
                }
                const std::uint32_t count=p<2u?3u:total-6u;
                master.primitives.push_back({p*3u,static_cast<std::uint32_t>(master.indices.size()),count,p,0u});
                for(std::uint32_t i=0;i<count;++i)master.indices.push_back(i%3u);
            }
        }
        for(std::size_t i=0;i<64u;++i) {
            const auto& contract=horde::gameplay::simulation::kForestTreePlacementContract[i];
            auto& placement=prepared->placements[i];
            placement.masterIndex=contract.species==horde::gameplay::simulation::ForestTreeSpecies::Pine?0u:1u;
            placement.origin={{contract.x,-contract.rootBurialMetres,contract.z}};
            placement.yawRadians=contract.yawRadians;placement.supportY=0;
        }
        StaticMeshAsset owner;owner.vertices.assign(prepared->masters[0].vertices.begin(),prepared->masters[0].vertices.begin()+3);
        // Three classified common triangles make a one-triangle certificate
        // structurally valid; shared policy must still reject that old route.
        owner.indices={0,1,2,0,1,2,0,1,2};owner.primitives.push_back({0,0,9,0,0});
        owner.triangleRegions.assign(3u,StaticTriangleRegion::ExteriorDressing);
        owner.materials.push_back({});owner.materials[0].name="synthetic common";
        owner.nodeTransforms.push_back({"synthetic-world",identity});owner.bounds=prepared->masters[0].bounds;
        ForestDressingSharedLayout layout;
        if(!SeedPreparedForestMaterialFamilies(*prepared,owner,error) ||
           !AppendPreparedForestMasters(prepared,owner,layout,error)) {
            Check(false,"synthetic retention layout prepares through the existing CPU helper");std::cerr<<error<<'\n';return 1;
        }
        const auto pack=[&](const StaticMeshAsset& asset,bool nonidentityGeometry=false) {
            auto slot=std::make_shared<RtStaticMeshSlot>();
            const std::array<StaticRtAssetRegistration,3> registrations{{
                {21u,21u,static_cast<unsigned>(RtInstanceFlag::StaticPbr),0u,&asset,nullptr,RtGeometryRole::Static,0u,1u},
                {26u,26u,static_cast<unsigned>(RtInstanceFlag::StaticPbr),0u,&asset,nullptr,RtGeometryRole::Static,1u,3u},
                {27u,27u,static_cast<unsigned>(RtInstanceFlag::StaticPbr),0u,&asset,nullptr,RtGeometryRole::Static,4u,3u}}};
            if(!slot->InitializeSelectedGeometry(registrations,error))return std::shared_ptr<const RtStaticMeshSlot>{};
            if(nonidentityGeometry) {
                // Loader node transforms are already baked; the packer emits
                // identity geometry transforms regardless of node metadata.
                // Inject this fault in the still-mutable synthetic slot BEFORE
                // its const handle is published, not in an immutable source.
                const auto primitive=slot->InstanceMetadata()[26u].primitiveBase;
                const_cast<std::array<float,12>&>(slot->GeometryTransforms()[primitive])[3]=.1f;
            }
            return std::shared_ptr<const RtStaticMeshSlot>(std::move(slot));
        };
        const auto source=pack(owner);
        if(!source){Check(false,"synthetic source packs without GPU work");std::cerr<<error<<'\n';return 1;}
        const auto seed=[&](Scene& s) {
#if defined(_DEBUG)
            s.forestRouteProofMode_=Scene::ForestRouteProofMode::Disabled;
#endif
            s.sharedForestGeometryEnabled_=s.sharedForestGeometryActive_=true;
            s.fullStaticGeometrySource_=source;s.forestSharedLayout_=layout;s.collapseStaticAsset_=owner;
            s.staticGeometryGeneration_=41;s.developmentRescueJourney_=true;s.sceneProfile_=RtSceneProfile::Showcase;
            // Explicit CPU closure inputs; the synthetic world is not evidence
            // that these water faces were uploaded or retained by a real GPU.
            s.regionalWaterDiagnosticEnabled_=true;
            const auto& water=ChapterRegionalWaterTriangles();s.regionalWaterFaces_.assign(water.begin(),water.end());
            s.rewardLanternRingAsset_.sockets={{"GripRing",0u,identity},{"Hinge",0u,identity}};
            s.rewardLanternBodyAsset_.sockets={{"Light",0u,identity}};
            s.ready_=s.ordinaryStaticFacesVerified_=s.tlasInstanceDefinitionsValid_=true;
            s.tlasInstanceCount_=92u;s.playerCpuSkinCadence_=PlayerCpuSkinCadence::Hz60;
            s.blas_=As(500);s.forestMasterBlas_={{As(501),As(502)}};
            for(unsigned i=0;i<28u;++i) {
                auto& instance=s.tlasBuiltInstances_[i];instance.instanceCustomIndex=i;instance.mask=0x3fu;
                instance.accelerationStructureReference=(600u+i)*4096u;
                instance.transform={{{1,0,0,float(i)},{0,1,0,0},{0,0,1,0}}};
            }
            s.tlasBuiltInstances_[0].accelerationStructureReference=s.blas_.address;
            s.tlasBuiltInstances_[0].transform.matrix[0][3]=0;
            for(std::size_t i=0;i<64u;++i) {
                const auto& p=prepared->placements[i];auto& instance=s.tlasBuiltInstances_[28u+i];
                const float c=std::cos(p.yawRadians),t=std::sin(p.yawRadians);
                instance.transform={{{c,0,t,p.origin[0]},{0,1,0,p.origin[1]},{-t,0,c,p.origin[2]}}};
                instance.instanceCustomIndex=26u+static_cast<unsigned>(p.masterIndex);instance.mask=1u;
                instance.flags=VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
                instance.accelerationStructureReference=s.forestMasterBlas_[p.masterIndex].address;
            }
            s.AdmitSharedForestRetentionProof();
        };
        RtSceneFrameInputs frame{};frame.developmentRescueJourney=true;
        frame.playerMountProfile=horde::gameplay::items::PlayerMountProfile::AnatomicalBody;
        frame.playerRenderRoute=kProductionPlayerRenderRoute;
        frame.interaction.heldLightKind=horde::gameplay::interactions::HeldLightKind::RewardLantern;
        frame.chestReward.phase=horde::gameplay::interactions::ChestRewardPhase::LanternClaimed;
        frame.lich.deathDissolveProgress=1.f;frame.lich.staffLightStrength=0;
        frame.tuning.glassFixtureVisible=frame.tuning.productionRewardPropsVisible=false;
        frame.cameraX=-22;frame.cameraZ=-15.2f;frame.rewardLanternWorldFromHinge=identity;
        frame.rewardLanternWorldFromHinge[12]=-22;frame.rewardLanternWorldFromHinge[13]=.6f;frame.rewardLanternWorldFromHinge[14]=-15.2f;
        const auto allRetained=[](const auto& instances) {
            for(unsigned i=28u;i<92u;++i)if(instances[i].mask!=1u)return false;return true;
        };
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);auto instances=s.tlasBuiltInstances_;const auto before=instances;
            failures+=!Check(s.forestRetentionProof_.admitted && s.ForestRetentionIdentityMatches(),
                "synthetic exact source/ranges admit the CPU retention identity");
            failures+=!Check(s.forestRetentionProof_.eligibleCount>0u,
                "synthetic bounds with canonical retained-water inputs admit a nonempty CPU retention plan");
            s.ApplySharedForestRetentionProof(instances,frame);
            bool masks=s.forestRetentionFrameEligible_ && s.forestRetentionFrameGeneration_==41u &&
                s.forestRetentionOmittedCount_==s.forestRetentionProof_.eligibleCount;
            for(unsigned i=0;i<64u;++i)masks&=instances[28u+i].mask==(s.forestRetentionProof_.eligible[i]?0u:1u);
            failures+=!Check(masks && std::memcmp(instances.data(),before.data(),28u*sizeof(instances[0]))==0 &&
                s.forestMasterBlas_[0].address==501u*4096u && s.forestMasterBlas_[1].address==502u*4096u,
                "safe CPU policy changes certified masks only, retaining uncertified roots, all28 legacy roles and both masters");
            auto outside=frame;outside.cameraX=-40;
            s.ApplySharedForestRetentionProof(instances,outside);
            failures+=!Check(allRetained(instances) && !s.forestRetentionFrameEligible_ && s.forestRetentionOmittedCount_==0 &&
                std::none_of(s.forestRetentionLastOmitted_.begin(),s.forestRetentionLastOmitted_.end(),[](bool b){return b;}),
                "unsafe next frame restores every tree before any policy decision");
            s.ApplySharedForestRetentionProof(instances,frame);
            failures+=!Check(s.forestRetentionFrameEligible_ &&
                s.forestRetentionOmittedCount_==s.forestRetentionProof_.eligibleCount,
                "returning to the same safe current owner reuses only its certified instance decisions");
        }
        constexpr std::array<const char*,21> runtimeFaults{{"different source owner","different prepared owner","stale generation",
            "changed primitive range","changed local bounds","unverified retained faces","nonidentity current root",
            "wrong master reference","wrong placement transform","wrong metadata index","incomplete live population",
            "unpublished definitions","pending definitions","retention-suppressing override or inactive owner","supplemental placements",
            "changed common prefix","active water droplet","exterior frame","visible glass fixture",
            "wrong submitted root reference","missing master address"}};
        for(unsigned fault=0;fault<runtimeFaults.size();++fault) {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);auto instances=s.tlasBuiltInstances_;auto unsafe=frame;
            for(unsigned i=28u;i<92u;++i)instances[i].mask=0u;
            if(fault==0)s.fullStaticGeometrySource_=std::make_shared<const RtStaticMeshSlot>(*source);
            if(fault==1)s.forestSharedLayout_.prepared=std::make_shared<const PreparedForestDressing>(*prepared);
            if(fault==2)++s.staticGeometryGeneration_;
            if(fault==3)++s.forestSharedLayout_.masters[0].primitiveOffset;
            if(fault==4)s.forestSharedLayout_.masters[0].localBounds.maximum[0]+=1.f;
            if(fault==5)s.ordinaryStaticFacesVerified_=false;
            if(fault==6)instances[0].transform.matrix[0][3]=.01f;
            if(fault==7)++instances[28].accelerationStructureReference;
            if(fault==8)instances[28].transform.matrix[0][3]+=.01f;
            if(fault==9)instances[28].instanceCustomIndex=27u;
            if(fault==10)s.tlasInstanceCount_=28u;
            if(fault==11)s.tlasInstanceDefinitionsValid_=false;
            if(fault==12)s.tlasPendingDefinitionsValid_=true;
            if(fault==13) {
#if defined(_DEBUG)
                s.forestRouteProofMode_=Scene::ForestRouteProofMode::SharedExisting64;
#else
                s.sharedForestGeometryActive_=false;
#endif
            }
            if(fault==14)s.forestSharedLayout_.supplementalPlacements.push_back({});
            if(fault==15)++s.forestSharedLayout_.commonPrimitiveCount;
            if(fault==16)unsafe.waterContact.droplets[0].active=true;
            if(fault==17)unsafe.rescue.exteriorSide=true;
            if(fault==18)unsafe.tuning.glassFixtureVisible=true;
            if(fault==19)++s.tlasBuiltInstances_[0].accelerationStructureReference;
            if(fault==20)s.forestMasterBlas_[0].address=0;
            const auto legacy=instances;
            s.ApplySharedForestRetentionProof(instances,unsafe);
            const auto message=std::string("CPU retention fallback: ")+runtimeFaults[fault];
            failures+=!Check(allRetained(instances) && !s.forestRetentionFrameEligible_ && !s.forestRetentionOmittedCount_ &&
                std::memcmp(instances.data(),legacy.data(),28u*sizeof(instances[0]))==0,
                message.c_str());
        }
        constexpr std::array<const char*,4> admissionFaults{{"changed packed master vertices","changed packed master indices",
            "nonidentity packed geometry transform","foreign transmission owner"}};
        for(unsigned fault=0;fault<admissionFaults.size();++fault) {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);auto changed=owner;
            if(fault==0)changed.vertices[changed.primitives[1].vertexOffset].position[0]+=.01f;
            if(fault==1)changed.indices[changed.primitives[1].indexOffset]=1u;
            if(fault==3)changed.materials[0].flags|=static_cast<unsigned>(RtMaterialFlag::Transmission);
            s.fullStaticGeometrySource_=pack(changed,fault==2);s.AdmitSharedForestRetentionProof();
            const auto message=std::string("CPU retention admission rejects ")+admissionFaults[fault];
            failures+=!Check(!s.forestRetentionProof_.admitted,
                message.c_str());
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);
            s.regionalWaterFaces_.clear();s.AdmitSharedForestRetentionProof();
            auto instances=s.tlasBuiltInstances_;s.ApplySharedForestRetentionProof(instances,frame);
            failures+=!Check(s.forestRetentionProof_.eligibleCount==0u && allRetained(instances) &&
                s.forestRetentionOmittedCount_==0u,"missing actual-water inputs cannot certify any omitted tree");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;
            failures+=!Check(s.sharedForestGeometryEnabled_==Scene::kDefaultSharedForestGeometryEnabled &&
                !s.SharedForestGeometryActive() && !s.SelectSharedForestGeometry(),
                "the configured normal default does not invent an active non-chapter owner");
            failures+=!Check(s.SetSharedForestGeometryEnabled(true) && !s.SelectSharedForestGeometry() &&
                !s.UsesSharedForestGeometry(),"a pre-init request does not invent an active owner or non-chapter profile");
            s.SetDevelopmentRescueJourney(true);
            failures+=!Check(s.SelectSharedForestGeometry() && !s.UsesSharedForestRetention(),
                "chapter selection consumes the request only after separate ownership admission");
            s.sceneProfile_=RtSceneProfile::GraphicsPreview;
            failures+=!Check(!s.SelectSharedForestGeometry(),"normal shared request does not enter unrelated preview scenes");
            s.sceneProfile_=RtSceneProfile::Showcase;
#if defined(_DEBUG)
            s.forestRouteProofMode_=Scene::ForestRouteProofMode::BakedResident;
            failures+=!Check(!s.SelectSharedForestGeometry(),"explicit baked control overrides the normal shared request");
            s.forestRouteProofMode_=Scene::ForestRouteProofMode::SharedExisting64;s.sharedForestGeometryActive_=true;
            failures+=!Check(s.SelectSharedForestGeometry() && !s.UsesSharedForestRetention(),
                "explicit shared64 control suppresses normal retention");
            s.forestRouteProofMode_=Scene::ForestRouteProofMode::SharedF01Sector76;
            failures+=!Check(!s.UsesSharedForestRetention(),"supplemental composition never inherits normal retention");
            s.forestRouteProofMode_=Scene::ForestRouteProofMode::Disabled;s.sharedForestGeometryActive_=false;
#endif
            s.ready_=true;
            failures+=!Check(!s.SetSharedForestGeometryEnabled(false) && s.SetSharedForestGeometryEnabled(true) &&
                s.sharedForestGeometryEnabled_,"a live scene rejects changed requests while accepting an unchanged recreation request");
            s.ready_=false;s.device_=Handle<VkDevice>(99);
            const bool deviceLocked=!s.SetSharedForestGeometryEnabled(false) && s.sharedForestGeometryEnabled_;
            s.device_=VK_NULL_HANDLE; // synthetic guard input; never a Vulkan device
            failures+=!Check(deviceLocked,"a retained device identity also locks representation selection");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);
            auto instances=s.tlasBuiltInstances_;s.ApplySharedForestRetentionProof(instances,frame);
            const auto omitted=s.forestRetentionLastOmitted_;const auto count=s.forestRetentionOmittedCount_;
            auto movedOwner=std::make_unique<Scene>(std::move(s));auto& moved=*movedOwner;
            failures+=!Check(moved.sharedForestGeometryEnabled_ && moved.SharedForestGeometryActive() &&
                moved.UsesSharedForestRetention() && moved.ForestRetentionIdentityMatches() &&
                moved.forestRetentionProof_.source==source && moved.forestSharedLayout_.prepared==prepared &&
                moved.forestMasterBlas_[0].address==501u*4096u && moved.forestMasterBlas_[1].address==502u*4096u &&
                moved.forestRetentionLastOmitted_==omitted && moved.forestRetentionOmittedCount_==count &&
                moved.forestRetentionFrameEligible_ && moved.forestRetentionFrameGeneration_==41u &&
                s.sharedForestGeometryEnabled_==Scene::kDefaultSharedForestGeometryEnabled &&
                !s.SharedForestGeometryActive() && !s.forestRetentionProof_.source &&
                !s.forestSharedLayout_.prepared && !s.forestMasterBlas_[0].address && !s.forestMasterBlas_[1].address,
                "move transfers normal shared source/master/retention custody and clears the moved-from owner");
            moved.Destroy();
            failures+=!Check(moved.sharedForestGeometryEnabled_ && !moved.SharedForestGeometryActive() &&
                !moved.UsesSharedForestRetention() && !moved.forestRetentionProof_.source &&
                !moved.forestSharedLayout_.prepared &&
                !moved.forestRetentionFrameEligible_ && !moved.forestRetentionFrameGeneration_ &&
                !moved.forestRetentionOmittedCount_ &&
                std::none_of(moved.forestRetentionLastOmitted_.begin(),moved.forestRetentionLastOmitted_.end(),[](bool b){return b;}),
                "device-null teardown clears CPU shared ownership while preserving the recreate request");
            // These handles are synthetic, with no device or GPU allocation.
            // Real BLAS destruction/recreation belongs to native RT validation.
            moved.forestMasterBlas_={};
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);
            const std::array<std::uint32_t,1> triangles{{0u}},families{{0u}};
            failures+=!Check(bool(RtStaticTriangleOmissions::Create(source,21u,triangles,error)),
                "synthetic common-triangle selection is structurally valid before scene-policy rejection");
            failures+=!Check(!s.CreateStaticGeometryTriangleOmissions(41u,triangles,error) &&
                error.find("Shared forest")!=std::string::npos,
                "normal shared ownership refuses legacy world-triangle certificates");
            RtStaticMeshSlot unchanged=*source;
            failures+=!Check(!s.PackLoadedStaticAssets(unchanged,families,error) &&
                error.find("Shared forest filtered packing")!=std::string::npos &&
                unchanged.PayloadBytes()==source->PayloadBytes(),
                "normal shared ownership refuses old family-filter packing before changing output");
            failures+=!Check(!s.PackLoadedStaticAssets(unchanged,{},error,
                static_cast<std::uint8_t>(StaticTriangleRegion::TombDressing)) &&
                error.find("Shared forest filtered packing")!=std::string::npos,
                "normal shared ownership refuses old triangle-region filtering");
            failures+=!Check(!s.BeginStaticGeometryReplacement(41u,{},error) && !s.preparedStaticGeometry_,
                "normal shared ownership cannot begin a combined baked-owner replacement");
            int oldMap=3,newMap=4;Seed(s,oldMap,newMap);const auto* candidate=s.preparedStaticGeometry_.get();
            Proof proof{&s};const auto api=Api(proof);
            failures+=!Check(!s.CommitStaticGeometryWithApi(41u,api,error) && !proof.proof && !proof.publish && !proof.retire &&
                s.preparedStaticGeometry_.get()==candidate && s.staticGeometryGeneration_==41u,
                "normal shared ownership rejects old combined cutover before external retirement or publication");
        }
        constexpr std::array<const char*,4> failureStates{{"submitted only","completion unproved only",
            "submitted and completion unproved","no owning work"}};
        for(unsigned state=0;state<failureStates.size();++state) {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;seed(s);
            // Execute the actual initialization-failure finalizer with a null
            // device. These are synthetic custody tokens, not submitted GPU
            // work; clearing them below never invokes a Vulkan API.
            s.ready_=false;
            s.forestMasterBuildScratch_=Buffer(503);
            s.forestMasterBuildWork_.command=Handle<VkCommandBuffer>(504);
            s.forestMasterBuildWork_.fence=Handle<VkFence>(505);
            const bool submitted=state==0u || state==2u;
            const bool unproved=state==1u || state==2u;
            s.forestMasterBuildWork_.submitted=submitted;
            s.forestMasterBuildWork_.completionUnproved=unproved;
            const auto scratch=s.forestMasterBuildScratch_.buffer;
            const auto master0=s.forestMasterBlas_[0].handle,master1=s.forestMasterBlas_[1].handle;
            const auto message=std::string("initialization-failure cleanup: ")+failureStates[state];
            s.CleanupFailedInitialisation();
            if(submitted || unproved) {
                failures+=!Check(s.forestMasterBuildWork_.submitted==submitted &&
                    s.forestMasterBuildWork_.completionUnproved==unproved &&
                    s.forestMasterBuildWork_.command==Handle<VkCommandBuffer>(504) &&
                    s.forestMasterBuildWork_.fence==Handle<VkFence>(505) &&
                    s.forestMasterBuildScratch_.buffer==scratch &&
                    s.forestMasterBlas_[0].handle==master0 && s.forestMasterBlas_[1].handle==master1 &&
                    s.fullStaticGeometrySource_==source && s.forestSharedLayout_.prepared==prepared &&
                    s.forestRetentionProof_.source==source && s.forestRetentionProof_.admitted &&
                    s.SharedForestGeometryActive() && s.sharedForestGeometryEnabled_ &&
                    s.staticGeometryGeneration_==41u && s.tlasInstanceCount_==92u,
                    message.c_str());
            }else {
                failures+=!Check(!s.forestMasterBuildWork_.command && !s.forestMasterBuildWork_.fence &&
                    !s.forestMasterBuildWork_.submitted && !s.forestMasterBuildWork_.completionUnproved &&
                    !s.SharedForestGeometryActive() && !s.forestRetentionProof_.source &&
                    !s.forestSharedLayout_.prepared && s.sharedForestGeometryEnabled_,
                    message.c_str());
            }
            // Device-null destruction cannot free fake Vulkan allocations.
            // Remove every synthetic GPU token before ordinary RAII teardown.
            s.forestMasterBuildWork_={};s.forestMasterBuildScratch_={};s.forestMasterBlas_={};s.blas_={};
            s.tlasBuiltInstances_={};s.tlasInstanceDefinitionsValid_=false;
        }
        return failures;
    }
    static int Run() {
        int failures=0,oldMap=3,newMap=4; std::string error;
        {
            using namespace horde::scene;
            using namespace corridor_detail_faces;
            struct Vertex {float position[3];};
            std::vector<Vertex> vertices(kExpectedImmutableTriangleCount*3);
            std::vector<std::uint32_t> indices(vertices.size()),codes(kExpectedImmutableTriangleCount);
            for(unsigned i=0;i<indices.size();++i)indices[i]=i;
            const auto install=[&](const auto& face) {
                for(const auto& t:face) {
                    codes[t.proceduralTriangleIndex]=t.materialId;
                    for(unsigned k=0;k<9;++k)vertices[t.proceduralTriangleIndex*3+k/3].position[k%3]=std::bit_cast<float>(t.xyzBits[k]);
                }
            };
            install(kNorthPlusZ);install(kRoofPlusY);install(kFloorMinusY);
            const auto matches=[&](bool opaque=true) {
                return MatchesAll(std::span<const Vertex>(vertices),std::span<const std::uint32_t>(indices),
                    std::span<const std::uint32_t>(codes),kExpectedImmutableTriangleCount,vertices.size(),opaque);
            };
            failures+=!Check(matches() && !matches(false),"actual blocker certificate requires exact opaque geometry");
            const auto mutate=[&](const auto& face) {
                for(const auto& t:face) {
                    auto& v=vertices[t.proceduralTriangleIndex*3].position[0];
                    const float saved=v;v=std::nextafter(v,INFINITY);
                    failures+=!Check(!matches(),"any certified face displacement invalidates contributor admission");v=saved;
                    ++codes[t.proceduralTriangleIndex];
                    failures+=!Check(!matches(),"changed blocker material invalidates contributor admission");--codes[t.proceduralTriangleIndex];
                }
            };
            mutate(kNorthPlusZ);mutate(kRoofPlusY);mutate(kFloorMinusY);
            const DetailBox forest{{-23.266034,-1.162538,24.000114},{20.060220,10.271029,44.170182}};
            failures+=!Check(DetailForestReceiverEnvelopeBlocked(forest) &&
                DetailTargetBehindCoveredFaces({{-.8,1.27,-5.3},{.7,1.4,-3.3}}),
                "full native target, bounded secondary and directional shadow envelope stays behind verified faces");
            auto bad=forest;bad.min[0]=-10000;
            failures+=!Check(!DetailForestReceiverEnvelopeBlocked(bad),"open west portal prevents an unsafe target certificate");
            bad=forest;bad.min[2]=-13.6;
            failures+=!Check(!DetailTargetBehindCoveredFaces(bad),"grazing or inside target cannot be omitted");
            bad=forest;bad.max[1]=NAN;
            failures+=!Check(!DetailTargetBehindCoveredFaces(bad),"nonfinite target fails closed");
            failures+=!Check(DetailSourceSphereInside({{-22,.6f,-15.2f}},.404) &&
                !DetailSourceSphereInside({{-22,.95f,-15.2f}},.404) &&
                !DetailSourceSphereInside({{-22,.6f,-13.8f}},.404),
                "rotated lantern envelope must fit vertically and horizontally before any sparse frame");
        }
        failures+=KeeperVertexLifetimeChecks();
        failures+=KeeperTextureLifetimeChecks();
        failures+=ForestRetentionChecks();
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;Seed(s,oldMap,newMap);
            horde::gameplay::simulation::GameSimulation simulation;
            Scene::StaticDetailFrame decision{};
            const auto* explicitCandidate=s.preparedStaticGeometry_.get();
            s.ordinaryStaticReduced_=true;s.staticGeometryRetainedAllTriangles_=false;
            failures+=!Check(s.AdvanceOrdinaryStaticDetail(simulation,{},false,decision,error) &&
                decision==Scene::StaticDetailFrame::WaitForRestore &&
                s.preparedStaticGeometry_.get()==explicitCandidate && s.OrdinaryStaticDetailEvidence().waitFrames==1,
                "explicit diagnostic transaction cannot bypass reduced-owner safety after a source change");
            s.ordinaryStaticReduced_=false;s.staticGeometryRetainedAllTriangles_=true;
            failures+=!Check(s.AdvanceOrdinaryStaticDetail(simulation,{},false,decision,error) &&
                decision==Scene::StaticDetailFrame::Render && s.preparedStaticGeometry_.get()==explicitCandidate,
                "complete owner remains renderable while an explicit diagnostic owns preparation");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;Seed(s,oldMap,newMap);s.preparedStaticGeometry_.reset();s.ordinaryStaticCandidate_=true;
            Scene::StaticDetailFrame decision{};
            failures+=!Check(s.RecoverOrdinaryStaticPreparation(true,decision,error) &&
                !s.ordinaryStaticCandidate_ && !s.ordinaryStaticRearm_ && decision==Scene::StaticDetailFrame::Render &&
                s.StaticGeometryHasCompleteContributors() && s.staticVertexBuffer_.buffer==Handle<VkBuffer>(1),
                "failed optional preparation retains full geometry and clears its cursor without retrying every frame");
            s.ordinaryStaticReduced_=true;s.staticGeometryRetainedAllTriangles_=false;s.ordinaryStaticCandidate_=true;
            failures+=!Check(s.RecoverOrdinaryStaticPreparation(false,decision,error) &&
                !s.ordinaryStaticCandidate_ && decision==Scene::StaticDetailFrame::WaitForRestore &&
                !s.StaticGeometryHasCompleteContributors(),
                "failed return preparation allows one new bounded attempt without tracing unsafe partial geometry");
            failures+=!Check(!s.RecoverOrdinaryStaticPreparation(false,decision,error) &&
                s.staticVertexBuffer_.buffer==Handle<VkBuffer>(1) && !s.StaticGeometryHasCompleteContributors(),
                "repeated restore failure reports the failure while retaining the safe-side owner");
            s.staticGeometryCompletionUnproved_=true;
            failures+=!Check(!s.RecoverOrdinaryStaticPreparation(true,decision,error),
                "unproved GPU work cannot enter optional-preparation recovery");
        }
        for(unsigned edge=0;edge<4;++edge) {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;Seed(s,oldMap,newMap);Proof p{&s};p.sharedVertices=true;
            auto& candidate=*s.preparedStaticGeometry_;
            candidate.vertices={};candidate.sharedVertexAddress=s.staticVertexBuffer_.address;
            auto api=FrameApi(p);
            if(edge==1)candidate.sharedVertexAddress+=256;
            if(edge==2)candidate.vertices=Buffer(101);
            if(edge==3)p.descriptorFailure=true;
            const auto committed=s.CommitStaticGeometryWithApi(41,api,error);
            failures+=!Check(s.staticVertexBuffer_.buffer==Handle<VkBuffer>(1),
                "borrowed static vertex owner never transfers or retires at cutover");
            if(edge==0) failures+=!Check(committed && p.publicationCorrect && p.retirementCorrect && p.recordingCorrect,
                "current-frame sparse cutover shares exact vertex address and retires only replaced resources");
            else failures+=!Check(!committed && p.retire==0 && p.record==0 && s.preparedStaticGeometry_,
                "stale vertex address, duplicate vertex ownership and rejected publication preserve the live owner");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;Seed(s,oldMap,newMap);Proof p{&s};auto api=Api(p);
            s.developmentWorldRoute_=true;s.worldRouteGeometry_.valid=true;
            s.vertexBuffer_=Buffer(201);s.indexBuffer_=Buffer(202);s.blas_=As(203);
            const horde::gameplay::simulation::WorldZoneToken token{
                1,horde::gameplay::simulation::WorldZoneId::Lookout};
            using horde::gameplay::simulation::ZoneReadiness;
            s.preparedStaticGeometry_->retainedAllTriangles=false;
            p.descriptorFailure=true;
            failures+=!Check(!s.CommitStaticGeometryWithApi(41,api,error) &&
                s.StaticGeometryHasCompleteContributors() && s.WorldZoneReadiness(token)==ZoneReadiness::Ready &&
                !s.preparedStaticGeometry_->retainedAllTriangles,
                "sparse candidate rollback preserves the full owner and private omission identity");
            p.descriptorFailure=false;p.proof=0;p.publish=0;
            failures+=!Check(s.CommitStaticGeometryWithApi(41,api,error) &&
                s.staticGeometryRetainedAllFamilies_ &&
                s.staticGeometryRetainedTriangleRegions_==horde::scene::assets::kAllStaticTriangleRegions &&
                !s.StaticGeometryHasCompleteContributors() && s.WorldZoneReadiness(token)==ZoneReadiness::Preparing,
                "both region bits and all family rows cannot hide omitted triangles from traversal readiness");
            auto movedOwner=std::make_unique<Scene>(std::move(s));auto& moved=*movedOwner;
            failures+=!Check(!moved.StaticGeometryHasCompleteContributors() &&
                moved.WorldZoneReadiness(token)==ZoneReadiness::Preparing && s.StaticGeometryHasCompleteContributors(),
                "move transfers sparse omission state and resets the old owner");
            moved.staticGeometryRetainedAllTriangles_=true;
            failures+=!Check(moved.WorldZoneReadiness(token)==ZoneReadiness::Ready,
                "complete triangle restoration can restore traversal readiness");
            moved.staticGeometryRetainedAllTriangles_=false;moved.Destroy();
            failures+=!Check(moved.StaticGeometryHasCompleteContributors(),
                "teardown clears sparse state for scene reconstruction");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;Seed(s,oldMap,newMap);Proof p{&s};auto api=Api(p);
            s.developmentWorldRoute_=true;s.worldRouteGeometry_.valid=true;
            s.vertexBuffer_=Buffer(201);s.indexBuffer_=Buffer(202);s.blas_=As(203);
            const horde::gameplay::simulation::WorldZoneToken token{
                1,horde::gameplay::simulation::WorldZoneId::Lookout};
            using horde::gameplay::simulation::ZoneReadiness;
            failures+=!Check(s.WorldZoneReadiness(token)==ZoneReadiness::Ready,
                "complete scene keeps its real zone readiness");
            s.preparedStaticGeometry_->retainedTriangleRegions=2u;
            p.descriptorFailure=true;
            failures+=!Check(!s.CommitStaticGeometryWithApi(41,api,error) &&
                s.StaticGeometryHasCompleteContributors() && s.WorldZoneReadiness(token)==ZoneReadiness::Ready,
                "rejected partial candidate cannot change admitted contributor readiness");
            p.descriptorFailure=false;p.proof=0;p.publish=0;
            failures+=!Check(s.CommitStaticGeometryWithApi(41,api,error) &&
                !s.StaticGeometryHasCompleteContributors() && s.WorldZoneReadiness(token)==ZoneReadiness::Preparing,
                "partial diagnostic owner cannot authorize normal traversal without a contributor proof");
            auto movedOwner=std::make_unique<Scene>(std::move(s));auto& moved=*movedOwner;
            failures+=!Check(!moved.StaticGeometryHasCompleteContributors() &&
                moved.WorldZoneReadiness(token)==ZoneReadiness::Preparing && s.StaticGeometryHasCompleteContributors(),
                "scene move carries partial contributor state and resets the moved-from owner");
            moved.staticGeometryRetainedTriangleRegions_=horde::scene::assets::kAllStaticTriangleRegions;
            moved.staticGeometryRetainedAllFamilies_=false;
            failures+=!Check(moved.WorldZoneReadiness(token)==ZoneReadiness::Preparing,
                "whole-family diagnostic omission also fails normal traversal readiness closed");
            moved.staticGeometryRetainedAllFamilies_=true;
            failures+=!Check(moved.WorldZoneReadiness(token)==ZoneReadiness::Ready,
                "complete restoration admits zone readiness again");
            moved.Destroy();
            failures+=!Check(moved.StaticGeometryHasCompleteContributors(),
                "teardown resets contributor state for reconstruction");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;Seed(s,oldMap,newMap);Proof p{&s};auto api=FrameApi(p);
            s.tlasBuiltInstances_[0].transform.matrix[0][3]=4.f;
            s.tlasBuiltInstances_[2].mask=0;
            failures+=!Check(s.CommitStaticGeometryWithApi(41,api,error) && p.recordingCorrect &&
                p.record==1 && p.retire==1 && s.tlasPendingDefinitionsValid_ &&
                s.staticGeometryGeneration_==42,
                "moving candidate publishes only with a fresh current-frame record before retirement");
            failures+=!Check(s.tlasBuiltInstances_[20].transform.matrix[0][3]!=9.f,
                "recording remains distinct from successful graphics submission");
            s.NotifyFrameSubmitted();
            failures+=!Check(s.tlasBuiltInstances_[20].transform.matrix[0][3]==9.f &&
                s.tlasBuiltInstances_[21].transform.matrix[1][3]==8.f &&
                s.tlasBuiltInstances_[2].transform.matrix[2][3]==7.f && s.tlasBuiltInstances_[2].mask==0x01 &&
                !s.tlasPendingDefinitionsValid_,
                "first successful frame preserves latest player, held and actor poses and masks");
        }
        for(unsigned edge=0;edge<5;++edge){
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;Seed(s,oldMap,newMap);Proof p{&s};auto api=FrameApi(p);
            p.recordFailure=edge==0;p.recordThrow=edge==1;p.omitDefinitions=edge==2;
            p.wrongWorld=edge==3;p.resetDuringRecord=edge==4;
            failures+=!Check(!s.CommitStaticGeometryWithApi(41,api,error) && p.record==1 && p.retire==0 &&
                !s.ready_ && s.staticGeometryCompletionUnproved_ && s.preparedStaticGeometry_ &&
                s.staticVertexBuffer_.buffer==Handle<VkBuffer>(101) &&
                s.preparedStaticGeometry_->vertices.buffer==Handle<VkBuffer>(1),
                "failed/invalid fresh recording retains both referenced owners and stops publication readiness");
            failures+=!Check(!s.CancelStaticGeometryReplacement(error),
                "failed fresh recording cannot destroy resources before recorded-work retirement proof");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner;Seed(s,oldMap,newMap);Proof p{&s};auto api=FrameApi(p);p.descriptorFailure=true;
            failures+=!Check(!s.CommitStaticGeometryWithApi(41,api,error) && p.record==0 && p.retire==0 &&
                s.staticVertexBuffer_.buffer==Handle<VkBuffer>(1) && s.preparedStaticGeometry_ && s.ready_,
                "descriptor rejection rolls back before any first-frame command references the candidate");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner; Seed(s,oldMap,newMap); Proof p{&s}; auto api=Api(p);
            failures+=!Check(s.CommitStaticGeometryWithApi(41,api,error),"valid current transaction commits");
            failures+=!Check(p.publicationCorrect && p.retirementCorrect && p.publish==1 && p.retire==1,
                "completion precedes exact publication, cache invalidation and exactly-once retirement");
            failures+=!Check(s.instanceMetadataBuffer_.mappedWriteData==&newMap && s.instanceBuffer_.mappedWriteData==&newMap,
                "mapped metadata/TLAS ownership transfers to replacement");
            failures+=!Check(!s.preparedStaticGeometry_ && !s.CommitStaticGeometryWithApi(41,api,error) && p.retire==1,
                "consumed/stale candidate cannot republish or retire twice");
        }
        for(unsigned edge=0;edge<16;++edge) {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner; Seed(s,oldMap,newMap); Proof p{&s}; auto api=Api(p); std::uint64_t token=41;
            switch(edge) {
                case 0: token=0; break; case 1: token=40; break;
                case 2: s.staticGeometryGeneration_=token=UINT64_MAX; s.preparedStaticGeometry_->sourceGeneration=token; break;
                case 3: s.preparedStaticGeometry_->ready=false; break;
                case 4: p.succeed=false; break; case 5: p.stale=true; break;
                case 6: s.staticGeometryCompletionUnproved_=true; break;
                case 7: s.tlasPendingDefinitionsValid_=true; break;
                case 8: api.publishDescriptors=nullptr; break; case 9: s.ready_=false; break;
                case 10: s.tlasBuiltInstances_[0].transform.matrix[0][3]+=1.0f; break;
                case 11: s.tlasBuiltInstances_[0].mask=0u; break;
                case 12: s.tlasBuiltInstances_[0].accelerationStructureReference=999u; break;
                case 13: p.poseChange=true; break;
                case 14: s.tlasInstanceDefinitionsValid_=false; break;
                case 15: s.preparedStaticGeometry_->buildWork.submitted=true; break;
            }
            failures+=!Check(!s.CommitStaticGeometryWithApi(token,api,error),"invalid/unproved cutover fails closed");
            failures+=!Check(s.staticVertexBuffer_.buffer==Handle<VkBuffer>(1) && s.tlas_.handle==Handle<VkAccelerationStructureKHR>(8) &&
                s.instanceMetadataBuffer_.mappedWriteData==&oldMap && s.preparedStaticGeometry_ &&
                s.preparedStaticGeometry_->vertices.buffer==Handle<VkBuffer>(101) && p.publish==0 && p.retire==0,
                "failed or stale proof preserves both owners, descriptors and mappings");
            failures+=!Check(p.proof == ((edge==4 || edge==5 || edge==13)?1u:0u),"ineligible candidate never invokes external retirement proof");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner; Seed(s,oldMap,newMap); Proof p{&s}; auto api=Api(p); p.succeed=false;
            failures+=!Check(!s.CommitStaticGeometryWithApi(41,api,error),"first proof fails");
            p.succeed=true; p.proof=0;
            failures+=!Check(s.CommitStaticGeometryWithApi(41,api,error) && p.retire==1,"unchanged candidate may retry after real proof succeeds");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner; Seed(s,oldMap,newMap); Proof p{&s}; auto api=Api(p); p.descriptorFailure=true;
            failures+=!Check(!s.CommitStaticGeometryWithApi(41,api,error),"descriptor contract rejection fails cutover");
            failures+=!Check(s.staticGeometryGeneration_==41 && s.staticVertexBuffer_.buffer==Handle<VkBuffer>(1) &&
                s.tlas_.handle==Handle<VkAccelerationStructureKHR>(8) && s.instanceBuffer_.mappedWriteData==&oldMap &&
                s.chapterActorPoseGeneration_==17 && !s.waterDropletGeometryVisible_ && s.preparedStaticGeometry_ &&
                s.preparedStaticGeometry_->vertices.buffer==Handle<VkBuffer>(101) && p.publish==1 && p.retire==0,
                "descriptor failure rolls back all owners, caches, mappings and generation without retirement");
            p.descriptorFailure=false; p.proof=0; p.publish=0;
            failures+=!Check(s.CommitStaticGeometryWithApi(41,api,error) && p.retire==1,"same candidate retries after descriptor contract succeeds");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner; Seed(s,oldMap,newMap); Proof p{&s}; auto api=Api(p); p.descriptorThrow=true;
            failures+=!Check(!s.CommitStaticGeometryWithApi(41,api,error) && p.retire==0 &&
                s.staticGeometryGeneration_==41 && s.staticVertexBuffer_.buffer==Handle<VkBuffer>(1) &&
                s.preparedStaticGeometry_ && s.chapterActorPoseGeneration_==17,
                "descriptor preparation exception preserves both owners and old cache state");
        }
        {
            auto originalOwner=std::make_unique<Scene>();auto& original=*originalOwner; Seed(original,oldMap,newMap); auto movedOwner=std::make_unique<Scene>(std::move(original));auto& moved=*movedOwner;
            Proof p{&moved}; auto api=Api(p);
            failures+=!Check(!original.preparedStaticGeometry_ && moved.preparedStaticGeometry_ &&
                moved.staticGeometryGeneration_==41 && moved.instanceBuffer_.mappedWriteData==&oldMap &&
                moved.CommitStaticGeometryWithApi(41,api,error) && p.retire==1,
                "scene move transfers the single candidate, generation and mapped owner before cutover");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner; Seed(s,oldMap,newMap);
            s.preparedStaticGeometry_->ready=false;
            s.preparedStaticGeometry_->sourceGeneration=40;
            failures+=!Check(!s.AdvanceStaticGeometryReplacement(41,256u,error) &&
                s.preparedStaticGeometry_ && s.staticGeometryGeneration_==41,
                "stale preparation cannot advance or publish; cancellation remains explicit");
            failures+=!Check(s.CancelStaticGeometryReplacement(error) && !s.preparedStaticGeometry_ &&
                s.CancelStaticGeometryReplacement(error) && s.staticGeometryGeneration_==41 &&
                s.staticVertexBuffer_.buffer==Handle<VkBuffer>(1) && s.tlas_.handle==Handle<VkAccelerationStructureKHR>(8),
                "stale cancellation is idempotent and preserves admitted owner for preparation retry");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner; Seed(s,oldMap,newMap); s.staticGeometryCompletionUnproved_=true;
            failures+=!Check(!s.CancelStaticGeometryReplacement(error) && s.preparedStaticGeometry_ &&
                s.preparedStaticGeometry_->vertices.buffer==Handle<VkBuffer>(101),
                "cancellation cannot free resources referenced by unproved GPU work");
            s.staticGeometryCompletionUnproved_=false;
            failures+=!Check(s.CancelStaticGeometryReplacement(error) && !s.preparedStaticGeometry_,
                "completed work permits explicit cancellation without retiring admitted resources");
        }
        {
            auto sceneOwner=std::make_unique<Scene>();auto& s=*sceneOwner; Seed(s,oldMap,newMap);
            auto latest=s.tlasBuiltInstances_;
            latest[20].transform.matrix[0][3]=3.f;
            latest[21].transform.matrix[1][3]=4.f;
            latest[2].transform.matrix[2][3]=5.f;
            failures+=!Check(!RequiresTlasInstanceRebuild(s.tlasBuiltInstances_,latest),
                "transform-only player/equipment/actor motion remains UPDATE, not BUILD");
            s.StageTlasSubmissionDefinitions(latest);
            failures+=!Check(s.tlasBuiltInstances_[20].transform.matrix[0][3]!=3.f,
                "unsubmitted recording cannot advance admitted pose definitions");
            s.NotifyFrameSubmitted();
            failures+=!Check(s.tlasBuiltInstances_[20].transform.matrix[0][3]==3.f &&
                s.tlasBuiltInstances_[21].transform.matrix[1][3]==4.f &&
                s.tlasBuiltInstances_[2].transform.matrix[2][3]==5.f,
                "every submitted UPDATE supplies exact latest pose to zone preparation");
            Proof p{&s};auto api=Api(p);
            failures+=!Check(!s.CommitStaticGeometryWithApi(41,api,error)&&p.proof==0,
                "transform-only submitted motion rejects stale candidate before publication");
            const auto vertices=s.preparedStaticGeometry_->vertices.buffer;
            const auto collapse=s.preparedStaticGeometry_->collapse.handle;
            failures+=!Check(!s.RefreshStaticGeometryReplacement(40,error)&&s.preparedStaticGeometry_->ready,
                "stale source generation cannot schedule candidate refresh");
            failures+=!Check(s.RefreshStaticGeometryReplacement(41,error)&&!s.preparedStaticGeometry_->ready&&
                s.preparedStaticGeometry_->phase==Scene::PreparedStaticGeometry::Phase::Tlas&&
                s.preparedStaticGeometry_->vertices.buffer==vertices&&s.preparedStaticGeometry_->collapse.handle==collapse,
                "fresh pose refresh reuses completed geometry without restarting CPU/upload/BLAS owners");
            failures+=!Check(!s.RefreshStaticGeometryReplacement(41,error),
                "incomplete refresh cannot overwrite its TLAS input/scratch lifetime");
        }
        return failures?1:0;
    }
};
}
int main() { std::cerr<<"sceneBytes="<<sizeof(horde::vulkan::raytracing::PresentableTinyRtScene)<<std::endl; return horde::vulkan::raytracing::PresentableTinyRtSceneStaticCutoverTestAccess::Run(); }
