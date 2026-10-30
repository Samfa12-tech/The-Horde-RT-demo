// DIAGNOSTIC ONLY. An extra hardware BVH over conservative point-light
// shadow-cone AABBs. These descriptors are not part of the production ABI.
layout(set=0,binding=28) uniform accelerationStructureEXT mistConeAS;
struct MistCone { vec4 planes[4]; uvec4 identity; };
layout(std430,set=0,binding=29) readonly buffer MistConeData { MistCone cones[]; };
const int kMistIntervalCapacity=64;
const int kMistCandidateCapacity=512;
vec2 mistIntervals[kMistIntervalCapacity];
int mistIntervalCount=0;
bool mistIntervalInvalid=false;
bool mistSkyExcludeSelected=false;
vec3 mistCameraOrigin, mistCameraDirection;
float mistCellLength=0.0;

void discoverMistIntervals(vec3 origin,vec3 direction,float first,float last,int source)
{
    mistIntervalCount=0;
    mistIntervalInvalid=false;
    rayQueryEXT query;
    rayQueryInitializeEXT(query,mistConeAS,0u,255u,origin,first,direction,last);
    int candidates=0;
    // Never confirm a broadphase candidate: confirmation would prune cones
    // whose intervals are also needed. Vulkan owns the entire BVH traversal.
    while(rayQueryProceedEXT(query))
    {
        if(++candidates>kMistCandidateCapacity) {mistIntervalInvalid=true;break;}
        if(rayQueryGetIntersectionTypeEXT(query,false)!=gl_RayQueryCandidateIntersectionAABBEXT)
            continue;
        uint primitive=uint(rayQueryGetIntersectionPrimitiveIndexEXT(query,false));
        MistCone cone=cones[primitive];
        if(cone.identity.x!=uint(source)) continue;
        vec2 interval=vec2(first,last);
        bool empty=false;
        for(int plane=0;plane<4;++plane)
        {
            vec4 p=cone.planes[plane];
            float intercept=dot(p.xyz,origin)+p.w;
            float slope=dot(p.xyz,direction);
            if(abs(slope)<1e-8) {
                if(intercept<0.0) empty=true;
            } else if(slope>0.0) interval.x=max(interval.x,-intercept/slope);
            else interval.y=min(interval.y,-intercept/slope);
        }
        if(empty || interval.y<=interval.x) continue;
        if(mistIntervalCount==kMistIntervalCapacity) {mistIntervalInvalid=true;break;}
        mistIntervals[mistIntervalCount++]=interval;
    }
    // Bounded insertion sort followed by union. Overlapping triangle cones
    // contribute once; disconnected shadows retain their separate intervals.
    for(int i=1;i<mistIntervalCount;++i) {
        vec2 v=mistIntervals[i];int j=i-1;
        while(j>=0 && mistIntervals[j].x>v.x) {
            mistIntervals[j+1]=mistIntervals[j];--j;
        }
        mistIntervals[j+1]=v;
    }
    int count=0;
    for(int i=0;i<mistIntervalCount;++i) {
        vec2 v=mistIntervals[i];
        if(count>0 && v.x<=mistIntervals[count-1].y)
            mistIntervals[count-1].y=max(mistIntervals[count-1].y,v.y);
        else mistIntervals[count++]=v;
    }
    mistIntervalCount=count;
}

float mistIntervalCoverage(vec3 samplePosition)
{
    if(!(mistCellLength>0.0) || isnan(mistCellLength) || isinf(mistCellLength)) {
        mistIntervalInvalid=true;return 0.0;
    }
    float centre=dot(samplePosition-mistCameraOrigin,mistCameraDirection);
    vec2 cell=vec2(centre-mistCellLength*.5,centre+mistCellLength*.5);
    float covered=0.0;
    for(int i=0;i<mistIntervalCount;++i)
        covered+=max(0.0,min(cell.y,mistIntervals[i].y)-max(cell.x,mistIntervals[i].x));
    return clamp(covered/mistCellLength,0.0,1.0);
}
