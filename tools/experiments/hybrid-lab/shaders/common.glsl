#extension GL_EXT_ray_query : require
layout(set=0,binding=0) uniform accelerationStructureEXT scene;
layout(set=0,binding=1,rgba8) uniform image2D target;
struct Vertex { vec4 p; vec4 n; vec4 material; };
layout(set=0,binding=2,std430) readonly buffer Mesh { Vertex v[]; } mesh;
layout(push_constant) uniform Controls {
    vec4 camera;
    vec4 forward;
    vec4 right;
    vec4 up;
    vec4 config; // width,height,light phase,reflections enabled
} pc;
const vec3 sky=vec3(0.012,0.018,0.030);
vec3 rayDirection(vec2 pixel) {
    vec2 s=2.0*pixel/pc.config.xy-1.0;
    return normalize(pc.forward.xyz+pc.right.xyz*s.x*(pc.config.x/pc.config.y)*0.72-pc.up.xyz*s.y*0.72);
}
bool hit(vec3 o,vec3 d,float distance,out uint triangle,out vec2 bary,out float t,float minDistance) {
    rayQueryEXT q;
    rayQueryInitializeEXT(q,scene,gl_RayFlagsOpaqueEXT,255,o,minDistance,d,distance);
    while(rayQueryProceedEXT(q)) {}
    if(rayQueryGetIntersectionTypeEXT(q,true)==gl_RayQueryCommittedIntersectionNoneEXT) return false;
    triangle=rayQueryGetIntersectionPrimitiveIndexEXT(q,true);
    bary=rayQueryGetIntersectionBarycentricsEXT(q,true);
    t=rayQueryGetIntersectionTEXT(q,true);
    return true;
}
float visibility(vec3 p,vec3 n,vec3 d,float distance) {
    rayQueryEXT q;
    rayQueryInitializeEXT(q,scene,gl_RayFlagsOpaqueEXT|gl_RayFlagsTerminateOnFirstHitEXT,
        255,p+n*0.002,0.001,d,max(0.002,distance-0.004));
    while(rayQueryProceedEXT(q)) {}
    return rayQueryGetIntersectionTypeEXT(q,true)==gl_RayQueryCommittedIntersectionNoneEXT?1.0:0.0;
}
void surface(uint tri,vec2 bary,out vec3 p,out vec3 n,out vec4 m) {
    uint i=tri*3;
    vec3 b=vec3(1-bary.x-bary.y,bary.x,bary.y);
    p=mesh.v[i].p.xyz*b.x+mesh.v[i+1].p.xyz*b.y+mesh.v[i+2].p.xyz*b.z;
    n=normalize(mesh.v[i].n.xyz*b.x+mesh.v[i+1].n.xyz*b.y+mesh.v[i+2].n.xyz*b.z);
    m=mesh.v[i].material;
}
vec3 directLight(vec3 p,vec3 n,vec3 view,vec4 m) {
    if(m.w>1.5) return m.rgb*3.0;
    vec3 albedo=m.rgb;
    // Shared procedural world-space masonry, not the production PBR textures.
    if(m.w<0.5) {
        vec2 uv=abs(n.y)>0.7?p.xz:p.zy;
        uv*=vec2(3.0,5.5);
        uv.x+=mod(floor(uv.y),2.0)*0.5;
        vec2 f=fract(uv);
        float grout=float(f.x<0.025||f.x>0.975||f.y<0.045||f.y>0.955);
        float grain=0.94+0.06*sin(dot(p,vec3(93.1,67.4,41.7)));
        albedo*=mix(grain,0.35,grout);
    }
    vec3 result=albedo*vec3(0.010,0.014,0.022); // explicit identical ambient approximation
    vec3 lights[3]=vec3[](vec3(-1.48,0.45,0.65),vec3(1.48,0.36,-2.0),
        vec3(0.45+0.35*sin(pc.config.z),0.30,-4.4));
    vec3 radiance[3]=vec3[](vec3(9.0,4.4,1.4),vec3(3.0,5.2,9.0),vec3(7.0,2.8,0.7));
    for(int i=0;i<3;i++) {
        vec3 delta=lights[i]-p;
        float d=length(delta);
        vec3 l=delta/d;
        float nl=max(dot(n,l),0.0);
        if(nl<=0.0) continue;
        vec3 h=normalize(l+view);
        float spec=pow(max(dot(n,h),0.0),m.w>0.5?100.0:24.0)*(m.w>0.5?0.18:0.035);
        result+=(albedo*nl/3.14159265+spec)*radiance[i]/(0.7+d*d)*visibility(p,n,l,d);
    }
    return result;
}
vec3 shade(vec3 p,vec3 n,vec4 m,vec3 d) {
    if(dot(n,d)>0.0)n=-n;
    vec3 c=directLight(p,n,-d,m);
    if(m.w>0.5&&m.w<1.5&&pc.config.w>0.5) {
        vec3 rd=reflect(d,n),rp,rn;
        vec4 rm;
        uint tri;vec2 b;float t;
        vec3 reflection=sky;
        if(hit(p+n*0.004,rd,30.0,tri,b,t,0.002)) {
            surface(tri,b,rp,rn,rm);
            if(dot(rn,rd)>0.0)rn=-rn;
            reflection=directLight(rp,rn,-rd,rm);
        }
        float fresnel=0.60+0.40*pow(1-max(dot(n,-d),0.0),5.0);
        c=mix(c,reflection,fresnel);
    }
    return c;
}
vec4 display(vec3 c) {
    c*=1.25;
    c=clamp((c*(2.51*c+0.03))/(c*(2.43*c+0.59)+0.14),0.0,1.0);
    c=mix(c*12.92,1.055*pow(c,vec3(1.0/2.4))-0.055,step(vec3(0.0031308),c));
    return vec4(c,1);
}
