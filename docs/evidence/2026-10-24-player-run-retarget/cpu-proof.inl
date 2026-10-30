// Temporary diagnostic through existing rig helpers; never a registered test.
if (const char* out=std::getenv("HORDE_LOCAL_RUN_CPU_PROOF")) {
    horde::scene::assets::AssetManifest manifest;
    horde::scene::assets::StaticMeshAsset layout;
    if(!horde::scene::assets::AssetManifest::Load(root/"assets/models/player/runtime/asset.manifest.json",manifest,diagnostic)||
       !horde::scene::assets::StaticMeshAsset::Load(rigPath,manifest,layout,diagnostic))
        return Require(false,diagnostic.c_str())?0:1;
    std::array<std::vector<unsigned>,2> boots;
    for(unsigned i=0;i<layout.vertices.size();++i) {
        const auto& p=layout.vertices[i].position;
        if(p[1]<=.45f&&p[0]!=0)boots[p[0]>0?0:1].push_back(i);
    }
    if(!Require(!boots[0].empty()&&!boots[1].empty(),"Run proof needs both actual boot subsets"))return 1;
    std::ofstream csv(out);csv.precision(10);
    csv<<"view,tick,speed,travel,phase,blend,root_y,left_x,left_y,left_z,right_x,right_y,right_z,left_min_y,right_min_y,left_grip_error,right_grip_error\n";
    unsigned poses=0;bool fullSpeed=false;
    for(unsigned view=0;view<2;++view) {
        auto config=ProductionGameSimulationConfig();config.swordStartsStowed=false;
        auto sim=std::make_unique<GameSimulation>(config);
        InputSnapshot input{};input.damageEnabled=false;input.tutorialEnabled=false;
        input.pitchRadians=view?-.95f:-.15f;input.moveForward=1;input.runHeld=true;
        for(unsigned tick=1;tick<=96;++tick) {
            sim->StepFixed(input);const auto& snap=sim->Snapshot();
            bool updated=false;Vec3 origin{};PlayerModelWorldBasis basis;
            if(!PrepareRenderedPose(rig,snap,++poses,updated,diagnostic,nullptr,nullptr,&origin,&basis))
                return Require(false,diagnostic.c_str())?0:1;
            if(!Require(updated&&rig.SolvedPose().IsValid()&&rig.UniqueVertices().size()==layout.vertices.size(),"Run pose owns original static stream"))return 1;
            if(!Require(rig.LeftGripAgreement().positionErrorMetres<=kPlayerGripSocketToleranceMetres&&
                rig.RightGripAgreement().positionErrorMetres<=kPlayerGripSocketToleranceMetres&&
                rig.LeftGripAgreement().orientationErrorRadians<=kPlayerGripOrientationToleranceRadians&&
                rig.RightGripAgreement().orientationErrorRadians<=kPlayerGripOrientationToleranceRadians,
                "Run retains existing Grip position/orientation gates"))return 1;
            std::array<Vec3,2> mean{};std::array<float,2> low{{100,100}};
            for(unsigned side=0;side<2;++side)for(auto i:boots[side]) {
                const auto& p=rig.UniqueVertices()[i].position;
                auto w=Add(origin,PlayerModelVectorToWorld(basis,{{p[0],p[1],p[2]}}));
                for(unsigned axis=0;axis<3;++axis)mean[side][axis]+=w[axis]/boots[side].size();
                low[side]=std::min(low[side],w[1]);
            }
            if(std::min(low[0],low[1])<snap.playerSupportWorldY-.00001f||std::min(low[0],low[1])>snap.playerSupportWorldY+.025f)
                std::cerr<<"RUN_GROUNDING_FAILURE view="<<view<<" tick="<<tick<<" support="<<snap.playerSupportWorldY<<" soles="<<low[0]<<","<<low[1]<<" originY="<<origin[1]<<" phase="<<snap.playerAnimation.locomotionTime<<" blend="<<snap.playerAnimation.locomotionBlend<<"\n";
            if(!Require(std::min(low[0],low[1])>=snap.playerSupportWorldY-.00001f&&
                        std::min(low[0],low[1])<=snap.playerSupportWorldY+.025f,
                        "Run retains established native sole grounding bound"))return 1;
            fullSpeed|=snap.playerMovementSpeedMetresPerSecond>3.19f;
            const auto& a=snap.playerAnimation;
            csv<<view<<','<<tick<<','<<snap.playerMovementSpeedMetresPerSecond<<','<<snap.playerTravelledThisTick<<','
                <<(a.locomotionTime+a.locomotionPhaseOffsetSeconds)<<','<<a.locomotionBlend<<','<<origin[1];
            for(const auto& p:mean)for(float v:p)csv<<','<<v;
            csv<<','<<low[0]<<','<<low[1]<<','<<rig.LeftGripAgreement().positionErrorMetres<<','
                <<rig.RightGripAgreement().positionErrorMetres<<'\n';
        }
    }
    if(!Require(fullSpeed&&poses==192&&bool(csv),"Run proof must reach existing3.2m/s in both views"))return 1;
    std::cout<<"RUN_PROOF_PASS actual_poses="<<poses<<" vertices="<<rig.UniqueVertices().size()<<" GPU=not-run\n";
    return 0;
}
