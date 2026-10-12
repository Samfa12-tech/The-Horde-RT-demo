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
        unsigned proof=0, publish=0, retire=0;
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
    static int Run() {
        int failures=0,oldMap=3,newMap=4; std::string error;
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
