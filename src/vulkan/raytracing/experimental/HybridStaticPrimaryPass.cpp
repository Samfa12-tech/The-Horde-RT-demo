#include "vulkan/raytracing/experimental/HybridStaticPrimaryPass.h"
#include "HybridStaticPrimaryShaders.h"
#include <algorithm>
#include <cstring>
#include <limits>
namespace horde::vulkan::raytracing::experimental {
static_assert(kHybridInstrumentation==HORDE_RT_SELECTED_INSTRUMENTATION);
static_assert(kHybridQuality==HORDE_RT_SELECTED_DIELECTRIC_QUALITY);
static_assert(sizeof(StaticRtVertex)==64);
namespace {
VkShaderModule Module(VkDevice d,std::span<const uint32_t> words){VkShaderModuleCreateInfo c{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};c.codeSize=words.size_bytes();c.pCode=words.data();VkShaderModule m{};return vkCreateShaderModule(d,&c,nullptr,&m)==VK_SUCCESS?m:VK_NULL_HANDLE;}
VkPipelineShaderStageCreateInfo Stage(VkShaderStageFlagBits s,VkShaderModule m){return {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,s,m,"main",nullptr};}
}
std::unique_ptr<HybridStaticPrimaryPass> HybridStaticPrimaryPass::Create(VkInstance instance,VkPhysicalDevice physical,VkDevice device,
 const RtGpuResources& gpu,VkDescriptorSetLayout sceneLayout,VkDescriptorSet sceneSet,VkExtent2D extent,
 const RtExecutionPolicy& policy,const RtPipelineBundleBuildApi& api,const RtStaticMeshSlot& slot,
 const RtGpuBuffer& vertices,const RtGpuBuffer& indices,std::string& error){
 auto p=std::unique_ptr<HybridStaticPrimaryPass>(new HybridStaticPrimaryPass);
 p->physical_=physical;p->device_=device;p->resources_=gpu;p->sceneSet_=sceneSet;p->policy_=policy;p->vertices_=vertices;p->indices_=indices;
 if(!p->Initialise(instance,physical,sceneLayout,extent,api,slot,error))return nullptr;return p;
}
HybridStaticPrimaryPass::~HybridStaticPrimaryPass(){
 if(!device_)return;
 DestroyTargets(replacement_);DestroyTargets(targets_);
 if(raster_)vkDestroyPipeline(device_,raster_,nullptr);
 for(auto& p:shade_){if(p.pipeline)vkDestroyPipeline(device_,p.pipeline,nullptr);resources_.DestroyBuffer(p.shaderBindingTable);}
 if(layout_)vkDestroyPipelineLayout(device_,layout_,nullptr);
 if(renderPass_)vkDestroyRenderPass(device_,renderPass_,nullptr);
 if(pool_)vkDestroyDescriptorPool(device_,pool_,nullptr);
 if(extraLayout_)vkDestroyDescriptorSetLayout(device_,extraLayout_,nullptr);
 resources_.DestroyAccelerationStructure(remaining_);resources_.DestroyBuffer(draws_);resources_.DestroyBuffer(instances_);resources_.DestroyBuffer(scratch_);
}
bool HybridStaticPrimaryPass::Initialise(VkInstance instance,VkPhysicalDevice physical,VkDescriptorSetLayout sceneLayout,VkExtent2D extent,
 const RtPipelineBundleBuildApi& api,const RtStaticMeshSlot& slot,std::string& error){
 if(!device_||!physical||!sceneLayout||!sceneSet_||!api.createStrategyPipeline||!api.createEntryShaderModule||!api.destroyShaderModule||!vertices_.buffer||!indices_.buffer)
  return Fail(error,"Hybrid static-primary requires the real production scene resources.");
 auto owner=slot.InstanceMetadata()[21];
 if(owner.primitiveBase>slot.PrimitiveMetadata().size()||owner.primitiveCount>slot.PrimitiveMetadata().size()-owner.primitiveBase)return Fail(error,"Hybrid static instance21 primitive range invalid.");
 for(uint32_t i=0;i<owner.primitiveCount;i++){
  const auto& p=slot.PrimitiveMetadata()[owner.primitiveBase+i];
  if(p.materialIndex>=slot.Materials().size())return Fail(error,"Hybrid material index invalid.");
  const auto& m=slot.Materials()[p.materialIndex];
  if((m.materialFlags[0]&static_cast<uint32_t>(RtMaterialFlag::Transmission))||m.metallicRoughnessOcclusionTransmission[3]>0)
   {unsupportedReason_="Hybrid static instance21 contains transmission; this bounded opaque adapter refuses it.";break;}
  const std::array<float,12> identity{1,0,0,0,0,1,0,0,0,0,1,0};
  if(slot.GeometryTransforms()[owner.primitiveBase+i]!=identity){unsupportedReason_="Hybrid static source requires baked identity primitive transforms.";break;}
  Draw draw{};draw.ids={21,i,p.indexOffset,p.vertexOffset};drawRecords_.push_back(draw);counts_.push_back(p.indexCount);
 }
 if(!unsupportedReason_.empty()){drawRecords_.clear();counts_.clear();}
 // Empty preview/menu instance21 is valid: the remaining scene is full RT.
 maxInstances_=kRtTlasInstanceCapacity;
 VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(physical,&props);
 VkPhysicalDeviceAccelerationStructurePropertiesKHR asp{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};VkPhysicalDeviceProperties2 props2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};props2.pNext=&asp;auto getProperties=reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(vkGetInstanceProcAddr(instance,"vkGetPhysicalDeviceProperties2"));
 if(!getProperties)getProperties=reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(vkGetInstanceProcAddr(instance,"vkGetPhysicalDeviceProperties2KHR"));
 if(!getProperties)return Fail(error,"Hybrid physical-device property query unavailable.");getProperties(physical,&props2);
 if(asp.maxPerStageDescriptorAccelerationStructures<2||props.limits.maxBoundDescriptorSets<2||props.limits.maxPerStageDescriptorStorageImages<2)
  return Fail(error,"Hybrid static-primary descriptor limits require two AS and two storage images.");
 const VkShaderStageFlags shadeStage=policy_.shaderStage;
 const VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1,shadeStage,nullptr},
  {1,VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,1,shadeStage,nullptr},
  {2,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,shadeStage|VK_SHADER_STAGE_VERTEX_BIT,nullptr},
  {3,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr},
  {4,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr}};
 VkDescriptorSetLayoutCreateInfo sl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};sl.bindingCount=5;sl.pBindings=bindings;
 if(vkCreateDescriptorSetLayout(device_,&sl,nullptr,&extraLayout_)!=VK_SUCCESS)return Fail(error,"Hybrid extra descriptor layout creation failed.");
 const VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1},{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,1},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,3}};
 VkDescriptorPoolCreateInfo pc{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pc.maxSets=1;pc.poolSizeCount=3;pc.pPoolSizes=sizes;
 if(vkCreateDescriptorPool(device_,&pc,nullptr,&pool_)!=VK_SUCCESS)return Fail(error,"Hybrid descriptor pool failed.");
 VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=pool_;da.descriptorSetCount=1;da.pSetLayouts=&extraLayout_;
 if(vkAllocateDescriptorSets(device_,&da,&extraSet_)!=VK_SUCCESS)return Fail(error,"Hybrid descriptor allocation failed.");
 const VkDescriptorSetLayout layouts[]={sceneLayout,extraLayout_};
 const VkPushConstantRange range{policy_.pushConstantStages|VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,128};
 VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=2;pl.pSetLayouts=layouts;pl.pushConstantRangeCount=1;pl.pPushConstantRanges=&range;
 if(vkCreatePipelineLayout(device_,&pl,nullptr,&layout_)!=VK_SUCCESS)return Fail(error,"Hybrid pipeline layout failed.");
 const auto host=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
 if(!resources_.CreateBuffer(std::max<size_t>(1,drawRecords_.size())*sizeof(Draw),VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,host,false,draws_,error)||
    !resources_.CreateBuffer(maxInstances_*sizeof(VkAccelerationStructureInstanceKHR),VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,host,true,instances_,error))return false;
 auto sizesFn=reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device_,"vkGetAccelerationStructureBuildSizesKHR"));
 auto createFn=reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device_,"vkCreateAccelerationStructureKHR"));
 build_=reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device_,"vkCmdBuildAccelerationStructuresKHR"));
 trace_=reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device_,"vkCmdTraceRaysKHR"));
 if(!sizesFn||!createFn||!build_||(policy_.requiresShaderBindingTable&&!trace_))return Fail(error,"Hybrid AS/dispatch entrypoint missing.");
 VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};geometry.geometryType=VK_GEOMETRY_TYPE_INSTANCES_KHR;geometry.geometry.instances.sType=VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;geometry.geometry.instances.data.deviceAddress=instances_.address;
 VkAccelerationStructureBuildGeometryInfoKHR bi{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};bi.type=VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;bi.flags=VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;bi.geometryCount=1;bi.pGeometries=&geometry;
 VkAccelerationStructureBuildSizesInfoKHR bs{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};sizesFn(device_,VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,&bi,&maxInstances_,&bs);
 if(!resources_.CreateBuffer(bs.accelerationStructureSize,VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,false,remaining_.backing,error)||
    !resources_.CreateAlignedBuffer(bs.buildScratchSize,asp.minAccelerationStructureScratchOffsetAlignment,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,scratch_,error))return false;
 scratchAddress_=scratch_.AlignedAddress();
 VkAccelerationStructureCreateInfoKHR ac{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};ac.buffer=remaining_.backing.buffer;ac.size=bs.accelerationStructureSize;ac.type=VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
 if(createFn(device_,&ac,nullptr,&remaining_.handle)!=VK_SUCCESS)return Fail(error,"Hybrid remaining-primary TLAS creation failed.");
 VkAttachmentDescription attachments[2]{};attachments[0].format=VK_FORMAT_R32G32B32A32_UINT;attachments[0].samples=VK_SAMPLE_COUNT_1_BIT;attachments[0].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachments[0].storeOp=VK_ATTACHMENT_STORE_OP_STORE;attachments[0].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachments[0].finalLayout=VK_IMAGE_LAYOUT_GENERAL;
 attachments[1].format=VK_FORMAT_D32_SFLOAT;attachments[1].samples=VK_SAMPLE_COUNT_1_BIT;attachments[1].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachments[1].storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;attachments[1].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachments[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
 VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},depth{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&color;sub.pDepthStencilAttachment=&depth;
 VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.attachmentCount=2;rp.pAttachments=attachments;rp.subpassCount=1;rp.pSubpasses=&sub;
 if(vkCreateRenderPass(device_,&rp,nullptr,&renderPass_)!=VK_SUCCESS)return Fail(error,"Hybrid raster visibility renderpass failed.");
 if(!MakeTargets(extent,targets_,error))return false;WriteDescriptors();
 VkShaderModule vs=Module(device_,hybrid_vert),fs=Module(device_,hybrid_frag);
 if(!vs||!fs){if(vs)vkDestroyShaderModule(device_,vs,nullptr);if(fs)vkDestroyShaderModule(device_,fs,nullptr);return Fail(error,"Hybrid raster shader module failed.");}
 const VkPipelineShaderStageCreateInfo stages[]={Stage(VK_SHADER_STAGE_VERTEX_BIT,vs),Stage(VK_SHADER_STAGE_FRAGMENT_BIT,fs)};
 VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
 VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;
 VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.polygonMode=VK_POLYGON_MODE_FILL;rs.cullMode=VK_CULL_MODE_NONE;rs.lineWidth=1;
 VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
 VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};ds.depthTestEnable=ds.depthWriteEnable=VK_TRUE;ds.depthCompareOp=VK_COMPARE_OP_LESS;
 VkPipelineColorBlendAttachmentState ba{};ba.colorWriteMask=15;VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&ba;
 const VkDynamicState dynamics[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=dynamics;
 VkGraphicsPipelineCreateInfo gp{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};gp.stageCount=2;gp.pStages=stages;gp.pVertexInputState=&vi;gp.pInputAssemblyState=&ia;gp.pViewportState=&vp;gp.pRasterizationState=&rs;gp.pMultisampleState=&ms;gp.pDepthStencilState=&ds;gp.pColorBlendState=&blend;gp.pDynamicState=&dynamic;gp.layout=layout_;gp.renderPass=renderPass_;
 auto result=vkCreateGraphicsPipelines(device_,VK_NULL_HANDLE,1,&gp,nullptr,&raster_);vkDestroyShaderModule(device_,vs,nullptr);vkDestroyShaderModule(device_,fs,nullptr);
 if(result!=VK_SUCCESS)return Fail(error,"Hybrid vertex-pulled production static-asset pipeline failed.");
 VkShaderModule miss{},hit{};
 if(policy_.requiresShaderBindingTable&&(!api.createSharedShaderModules||!api.createStrategySbt||!api.createSharedShaderModules(api.user,miss,hit,error)))return false;
 for(size_t i=0;i<2;i++){
  auto& s=shade_[i];s.artifact.words=policy_.requiresShaderBindingTable?(i?std::span<const uint32_t>(hybrid1_rgen):std::span<const uint32_t>(hybrid0_rgen)):(i?std::span<const uint32_t>(hybrid1_comp):std::span<const uint32_t>(hybrid0_comp));s.artifact.canonicalKey=Key(i?RtMaterialStrategy::GenericDielectric:RtMaterialStrategy::OpaqueFast);
  VkShaderModule entry{};auto strategy=i?RtMaterialStrategy::GenericDielectric:RtMaterialStrategy::OpaqueFast;
  bool ok=api.createEntryShaderModule(api.user,s.artifact,entry,error)&&api.createStrategyPipeline(api.user,strategy,entry,miss,hit,layout_,s.pipeline,error);
  if(ok&&policy_.requiresShaderBindingTable)ok=api.createStrategySbt(api.user,strategy,s.pipeline,s.shaderBindingTable,s.sbtRegions,error);
  api.destroyShaderModule(api.user,entry);
  if(!ok){api.destroyShaderModule(api.user,miss);api.destroyShaderModule(api.user,hit);return false;}
 }
 api.destroyShaderModule(api.user,miss);api.destroyShaderModule(api.user,hit);error.clear();return true;
}
bool HybridStaticPrimaryPass::MakeTargets(VkExtent2D extent,Targets& out,std::string& error){
 if(!extent.width||!extent.height)return Fail(error,"Hybrid raster extent is empty.");out.extent=extent;
 auto image=[&](VkFormat format,VkImageUsageFlags usage,VkImageAspectFlags aspect,VkImage& image,VkDeviceMemory& memory,VkImageView& view,VkDeviceSize& bytes){
  VkFormatProperties formatProps{};vkGetPhysicalDeviceFormatProperties(physical_,format,&formatProps);
  auto required=aspect==VK_IMAGE_ASPECT_COLOR_BIT?(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT|VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT):VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  if((formatProps.optimalTilingFeatures&required)!=required)return false;
  VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.imageType=VK_IMAGE_TYPE_2D;ci.format=format;ci.extent={extent.width,extent.height,1};ci.mipLevels=ci.arrayLayers=1;ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=usage;
  if(vkCreateImage(device_,&ci,nullptr,&image)!=VK_SUCCESS)return false;VkMemoryRequirements r{};vkGetImageMemoryRequirements(device_,image,&r);bytes=r.size;
  VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=r.size;a.memoryTypeIndex=resources_.FindMemoryType(r.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if(vkAllocateMemory(device_,&a,nullptr,&memory)!=VK_SUCCESS||vkBindImageMemory(device_,image,memory,0)!=VK_SUCCESS)return false;
  VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=image;v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=format;v.subresourceRange={aspect,0,1,0,1};return vkCreateImageView(device_,&v,nullptr,&view)==VK_SUCCESS;
 };
 if(!image(VK_FORMAT_R32G32B32A32_UINT,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_STORAGE_BIT,VK_IMAGE_ASPECT_COLOR_BIT,out.visibility,out.visMemory,out.visView,out.visBytes)||!image(VK_FORMAT_D32_SFLOAT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,VK_IMAGE_ASPECT_DEPTH_BIT,out.depth,out.depthMemory,out.depthView,out.depthBytes))return Fail(error,"Hybrid visibility/depth allocation failed.");
 VkImageView views[]={out.visView,out.depthView};VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fb.renderPass=renderPass_;fb.attachmentCount=2;fb.pAttachments=views;fb.width=extent.width;fb.height=extent.height;fb.layers=1;
 return vkCreateFramebuffer(device_,&fb,nullptr,&out.framebuffer)==VK_SUCCESS||Fail(error,"Hybrid framebuffer failed.");
}
void HybridStaticPrimaryPass::DestroyTargets(Targets& t){if(t.framebuffer)vkDestroyFramebuffer(device_,t.framebuffer,nullptr);for(auto v:{t.visView,t.depthView})if(v)vkDestroyImageView(device_,v,nullptr);for(auto i:{t.visibility,t.depth})if(i)vkDestroyImage(device_,i,nullptr);for(auto m:{t.visMemory,t.depthMemory})if(m)vkFreeMemory(device_,m,nullptr);t={};}
bool HybridStaticPrimaryPass::PrepareResizeAfterDeviceIdle(VkExtent2D e,std::string& error){CancelResizeAfterDeviceIdle();if(!MakeTargets(e,replacement_,error)){CancelResizeAfterDeviceIdle();return false;}return true;}
void HybridStaticPrimaryPass::CommitResizeAfterDeviceIdle(){DestroyTargets(targets_);targets_=replacement_;replacement_={};WriteDescriptors();}
void HybridStaticPrimaryPass::CancelResizeAfterDeviceIdle(){DestroyTargets(replacement_);}
void HybridStaticPrimaryPass::WriteDescriptors(){
 VkDescriptorImageInfo image{VK_NULL_HANDLE,targets_.visView,VK_IMAGE_LAYOUT_GENERAL};VkWriteDescriptorSetAccelerationStructureKHR as{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};as.accelerationStructureCount=1;as.pAccelerationStructures=&remaining_.handle;
 VkDescriptorBufferInfo infos[]={{draws_.buffer,0,draws_.size},{vertices_.buffer,0,vertices_.size},{indices_.buffer,0,indices_.size}};
 VkWriteDescriptorSet w[5]{};for(unsigned i=0;i<5;i++){w[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;w[i].dstSet=extraSet_;w[i].dstBinding=i;w[i].descriptorCount=1;w[i].descriptorType=i==0?VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:i==1?VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;if(i>=2)w[i].pBufferInfo=&infos[i-2];}w[0].pImageInfo=&image;w[1].pNext=&as;vkUpdateDescriptorSets(device_,5,w,0,nullptr);
}
bool HybridStaticPrimaryPass::PrepareFrame(std::span<const VkAccelerationStructureInstanceKHR> input,const RtGpuBuffer& vertices,const RtGpuBuffer& indices,std::string& error,RtSceneRecordObservation* observation){
 if(!unsupportedReason_.empty())return Fail(error,unsupportedReason_.c_str());
 if(vertices.buffer!=vertices_.buffer||indices.buffer!=indices_.buffer)return Fail(error,"Hybrid static resources changed; this bounded adapter requires scene recreation.");
 if(input.empty()||input.size()>maxInstances_)return Fail(error,"Hybrid remaining TLAS instance range invalid.");
 if(drawRecords_.empty() && std::any_of(input.begin(),input.end(),[](const auto& i){return i.instanceCustomIndex==21 && i.mask!=0;}))return Fail(error,"Hybrid static instance21 has no admitted production primitive rows.");
 if(drawRecords_.empty() && std::any_of(input.begin(),input.end(),[](const auto& i){return i.instanceCustomIndex==21 && i.mask!=0;}))return Fail(error,"Hybrid static instance21 has no admitted production primitive rows.");
 frameInstances_.assign(input.begin(),input.end());instanceCount_=static_cast<uint32_t>(input.size());drawCount_=0;
 unsigned selectedOwners=0;
 for(auto& instance:frameInstances_)if(instance.instanceCustomIndex==21){
  if(++selectedOwners>1)return Fail(error,"Hybrid adapter requires one immutable asset owner for instance21.");
  if(instance.mask!=0){drawCount_=static_cast<uint32_t>(drawRecords_.size());for(auto& d:drawRecords_)for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)d.transform[c*4+r]=r==3?(c==3?1.f:0.f):instance.transform.matrix[r][c];}
  instance.mask=0;
 }
 if(drawCount_&&!resources_.WriteBuffer(draws_,drawRecords_.data(),drawRecords_.size()*sizeof(Draw),"hybrid production primitive draws",error,observation))return false;
 return resources_.WriteBuffer(instances_,frameInstances_.data(),frameInstances_.size()*sizeof(frameInstances_[0]),"hybrid remaining-primary instances",error,observation);
}
void HybridStaticPrimaryPass::Record(VkCommandBuffer cmd,RtMaterialStrategy strategy,std::span<const std::byte> controls,float eyeOffset) const{
 VkAccelerationStructureGeometryKHR g{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};g.geometryType=VK_GEOMETRY_TYPE_INSTANCES_KHR;g.geometry.instances.sType=VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;g.geometry.instances.data.deviceAddress=instances_.address;
 VkAccelerationStructureBuildGeometryInfoKHR b{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};b.type=VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;b.flags=VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;b.mode=VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;b.dstAccelerationStructure=remaining_.handle;b.geometryCount=1;b.pGeometries=&g;b.scratchData.deviceAddress=scratchAddress_;
 VkAccelerationStructureBuildRangeInfoKHR range{};range.primitiveCount=instanceCount_;const auto* p=&range;build_(cmd,1,&b,&p);
 VkMemoryBarrier before{VK_STRUCTURE_TYPE_MEMORY_BARRIER};before.srcAccessMask=VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;before.dstAccessMask=VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
 vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,policy_.shaderPipelineStage,0,1,&before,0,nullptr,0,nullptr);
 std::array<float,32> rasterControls{};std::memcpy(rasterControls.data(),controls.data(),128);rasterControls[29]=float(targets_.extent.width);rasterControls[30]=float(targets_.extent.height);rasterControls[31]=eyeOffset;
 const auto pushStages=policy_.pushConstantStages|VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;
 vkCmdPushConstants(cmd,layout_,pushStages,0,128,rasterControls.data());
 VkClearValue clears[2]{};for(auto& u:clears[0].color.uint32)u=0xffffffffu;clears[1].depthStencil={1,0};
 VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};rp.renderPass=renderPass_;rp.framebuffer=targets_.framebuffer;rp.renderArea={{0,0},targets_.extent};rp.clearValueCount=2;rp.pClearValues=clears;vkCmdBeginRenderPass(cmd,&rp,VK_SUBPASS_CONTENTS_INLINE);
 vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,raster_);const VkDescriptorSet sets[]={sceneSet_,extraSet_};vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,layout_,0,2,sets,0,nullptr);
 VkViewport vp{0,0,float(targets_.extent.width),float(targets_.extent.height),0,1};VkRect2D sc{{0,0},targets_.extent};vkCmdSetViewport(cmd,0,1,&vp);vkCmdSetScissor(cmd,0,1,&sc);
 for(uint32_t i=0;i<drawCount_;i++)vkCmdDraw(cmd,counts_[i],1,0,i);vkCmdEndRenderPass(cmd);
 VkMemoryBarrier raster{VK_STRUCTURE_TYPE_MEMORY_BARRIER};raster.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;raster.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
 vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,policy_.shaderPipelineStage,0,1,&raster,0,nullptr,0,nullptr);
 vkCmdPushConstants(cmd,layout_,pushStages,0,128,controls.data());const auto& s=shade_[strategy==RtMaterialStrategy::GenericDielectric?1:0];vkCmdBindPipeline(cmd,policy_.bindPoint,s.pipeline);vkCmdBindDescriptorSets(cmd,policy_.bindPoint,layout_,0,2,sets,0,nullptr);
 if(policy_.requiresShaderBindingTable)trace_(cmd,&s.sbtRegions[0],&s.sbtRegions[1],&s.sbtRegions[2],&s.sbtRegions[3],targets_.extent.width,targets_.extent.height,1);
 else vkCmdDispatch(cmd,(targets_.extent.width+7)/8,(targets_.extent.height+7)/8,1);
}
std::string_view HybridStaticPrimaryPass::Key(RtMaterialStrategy s)const noexcept{return policy_.requiresShaderBindingTable?(s==RtMaterialStrategy::GenericDielectric?"hybrid_static21_pipeline_generic":"hybrid_static21_pipeline_opaque"):(s==RtMaterialStrategy::GenericDielectric?"hybrid_static21_compute_generic":"hybrid_static21_compute_opaque");}
std::string_view HybridStaticPrimaryPass::Sha(RtMaterialStrategy s)const noexcept{return policy_.requiresShaderBindingTable?(s==RtMaterialStrategy::GenericDielectric?hybrid1_rgen_sha:hybrid0_rgen_sha):(s==RtMaterialStrategy::GenericDielectric?hybrid1_comp_sha:hybrid0_comp_sha);}
void HybridStaticPrimaryPass::AccumulateResourceInventory(horde::telemetry::RtResourceInventory& i)const{for(auto* b:{&draws_,&instances_,&scratch_,&remaining_.backing})AccumulateRtGpuBuffer(i,*b);for(auto& p:shade_){AccumulateRtGpuBuffer(i,p.shaderBindingTable);if(p.pipeline)AccumulateRtResourceCount(i.pipelineCount);if(p.shaderBindingTable.buffer)AccumulateRtResourceCount(i.shaderBindingTableCount);}if(remaining_.handle){AccumulateRtResourceCount(i.topLevelAccelerationStructureCount);i.tlasInstanceCount+=instanceCount_;}if(raster_)AccumulateRtResourceCount(i.pipelineCount);if(extraSet_)AccumulateRtResourceCount(i.descriptorSetCount);for(auto t:{targets_,replacement_}){AccumulateRtMemoryAllocation(i,t.visMemory,t.visBytes,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);AccumulateRtMemoryAllocation(i,t.depthMemory,t.depthBytes,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);}}
}
