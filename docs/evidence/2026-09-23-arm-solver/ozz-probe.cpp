#include "ozz/animation/runtime/ik_two_bone_job.h"
#include "ozz/base/maths/simd_quaternion.h"
#include <cmath>
#include <cstdio>

int main() {
  using namespace ozz::math;
  const auto shoulder = Float4x4::identity();
  const auto elbow = Float4x4::Translation(simd_float4::Load(0, -.3f, 0, 0));
  const auto hand = Float4x4::Translation(simd_float4::Load(0, -.7f, 0, 0));
  ozz::animation::IKTwoBoneJob job;
  job.start_joint = &shoulder; job.mid_joint = &elbow; job.end_joint = &hand;
  job.target = simd_float4::Load(.2f, 0, 0, 0);
  job.pole_vector = simd_float4::Load(0, -1, 0, 0);
  job.mid_axis = simd_float4::Load(0, 0, -1, 0);
  job.soften = 1; job.weight = 1;
  SimdQuaternion upper, lower;
  bool reached = false;
  job.start_joint_correction = &upper; job.mid_joint_correction = &lower;
  job.reached = &reached;
  if (!job.Run()) return 2;
  // Bind local rotations are identity. Apply local corrections, then rebuild
  // the hierarchy, as in the upstream sample (no Horde solver code involved).
  const auto upperModel = Float4x4::FromQuaternion(upper.xyzw);
  const auto lowerModel = upperModel * Float4x4::FromAffine(
    simd_float4::Load(0, -.3f, 0, 0), lower.xyzw, simd_float4::one());
  const auto handModel = lowerModel * Float4x4::Translation(simd_float4::Load(0, -.4f, 0, 0));
  float e[4], h[4];
  StorePtrU(lowerModel.cols[3], e); StorePtrU(handModel.cols[3], h);
  const float u = std::hypot(e[0], e[1], e[2]);
  const float l = std::hypot(h[0]-e[0], h[1]-e[1], h[2]-e[2]);
  std::printf("ozz 744eb9d soften=1 weight=1 reached=%d elbow=(%.9f,%.9f,%.9f) hand=(%.9f,%.9f,%.9f) lengths=(%.9f,%.9f)\n", reached, e[0],e[1],e[2],h[0],h[1],h[2],u,l);
  return reached && std::abs(e[0]+.075f)<2e-5f &&
    std::abs(e[1]+std::sqrt(.09f-.075f*.075f))<2e-5f &&
    std::abs(h[0]-.2f)<2e-5f && std::abs(h[1])<2e-5f &&
    std::abs(e[2])<2e-4f && std::abs(h[2])<2e-5f &&
    std::abs(u-.3f)<2e-5f && std::abs(l-.4f)<2e-5f ? 0 : 1;
}
