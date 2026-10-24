    // Included privately by CompanionRouteFixture. This shared turn/start branch owns
    // no resources, platform clock, saved state or alternate collision policy.
    void ClearMotion() {
        state_.motion={};state_.motion.turnStartEnabled=turnStartEnabled_;
        turnElapsed_=entryElapsed_=0;turnStartYaw_=entryStartWeight_=0;
    }
    static float HeadingError(float target,float current) {
        return std::remainder(target-current,6.28318530718f);
    }
    void BeginTurn(float difference) {
        auto& m=state_.motion;
        m.mode=CompanionMotionMode::Turning;m.speed=0;m.turnPhase=0;
        m.turnAngleRadians=std::clamp(difference,-companion_turn::maxAngle,companion_turn::maxAngle);
        turnStartYaw_=state_.yaw;turnElapsed_=entryElapsed_=0;
        entryStartWeight_=m.idleZeroWeight;m.entryWeight=entryStartWeight_;
    }
    void StepTurnStart(float dt,float playerX,float playerZ,bool exterior,bool proof,bool paused,
                       const simulation::TriangleSupportSurface* terrain,bool requireTerrain,bool active) {
        if(paused) {state_.wait=CompanionWait::Paused;return;}
        if(!std::isfinite(dt)||dt<=0)return;
        if(!active||!exterior) {state_.walking=false;state_.wait=CompanionWait::PlayerInTomb;return;}
        if(requireTerrain&&!terrain) {state_.walking=false;state_.grounded=false;state_.wait=CompanionWait::Preparing;return;}
        // A one-tick waypoint bookkeeping pause is still continuous traversal,
        // not a planted departure. Reuse the proven legacy motor and yaw/gait
        // policy through ordinary bends; only genuine stops return to Idle.
        if(state_.motion.mode==CompanionMotionMode::Walking && proof &&
           std::hypot(playerX-state_.x,playerZ-state_.z)<=6.f &&
           state_.waypoint<simulation::kWorldRoutePoints.size()) {
            const auto oldYaw=state_.yaw,oldX=state_.x,oldZ=state_.z;
            const auto oldWaypoint=state_.waypoint;
            StepRoute(dt,playerX,playerZ,exterior,proof,false,terrain,requireTerrain);
            if(!state_.grounded) {state_.yaw=oldYaw;return;}
            dt=std::min(dt,.05f);
            const float target=state_.walking||state_.waypoint!=oldWaypoint?state_.yaw:oldYaw;
            state_.yaw=oldYaw+std::clamp(HeadingError(target,oldYaw),-2.4f*dt,2.4f*dt);
            state_.animationSeconds+=dt;
            locomotion_.Step(dt,std::hypot(state_.x-oldX,state_.z-oldZ),state_.walking);
            state_.locomotion=locomotion_.State();
            state_.motion.speed=state_.walking?1.8f:0.f;
            if(!state_.walking && state_.waypoint==oldWaypoint && state_.locomotion.walkWeight==0) {
                state_.motion.mode=CompanionMotionMode::Idle;state_.motion.idleZeroWeight=0;
            }
            return;
        }
        const auto under=requireTerrain?traversal::ChapterExteriorStepSupport(*terrain,state_.x,state_.z,.24f):
            simulation::ResolveWorldRouteSupport(state_.x,state_.z);
        state_.grounded=under.grounded&&(!requireTerrain||traversal::ResolveChapterStepMovement(
            *terrain,state_.x,state_.z,state_.x,state_.z,under.worldY,.24f).accepted);
        if(!state_.grounded) {state_.walking=false;state_.wait=CompanionWait::Obstacle;return;}
        state_.y=under.worldY;
        if(!std::isfinite(playerX)||!std::isfinite(playerZ)) {state_.walking=false;state_.wait=CompanionWait::PlayerLagging;return;}
        dt=std::min(dt,.05f);state_.animationSeconds+=dt;state_.walking=false;
        auto& m=state_.motion;
        const float playerDistance=std::hypot(playerX-state_.x,playerZ-state_.z);
        bool lead=proof&&playerDistance<=6.f&&state_.waypoint<simulation::kWorldRoutePoints.size();
        state_.wait=!proof?CompanionWait::Reunion:playerDistance>6.f?CompanionWait::PlayerLagging:
            state_.waypoint>=simulation::kWorldRoutePoints.size()?CompanionWait::Lookout:CompanionWait::Reunion;
        float dx=0,dz=0,distance=0;
        if(lead) {
            const auto p=simulation::kWorldRoutePoints[state_.waypoint];
            dx=p.x+1-state_.x;dz=p.z-state_.z;distance=std::hypot(dx,dz);
            if(distance<.06f) {
                // Preserve accepted bookkeeping: no movement, yaw or gait-phase
                // kick at an arrival. Next tick evaluates the new segment.
                ++state_.waypoint;m.mode=CompanionMotionMode::Idle;m.speed=0;
                locomotion_.StepWeighted(dt,0,0);state_.locomotion=locomotion_.State();return;
            }
        }
        const float target=lead?std::atan2(dx,-dz):playerDistance>.01f?
            std::atan2(playerX-state_.x,state_.z-playerZ):state_.yaw;
        const float error=HeadingError(target,state_.yaw);
        constexpr float departureBand=.01745329252f,idleBand=.1745329252f;
        if(m.mode==CompanionMotionMode::Turning) {
            locomotion_.StepWeighted(dt,0,0);state_.locomotion=locomotion_.State();
            if(m.entryWeight<1) {
                entryElapsed_+=dt;
                m.entryWeight=entryStartWeight_+(1-entryStartWeight_)*std::min(float(entryElapsed_/.18),1.f);
                m.idleZeroWeight=m.entryWeight;
                // The entry uses the entire fixed step; active root/joint time
                // starts on the next step. No double advancement at the join.
                return;
            }
            if(std::abs(m.turnAngleRadians)>departureBand) {
                turnElapsed_+=dt;m.turnPhase=std::min(float(turnElapsed_/companion_turn::playbackDuration),1.f);
                state_.yaw=turnStartYaw_-companion_turn::ModelYaw(m.turnAngleRadians>0,m.turnPhase)*
                    (std::abs(m.turnAngleRadians)/companion_turn::maxAngle);
                if(m.turnPhase<1)return;
            }
            m.idleZeroWeight=1;
            const float latest=HeadingError(target,state_.yaw);
            if(std::abs(latest)>(lead?departureBand:idleBand)) {BeginTurn(latest);return;}
            m.mode=lead?CompanionMotionMode::Starting:CompanionMotionMode::Idle;m.speed=0;
            return;
        }
        if(m.mode==CompanionMotionMode::Idle) {
            locomotion_.StepWeighted(dt,0,0);state_.locomotion=locomotion_.State();
            if(state_.locomotion.walkWeight>0)return;
            if(std::abs(error)>(lead?departureBand:idleBand)) {BeginTurn(error);return;}
            if(lead) {
                // Already aligned while stopped: use the ordinary Idle/Walk
                // blend, without inventing a zero-angle foot exchange.
                if(m.idleZeroWeight>0) {m.idleZeroWeight=std::max(0.f,m.idleZeroWeight-dt/.18f);return;}
                m.mode=CompanionMotionMode::Walking;m.speed=0;m.idleZeroWeight=0;return;
            }
            m.idleZeroWeight=std::max(0.f,m.idleZeroWeight-dt/.18f);return;
        }
        // Starts and ordinary walking share the same accepted-distance motor.
        // Stop immediately for tether/proof/obstacles; let the current gait
        // fade before scheduling another stationary turn. No turn/walk overlay.
        if(!lead||std::abs(error)>.35f) {
            m.speed=0;locomotion_.StepWeighted(dt,0,0);state_.locomotion=locomotion_.State();
            if(state_.locomotion.walkWeight==0) {
                m.idleZeroWeight=m.mode==CompanionMotionMode::Starting?1.f:0.f;
                m.mode=CompanionMotionMode::Idle;
            }
            return;
        }
        constexpr float acceleration=9.f;
        const float requested=std::min(1.8f,std::sqrt(2*acceleration*std::max(distance-.055f,0.f)));
        m.speed+=std::clamp(requested-m.speed,-acceleration*dt,acceleration*dt);
        const float travel=std::min(distance,m.speed*dt);
        const float x=state_.x+dx/distance*travel,z=state_.z+dz/distance*travel;
        const auto step=requireTerrain?traversal::ResolveChapterStepMovement(
            *terrain,state_.x,state_.z,x,z,state_.y,.24f):traversal::ChapterStepMovement{};
        const auto support=requireTerrain?step.support:simulation::ResolveWorldRouteSupport(x,z);
        const bool clear=requireTerrain?step.accepted:simulation::WorldRouteBlockoutMovementClear(
            state_.x,state_.z,x,z,state_.y,.24f,true);
        float accepted=0;
        if(!support.grounded||!clear) {m.speed=0;state_.wait=CompanionWait::Obstacle;}
        else {
            accepted=std::hypot(x-state_.x,z-state_.z);state_.walking=accepted>0;
            state_.x=x;state_.z=z;state_.y=support.worldY;
            state_.yaw+=std::clamp(error,-2.4f*dt,2.4f*dt);
        }
        locomotion_.StepWeighted(dt,accepted,m.speed/1.8f);state_.locomotion=locomotion_.State();
        if(m.mode==CompanionMotionMode::Starting&&state_.locomotion.walkWeight==1) {
            m.mode=CompanionMotionMode::Walking;m.idleZeroWeight=0;
        }
        if((!support.grounded||!clear)&&state_.locomotion.walkWeight==0) {
            m.idleZeroWeight=m.mode==CompanionMotionMode::Starting?1.f:0.f;m.mode=CompanionMotionMode::Idle;
        }
    }
