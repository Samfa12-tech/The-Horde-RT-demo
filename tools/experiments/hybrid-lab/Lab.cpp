#include "Lab.h"
#include "Shaders.h"
#include "BuildIdentity.h"
#include "scene/TombStoneBevels.h"
#include "gameplay/ShowcaseRoute.h"
#include <vulkan/vulkan.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
void check(VkResult r,const char* what) { if(r!=VK_SUCCESS)throw std::runtime_error(std::string(what)+": VkResult "+std::to_string(r)); }
using P=std::array<float,3>;
P sub(P a,P b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
float dot(P a,P b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
P cross(P a,P b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
P unit(P p){float n=std::sqrt(dot(p,p));for(auto& v:p)v/=n;return p;}
struct V { std::array<float,4> p,n,m; };
static_assert(sizeof(V)==48);
struct Push { std::array<float,4> camera,forward,right,up,config; };
struct Buffer { VkBuffer handle{};VkDeviceMemory memory{};VkDeviceAddress address{};VkDeviceSize allocation{}; };
struct Image { VkImage handle{};VkDeviceMemory memory{};VkImageView view{};VkDeviceSize allocation{}; };
struct AS { VkAccelerationStructureKHR handle{};Buffer buffer;VkDeviceAddress address{}; };
struct Sample { int scene,block,frame;bool hybrid;double gpu,cpu,record,submitWait;float phase; };
std::string escape(std::string s){std::string r;for(char c:s){if(c=='"'||c=='\\')r+='\\';if(c=='\n')r+="\\n";else r+=c;}return r;}
std::vector<V> geometry() {
    std::vector<V> out;
    const auto quad=[&](P a,P b,P c,P d,std::array<float,4> m) {
        auto n=unit(cross(sub(b,a),sub(c,a)));
        for(auto p:{a,b,c,a,c,d})out.push_back({{p[0],p[1],p[2],1},{n[0],n[1],n[2],0},m});
    };
    const std::array<float,4> stone{.31f,.29f,.24f,0};
    const float floor=horde::gameplay::kRouteFloorWorldY;
    quad({-1.85f,floor,3.4f},{1.85f,floor,3.4f},{1.85f,floor,-10},{-1.85f,floor,-10},stone);
    // Opening side walls share the current native tomb's authored coordinates.
    quad({-1.85f,floor,-6.4f},{-1.85f,floor,3.4f},{-1.85f,1.35f,3.4f},{-1.85f,1.35f,-6.4f},stone);
    quad({1.85f,floor,3.4f},{1.85f,floor,-6.4f},{1.85f,1.35f,-6.4f},{1.85f,1.35f,3.4f},stone);
    quad({-1.85f,floor,3.4f},{-1.85f,1.35f,3.4f},{1.85f,1.35f,3.4f},{1.85f,floor,3.4f},stone);
    quad({-1.2f,floor,-10},{1.2f,floor,-10},{1.2f,1.35f,-10},{-1.2f,1.35f,-10},stone);
    for(unsigned index:{0u,1u,2u,3u,4u,5u}) {
        const auto& v=horde::scene::kShowcaseCeilingPatches[index];
        P a{v.footprint[0][0],v.bottomY,v.footprint[0][1]},b{v.footprint[1][0],v.bottomY,v.footprint[1][1]},
          c{v.footprint[2][0],v.bottomY,v.footprint[2][1]},d{v.footprint[3][0],v.bottomY,v.footprint[3][1]};
        quad(a,b,c,d,stone);
    }
    std::vector<horde::scene::assets::StaticRtVertex> v;std::vector<uint32_t> ix;
    horde::scene::AppendTombStoneBevels(v,ix);
    for(auto i:ix)out.push_back({v[i].position,v[i].normal,stone});
    // Explicit lab fixtures: a planar metal mirror and two emissive light markers.
    quad({-1.835f,-.30f,-1.65f},{-1.835f,-.30f,-.35f},{-1.835f,.82f,-.35f},{-1.835f,.82f,-1.65f},{.4f,.38f,.32f,1});
    quad({-1.49f,.30f,.62f},{-1.49f,.61f,.62f},{-1.49f,.61f,.68f},{-1.49f,.30f,.68f},{1,.43f,.08f,2});
    quad({1.49f,.20f,-2.03f},{1.49f,.53f,-2.03f},{1.49f,.53f,-1.97f},{1.49f,.20f,-1.97f},{.2f,.48f,1,2});
    return out;
}
class Lab {
public:
    VkInstance instance{};VkPhysicalDevice physical{};VkDevice device{};VkQueue queue{};
    VkPhysicalDeviceProperties properties{};VkPhysicalDeviceMemoryProperties memoryProperties{};
    uint32_t family{},timestampBits{};VkDeviceSize scratchAlignment=256;
    VkCommandPool pool{};VkCommandBuffer command{};VkFence fence{};VkQueryPool queries{};
    VkDescriptorSetLayout setLayout{};VkDescriptorPool descriptorPool{};VkDescriptorSet set{};
    VkPipelineLayout layout{};VkRenderPass renderPass{};VkFramebuffer framebuffer{};
    VkPipeline raster{},compute{},pipeline{};VkDebugUtilsMessengerEXT messenger{};
    Buffer vertices,instances,scratch,readback,sbt;AS blas,tlas;Image color,depth;
    VkStridedDeviceAddressRegionKHR raygen{},miss{},hitgroup{},callable{};
    std::vector<V> mesh;std::vector<VkShaderModule> modules;uint32_t width{},height{};
    bool usePipeline=false,validationEnabled=false;unsigned errors=0,warnings=0;std::string baseline;
    double blasBuildGpuMs=0,tlasBuildGpuMs=0,initialiseCpuMs=0;
    PFN_vkGetBufferDeviceAddress getBufferAddress{};
    PFN_vkCreateAccelerationStructureKHR createAS{};
    PFN_vkDestroyAccelerationStructureKHR destroyAS{};
    PFN_vkGetAccelerationStructureBuildSizesKHR getSizes{};
    PFN_vkCmdBuildAccelerationStructuresKHR buildAS{};
    PFN_vkGetAccelerationStructureDeviceAddressKHR getASAddress{};
    PFN_vkCreateRayTracingPipelinesKHR createPipelines{};
    PFN_vkGetRayTracingShaderGroupHandlesKHR getGroups{};
    PFN_vkCmdTraceRaysKHR trace{};
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT* data,void* user) {
        auto* self=static_cast<Lab*>(user);
        if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)self->errors++;
        else self->warnings++;
        std::cerr<<"Vulkan: "<<data->pMessage<<std::endl;return VK_FALSE;
    }
    ~Lab() {
        if(device) {
            vkDeviceWaitIdle(device);
            if(framebuffer)vkDestroyFramebuffer(device,framebuffer,nullptr);
            for(auto p:{raster,compute,pipeline})if(p)vkDestroyPipeline(device,p,nullptr);
            if(renderPass)vkDestroyRenderPass(device,renderPass,nullptr);
            for(auto m:modules)vkDestroyShaderModule(device,m,nullptr);
            if(layout)vkDestroyPipelineLayout(device,layout,nullptr);
            if(descriptorPool)vkDestroyDescriptorPool(device,descriptorPool,nullptr);
            if(setLayout)vkDestroyDescriptorSetLayout(device,setLayout,nullptr);
            if(tlas.handle)destroyAS(device,tlas.handle,nullptr);
            if(blas.handle)destroyAS(device,blas.handle,nullptr);
            for(auto b:{vertices,instances,scratch,readback,sbt,tlas.buffer,blas.buffer}) {
                if(b.handle)vkDestroyBuffer(device,b.handle,nullptr);if(b.memory)vkFreeMemory(device,b.memory,nullptr);
            }
            for(auto i:{color,depth}) {
                if(i.view)vkDestroyImageView(device,i.view,nullptr);
                if(i.handle)vkDestroyImage(device,i.handle,nullptr);if(i.memory)vkFreeMemory(device,i.memory,nullptr);
            }
            if(queries)vkDestroyQueryPool(device,queries,nullptr);
            if(fence)vkDestroyFence(device,fence,nullptr);
            if(pool)vkDestroyCommandPool(device,pool,nullptr);
            vkDestroyDevice(device,nullptr);
        }
        if(messenger)reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"))(instance,messenger,nullptr);
        if(instance)vkDestroyInstance(instance,nullptr);
    }
    uint32_t memoryType(uint32_t bits,VkMemoryPropertyFlags flags) {
        for(uint32_t i=0;i<memoryProperties.memoryTypeCount;i++)
            if((bits&(1u<<i))&&(memoryProperties.memoryTypes[i].propertyFlags&flags)==flags)return i;
        throw std::runtime_error("Required Vulkan memory type is unavailable");
    }
    Buffer buffer(VkDeviceSize size,VkBufferUsageFlags usage,bool host=false,const void* data=nullptr) {
        Buffer b;
        VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};ci.size=size;ci.usage=usage;
        check(vkCreateBuffer(device,&ci,nullptr,&b.handle),"create buffer");
        VkMemoryRequirements req{};vkGetBufferMemoryRequirements(device,b.handle,&req);b.allocation=req.size;
        VkMemoryAllocateFlagsInfo flags{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};flags.flags=VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=req.size;
        ai.memoryTypeIndex=memoryType(req.memoryTypeBits,host?VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if(usage&VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)ai.pNext=&flags;
        check(vkAllocateMemory(device,&ai,nullptr,&b.memory),"allocate buffer");
        check(vkBindBufferMemory(device,b.handle,b.memory,0),"bind buffer");
        if(usage&VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT){VkBufferDeviceAddressInfo a{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};a.buffer=b.handle;b.address=getBufferAddress(device,&a);}
        if(data){void* mapped{};check(vkMapMemory(device,b.memory,0,size,0,&mapped),"map buffer");std::memcpy(mapped,data,size);vkUnmapMemory(device,b.memory);}
        return b;
    }
    Image image(VkFormat format,VkImageUsageFlags usage,VkImageAspectFlags aspect) {
        Image i;VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.imageType=VK_IMAGE_TYPE_2D;
        ci.format=format;ci.extent={width,height,1};ci.mipLevels=ci.arrayLayers=1;ci.samples=VK_SAMPLE_COUNT_1_BIT;
        ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=usage;
        check(vkCreateImage(device,&ci,nullptr,&i.handle),"create image");
        VkMemoryRequirements r{};vkGetImageMemoryRequirements(device,i.handle,&r);i.allocation=r.size;
        VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=r.size;a.memoryTypeIndex=memoryType(r.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(device,&a,nullptr,&i.memory),"allocate image");check(vkBindImageMemory(device,i.handle,i.memory,0),"bind image");
        VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=i.handle;v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=format;v.subresourceRange={aspect,0,1,0,1};
        check(vkCreateImageView(device,&v,nullptr,&i.view),"image view");return i;
    }
    void begin(){check(vkResetCommandBuffer(command,0),"reset commands");VkCommandBufferBeginInfo b{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};b.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;check(vkBeginCommandBuffer(command,&b),"begin commands");}
    void submit(){check(vkEndCommandBuffer(command),"end commands");check(vkResetFences(device,1,&fence),"reset fence");VkSubmitInfo s{VK_STRUCTURE_TYPE_SUBMIT_INFO};s.commandBufferCount=1;s.pCommandBuffers=&command;check(vkQueueSubmit(queue,1,&s,fence),"submit");check(vkWaitForFences(device,1,&fence,VK_TRUE,30000000000ull),"wait GPU (30s deadline)");}
    void barrier(VkPipelineStageFlags src,VkPipelineStageFlags dst,VkAccessFlags write,VkAccessFlags read) {
        VkMemoryBarrier m{VK_STRUCTURE_TYPE_MEMORY_BARRIER};m.srcAccessMask=write;m.dstAccessMask=read;
        vkCmdPipelineBarrier(command,src,dst,0,1,&m,0,nullptr,0,nullptr);
    }
    template<size_t N>VkShaderModule module(const uint32_t(&code)[N]) {
        VkShaderModuleCreateInfo c{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};c.codeSize=sizeof(code);c.pCode=code;
        VkShaderModule m{};check(vkCreateShaderModule(device,&c,nullptr,&m),"shader module");modules.push_back(m);return m;
    }
    VkPipelineShaderStageCreateInfo stage(VkShaderStageFlagBits bit,VkShaderModule m) {
        VkPipelineShaderStageCreateInfo s{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};s.stage=bit;s.module=m;s.pName="main";return s;
    }
    void initialise(int w,int h,bool validation,bool forceQuery) {
        auto initialiseStart=std::chrono::steady_clock::now();
        width=w;height=h;mesh=geometry();
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Horde Hybrid Lab (experimental offscreen)";app.apiVersion=VK_API_VERSION_1_2;
        VkInstanceCreateInfo ic{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ic.pApplicationInfo=&app;
        const char* layer="VK_LAYER_KHRONOS_validation";const char* debugExt=VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        if(validation) {
            uint32_t n=0;check(vkEnumerateInstanceLayerProperties(&n,nullptr),"instance layers");std::vector<VkLayerProperties> layers(n);check(vkEnumerateInstanceLayerProperties(&n,layers.data()),"instance layers");
            validationEnabled=std::any_of(layers.begin(),layers.end(),[&](auto& l){return std::strcmp(l.layerName,layer)==0;});
            if(!validationEnabled)throw std::runtime_error("Validation was requested but VK_LAYER_KHRONOS_validation is unavailable");
            ic.enabledLayerCount=1;ic.ppEnabledLayerNames=&layer;ic.enabledExtensionCount=1;ic.ppEnabledExtensionNames=&debugExt;
        }
        check(vkCreateInstance(&ic,nullptr,&instance),"create Vulkan 1.2 instance");
        if(validationEnabled){VkDebugUtilsMessengerCreateInfoEXT c{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};c.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;c.messageType=7;c.pfnUserCallback=debug;c.pUserData=this;check(reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"))(instance,&c,nullptr,&messenger),"validation messenger");}
        uint32_t count=0;check(vkEnumeratePhysicalDevices(instance,&count,nullptr),"enumerate GPUs");std::vector<VkPhysicalDevice> devices(count);check(vkEnumeratePhysicalDevices(instance,&count,devices.data()),"enumerate GPUs");
        std::vector<const char*> extensions;
        for(auto candidate:devices) {
            uint32_t n=0;check(vkEnumerateDeviceExtensionProperties(candidate,nullptr,&n,nullptr),"device extensions");std::vector<VkExtensionProperties> ext(n);check(vkEnumerateDeviceExtensionProperties(candidate,nullptr,&n,ext.data()),"device extensions");
            auto has=[&](const char* name){return std::any_of(ext.begin(),ext.end(),[&](auto& e){return std::strcmp(e.extensionName,name)==0;});};
            if(!has(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)||!has(VK_KHR_RAY_QUERY_EXTENSION_NAME)||!has(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME))continue;
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
            VkPhysicalDeviceRayQueryFeaturesKHR rq{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};rq.pNext=&rt;
            VkPhysicalDeviceAccelerationStructureFeaturesKHR as{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};as.pNext=&rq;
            VkPhysicalDeviceVulkan12Features f12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};f12.pNext=&as;
            VkPhysicalDeviceFeatures2 f{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};f.pNext=&f12;vkGetPhysicalDeviceFeatures2(candidate,&f);
            if(!rq.rayQuery||!as.accelerationStructure||!f12.bufferDeviceAddress)continue;
            uint32_t queuesCount=0;vkGetPhysicalDeviceQueueFamilyProperties(candidate,&queuesCount,nullptr);std::vector<VkQueueFamilyProperties> qs(queuesCount);vkGetPhysicalDeviceQueueFamilyProperties(candidate,&queuesCount,qs.data());
            for(uint32_t i=0;i<queuesCount;i++)if((qs[i].queueFlags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))==(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT)&&qs[i].timestampValidBits) {
                physical=candidate;family=i;timestampBits=qs[i].timestampValidBits;
                usePipeline=!forceQuery&&has(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)&&rt.rayTracingPipeline;break;
            }
            if(physical)break;
        }
        if(!physical)throw std::runtime_error("No Vulkan GPU with acceleration structures, rayQuery, bufferDeviceAddress and graphics+compute timestamps. No software/raster fallback.");
        vkGetPhysicalDeviceProperties(physical,&properties);vkGetPhysicalDeviceMemoryProperties(physical,&memoryProperties);
        if(properties.apiVersion<VK_API_VERSION_1_2)throw std::runtime_error("Vulkan 1.2 is required");
        baseline=usePipeline?"Full hardware RT (vkCmdTraceRaysKHR primary)":forceQuery?"Full hardware RT (ray-query compute primary; explicitly selected)":"Full hardware RT (ray-query compute primary; RT pipeline unavailable)";
        extensions={VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,VK_KHR_RAY_QUERY_EXTENSION_NAME,VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};if(usePipeline)extensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        VkPhysicalDeviceAccelerationStructurePropertiesKHR asp{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};VkPhysicalDeviceProperties2 props{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};props.pNext=&asp;vkGetPhysicalDeviceProperties2(physical,&props);scratchAlignment=asp.minAccelerationStructureScratchOffsetAlignment;
        float priority=1;VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};rt.rayTracingPipeline=usePipeline;
        VkPhysicalDeviceRayQueryFeaturesKHR rq{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};rq.rayQuery=VK_TRUE;if(usePipeline)rq.pNext=&rt;
        VkPhysicalDeviceAccelerationStructureFeaturesKHR as{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};as.accelerationStructure=VK_TRUE;as.pNext=&rq;
        VkPhysicalDeviceVulkan12Features f12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};f12.bufferDeviceAddress=VK_TRUE;f12.pNext=&as;
        VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dc.pNext=&f12;dc.queueCreateInfoCount=1;dc.pQueueCreateInfos=&q;dc.enabledExtensionCount=static_cast<uint32_t>(extensions.size());dc.ppEnabledExtensionNames=extensions.data();
        check(vkCreateDevice(physical,&dc,nullptr,&device),"create hardware RT device");vkGetDeviceQueue(device,family,0,&queue);
#define LOAD(member,fn) member=reinterpret_cast<PFN_##fn>(vkGetDeviceProcAddr(device,#fn));if(!member)throw std::runtime_error("Missing " #fn)
        LOAD(createAS,vkCreateAccelerationStructureKHR);LOAD(destroyAS,vkDestroyAccelerationStructureKHR);LOAD(getSizes,vkGetAccelerationStructureBuildSizesKHR);LOAD(buildAS,vkCmdBuildAccelerationStructuresKHR);LOAD(getASAddress,vkGetAccelerationStructureDeviceAddressKHR);
        LOAD(getBufferAddress,vkGetBufferDeviceAddress);
        if(usePipeline){LOAD(createPipelines,vkCreateRayTracingPipelinesKHR);LOAD(getGroups,vkGetRayTracingShaderGroupHandlesKHR);LOAD(trace,vkCmdTraceRaysKHR);}
#undef LOAD
        VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};cp.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;cp.queueFamilyIndex=family;check(vkCreateCommandPool(device,&cp,nullptr,&pool),"command pool");
        VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=pool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;check(vkAllocateCommandBuffers(device,&ca,&command),"command buffer");
        VkFenceCreateInfo fc{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};check(vkCreateFence(device,&fc,nullptr,&fence),"fence");
        VkQueryPoolCreateInfo qp{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};qp.queryType=VK_QUERY_TYPE_TIMESTAMP;qp.queryCount=2;check(vkCreateQueryPool(device,&qp,nullptr,&queries),"timestamps");
        buildScene();createRenderTargets();createDescriptors();createShaderPipelines();
        initialiseCpuMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-initialiseStart).count();
    }
    void buildOne(AS& result,VkAccelerationStructureTypeKHR type,VkAccelerationStructureGeometryKHR& geom,uint32_t primitives) {
        VkAccelerationStructureBuildGeometryInfoKHR info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};info.type=type;info.flags=VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;info.mode=VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;info.geometryCount=1;info.pGeometries=&geom;
        VkAccelerationStructureBuildSizesInfoKHR sizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};getSizes(device,VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,&info,&primitives,&sizes);
        result.buffer=buffer(sizes.accelerationStructureSize,VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        VkAccelerationStructureCreateInfoKHR ci{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};ci.buffer=result.buffer.handle;ci.size=sizes.accelerationStructureSize;ci.type=type;check(createAS(device,&ci,nullptr,&result.handle),"acceleration structure");
        // Scratch is temporary; retain only the final allocation in the lab owner.
        if(scratch.handle){vkDestroyBuffer(device,scratch.handle,nullptr);vkFreeMemory(device,scratch.memory,nullptr);scratch={};}
        scratch=buffer(sizes.buildScratchSize+scratchAlignment,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        info.dstAccelerationStructure=result.handle;info.scratchData.deviceAddress=(scratch.address+scratchAlignment-1)&~(scratchAlignment-1);
        VkAccelerationStructureBuildRangeInfoKHR range{};range.primitiveCount=primitives;const auto* ptr=&range;
        begin();vkCmdResetQueryPool(command,queries,0,2);vkCmdWriteTimestamp(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,queries,0);
        buildAS(command,1,&info,&ptr);barrier(VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);
        vkCmdWriteTimestamp(command,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,queries,1);submit();
        uint64_t ts[2]{};check(vkGetQueryPoolResults(device,queries,0,2,sizeof(ts),ts,sizeof(uint64_t),VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WAIT_BIT),"AS build timestamps");
        const uint64_t mask=timestampBits==64?~uint64_t(0):(uint64_t(1)<<timestampBits)-1;
        const double ms=double((ts[1]-ts[0])&mask)*properties.limits.timestampPeriod/1e6;
        (type==VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR?blasBuildGpuMs:tlasBuildGpuMs)=ms;
        VkAccelerationStructureDeviceAddressInfoKHR a{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};a.accelerationStructure=result.handle;result.address=getASAddress(device,&a);
    }
    void buildScene() {
        vertices=buffer(mesh.size()*sizeof(V),VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,true,mesh.data());
        VkAccelerationStructureGeometryKHR g{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};g.geometryType=VK_GEOMETRY_TYPE_TRIANGLES_KHR;g.flags=VK_GEOMETRY_OPAQUE_BIT_KHR;
        auto& t=g.geometry.triangles;t.sType=VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;t.vertexFormat=VK_FORMAT_R32G32B32_SFLOAT;t.vertexData.deviceAddress=vertices.address;t.vertexStride=sizeof(V);t.maxVertex=static_cast<uint32_t>(mesh.size()-1);t.indexType=VK_INDEX_TYPE_NONE_KHR;
        buildOne(blas,VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,g,static_cast<uint32_t>(mesh.size()/3));
        VkAccelerationStructureInstanceKHR inst{};inst.transform.matrix[0][0]=inst.transform.matrix[1][1]=inst.transform.matrix[2][2]=1;inst.mask=255;inst.flags=VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;inst.accelerationStructureReference=blas.address;
        instances=buffer(sizeof(inst),VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,true,&inst);
        VkAccelerationStructureGeometryKHR tg{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};tg.geometryType=VK_GEOMETRY_TYPE_INSTANCES_KHR;tg.geometry.instances.sType=VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;tg.geometry.instances.data.deviceAddress=instances.address;
        buildOne(tlas,VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,tg,1);
    }
    void createRenderTargets() {
        color=image(VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT);
        depth=image(VK_FORMAT_D32_SFLOAT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,VK_IMAGE_ASPECT_DEPTH_BIT);
        readback=buffer(VkDeviceSize(width)*height*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT,true);
        begin();VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;b.newLayout=VK_IMAGE_LAYOUT_GENERAL;b.dstAccessMask=VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.image=color.handle;b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,1,&b);submit();
        VkAttachmentDescription a[2]{};a[0].format=VK_FORMAT_R8G8B8A8_UNORM;a[0].samples=VK_SAMPLE_COUNT_1_BIT;a[0].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;a[0].storeOp=VK_ATTACHMENT_STORE_OP_STORE;a[0].stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a[0].stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;a[0].initialLayout=a[0].finalLayout=VK_IMAGE_LAYOUT_GENERAL;
        a[1].format=VK_FORMAT_D32_SFLOAT;a[1].samples=VK_SAMPLE_COUNT_1_BIT;a[1].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;a[1].storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;a[1].stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;a[1].stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;a[1].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;a[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference c{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},d{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;subpass.colorAttachmentCount=1;subpass.pColorAttachments=&c;subpass.pDepthStencilAttachment=&d;
        VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.attachmentCount=2;rp.pAttachments=a;rp.subpassCount=1;rp.pSubpasses=&subpass;check(vkCreateRenderPass(device,&rp,nullptr,&renderPass),"forward render pass");
        VkImageView views[]={color.view,depth.view};VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fb.renderPass=renderPass;fb.attachmentCount=2;fb.pAttachments=views;fb.width=width;fb.height=height;fb.layers=1;check(vkCreateFramebuffer(device,&fb,nullptr,&framebuffer),"forward framebuffer");
    }
    void createDescriptors() {
        const VkShaderStageFlags stages=VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT|(usePipeline?VK_SHADER_STAGE_RAYGEN_BIT_KHR:0);
        VkDescriptorSetLayoutBinding bindings[]={{0,VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,1,stages,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1,stages,nullptr},{2,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,stages,nullptr}};
        VkDescriptorSetLayoutCreateInfo l{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};l.bindingCount=3;l.pBindings=bindings;check(vkCreateDescriptorSetLayout(device,&l,nullptr,&setLayout),"descriptor layout");
        VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,1},{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1}};VkDescriptorPoolCreateInfo p{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};p.maxSets=1;p.poolSizeCount=3;p.pPoolSizes=sizes;check(vkCreateDescriptorPool(device,&p,nullptr,&descriptorPool),"descriptor pool");
        VkDescriptorSetAllocateInfo a{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};a.descriptorPool=descriptorPool;a.descriptorSetCount=1;a.pSetLayouts=&setLayout;check(vkAllocateDescriptorSets(device,&a,&set),"descriptor set");
        VkWriteDescriptorSetAccelerationStructureKHR as{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};as.accelerationStructureCount=1;as.pAccelerationStructures=&tlas.handle;
        VkDescriptorImageInfo i{};i.imageView=color.view;i.imageLayout=VK_IMAGE_LAYOUT_GENERAL;VkDescriptorBufferInfo b{vertices.handle,0,VK_WHOLE_SIZE};
        VkWriteDescriptorSet writes[3]{};for(unsigned k=0;k<3;k++){writes[k].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[k].dstSet=set;writes[k].dstBinding=k;writes[k].descriptorCount=1;writes[k].descriptorType=bindings[k].descriptorType;}writes[0].pNext=&as;writes[1].pImageInfo=&i;writes[2].pBufferInfo=&b;vkUpdateDescriptorSets(device,3,writes,0,nullptr);
        VkPushConstantRange range{stages,0,sizeof(Push)};VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=1;pl.pSetLayouts=&setLayout;pl.pushConstantRangeCount=1;pl.pPushConstantRanges=&range;check(vkCreatePipelineLayout(device,&pl,nullptr,&layout),"pipeline layout");
    }
    void createShaderPipelines() {
        auto vs=stage(VK_SHADER_STAGE_VERTEX_BIT,module(forward_vert));auto fs=stage(VK_SHADER_STAGE_FRAGMENT_BIT,module(forward_frag));VkPipelineShaderStageCreateInfo stages[]={vs,fs};
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkViewport viewport{0,0,float(width),float(height),0,1};VkRect2D scissor{{0,0},{width,height}};VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;vp.pViewports=&viewport;vp.pScissors=&scissor;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.polygonMode=VK_POLYGON_MODE_FILL;rs.cullMode=VK_CULL_MODE_NONE;rs.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};ds.depthTestEnable=ds.depthWriteEnable=VK_TRUE;ds.depthCompareOp=VK_COMPARE_OP_LESS;
        VkPipelineColorBlendAttachmentState ba{};ba.colorWriteMask=15;VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&ba;
        VkGraphicsPipelineCreateInfo gp{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};gp.stageCount=2;gp.pStages=stages;gp.pVertexInputState=&vi;gp.pInputAssemblyState=&ia;gp.pViewportState=&vp;gp.pRasterizationState=&rs;gp.pMultisampleState=&ms;gp.pDepthStencilState=&ds;gp.pColorBlendState=&blend;gp.layout=layout;gp.renderPass=renderPass;
        check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&gp,nullptr,&raster),"forward raster + hardware ray-query pipeline");
        VkComputePipelineCreateInfo cp{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};cp.stage=stage(VK_SHADER_STAGE_COMPUTE_BIT,module(full_comp));cp.layout=layout;check(vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&cp,nullptr,&compute),"full ray-query compute pipeline");
        if(!usePipeline)return;
        VkPipelineShaderStageCreateInfo rtstages[]={stage(VK_SHADER_STAGE_RAYGEN_BIT_KHR,module(full_rgen)),stage(VK_SHADER_STAGE_MISS_BIT_KHR,module(full_rmiss)),stage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,module(full_rchit))};
        VkRayTracingShaderGroupCreateInfoKHR groups[3]{};
        for(unsigned i=0;i<3;i++){groups[i].sType=VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;groups[i].type=i==2?VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR:VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;groups[i].generalShader=i<2?i:VK_SHADER_UNUSED_KHR;groups[i].closestHitShader=i==2?2:VK_SHADER_UNUSED_KHR;groups[i].anyHitShader=groups[i].intersectionShader=VK_SHADER_UNUSED_KHR;}
        VkRayTracingPipelineCreateInfoKHR rp{VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};rp.stageCount=3;rp.pStages=rtstages;rp.groupCount=3;rp.pGroups=groups;rp.maxPipelineRayRecursionDepth=1;rp.layout=layout;check(createPipelines(device,VK_NULL_HANDLE,VK_NULL_HANDLE,1,&rp,nullptr,&pipeline),"full RT pipeline");
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rt{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};VkPhysicalDeviceProperties2 p{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};p.pNext=&rt;vkGetPhysicalDeviceProperties2(physical,&p);
        const uint64_t alignment=rt.shaderGroupBaseAlignment;const uint64_t stride=(rt.shaderGroupHandleSize+alignment-1)&~(alignment-1);
        sbt=buffer(stride*3+alignment,VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,true);
        std::vector<unsigned char> handles(rt.shaderGroupHandleSize*3);check(getGroups(device,pipeline,0,3,handles.size(),handles.data()),"SBT handles");
        const auto base=(sbt.address+alignment-1)&~(alignment-1);void* data{};check(vkMapMemory(device,sbt.memory,0,VK_WHOLE_SIZE,0,&data),"map SBT");
        for(unsigned i=0;i<3;i++)std::memcpy(static_cast<unsigned char*>(data)+(base-sbt.address)+i*stride,handles.data()+i*rt.shaderGroupHandleSize,rt.shaderGroupHandleSize);vkUnmapMemory(device,sbt.memory);
        raygen={base,stride,stride};miss={base+stride,stride,stride};hitgroup={base+stride*2,stride,stride};
    }
    Push controls(int scene,float phase) {
        const P eyes[]={{0,.70f,1.85f},{.15f,.60f,-2.40f},{.65f,.55f,.30f}};
        const P targets[]={{0,.05f,-4.8f},{0,.15f,-6.1f},{-1.83f,.10f,-1.0f}};
        auto forward=unit(sub(targets[scene],eyes[scene])),right=unit(cross(forward,{0,1,0})),up=unit(cross(right,forward));
        auto v=[](P p){return std::array<float,4>{p[0],p[1],p[2],0};};
        return {v(eyes[scene]),v(forward),v(right),v(up),{float(width),float(height),phase,1}};
    }
    Sample render(int scene,bool hybrid,int block,int frame,float phase) {
        auto start=std::chrono::steady_clock::now();auto pc=controls(scene,phase);begin();vkCmdResetQueryPool(command,queries,0,2);
        // Common prior-write dependency is outside the measured render interval.
        barrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_ACCESS_MEMORY_WRITE_BIT,VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT);
        vkCmdWriteTimestamp(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,queries,0);
        const VkShaderStageFlags stages=VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT|(usePipeline?VK_SHADER_STAGE_RAYGEN_BIT_KHR:0);
        vkCmdPushConstants(command,layout,stages,0,sizeof(pc),&pc);
        if(hybrid) {
            // Same display transform as RT miss shader; includes clear and depth costs.
            const auto encode=[](float c){c*=1.25f;c=std::clamp(c*(2.51f*c+.03f)/(c*(2.43f*c+.59f)+.14f),0.f,1.f);return c<=.0031308f?12.92f*c:1.055f*std::pow(c,1.f/2.4f)-.055f;};
            VkClearValue clears[2]{};clears[0].color={{encode(.012f),encode(.018f),encode(.030f),1}};clears[1].depthStencil={1,0};
            VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};rp.renderPass=renderPass;rp.framebuffer=framebuffer;rp.renderArea={{0,0},{width,height}};rp.clearValueCount=2;rp.pClearValues=clears;vkCmdBeginRenderPass(command,&rp,VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,raster);vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&set,0,nullptr);vkCmdDraw(command,static_cast<uint32_t>(mesh.size()),1,0,0);vkCmdEndRenderPass(command);
        } else if(usePipeline) {
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,pipeline);vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,layout,0,1,&set,0,nullptr);trace(command,&raygen,&miss,&hitgroup,&callable,width,height,1);
        } else {
            vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,compute);vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&set,0,nullptr);vkCmdDispatch(command,(width+7)/8,(height+7)/8,1);
        }
        vkCmdWriteTimestamp(command,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,queries,1);
        auto recorded=std::chrono::steady_clock::now();submit();
        auto end=std::chrono::steady_clock::now();uint64_t ts[2]{};check(vkGetQueryPoolResults(device,queries,0,2,sizeof(ts),ts,sizeof(uint64_t),VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WAIT_BIT),"completed timestamps");
        const uint64_t mask=timestampBits==64?~uint64_t(0):(uint64_t(1)<<timestampBits)-1;
        return {scene,block,frame,hybrid,double((ts[1]-ts[0])&mask)*properties.limits.timestampPeriod/1e6,std::chrono::duration<double,std::milli>(end-start).count(),std::chrono::duration<double,std::milli>(recorded-start).count(),std::chrono::duration<double,std::milli>(end-recorded).count(),phase};
    }
    std::vector<unsigned char> pixels() {
        begin();barrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_ACCESS_MEMORY_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT);
        VkBufferImageCopy c{};c.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};c.imageExtent={width,height,1};vkCmdCopyImageToBuffer(command,color.handle,VK_IMAGE_LAYOUT_GENERAL,readback.handle,1,&c);
        barrier(VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_HOST_READ_BIT);submit();
        std::vector<unsigned char> out(size_t(width)*height*4);void* p{};check(vkMapMemory(device,readback.memory,0,VK_WHOLE_SIZE,0,&p),"map capture");std::memcpy(out.data(),p,out.size());vkUnmapMemory(device,readback.memory);return out;
    }
    uint64_t allocationBytes()const {uint64_t n=color.allocation+depth.allocation;for(auto b:{vertices,instances,scratch,readback,sbt,tlas.buffer,blas.buffer})n+=b.allocation;return n;}
};
void bmp(const std::filesystem::path& file,const std::vector<unsigned char>& rgba,int w,int h) {
    const uint32_t stride=(w*3+3)&~3;const uint32_t bytes=stride*h;
    std::vector<unsigned char> b(54+bytes,0);b[0]='B';b[1]='M';
    auto u32=[&](int offset,uint32_t n){for(int i=0;i<4;i++)b[offset+i]=static_cast<unsigned char>(n>>(i*8));};
    u32(2,static_cast<uint32_t>(b.size()));u32(10,54);u32(14,40);u32(18,w);u32(22,h);b[26]=1;b[28]=24;u32(34,bytes);
    for(int y=0;y<h;y++)for(int x=0;x<w;x++)for(int k=0;k<3;k++)b[54+(h-1-y)*stride+x*3+k]=rgba[(size_t(y)*w+x)*4+2-k];
    std::ofstream f(file,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());if(!f)throw std::runtime_error("Capture write failed");
}
double percentile(std::vector<double> v,double q){if(v.empty())return 0;std::sort(v.begin(),v.end());return v[static_cast<size_t>(std::ceil(q*(v.size()-1)))];}
} // namespace

