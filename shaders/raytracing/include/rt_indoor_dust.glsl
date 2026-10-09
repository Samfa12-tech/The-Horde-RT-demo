// Sparse primary-path analytic motes. No particle geometry, light or secondary recursion.
// CPU conservative sphere projection selects at most four world-space candidates.
void dustSelectSource(vec3 position, vec3 radiance, bool sky, inout vec3 p0, inout vec3 l0,
                      inout bool sky0, inout vec3 p1, inout vec3 l1, inout bool sky1)
{
    float energy = max(radiance.r,max(radiance.g,radiance.b));
    if (!mistRadianceActive(radiance)) return;
    float first=max(l0.r,max(l0.g,l0.b)), second=max(l1.r,max(l1.g,l1.b));
    if(energy>first) { p1=p0;l1=l0;sky1=sky0;p0=position;l0=radiance;sky0=sky; }
    else if(energy>second) { p1=position;l1=radiance;sky1=sky; }
}
void dustSelectFire(uint index, vec3 p, inout vec3 p0, inout vec3 l0, inout bool sky0,
                    inout vec3 p1, inout vec3 l1, inout bool sky1)
{
    RtFireEmitterGpu emitter=rtFireEmitters.values[index];
    if(emitter.identity.x==0u || emitter.lightPositionStrength.w<=0.001) return;
    vec3 position=emitter.lightPositionStrength.xyz;
    vec3 light=tunedLightColor(emitter.colourIntensity.rgb,kLightTorch)*emitter.lightPositionStrength.w;
    dustSelectSource(position,light*mistPointAttenuation(p,position),false,p0,l0,sky0,p1,l1,sky1);
}
vec3 dustSourceVisibility(vec3 p,vec3 source,vec3 radiance,bool sky,float skyDistance)
{
    if(!mistRadianceActive(radiance)) return vec3(0.0);
    vec3 delta=sky ? source : source-p;
    float distance=sky ? skyDistance : length(delta);
    if(distance<=0.024) return vec3(0.0);
    return radiance*mistSegmentTransmittance(p,sky ? source : delta/distance,distance);
}
vec3 dustIncidentLight(vec3 p,uint quality)
{
    vec3 direction,radiance;float distance,gain;
    activeSkyLight(p,areaShadowSampleIndex(),direction,distance,radiance,gain);
    vec3 p0=vec3(0),l0=vec3(0),p1=vec3(0),l1=vec3(0);bool sky0=false,sky1=false;
    if(gain>0.0) dustSelectSource(direction,radiance*gain,true,p0,l0,sky0,p1,l1,sky1);
    // Select by unoccluded energy; a blocked dominant source fails dark. Never
    // compensate for occlusion or invent ambient radiance. Standard adds one source.
    dustSelectFire(0u,p,p0,l0,sky0,p1,l1,sky1);
    dustSelectFire(1u,p,p0,l0,sky0,p1,l1,sky1);
    dustSelectFire(2u,p,p0,l0,sky0,p1,l1,sky1);
    dustSelectFire(3u,p,p0,l0,sky0,p1,l1,sky1);
    if(controls.staffLightStrength>0.001) {
        vec3 staff=vec3(controls.staffX,controls.staffY,controls.staffZ);
        dustSelectSource(staff,tunedLightColor(vec3(0.58,0.10,1.0),kLightStaff)*
            controls.staffLightStrength*mistPointAttenuation(p,staff),false,p0,l0,sky0,p1,l1,sky1);
    }
    vec3 result=vec3(0.0);
    // One retained query site. Dynamic bound is strictly one/two sources;
    // do not unroll this into repeated mobile driver ray-query state machines.
    for(uint sourceIndex=0u;sourceIndex<quality;++sourceIndex)
        result+=dustSourceVisibility(p,sourceIndex==0u?p0:p1,sourceIndex==0u?l0:l1,
            sourceIndex==0u?sky0:sky1,distance);
    return result;
}
vec4 dustCandidate(uint index,vec3 origin,vec3 direction,float depth,float pixelAngle,uint quality)
{
    if(index>=64u) return vec4(0.0,0.0,0.0,1.0);
    RtDustMoteGpu mote=rtQualityControls.dustMotes[index];
    vec3 p=mote.positionRadius.xyz;float radius=mote.positionRadius.w;
    if(radius<=0.0||mote.response.x<=0.0) return vec4(0.0,0.0,0.0,1.0);
    float t=dot(p-origin,direction);
    // Camera depth, including first water/pane/mirror interface, is authoritative.
    // No motes are evaluated down transmitted or reflected secondary paths.
    if(t<=0.20||t>=depth) return vec4(0.0,0.0,0.0,1.0);
    float separation=length(origin+direction*t-p);
    if(separation>=radius) return vec4(0.0,0.0,0.0,1.0);
    float footprint=radius/max(t*pixelAngle,0.00001);
    // Fade subpixel specks instead of expanding/brightening them at reduced scale.
    float alpha=mote.response.x*(1.0-smoothstep(radius*0.15,radius,separation))*
        smoothstep(0.25,0.8,footprint)*smoothstep(0.0,radius,depth-t);
    if(alpha<=0.00001) return vec4(0.0,0.0,0.0,1.0);
    return vec4(dustIncidentLight(p,quality)*alpha*0.82,1.0-alpha);
}
void dustSort(inout uint a,inout uint b,vec3 origin,vec3 direction)
{
    float ta=a<64u ? dot(rtQualityControls.dustMotes[a].positionRadius.xyz-origin,direction) : -1.0;
    float tb=b<64u ? dot(rtQualityControls.dustMotes[b].positionRadius.xyz-origin,direction) : -1.0;
    if(ta<tb){uint temp=a;a=b;b=temp;}
}
vec3 composeIndoorDust(vec3 color,vec2 uprightUv,vec3 origin,vec3 direction,float depth,float uprightHeight)
{
    uint quality=(rtQualityControls.value.controls.w>>1u)&3u;
    // Neutral before tile selection, mote loads, evaluation or visibility work.
    if(quality==0u || quality>2u) return color;
    uvec2 tile=min(uvec2(uprightUv*vec2(32.0,18.0)),uvec2(31u,17u));
    uvec4 ids=rtQualityControls.dustTiles[tile.y*32u+tile.x];
    // Fixed four-entry depth sort makes overlapping motes compose far-to-near.
    dustSort(ids.x,ids.y,origin,direction);dustSort(ids.z,ids.w,origin,direction);
    dustSort(ids.x,ids.z,origin,direction);dustSort(ids.y,ids.w,origin,direction);dustSort(ids.y,ids.z,origin,direction);
    float angle=1.48/(1.22*max(uprightHeight,1.0));
    for(uint candidate=0u;candidate<4u;++candidate) {
        vec4 v=dustCandidate(ids[candidate],origin,direction,depth,angle,quality);
        color=color*v.a+v.rgb;
    }
    return color;
}
