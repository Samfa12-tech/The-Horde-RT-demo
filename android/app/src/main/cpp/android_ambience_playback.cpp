#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <jni.h>
#include <array>
#include <cstdint>
#include <memory>
#include <vector>
#include "audio/AmbiencePcmLoop.h"

namespace
{
struct NativeAmbience { horde::audio::AmbiencePcmLoop loop; jlong epoch = -1; };
NativeAmbience* Get(jlong handle) { return reinterpret_cast<NativeAmbience*>(static_cast<std::uintptr_t>(handle)); }
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samfa12_hordelanternrt_HordeAmbiencePlayback_nativeCreate(JNIEnv* env, jclass, jobject assets)
{
    try
    {
        auto* manager = AAssetManager_fromJava(env, assets);
        if (!manager) return 0;
        std::unique_ptr<AAsset, decltype(&AAsset_close)> asset(
            AAssetManager_open(manager, horde::audio::kWaterfallCoreAsset, AASSET_MODE_STREAMING), &AAsset_close);
        if (!asset) return 0;
        const auto length = AAsset_getLength64(asset.get());
        if (length <= 0 || static_cast<std::uint64_t>(length) > horde::audio::kWaterfallCoreMaximumFileBytes) return 0;
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
        std::size_t offset = 0;
        while (offset < bytes.size())
        {
            const int count = AAsset_read(asset.get(), bytes.data() + offset, bytes.size() - offset);
            if (count <= 0) return 0;
            offset += static_cast<std::size_t>(count);
        }
        auto ambience = std::make_unique<NativeAmbience>();
        const auto status = ambience->loop.Load(bytes);
        if (status != pocket_audio::PcmWaveStatus::Ok) return 0;
        __android_log_print(ANDROID_LOG_INFO, "HordeAmbiencePCM",
            "Canonical Core waterfall ready: frames=%u decodedBytes=%zu, no per-wrap decode/restart",
            horde::audio::kWaterfallCoreFrames, ambience->loop.DecodedBytes());
        return static_cast<jlong>(reinterpret_cast<std::uintptr_t>(ambience.release()));
    }
    catch (...) { return 0; }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_HordeAmbiencePlayback_nativeControl(
    JNIEnv* env, jclass, jlong handle, jboolean suspended, jlong epoch, jlongArray control)
{
    auto* ambience = Get(handle);
    if (!ambience || epoch < 0 || !control || env->GetArrayLength(control) != 2) return JNI_FALSE;
    if (epoch < ambience->epoch) return JNI_FALSE;
    const auto status = epoch != ambience->epoch
        ? ambience->loop.Reset(suspended == JNI_TRUE)
        : ambience->loop.SetSuspended(suspended == JNI_TRUE);
    if (status != pocket_audio::PcmStatus::Ok && status != pocket_audio::PcmStatus::Suspended) return JNI_FALSE;
    ambience->epoch = epoch;
    const std::array<jlong, 2u> state{epoch, static_cast<jlong>(ambience->loop.GeneratedFrames())};
    env->SetLongArrayRegion(control, 0, 2, state.data());
    return env->ExceptionCheck() ? JNI_FALSE : JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_HordeAmbiencePlayback_nativeRender(JNIEnv* env, jclass, jlong handle, jfloatArray output)
{
    auto* ambience = Get(handle);
    if (!ambience || !output || env->GetArrayLength(output) != horde::audio::kAmbienceChunkFrames * 2u) return JNI_FALSE;
    // Fixed stack transfer avoids a JVM-pinned/possibly allocated array copy.
    std::array<float, horde::audio::kAmbienceChunkFrames * 2u> samples{};
    const auto status = ambience->loop.Render(samples);
    if (status == pocket_audio::PcmStatus::Ok)
        env->SetFloatArrayRegion(output, 0, static_cast<jsize>(samples.size()), samples.data());
    return status == pocket_audio::PcmStatus::Ok && !env->ExceptionCheck() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_HordeAmbiencePlayback_nativeDestroy(JNIEnv*, jclass, jlong handle)
{
    delete Get(handle); // Worker has released device output before storage.
}