std::string RunHordeHybridLab(const std::string& output,int width,int height,int frames,bool validation,bool forceQuery) {
    if(width<64||height<64||width>1920||height>1080||frames<4||frames>240)throw std::runtime_error("Use 64..1920 x 64..1080 and 4..240 frames/block");
    const auto directory=std::filesystem::absolute(output);std::filesystem::create_directories(directory);
    if(std::filesystem::exists(directory/"results.json"))throw std::runtime_error("Results already exist; choose a new output directory to preserve evidence");
    Lab lab;lab.initialise(width,height,validation,forceQuery);
    const char* scenes[]={"Opening tomb","Portal and moving light","Metal mirror"};
    std::vector<Sample> samples;
    std::ostringstream summary,json,html;json<<std::setprecision(10);summary<<std::fixed<<std::setprecision(3);
    json<<"{\n\"buildIdentity\":\""<<kLabBuildIdentity<<"\",\n\"schema\":1,\n\"device\":\""<<escape(lab.properties.deviceName)<<"\",\n\"vendorId\":"<<lab.properties.vendorID<<",\"deviceId\":"<<lab.properties.deviceID<<",\"driverVersion\":"<<lab.properties.driverVersion<<",\"apiVersion\":"<<lab.properties.apiVersion<<",\n\"width\":"<<width<<",\"height\":"<<height<<",\"triangles\":"<<lab.mesh.size()/3<<",\"baseline\":\""<<escape(lab.baseline)<<"\",\"allocationBytes\":"<<lab.allocationBytes()<<",\"initialiseCpuMs\":"<<lab.initialiseCpuMs<<",\"blasBuildGpuMs\":"<<lab.blasBuildGpuMs<<",\"tlasBuildGpuMs\":"<<lab.tlasBuildGpuMs<<",\n\"scope\":\"Offscreen finite lab. No WSI, gameplay, sustained thermal or shipping-engine FPS evidence.\",\n\"scenes\":[\n";
    html<<"<!doctype html><html lang='en'><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Horde Hybrid Lab</title><style>body{background:#12181b;color:#e2e6df;font:17px system-ui;max-width:1200px;margin:auto;padding:24px}h1{font-family:Georgia}p{line-height:1.55}table{border-collapse:collapse;width:100%;margin:20px 0}td,th{border:1px solid #647177;padding:10px;text-align:left}.pair{display:grid;grid-template-columns:1fr 1fr;gap:12px}img{width:100%;image-rendering:auto}button,select{font:inherit;padding:12px;margin:4px;background:#344342;color:white;border:1px solid #90a6a1}figure{margin:0}figcaption{padding:8px}a{color:#a7dbca}@media(max-width:650px){.pair{grid-template-columns:1fr}body{padding:14px}table{font-size:14px}}</style><h1>The Horde / Hybrid Lab</h1><p>Experimental native Vulkan comparison • "<<width<<" × "<<height<<" • "<<lab.properties.deviceName<<"</p><p><b>Scope:</b> a bounded tomb geometry slice, shared procedural masonry, three direct lights with hardware ray-query hard shadows, one mirror bounce. The moving light is deterministic. Production textures, water, glass, volumetric fire/mist, characters, gameplay and swapchain presentation are outside this test. Existing full RT builds are unchanged.</p><p>Baseline: "<<lab.baseline<<". Candidate: forward raster primary visibility + BLAS/TLAS-backed hardware ray-query lighting. No software traversal or raster-only fallback.</p><p>GPU times include the complete offscreen render pass/dispatch, including candidate depth and clear. CPU cycle includes command recording, submission and fence wait; it overlaps GPU execution. <b>1000 / GPU ms is an isolated render ceiling, not measured game FPS.</b> Readback and report generation are excluded. One frame in flight; no VSync or dynamic scaling.</p><p>Eight ABBA/ABBA blocks per scene, 8 warm-up frames per block, "<<frames<<" measured frames/block, same phase sequence. Repeat runs in reverse scene order in a future harness and use multiple sessions for thermal conclusions.</p>";
    for(int scene=0;scene<3;scene++) {
        for(int block=0;block<8;block++) {
            const bool hybrid=block%4==1||block%4==2;
            for(int i=0;i<8;i++)lab.render(scene,hybrid,block,-1,float(i)*.1f);
            for(int i=0;i<frames;i++)samples.push_back(lab.render(scene,hybrid,block,i,6.2831853f*float(i)/float(frames-1)));
        }
        lab.render(scene,false,-1,-1,0);auto full=lab.pixels();
        lab.render(scene,true,-1,-1,0);auto hybrid=lab.pixels();
        bmp(directory/("scene"+std::to_string(scene)+"-rt.bmp"),full,width,height);bmp(directory/("scene"+std::to_string(scene)+"-hybrid.bmp"),hybrid,width,height);
        std::vector<unsigned char> diff(full.size(),255);double mae=0;uint64_t changed=0;unsigned maxError=0;
        for(size_t i=0;i<full.size();i+=4){bool different=false;for(int k=0;k<3;k++){unsigned e=unsigned(std::abs(int(full[i+k])-int(hybrid[i+k])));mae+=e;maxError=std::max(maxError,e);diff[i+k]=static_cast<unsigned char>(std::min(255u,e*8));if(e>8)different=true;}changed+=different;}
        mae/=double(width)*height*3;bmp(directory/("scene"+std::to_string(scene)+"-diff.bmp"),diff,width,height);
        html<<"<h2>"<<scenes[scene]<<"</h2><div class='pair'><figure><img alt='Full RT "<<scenes[scene]<<"' src='scene"<<scene<<"-rt.bmp'><figcaption>Full hardware RT primary</figcaption></figure><figure><img alt='Hybrid "<<scenes[scene]<<"' src='scene"<<scene<<"-hybrid.bmp'><figcaption>Hybrid primary + hardware ray queries</figcaption></figure></div><details><summary>Difference ×8 and raw image metrics</summary><img alt='Absolute RGB difference amplified eight times' src='scene"<<scene<<"-diff.bmp'><p>Mean absolute RGB error: "<<mae<<" /255; max: "<<maxError<<"; pixels with any channel error &gt;8: "<<100.0*changed/(double(width)*height)<<"%. Pixel differences are diagnostic, not human visual acceptance.</p></details><table><tr><th>Mode</th><th>GPU median / p95 ms</th><th>CPU cycle median / p95 ms</th><th>Isolated GPU ceiling (frames/s)</th></tr>";
        if(scene)json<<",\n";json<<"{\"name\":\""<<scenes[scene]<<"\",\"rgbMae255\":"<<mae<<",\"maxChannelError\":"<<maxError<<",\"pixelsOver8Fraction\":"<<double(changed)/(double(width)*height)<<",\"modes\":[";
        for(int mode=0;mode<2;mode++) {
            std::vector<double> gpu,cpu;for(auto& s:samples)if(s.scene==scene&&s.hybrid==bool(mode)){gpu.push_back(s.gpu);cpu.push_back(s.cpu);}
            const auto gm=percentile(gpu,.5),gp=percentile(gpu,.95),cm=percentile(cpu,.5),cp=percentile(cpu,.95);
            if(gm<=0)throw std::runtime_error("GPU timer returned no positive render duration");
            summary<<scenes[scene]<<" / "<<(mode?"hybrid":"full RT")<<": GPU median "<<gm<<" ms, p95 "<<gp<<" ms; CPU cycle median "<<cm<<" ms\n";
            html<<"<tr><td>"<<(mode?"Hybrid":"Full RT")<<"</td><td>"<<gm<<" / "<<gp<<"</td><td>"<<cm<<" / "<<cp<<"</td><td>"<<1000.0/gm<<"</td></tr>";
            if(mode)json<<',';json<<"{\"mode\":\""<<(mode?"hybrid":"full-rt")<<"\",\"samples\":"<<gpu.size()<<",\"gpuMedianMs\":"<<gm<<",\"gpuP95Ms\":"<<gp<<",\"cpuCycleMedianMs\":"<<cm<<",\"cpuCycleP95Ms\":"<<cp<<",\"isolatedGpuCeilingFps\":"<<1000.0/gm<<"}";
        }
        json<<"]}";html<<"</table>";
    }
    json<<"\n],\"validationEnabled\":"<<(lab.validationEnabled?"true":"false")<<",\"validationErrors\":"<<lab.errors<<",\"validationWarnings\":"<<lab.warnings<<"}\n";
    html<<"<p>Validation layer: "<<(lab.validationEnabled?"enabled":"not enabled")<<"; errors "<<lab.errors<<", warnings "<<lab.warnings<<". Actual lab-owned Vulkan allocation: "<<lab.allocationBytes()<<" bytes (driver overhead excluded).</p><p><a href='results.json'>Structured results</a> · <a href='frames.csv'>Every measured frame</a></p><p>Next decision: measure on the phone, inspect shadow and mirror edges, then decide whether to integrate a raster visibility adapter into the full engine. These numbers cannot predict the cost of its large material shader, animated BLAS/TLAS work or bandwidth.</p></html>";
    std::ofstream(directory/"results.json")<<json.str();std::ofstream(directory/"index.html")<<html.str();
    std::ofstream csv(directory/"frames.csv");csv<<"scene,block,frame,mode,phase,gpu_ms,cpu_cycle_ms,cpu_record_ms,cpu_submit_wait_ms\n"<<std::setprecision(10);for(auto& s:samples)csv<<s.scene<<','<<s.block<<','<<s.frame<<','<<(s.hybrid?"hybrid":"full-rt")<<','<<s.phase<<','<<s.gpu<<','<<s.cpu<<','<<s.record<<','<<s.submitWait<<'\n';
    std::ofstream(directory/"build-identity.txt")<<kLabBuildIdentity<<"\n";
    if(lab.errors)throw std::runtime_error("Validation errors were recorded; results are diagnostic only, not an accepted run");
    summary<<"Report: "<<(directory/"index.html").string()<<"\nDevice: "<<lab.properties.deviceName<<"\nBaseline: "<<lab.baseline;return summary.str();
}
