// Compiled ONLY into the copied Windows capture harness. No runtime package
// or production descriptor/policy is changed by this diagnostic recipe.
namespace horde::vulkan::raytracing {
bool PresentableTinyRtScene::BuildMistConeDiagnostic(std::string& diagnostic)
{
    const auto begin=std::chrono::steady_clock::now();
    horde::gameplay::simulation::GameSimulation simulation;
    const auto* checkpoint=horde::gameplay::FindDevelopmentCheckpoint("rescue-journey-start");
    if(!checkpoint || !horde::gameplay::StageDevelopmentCheckpointSimulation(simulation,*checkpoint)) {
        diagnostic="Cannot stage exact cone discovery fixture";return false;
    }
    simulation.SetDevelopmentRescueJourney(true);
    horde::gameplay::simulation::InputSnapshot input;
    input.damageEnabled=false;input.hasAuthoritativePlayerPose=true;
    input.authoritativePlayerX=checkpoint->cameraX;input.authoritativePlayerZ=checkpoint->cameraZ;
    input.yawRadians=checkpoint->yaw;input.pitchRadians=checkpoint->pitch;input.torchLightStrength=0;
    for(unsigned tick=0;tick<600;++tick) simulation.StepFixed(input);
    const auto rope=horde::scene::RescueRopeTriangleVertices(simulation.Snapshot().rescue);
    if(rope.size()!=std::size(kProbeRope) || rescueWorldPrimitiveCount_-rope.size()/3!=3022u) {
        diagnostic="Current rope topology/primitive range differs from cone input: vertices="+std::to_string(rope.size())+
            ", world primitives="+std::to_string(rescueWorldPrimitiveCount_)+", rope first="+std::to_string(rescueWorldPrimitiveCount_-rope.size()/3);return false;
    }
    for(std::size_t i=0;i<rope.size();++i) if(rope[i]!=kProbeRope[i]) {
        diagnostic="Current authoritative rope differs from cone input";return false;
    }
    const auto upload=[&](const void* data,VkDeviceSize size,VkBufferUsageFlags usage,bool address,Buffer& out) {
        return CreateBuffer(size,usage,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            address,out,diagnostic) && WriteBuffer(out,data,size,"mist interval diagnostic",diagnostic);
    };
    const auto inputUsage=VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    if(!upload(kProbeBounds,sizeof(kProbeBounds),inputUsage,true,probeAabbs_) ||
       !upload(kProbeCones,sizeof(kProbeCones),VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,false,probePlanes_)) return false;
    VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometryType=VK_GEOMETRY_TYPE_AABBS_KHR;
    geometry.geometry.aabbs={VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR};
    geometry.geometry.aabbs.data.deviceAddress=probeAabbs_.address;
    geometry.geometry.aabbs.stride=sizeof(kProbeBounds[0]);
    VkAccelerationStructureBuildRangeInfoKHR range{};range.primitiveCount=static_cast<unsigned>(std::size(kProbeBounds));
    if(!BuildProfileAccelerationStructure({&geometry,1},{&range,1},VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,probeBlas_,nullptr,diagnostic)) return false;
    VkAccelerationStructureInstanceKHR instance{};
    instance.transform.matrix[0][0]=instance.transform.matrix[1][1]=instance.transform.matrix[2][2]=1;
    instance.mask=255;instance.accelerationStructureReference=probeBlas_.address;
    if(!upload(&instance,sizeof(instance),inputUsage,true,probeInstances_)) return false;
    VkAccelerationStructureGeometryKHR top{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    top.geometryType=VK_GEOMETRY_TYPE_INSTANCES_KHR;
    top.geometry.instances={VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
    top.geometry.instances.data.deviceAddress=probeInstances_.address;
    range.primitiveCount=1;
    if(!BuildProfileAccelerationStructure({&top,1},{&range,1},VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,probeTlas_,nullptr,diagnostic)) return false;
    std::ofstream output("reports/mist-interval-prototype/native-resources.json");
    output<<"{\"source\":\"frozen authoritative 176-triangle rope, two exact point apertures\",\"cones\":"<<std::size(kProbeBounds)
        <<",\"aabbBytes\":"<<probeAabbs_.size<<",\"planeBytes\":"<<probePlanes_.size
        <<",\"blasBytes\":"<<probeBlas_.backing.size<<",\"tlasBytes\":"<<probeTlas_.backing.size
        <<",\"cpuBuildNanoseconds\":"<<std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-begin).count()<<"}\n";
    return true;
}
}
