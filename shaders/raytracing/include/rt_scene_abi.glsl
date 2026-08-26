layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 1, rgba8) uniform writeonly image2D outputImage;

struct SkeletonVertex
{
    vec4 position;
    vec4 normal;
};
layout(std430, set = 0, binding = 2) readonly buffer SkeletonVertices
{
    SkeletonVertex vertices[];
} skeleton;
layout(set = 0, binding = 3) uniform sampler2DArray materialDiffuse;
layout(set = 0, binding = 4) uniform sampler2DArray materialNormal;
layout(set = 0, binding = 5) uniform sampler2DArray materialArm;
layout(std430, set = 0, binding = 6) readonly buffer WorldSurfaceCodes
{
    uint codes[];
} worldSurfaces;

struct LichVertex
{
    vec4 position;
    vec4 normal;
    vec4 texcoord;
};
layout(std430, set = 0, binding = 7) readonly buffer LichVertices
{
    LichVertex vertices[];
} lich;
layout(set = 0, binding = 8) uniform sampler2DArray lichBaseColor;
layout(set = 0, binding = 9) uniform sampler2DArray lichEmissive;
layout(std430, set = 0, binding = 10) readonly buffer SecondSkeletonVertices
{
    SkeletonVertex vertices[];
} secondSkeleton;

#include "rt_scene_abi.generated.glsl"

layout(push_constant) uniform SceneControls
{
    float yaw;
    float pitch;
    float torchLight;
    float time;
    float cameraX;
    float cameraZ;
    float walkAmount;
    float outputRedBlueSwap;
    float outputExposure;
    float damageFlash;
    float enemyKind;
    float staffLightStrength;
    float staffX;
    float staffY;
    float staffZ;
    float finaleSkylightOpen;
    float finaleDawnReveal;
    float heldPropDepth;
    float waterQuality;
    float waterfallWidthScale;
    float fogDensityScale;
    float torchHueDegrees;
    float torchIntensityScale;
    float skylightHueDegrees;
    float skylightIntensityScale;
    float passageHueDegrees;
    float passageIntensityScale;
    float staffHueDegrees;
    float staffIntensityScale;
    float workloadPreset;
} controls;

struct HitInfo
{
    bool hit;
    float t;
    int primitive;
    int instance;
    int material;
    vec3 position;
    vec3 normal;
    vec3 geometricNormal;
    vec3 base;
    float metallic;
    float reflectivity;
    float roughness;
    float emissive;
    float transmission;
    float ior;
    float thickness;
    float attenuationDistance;
    vec3 attenuationColor;
    uint materialFlags;
};

const vec3 kMoonDirection = vec3(-0.180027, 0.930140, -0.320048);
const int kMaterialDryStone = 0;
const int kMaterialWetCobble = 1;
const int kMaterialMossyStone = 2;
const int kMaterialDampGround = 3;
const int kMaterialAgedMetal = 4;
const int kMaterialFlame = 5;
const int kMaterialDarkFigure = 6;
const int kMaterialHiddenShell = 7;
const int kMaterialMirror = 8;
const int kMaterialClearGlass = 9;
const int kMaterialWater = 10;
const int kWaterfallInstance = 19;
const int kLightTorch = 0;
const int kLightSkylight = 1;
const int kLightPassage = 2;
const int kLightStaff = 3;
