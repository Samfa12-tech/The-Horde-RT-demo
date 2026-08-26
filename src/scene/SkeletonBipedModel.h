#pragma once

#include "scene/assets/SkinnedMeshAsset.h"

namespace horde::scene
{

// Compatibility name for the audited enemy path. New player and character
// integrations use SkinnedMeshAsset directly so the loader no longer carries
// enemy-specific ownership in its public interface.
using SkeletonBipedModel = SkinnedMeshAsset;

} // namespace horde::scene
