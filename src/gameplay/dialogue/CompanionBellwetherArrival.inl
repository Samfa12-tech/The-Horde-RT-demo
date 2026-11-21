    // These stations are the same admitted road, ramp and room used by the
    // player. The final three marks lead to the anchored native sit.
    static constexpr auto kKitBellwetherRoute=[] {
        using namespace horde::gameplay;
        std::array<traversal::BellwetherStation,traversal::kBellwetherTavernRoute.size()+3> route{};
        std::copy(traversal::kBellwetherTavernRoute.begin(),traversal::kBellwetherTavernRoute.end(),route.begin());
        const auto room=traversal::kBellwetherTavernRoute.size();
        route[room]={bellwether::kChairTurn[0],bellwether::kChairTurn[2]};
        route[room+1]={bellwether::kChairApproach[0],bellwether::kChairApproach[2]};
        route[room+2]={bellwether::kChairSitRoot[0],bellwether::kChairSitRoot[2]};
        return route;
    }();

    void StepBellwetherArrival(float dt,const simulation::TriangleSupportSurface& terrain) {
        using Stage=BellwetherArrivalStage;
        auto& stage=state_.bellwetherArrival;
        auto& motion=state_.motion;
        state_.walking=false;
        state_.wait=CompanionWait::Tavern;
        state_.animationSeconds+=dt;
        if(stage==Stage::AnkleAssessment) {
            motion.anklePlaybackSeconds=std::min(motion.anklePlaybackSeconds+dt,kAnkleAssessmentDurationSeconds);
            if(motion.anklePlaybackSeconds>=kAnkleAssessmentDurationSeconds-.00001f) {
                motion.anklePlaybackSeconds=kAnkleAssessmentDurationSeconds;
                locomotion_.Reset();state_.locomotion=locomotion_.State();
                motion.mode=CompanionMotionMode::Walking;
                stage=Stage::ProtectiveApproach;
            }
            return;
        }
        constexpr float aligned=.01745329252f;
        if(stage==Stage::ChairTurn) {
            if(motion.mode==CompanionMotionMode::Turning && !AdvanceTurn(dt))return;
            locomotion_.Step(dt,0,false);state_.locomotion=locomotion_.State();
            if(state_.locomotion.walkWeight>0)return;
            const float error=HeadingError(bellwether::kChairFacingRadians,state_.yaw);
            if(std::abs(error)>aligned) {BeginTurn(error,false);return;}
            state_.yaw=bellwether::kChairFacingRadians;
            if(BeginTavernSit())stage=Stage::Lowering;
            return;
        }

        const auto target=kKitBellwetherRoute[state_.bellwetherStation];
        const float dx=target.x-state_.x,dz=target.z-state_.z;
        const float distance=std::hypot(dx,dz);
        if(distance<.001f) {
            const auto arrived=state_.bellwetherStation++;
            if(arrived==traversal::kBellwetherAnkleStation) {
                ClearMotion();stage=Stage::AnkleAssessment;
                acting_.Reset();state_.acting=acting_.State();
            } else if(state_.bellwetherStation==kKitBellwetherRoute.size()) {
                ClearMotion();stage=Stage::ChairTurn;
            }
            return;
        }

        const float error=HeadingError(std::atan2(dx,-dz),state_.yaw);
        if(motion.mode==CompanionMotionMode::Turning) {
            if(!AdvanceTurn(dt))return;
            motion.mode=CompanionMotionMode::Idle;
            return;
        }
        if(motion.mode==CompanionMotionMode::Idle && std::abs(error)>aligned) {
            BeginTurn(error,true);
            return;
        }
        const bool protective=stage==Stage::ProtectiveApproach;
        const float speed=protective?.88f:1.65f;
        const float travel=std::min(distance,speed*dt);
        const float x=travel==distance ? target.x : state_.x+dx/distance*travel;
        const float z=travel==distance ? target.z : state_.z+dz/distance*travel;
        const auto move=traversal::ResolveChapterStepMovement(terrain,state_.x,state_.z,x,z,state_.y,.24f);
        if(!move.accepted) {
            state_.wait=CompanionWait::Obstacle;motion.speed=0;
            locomotion_.Step(dt,0,false);state_.locomotion=locomotion_.State();
            return;
        }
        state_.walking=true;motion.mode=CompanionMotionMode::Walking;motion.speed=speed;
        state_.x=x;state_.y=move.support.worldY;state_.z=z;state_.grounded=move.support.grounded;
        state_.yaw+=std::clamp(error,-2.4f*dt,2.4f*dt);
        const float stride=protective?kProtectiveWalkStrideMetres:CompanionLocomotion::kWalkCycleDistanceMetres;
        locomotion_.Step(dt,travel,true,stride);state_.locomotion=locomotion_.State();
    }
