#include "vulkan/raytracing/PresentableTinyRtScene.h"
#include "vulkan/raytracing/TlasInstanceRefresh.h"
#include <iostream>
#include <type_traits>
#include <stdexcept>
#include <utility>

namespace horde::vulkan::raytracing {
struct PresentableTinyRtSceneStaticCutoverTestAccess {
    using Scene = PresentableTinyRtScene;
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
                    s.staticVertexBuffer_.buffer==Handle<VkBuffer>(101) && s.tlas_.handle==Handle<VkAccelerationStructureKHR>(108);
                if(p.descriptorThrow) throw std::runtime_error("Injected descriptor preparation exception");
                if(p.descriptorFailure) error="Injected descriptor contract rejection before publication";
                return !p.descriptorFailure;
            },
            [](void* u,Scene& s,Scene::PreparedStaticGeometry& c) noexcept {
                auto& p=*static_cast<Proof*>(u); ++p.retire;
                p.retirementCorrect = p.publish==1 && s.staticGeometryGeneration_==42 &&
                    s.collapseBlas_.address==107*4096 && s.tlasBuiltInstances_[Scene::kCollapseInstanceIndex].accelerationStructureReference==107*4096 &&
                    s.tlasBuiltInstances_[0].instanceCustomIndex==21 && s.tlasInstanceDefinitionsValid_ && !s.tlasPendingDefinitionsValid_ &&
                    s.lastInstanceMasks_[0]==0x41 && s.chapterActorPoseGeneration_==UINT64_MAX && s.chapterActorPoseSeconds_<0 &&
                    s.chapterActorNeedsRefit_ && s.waterDropletGeometryVisible_ &&
                    s.worldPlayerVertexBuffer_.buffer==Handle<VkBuffer>(10) && s.viewmodelVertexBuffer_.buffer==Handle<VkBuffer>(11) && c.vertices.buffer==Handle<VkBuffer>(1) &&
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
                s.staticGeometryGeneration_==41 && s.staticVertexBuffer_.buffer==Handle<VkBuffer>(101) &&
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
            Scene s;seed(s);Calls c{&s};auto a=api(c);
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
            Scene s;seed(s);Calls c{&s};auto a=api(c);
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
            Scene s;seed(s);s.characterSlot_.LichGpu().vertices={};
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
            Scene s;seed(s);Calls c{&s};auto a=api(c);
            failures+=!Check(s.PrepareThirdActorVertexLifetimeWithApi(frame,a,error),
                "retired actor owner may move only after completed-work retirement");
            Scene moved(std::move(s));c={&moved};a=api(c);
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
    static int Run() {
        int failures=0,oldMap=3,newMap=4; std::string error;
        failures+=KeeperVertexLifetimeChecks();
        {
            Scene s;Seed(s,oldMap,newMap);Proof p{&s};auto api=Api(p);
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
            Scene moved(std::move(s));
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
            Scene s;Seed(s,oldMap,newMap);Proof p{&s};auto api=FrameApi(p);
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
            Scene s;Seed(s,oldMap,newMap);Proof p{&s};auto api=FrameApi(p);
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
            Scene s;Seed(s,oldMap,newMap);Proof p{&s};auto api=FrameApi(p);p.descriptorFailure=true;
            failures+=!Check(!s.CommitStaticGeometryWithApi(41,api,error) && p.record==0 && p.retire==0 &&
                s.staticVertexBuffer_.buffer==Handle<VkBuffer>(1) && s.preparedStaticGeometry_ && s.ready_,
                "descriptor rejection rolls back before any first-frame command references the candidate");
        }
        {
            Scene s; Seed(s,oldMap,newMap); Proof p{&s}; auto api=Api(p);
            failures+=!Check(s.CommitStaticGeometryWithApi(41,api,error),"valid current transaction commits");
            failures+=!Check(p.publicationCorrect && p.retirementCorrect && p.publish==1 && p.retire==1,
                "completion precedes exact publication, cache invalidation and exactly-once retirement");
            failures+=!Check(s.instanceMetadataBuffer_.mappedWriteData==&newMap && s.instanceBuffer_.mappedWriteData==&newMap,
                "mapped metadata/TLAS ownership transfers to replacement");
            failures+=!Check(!s.preparedStaticGeometry_ && !s.CommitStaticGeometryWithApi(41,api,error) && p.retire==1,
                "consumed/stale candidate cannot republish or retire twice");
        }
        for(unsigned edge=0;edge<16;++edge) {
            Scene s; Seed(s,oldMap,newMap); Proof p{&s}; auto api=Api(p); std::uint64_t token=41;
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
            Scene s; Seed(s,oldMap,newMap); Proof p{&s}; auto api=Api(p); p.succeed=false;
            failures+=!Check(!s.CommitStaticGeometryWithApi(41,api,error),"first proof fails");
            p.succeed=true; p.proof=0;
            failures+=!Check(s.CommitStaticGeometryWithApi(41,api,error) && p.retire==1,"unchanged candidate may retry after real proof succeeds");
        }
        {
            Scene s; Seed(s,oldMap,newMap); Proof p{&s}; auto api=Api(p); p.descriptorFailure=true;
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
            Scene s; Seed(s,oldMap,newMap); Proof p{&s}; auto api=Api(p); p.descriptorThrow=true;
            failures+=!Check(!s.CommitStaticGeometryWithApi(41,api,error) && p.retire==0 &&
                s.staticGeometryGeneration_==41 && s.staticVertexBuffer_.buffer==Handle<VkBuffer>(1) &&
                s.preparedStaticGeometry_ && s.chapterActorPoseGeneration_==17,
                "descriptor preparation exception preserves both owners and old cache state");
        }
        {
            Scene original; Seed(original,oldMap,newMap); Scene moved(std::move(original));
            Proof p{&moved}; auto api=Api(p);
            failures+=!Check(!original.preparedStaticGeometry_ && moved.preparedStaticGeometry_ &&
                moved.staticGeometryGeneration_==41 && moved.instanceBuffer_.mappedWriteData==&oldMap &&
                moved.CommitStaticGeometryWithApi(41,api,error) && p.retire==1,
                "scene move transfers the single candidate, generation and mapped owner before cutover");
        }
        {
            Scene s; Seed(s,oldMap,newMap);
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
            Scene s; Seed(s,oldMap,newMap); s.staticGeometryCompletionUnproved_=true;
            failures+=!Check(!s.CancelStaticGeometryReplacement(error) && s.preparedStaticGeometry_ &&
                s.preparedStaticGeometry_->vertices.buffer==Handle<VkBuffer>(101),
                "cancellation cannot free resources referenced by unproved GPU work");
            s.staticGeometryCompletionUnproved_=false;
            failures+=!Check(s.CancelStaticGeometryReplacement(error) && !s.preparedStaticGeometry_,
                "completed work permits explicit cancellation without retiring admitted resources");
        }
        {
            Scene s; Seed(s,oldMap,newMap);
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
int main() { return horde::vulkan::raytracing::PresentableTinyRtSceneStaticCutoverTestAccess::Run(); }
