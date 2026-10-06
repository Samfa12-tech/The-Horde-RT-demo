// Standalone probe linked to the unchanged repository skin reader.
// Scope: host parser, CPU pose evaluation, named transforms. No GPU/RT claims.
#include "scene/assets/SkinnedMeshAsset.h"
#include "gameplay/items/HeldItemKinematics.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: native_skin_probe model.glb clipName [clipName ...]\n";
        return 2;
    }
    std::cout << std::setprecision(9);
    for (int clipArg = 2; clipArg < argc; ++clipArg) {
        horde::scene::SkinnedClipSet bindings{};
        for (auto& binding : bindings.clips) binding = {"", false, false};
        bindings.clips[0] = {argv[clipArg], false, true};
        horde::scene::SkinnedMeshAsset asset;
        std::string diagnostic;
        if (!asset.LoadClips(argv[1], bindings, diagnostic)) {
            std::cerr << "FAIL LoadClips " << argv[clipArg] << ": " << diagnostic << '\n';
            return 1;
        }
        if (!asset.HasTexcoords() || !asset.HasTangents()) {
            std::cerr << "FAIL missing UV0 or tangents\n";
            return 1;
        }
        const auto clip = horde::scene::SkinnedClip::Idle;
        const float duration = asset.ClipDuration(clip);
        const int samples = std::max(2, static_cast<int>(std::ceil(duration * 60.0f)) + 1);
        std::vector<horde::scene::TexturedSkinnedRtVertex> vertices, firstVertices;
        float minimumPerFrameMin=INFINITY, minimumPerFrameMax=-INFINITY, loopMaxDistance=0;
        float lower[3] = {INFINITY, INFINITY, INFINITY};
        float upper[3] = {-INFINITY, -INFINITY, -INFINITY};
        float rootStart[3]{}, rootEnd[3]{};
        float rootLower[3] = {INFINITY, INFINITY, INFINITY};
        float rootUpper[3] = {-INFINITY, -INFINITY, -INFINITY};
        for (int sample = 0; sample < samples; ++sample) {
            const float t = duration * sample / (samples-1);
            if (!asset.SkinUniqueTextured(clip, t, vertices, diagnostic)) {
                std::cerr << "FAIL Skin " << argv[clipArg] << " at " << t << ": " << diagnostic << '\n';
                return 1;
            }
            if (vertices.empty() || vertices.size() != asset.UniqueVertexCount()) {
                std::cerr << "FAIL inconsistent vertex count\n"; return 1;
            }
            if (sample==0) firstVertices=vertices;
            float frameMin=INFINITY;
            for (std::size_t vi=0;vi<vertices.size();++vi) {
                const auto& v=vertices[vi];
                frameMin=std::min(frameMin,v.position[1]);
                if(sample==samples-1) { float ds=0;for(int axis=0;axis<3;++axis){float d=v.position[axis]-firstVertices[vi].position[axis];ds+=d*d;}loopMaxDistance=std::max(loopMaxDistance,std::sqrt(ds)); }
                for (int axis = 0; axis < 3; ++axis) {
                    if (!std::isfinite(v.position[axis]) || !std::isfinite(v.normal[axis])) {
                        std::cerr << "FAIL nonfinite skinned vertex\n"; return 1;
                    }
                    lower[axis] = std::min(lower[axis], v.position[axis]);
                    upper[axis] = std::max(upper[axis], v.position[axis]);
                }
                if (!std::isfinite(v.texcoord[0]) || !std::isfinite(v.texcoord[1])) {
                    std::cerr << "FAIL nonfinite UV\n"; return 1;
                }
            }
            minimumPerFrameMin=std::min(minimumPerFrameMin,frameMin);minimumPerFrameMax=std::max(minimumPerFrameMax,frameMin);
            for (const char* name : {"Hips", "LeftHand", "RightHand", "LeftGrip", "RightGrip"}) {
                if (!asset.HasNode(name)) continue;
                horde::scene::SkinnedNodeTransform transform{};
                if (!asset.NodeTransform(clip, t, name, transform, diagnostic)) {
                    std::cerr << "FAIL NodeTransform " << name << ": " << diagnostic << '\n'; return 1;
                }
                if (!std::all_of(transform.begin(), transform.end(), [](float x){ return std::isfinite(x); })) {
                    std::cerr << "FAIL nonfinite named transform\n"; return 1;
                }
                if (std::string(name)=="RightGrip" && !horde::gameplay::items::ValidateHeldItemSocketTransform(transform,diagnostic)) {std::cerr<<"FAIL actual rigid socket validator: "<<diagnostic<<"\n";return 1;}
                if (std::string(name) == "Hips") {
                    for (int axis = 0; axis < 3; ++axis) {
                        if (sample == 0) rootStart[axis] = transform[12+axis];
                        if (sample == samples-1) rootEnd[axis] = transform[12+axis];
                        rootLower[axis] = std::min(rootLower[axis], transform[12+axis]);
                        rootUpper[axis] = std::max(rootUpper[axis], transform[12+axis]);
                    }
                }
            }
        }
        std::cout << "PASS host-reader clip=" << argv[clipArg] << " duration=" << duration
                  << " samples=" << samples << " uniqueVertices=" << asset.UniqueVertexCount()
                  << " expandedVertices=" << asset.ExpandedVertexCount()
                  << " primitives=" << asset.PrimitiveRanges().size() << " groundMinRange=" << minimumPerFrameMin << "," << minimumPerFrameMax << " endStartMaxVertexDistance=" << loopMaxDistance << " rightGripRigid=" << asset.HasNode("RightGrip") << " boundsMin=";
        for (float v : lower) std::cout << v << ',';
        std::cout << " boundsMax="; for (float v : upper) std::cout << v << ',';
        if (asset.HasNode("Hips")) {
            std::cout << " hipsEndMinusStart=";
            for (int axis = 0; axis < 3; ++axis) std::cout << rootEnd[axis]-rootStart[axis] << ',';
            std::cout << " hipsRange=";
            for (int axis = 0; axis < 3; ++axis) std::cout << rootUpper[axis]-rootLower[axis] << ',';
        }
        std::cout << '\n';
    }
    std::cout << "NOT RUN: GPU skinning, native RT materials/presentation, gameplay, device performance.\n";
    return 0;
}
