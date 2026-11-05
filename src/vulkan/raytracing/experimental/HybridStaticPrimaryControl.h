#pragma once
#ifndef HORDE_RT_HYBRID_STATIC_PRIMARY
#define HORDE_RT_HYBRID_STATIC_PRIMARY 0
#endif
#include <atomic>
namespace horde::vulkan::raytracing::experimental {
inline std::atomic<bool> gHybridStaticPrimaryRequested{false};
inline std::atomic<bool> gHybridStaticPrimaryApplied{false};
inline void SetHybridStaticPrimaryRequested(bool value) noexcept {gHybridStaticPrimaryRequested.store(value,std::memory_order_relaxed);}
inline bool HybridStaticPrimaryRequested() noexcept {return gHybridStaticPrimaryRequested.load(std::memory_order_relaxed);}
inline void SetHybridStaticPrimaryApplied(bool value) noexcept {gHybridStaticPrimaryApplied.store(value,std::memory_order_relaxed);}
inline bool HybridStaticPrimaryApplied() noexcept {return gHybridStaticPrimaryApplied.load(std::memory_order_relaxed);}
}
