#include "vulkan/raytracing/experimental/HybridOpaqueRoomPass.h"
#include "vulkan/raytracing/experimental/HybridStaticPrimaryControl.h"
#include "HybridOpaqueRoomShaders.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
namespace horde::vulkan::raytracing::experimental {
namespace {
VkShaderModule Module(VkDevice d,std::span<const uint32_t> words){
    VkShaderModuleCreateInfo c{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};c.codeSize=words.size_bytes();c.pCode=words.data();
    VkShaderModule m{};return vkCreateShaderModule(d,&c,nullptr,&m)==VK_SUCCESS?m:VK_NULL_HANDLE;
}
VkPipelineShaderStageCreateInfo Stage(VkShaderStageFlagBits s,VkShaderModule m){return {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,s,m,"main",nullptr};}
VkAccelerationStructureGeometryKHR WorldGeometry(const RtGpuBuffer& vertices,const RtGpuBuffer& indices){
    VkAccelerationStructureGeometryKHR g{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    g.geometryType=VK_GEOMETRY_TYPE_TRIANGLES_KHR;g.flags=VK_GEOMETRY_OPAQUE_BIT_KHR;
    auto& t=g.geometry.triangles;t.sType=VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    t.vertexFormat=VK_FORMAT_R32G32B32_SFLOAT;t.vertexData.deviceAddress=vertices.address;t.vertexStride=12;
    t.maxVertex=static_cast<uint32_t>(vertices.size/12-1);t.indexType=VK_INDEX_TYPE_UINT32;t.indexData.deviceAddress=indices.address;return g;
}
VkAccelerationStructureGeometryKHR InstanceGeometry(const RtGpuBuffer& instances){
    VkAccelerationStructureGeometryKHR g{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    g.geometryType=VK_GEOMETRY_TYPE_INSTANCES_KHR;g.geometry.instances.sType=VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    g.geometry.instances.data.deviceAddress=instances.address;return g;
}
void ImageBarrier(VkCommandBuffer c,VkImage image,VkImageLayout oldLayout,VkImageLayout newLayout,
    VkPipelineStageFlags source,VkPipelineStageFlags dest,VkAccessFlags sourceAccess,VkAccessFlags destAccess){
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.srcAccessMask=sourceAccess;b.dstAccessMask=destAccess;
    b.oldLayout=oldLayout;b.newLayout=newLayout;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.image=image;
    b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};vkCmdPipelineBarrier(c,source,dest,0,0,nullptr,0,nullptr,1,&b);
}
}
std::unique_ptr<HybridOpaqueRoomPass> HybridOpaqueRoomPass::Create(VkInstance instance,VkPhysicalDevice physical,VkDevice device,
    const RtGpuResources& gpu,VkDescriptorSetLayout sceneLayout,VkDescriptorSet sceneSet,VkExtent2D extent,
    const RtExecutionPolicy& policy,const RtPipelineBundleBuildApi& api,const RtStaticMeshSlot& slot,
    const RtGpuBuffer& vertices,const RtGpuBuffer& indices,const RtGpuBuffer& world,
    std::span<const uint32_t> worldIndices,std::span<const uint32_t> worldCodes,std::string& error){
    auto p=std::unique_ptr<HybridOpaqueRoomPass>(new HybridOpaqueRoomPass);
    p->device_=device;p->physical_=physical;p->resources_=gpu;p->policy_=policy;p->sceneSet_=sceneSet;
    p->vertices_=vertices;p->indices_=indices;p->worldVertices_=world;
    if(!p->Initialise(instance,sceneLayout,extent,api,slot,worldIndices,worldCodes,error))return nullptr;return p;
}
HybridOpaqueRoomPass::~HybridOpaqueRoomPass(){
    if(!device_)return;
    DestroyTargets(replacement_);DestroyTargets(targets_);
    if(raster_)vkDestroyPipeline(device_,raster_,nullptr);
    if(reference_.pipeline)vkDestroyPipeline(device_,reference_.pipeline,nullptr);
    resources_.DestroyBuffer(reference_.shaderBindingTable);
    if(layout_)vkDestroyPipelineLayout(device_,layout_,nullptr);
    if(renderPass_)vkDestroyRenderPass(device_,renderPass_,nullptr);
    if(pool_)vkDestroyDescriptorPool(device_,pool_,nullptr);
    if(extraLayout_)vkDestroyDescriptorSetLayout(device_,extraLayout_,nullptr);
    resources_.DestroyAccelerationStructure(roomTlas_);resources_.DestroyAccelerationStructure(worldBlas_);
    for(auto* b:{&drawBuffer_,&worldIndices_,&worldMap_,&instances_,&worldScratch_,&tlasScratch_})resources_.DestroyBuffer(*b);
}
bool HybridOpaqueRoomPass::MakeAs(VkAccelerationStructureTypeKHR type,const VkAccelerationStructureGeometryKHR& geometry,
    uint32_t count,RtAccelerationStructure& out,RtGpuBuffer& scratch,std::string& error){
    VkAccelerationStructureBuildGeometryInfoKHR b{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    b.type=type;b.flags=VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;b.geometryCount=1;b.pGeometries=&geometry;
    VkAccelerationStructureBuildSizesInfoKHR size{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    sizes_(device_,VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,&b,&count,&size);
    if(!resources_.CreateBuffer(size.accelerationStructureSize,VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,true,out.backing,error)||
       !resources_.CreateAlignedBuffer(size.buildScratchSize,scratchAlignment_,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,scratch,error))return false;
    VkAccelerationStructureCreateInfoKHR c{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};c.buffer=out.backing.buffer;c.size=size.accelerationStructureSize;c.type=type;
    if(createAs_(device_,&c,nullptr,&out.handle)!=VK_SUCCESS)return Fail(error,"Opaque room AS allocation failed.");
    VkAccelerationStructureDeviceAddressInfoKHR address{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};address.accelerationStructure=out.handle;
    out.address=asAddress_(device_,&address);return out.address!=0||Fail(error,"Opaque room AS address unavailable.");
}
bool HybridOpaqueRoomPass::Initialise(VkInstance instance,VkDescriptorSetLayout sceneLayout,VkExtent2D extent,
    const RtPipelineBundleBuildApi& api,const RtStaticMeshSlot& slot,std::span<const uint32_t> inputIndices,
    std::span<const uint32_t> codes,std::string& error){
    if(!sceneLayout||!sceneSet_||!worldVertices_.buffer||!vertices_.buffer||!indices_.buffer||inputIndices.size()!=codes.size()*3)
        return Fail(error,"Opaque architecture profile requires real matching world and PBR source buffers.");
    for(size_t i=0;i<codes.size();++i){
        const uint32_t material=codes[i]&255u;
        if(material==5u||material==9u||material==10u){++excludedTriangles_;continue;}
        mapCpu_.push_back(static_cast<uint32_t>(i));indicesCpu_.insert(indicesCpu_.end(),inputIndices.begin()+i*3,inputIndices.begin()+i*3+3);
    }
    if(indicesCpu_.empty())return Fail(error,"Opaque architecture profile has no world triangles.");
    Draw world{};world.ids={0,0,0,0};drawsCpu_.push_back(world);counts_.push_back(static_cast<uint32_t>(indicesCpu_.size()));
    const auto owner=slot.InstanceMetadata()[21];
    if(owner.primitiveBase>slot.PrimitiveMetadata().size()||owner.primitiveCount>slot.PrimitiveMetadata().size()-owner.primitiveBase)
        return Fail(error,"Opaque architecture PBR range invalid.");
    const std::array<float,12> identity{1,0,0,0,0,1,0,0,0,0,1,0};
    for(uint32_t i=0;i<owner.primitiveCount;++i){
        const auto& p=slot.PrimitiveMetadata()[owner.primitiveBase+i];const auto& m=slot.Materials()[p.materialIndex];
        if((m.materialFlags[0]&(static_cast<uint32_t>(RtMaterialFlag::Transmission)|static_cast<uint32_t>(RtMaterialFlag::Alpha)))||
            m.metallicRoughnessOcclusionTransmission[3]>0||slot.GeometryTransforms()[owner.primitiveBase+i]!=identity)
            return Fail(error,"Opaque architecture profile refuses transmitting/alpha or unbaked asset geometry.");
        Draw d{};d.ids={21,i,p.indexOffset,p.vertexOffset};drawsCpu_.push_back(d);counts_.push_back(p.indexCount);
    }
    VkPhysicalDeviceAccelerationStructurePropertiesKHR asp{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
    VkPhysicalDeviceProperties2 props{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};props.pNext=&asp;
    auto getProps=reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(vkGetInstanceProcAddr(instance,"vkGetPhysicalDeviceProperties2"));
    if(!getProps)getProps=reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(vkGetInstanceProcAddr(instance,"vkGetPhysicalDeviceProperties2KHR"));
    if(!getProps)return Fail(error,"Opaque room physical properties query unavailable.");
    getProps(physical_,&props);scratchAlignment_=asp.minAccelerationStructureScratchOffsetAlignment;
    sizes_=reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device_,"vkGetAccelerationStructureBuildSizesKHR"));
    createAs_=reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device_,"vkCreateAccelerationStructureKHR"));
    asAddress_=reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device_,"vkGetAccelerationStructureDeviceAddressKHR"));
    build_=reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device_,"vkCmdBuildAccelerationStructuresKHR"));
    trace_=reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device_,"vkCmdTraceRaysKHR"));
    if(!sizes_||!createAs_||!asAddress_||!build_||(policy_.requiresShaderBindingTable&&!trace_))
        return Fail(error,"Opaque architecture AS/dispatch functions unavailable.");
    const auto host=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const auto indexUsage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT|VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    if(!resources_.CreateBuffer(indicesCpu_.size()*4,indexUsage,host,true,worldIndices_,error)||
       !resources_.WriteBuffer(worldIndices_,indicesCpu_.data(),indicesCpu_.size()*4,"opaque world indices",error)||
       !resources_.CreateBuffer(mapCpu_.size()*4,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,false,worldMap_,error)||
       !resources_.WriteBuffer(worldMap_,mapCpu_.data(),mapCpu_.size()*4,"opaque world primitive map",error)||
       !resources_.CreateBuffer(drawsCpu_.size()*sizeof(Draw),VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,false,drawBuffer_,error)||
       !resources_.CreateBuffer(2*sizeof(VkAccelerationStructureInstanceKHR),VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT|
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,host,true,instances_,error))return false;
    if(!MakeAs(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,WorldGeometry(worldVertices_,worldIndices_),
            static_cast<uint32_t>(mapCpu_.size()),worldBlas_,worldScratch_,error)||
       !MakeAs(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,InstanceGeometry(instances_),2,roomTlas_,tlasScratch_,error))return false;
    const VkShaderStageFlags shade=policy_.shaderStage|VK_SHADER_STAGE_FRAGMENT_BIT;
    const VkDescriptorSetLayoutBinding bindings[]={{1,VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,1,shade,nullptr},
        {2,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,shade|VK_SHADER_STAGE_VERTEX_BIT,nullptr},
        {3,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr},{4,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr},
        {5,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr},{6,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr},
        {7,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,shade|VK_SHADER_STAGE_VERTEX_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo sl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};sl.bindingCount=7;sl.pBindings=bindings;
    if(vkCreateDescriptorSetLayout(device_,&sl,nullptr,&extraLayout_)!=VK_SUCCESS)return Fail(error,"Opaque room descriptors unavailable.");
    const VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,1},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,6}};
    VkDescriptorPoolCreateInfo pc{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pc.maxSets=1;pc.poolSizeCount=2;pc.pPoolSizes=sizes;
    if(vkCreateDescriptorPool(device_,&pc,nullptr,&pool_)!=VK_SUCCESS)return Fail(error,"Opaque room descriptor pool failed.");
    VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=pool_;da.descriptorSetCount=1;da.pSetLayouts=&extraLayout_;
    if(vkAllocateDescriptorSets(device_,&da,&extraSet_)!=VK_SUCCESS)return Fail(error,"Opaque room descriptor allocation failed.");
    VkWriteDescriptorSetAccelerationStructureKHR as{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    as.accelerationStructureCount=1;as.pAccelerationStructures=&roomTlas_.handle;
    const RtGpuBuffer* buffers[]={&drawBuffer_,&vertices_,&indices_,&worldVertices_,&worldIndices_,&worldMap_};
    VkDescriptorBufferInfo bi[6]{};VkWriteDescriptorSet writes[7]{};
    for(unsigned i=0;i<7;++i){auto& w=writes[i];w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w.dstSet=extraSet_;w.dstBinding=i+1;w.descriptorCount=1;
        w.descriptorType=i==0?VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        if(i){bi[i-1]={buffers[i-1]->buffer,0,buffers[i-1]->size};w.pBufferInfo=&bi[i-1];}else w.pNext=&as;}
    vkUpdateDescriptorSets(device_,7,writes,0,nullptr);
    const VkDescriptorSetLayout layouts[]={sceneLayout,extraLayout_};
    const VkPushConstantRange push{policy_.pushConstantStages|VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,128};
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=2;pl.pSetLayouts=layouts;pl.pushConstantRangeCount=1;pl.pPushConstantRanges=&push;
    if(vkCreatePipelineLayout(device_,&pl,nullptr,&layout_)!=VK_SUCCESS)return Fail(error,"Opaque room pipeline layout failed.");
    VkAttachmentDescription attachments[2]{};
    attachments[0].format=VK_FORMAT_R8G8B8A8_UNORM;attachments[0].samples=VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachments[0].storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachments[0].finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    attachments[1].format=VK_FORMAT_D32_SFLOAT;attachments[1].samples=VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachments[1].storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachments[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    const VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},depth{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&color;sub.pDepthStencilAttachment=&depth;
    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.attachmentCount=2;rp.pAttachments=attachments;rp.subpassCount=1;rp.pSubpasses=&sub;
    if(vkCreateRenderPass(device_,&rp,nullptr,&renderPass_)!=VK_SUCCESS||!MakeTargets(extent,targets_,error))return false;
    VkShaderModule vs=Module(device_,room_vert),fs=Module(device_,room_frag);
    if(!vs||!fs){if(vs)vkDestroyShaderModule(device_,vs,nullptr);if(fs)vkDestroyShaderModule(device_,fs,nullptr);return Fail(error,"Opaque room graphics shaders failed.");}
    const VkPipelineShaderStageCreateInfo stages[]={Stage(VK_SHADER_STAGE_VERTEX_BIT,vs),Stage(VK_SHADER_STAGE_FRAGMENT_BIT,fs)};
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.polygonMode=VK_POLYGON_MODE_FILL;rs.cullMode=VK_CULL_MODE_NONE;rs.lineWidth=1;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};ds.depthTestEnable=ds.depthWriteEnable=VK_TRUE;ds.depthCompareOp=VK_COMPARE_OP_LESS;
    VkPipelineColorBlendAttachmentState ba{};ba.colorWriteMask=15;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&ba;
    const VkDynamicState dynamicStates[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=dynamicStates;
    VkGraphicsPipelineCreateInfo gp{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};gp.stageCount=2;gp.pStages=stages;gp.pVertexInputState=&vi;gp.pInputAssemblyState=&ia;
    gp.pViewportState=&vp;gp.pRasterizationState=&rs;gp.pMultisampleState=&ms;gp.pDepthStencilState=&ds;gp.pColorBlendState=&blend;gp.pDynamicState=&dynamic;gp.layout=layout_;gp.renderPass=renderPass_;
    const auto result=vkCreateGraphicsPipelines(device_,VK_NULL_HANDLE,1,&gp,nullptr,&raster_);
    vkDestroyShaderModule(device_,vs,nullptr);vkDestroyShaderModule(device_,fs,nullptr);
    if(result!=VK_SUCCESS)return Fail(error,"Opaque room forward pipeline failed.");
    reference_.artifact.words=policy_.requiresShaderBindingTable?std::span<const uint32_t>(room_reference_rgen):std::span<const uint32_t>(room_reference_comp);
    reference_.artifact.canonicalKey="opaque_room_rt_primary";
    VkShaderModule entry{},miss{},hit{};
    bool ok=!policy_.requiresShaderBindingTable||(api.createSharedShaderModules&&api.createStrategySbt&&api.createSharedShaderModules(api.user,miss,hit,error));
    if(ok)ok=api.createEntryShaderModule(api.user,reference_.artifact,entry,error)&&
        api.createStrategyPipeline(api.user,RtMaterialStrategy::OpaqueFast,entry,miss,hit,layout_,reference_.pipeline,error);
    if(ok&&policy_.requiresShaderBindingTable)ok=api.createStrategySbt(api.user,RtMaterialStrategy::OpaqueFast,reference_.pipeline,reference_.shaderBindingTable,reference_.sbtRegions,error);
    api.destroyShaderModule(api.user,entry);api.destroyShaderModule(api.user,miss);api.destroyShaderModule(api.user,hit);
    std::fprintf(stderr,"HORDE_OPAQUE_ROOM source_world_triangles=%zu retained_world_triangles=%zu excluded_nonopaque_triangles=%u pbr_draws=%zu pbr_triangles=%llu\n",
        codes.size(),mapCpu_.size(),excludedTriangles_,drawsCpu_.size()-1,static_cast<unsigned long long>([&]{uint64_t n=0;for(size_t i=1;i<counts_.size();++i)n+=counts_[i]/3;return n;}()));
    return ok;
}
bool HybridOpaqueRoomPass::PrepareFrame(std::span<const VkAccelerationStructureInstanceKHR> input,
    const RtGpuBuffer& v,const RtGpuBuffer& i,std::string& error,RtSceneRecordObservation* observation){
    rasterSelected_=HybridStaticPrimaryRequested();
    if(v.buffer!=vertices_.buffer||i.buffer!=indices_.buffer)return Fail(error,"Opaque scene source changed; recreate the static inspection scene.");
    if(built_)return true;
    std::array<VkAccelerationStructureInstanceKHR,2> selected{};bool world=false,asset=false;
    for(const auto& in:input)if(in.accelerationStructureReference!=0 && in.mask!=0 && (in.instanceCustomIndex==0||in.instanceCustomIndex==21)){
        const bool isWorld=in.instanceCustomIndex==0;
        if((isWorld&&world)||(!isWorld&&asset))return Fail(error,"Opaque profile requires exclusive world/asset owners.");
        if(isWorld)world=true;else asset=true;
        auto copy=in;copy.mask=0xffu;if(isWorld)copy.accelerationStructureReference=worldBlas_.address;
        selected[isWorld?0:1]=copy;
        for(auto& draw:drawsCpu_)if(draw.ids[0]==in.instanceCustomIndex)
            for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)draw.transform[c*4+r]=r==3?(c==3?1.f:0.f):in.transform.matrix[r][c];
    }
    if(!world)return Fail(error,"Opaque profile world owner unavailable.");
    if(drawsCpu_.size()>1 && !asset)return Fail(error,"Opaque profile asset owner is not active in this scene.");
    instanceCount_=asset&&drawsCpu_.size()>1?2:1;
    return resources_.WriteBuffer(instances_,selected.data(),instanceCount_*sizeof(selected[0]),"opaque room static TLAS",error,observation)&&
        resources_.WriteBuffer(drawBuffer_,drawsCpu_.data(),drawsCpu_.size()*sizeof(Draw),"opaque room static draws",error,observation);
}
void HybridOpaqueRoomPass::Record(VkCommandBuffer cmd,RtMaterialStrategy,std::span<const std::byte> controls,float eyeOffset,VkImage output){
    if(!built_){
        auto geometry=WorldGeometry(worldVertices_,worldIndices_);
        VkAccelerationStructureBuildGeometryInfoKHR b{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        b.type=VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;b.flags=VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        b.mode=VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;b.dstAccelerationStructure=worldBlas_.handle;b.geometryCount=1;b.pGeometries=&geometry;b.scratchData.deviceAddress=worldScratch_.AlignedAddress();
        VkAccelerationStructureBuildRangeInfoKHR range{};range.primitiveCount=static_cast<uint32_t>(mapCpu_.size());const auto* p=&range;build_(cmd,1,&b,&p);
        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};barrier.srcAccessMask=VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;barrier.dstAccessMask=VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,0,1,&barrier,0,nullptr,0,nullptr);
        geometry=InstanceGeometry(instances_);b.type=VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;b.dstAccelerationStructure=roomTlas_.handle;b.scratchData.deviceAddress=tlasScratch_.AlignedAddress();range.primitiveCount=instanceCount_;build_(cmd,1,&b,&p);
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,policy_.shaderPipelineStage|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,1,&barrier,0,nullptr,0,nullptr);
        recordedBuild_=true;std::fprintf(stderr,"HORDE_OPAQUE_ROOM static_AS_build_recorded=1 world_BLAS=1 room_TLAS=1 scene_instances=%u\n",instanceCount_);
    }
    // Profile-local push ABI uses the unused actor guidance field for the
    // current camera support/crouch height; selected lights remain world-fixed.
    std::array<float,32> roomControls{};std::memcpy(roomControls.data(),controls.data(),128);roomControls[31]=eyeOffset;
    const VkDescriptorSet sets[]={sceneSet_,extraSet_};
    vkCmdPushConstants(cmd,layout_,policy_.pushConstantStages|VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,128,roomControls.data());
    if(!rasterSelected_){
        vkCmdBindPipeline(cmd,policy_.bindPoint,reference_.pipeline);vkCmdBindDescriptorSets(cmd,policy_.bindPoint,layout_,0,2,sets,0,nullptr);
        if(policy_.requiresShaderBindingTable)trace_(cmd,&reference_.sbtRegions[0],&reference_.sbtRegions[1],&reference_.sbtRegions[2],&reference_.sbtRegions[3],targets_.extent.width,targets_.extent.height,1);
        else vkCmdDispatch(cmd,(targets_.extent.width+7)/8,(targets_.extent.height+7)/8,1);
        return;
    }
    VkClearValue clears[2]{};clears[0].color.float32[0]=.012f;clears[0].color.float32[1]=.018f;clears[0].color.float32[2]=.030f;clears[0].color.float32[3]=1;clears[1].depthStencil={1,0};
    VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};rp.renderPass=renderPass_;rp.framebuffer=targets_.framebuffer;rp.renderArea={{0,0},targets_.extent};rp.clearValueCount=2;rp.pClearValues=clears;
    vkCmdBeginRenderPass(cmd,&rp,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,raster_);
    vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,layout_,0,2,sets,0,nullptr);
    VkViewport vp{0,0,float(targets_.extent.width),float(targets_.extent.height),0,1};VkRect2D sc{{0,0},targets_.extent};
    vkCmdSetViewport(cmd,0,1,&vp);vkCmdSetScissor(cmd,0,1,&sc);
    for(uint32_t i=0;i<drawsCpu_.size();++i)vkCmdDraw(cmd,counts_[i],1,0,i);
    vkCmdEndRenderPass(cmd);
    ImageBarrier(cmd,targets_.color,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT);
    ImageBarrier(cmd,output,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        policy_.shaderPipelineStage|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_SHADER_WRITE_BIT,VK_ACCESS_TRANSFER_WRITE_BIT);
    VkImageCopy copy{};copy.srcSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.dstSubresource=copy.srcSubresource;copy.extent={targets_.extent.width,targets_.extent.height,1};
    vkCmdCopyImage(cmd,targets_.color,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,output,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
    ImageBarrier(cmd,output,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL,VK_PIPELINE_STAGE_TRANSFER_BIT,
        policy_.shaderPipelineStage|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_READ_BIT);
}
bool HybridOpaqueRoomPass::MakeTargets(VkExtent2D extent,Targets& out,std::string& error){
    if(!extent.width||!extent.height)return Fail(error,"Opaque room target extent empty.");out.extent=extent;
    auto image=[&](VkFormat format,VkImageUsageFlags usage,VkImageAspectFlags aspect,VkImage& image,VkDeviceMemory& memory,VkImageView& view,VkDeviceSize& bytes){
        VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.imageType=VK_IMAGE_TYPE_2D;ci.format=format;ci.extent={extent.width,extent.height,1};ci.mipLevels=ci.arrayLayers=1;ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=usage;
        if(vkCreateImage(device_,&ci,nullptr,&image)!=VK_SUCCESS)return false;VkMemoryRequirements r{};vkGetImageMemoryRequirements(device_,image,&r);bytes=r.size;
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};alloc.allocationSize=r.size;alloc.memoryTypeIndex=resources_.FindMemoryType(r.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if(vkAllocateMemory(device_,&alloc,nullptr,&memory)!=VK_SUCCESS||vkBindImageMemory(device_,image,memory,0)!=VK_SUCCESS)return false;
        VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=image;v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=format;v.subresourceRange={aspect,0,1,0,1};return vkCreateImageView(device_,&v,nullptr,&view)==VK_SUCCESS;
    };
    if(!image(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT,out.color,out.colorMemory,out.colorView,out.colorBytes)||
       !image(VK_FORMAT_D32_SFLOAT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,VK_IMAGE_ASPECT_DEPTH_BIT,out.depth,out.depthMemory,out.depthView,out.depthBytes))return Fail(error,"Opaque room color/depth allocation failed.");
    const VkImageView views[]={out.colorView,out.depthView};VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fb.renderPass=renderPass_;fb.attachmentCount=2;fb.pAttachments=views;fb.width=extent.width;fb.height=extent.height;fb.layers=1;
    return vkCreateFramebuffer(device_,&fb,nullptr,&out.framebuffer)==VK_SUCCESS||Fail(error,"Opaque room framebuffer failed.");
}
void HybridOpaqueRoomPass::DestroyTargets(Targets& t){
    if(t.framebuffer)vkDestroyFramebuffer(device_,t.framebuffer,nullptr);
    for(auto v:{t.colorView,t.depthView})if(v)vkDestroyImageView(device_,v,nullptr);
    for(auto i:{t.color,t.depth})if(i)vkDestroyImage(device_,i,nullptr);
    for(auto m:{t.colorMemory,t.depthMemory})if(m)vkFreeMemory(device_,m,nullptr);t={};
}
bool HybridOpaqueRoomPass::PrepareResizeAfterDeviceIdle(VkExtent2D e,std::string& error){CancelResizeAfterDeviceIdle();if(!MakeTargets(e,replacement_,error)){CancelResizeAfterDeviceIdle();return false;}return true;}
void HybridOpaqueRoomPass::CommitResizeAfterDeviceIdle(){DestroyTargets(targets_);targets_=replacement_;replacement_={};}
void HybridOpaqueRoomPass::CancelResizeAfterDeviceIdle(){DestroyTargets(replacement_);}
std::string_view HybridOpaqueRoomPass::Key(RtMaterialStrategy)const noexcept{return rasterSelected_?"opaque_architecture_forward_rt_shadows":policy_.requiresShaderBindingTable?"opaque_architecture_rt_primary_pipeline":"opaque_architecture_rt_primary_compute";}
std::string_view HybridOpaqueRoomPass::Sha(RtMaterialStrategy)const noexcept{return rasterSelected_?room_frag_sha:policy_.requiresShaderBindingTable?room_reference_rgen_sha:room_reference_comp_sha;}
void HybridOpaqueRoomPass::AccumulateResourceInventory(horde::telemetry::RtResourceInventory& i)const{
    for(auto* b:{&drawBuffer_,&worldIndices_,&worldMap_,&instances_,&worldScratch_,&tlasScratch_,&reference_.shaderBindingTable,&worldBlas_.backing,&roomTlas_.backing})AccumulateRtGpuBuffer(i,*b);
    if(worldBlas_.handle)AccumulateRtResourceCount(i.bottomLevelAccelerationStructureCount);
    if(roomTlas_.handle){AccumulateRtResourceCount(i.topLevelAccelerationStructureCount);i.tlasInstanceCount+=instanceCount_;}
    if(raster_)AccumulateRtResourceCount(i.pipelineCount);if(reference_.pipeline)AccumulateRtResourceCount(i.pipelineCount);
    if(reference_.shaderBindingTable.buffer)AccumulateRtResourceCount(i.shaderBindingTableCount);if(extraSet_)AccumulateRtResourceCount(i.descriptorSetCount);
    for(auto t:{targets_,replacement_}){AccumulateRtMemoryAllocation(i,t.colorMemory,t.colorBytes,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);AccumulateRtMemoryAllocation(i,t.depthMemory,t.depthBytes,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);}
}
}
