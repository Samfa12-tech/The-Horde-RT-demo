#pragma once
#include <string>
// Finite offscreen comparison, not the shipping application/presentation path.
std::string RunHordeHybridLab(const std::string& output, int width, int height,
                             int framesPerBlock = 24, bool validation = false,
                             bool forceRayQueryBaseline = false);
