#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <jni.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include "audio/MusicPcmAssetBank.h"
#include "audio/MusicPlaybackSession.h"
#include "platform/android/AndroidMusicPlayback.h"

namespace
{
using namespace horde::audio;
MusicPlaybackInbox inbox;
std::uint64_t lifetimeToken = 1u; // Accessed by gameplay/render owner only.
std::uint64_t resetToken = 0u;
std::atomic<bool> musicPublicationFailed{false};

void FailMusicPublication(const char* reason)
{
    if (!musicPublicationFailed.exchange(true, std::memory_order_acq_rel))
    {
        __android_log_print(ANDROID_LOG_ERROR, "HordeLanternMusic",
                            "Music publication failed; disabling playback: %s", reason);
    }
}

bool ReadAsset(void* context, std::string_view path, std::size_t maximumBytes,
               std::vector<std::uint8_t>& output)
{
    const std::string name(path);
    std::unique_ptr<AAsset, decltype(&AAsset_close)> asset(
        AAssetManager_open(static_cast<AAssetManager*>(context), name.c_str(), AASSET_MODE_STREAMING),
        &AAsset_close);
    if (!asset) return false;
    const auto bytes = AAsset_getLength64(asset.get());
    if (bytes < 0 || static_cast<std::uint64_t>(bytes) > maximumBytes) return false;
    output.resize(static_cast<std::size_t>(bytes));
    std::size_t offset = 0u;
    while (offset < output.size())
    {
        const int count = AAsset_read(asset.get(), output.data() + offset, output.size() - offset);
        if (count <= 0) return false;
        offset += static_cast<std::size_t>(count);
    }
    return true;
}

// All fields, including the Core cursor, are confined to the Java audio worker.
// Java retains the AssetManager until this object is destroyed on that worker.
struct NativeMusic
{
    MusicPcmAssetBank bank;
    std::unique_ptr<MusicPlaybackSession> session;
    MusicPlaybackInput input;
};
NativeMusic* Get(jlong handle) { return reinterpret_cast<NativeMusic*>(static_cast<std::uintptr_t>(handle)); }
} // namespace

namespace horde::platform::android
{
void PublishMusicSnapshot(const gameplay::simulation::SimulationSnapshot& snapshot,
                          const std::span<const gameplay::simulation::GameplayEvent> events,
                          const bool externallySuspended)
{
    if (musicPublicationFailed.load(std::memory_order_acquire)) return;
    try
    {
        if (!inbox.Publish(snapshot, events, lifetimeToken, resetToken, externallySuspended))
            FailMusicPublication("stale snapshot or inbox failure");
    }
    catch (...)
    {
        // A mutex/system failure is a music-side failure, never a render-thread
        // exception. The worker observes the latch in nativePoll and exits.
        FailMusicPublication("inbox publication threw");
    }
}
void ResetMusicSession(const bool newEventQueue)
{
    ++resetToken;
    if (newEventQueue) ++lifetimeToken;
}
} // namespace horde::platform::android

extern "C" JNIEXPORT jlong JNICALL
Java_com_samfa12_hordelanternrt_HordeMusicPlayback_nativeCreate(JNIEnv* env, jclass, jobject assets)
{
    try
    {
        auto music = std::make_unique<NativeMusic>();
        auto* manager = AAssetManager_fromJava(env, assets);
        if (manager == nullptr) return 0;
        const auto loaded = music->bank.Load(ReadAsset, manager);
        if (loaded.status != MusicPcmBankStatus::Loaded)
        {
            __android_log_print(ANDROID_LOG_ERROR, "HordeLanternMusic", "Music bank load failed status=%u decoder=%u asset=%.*s",
                static_cast<unsigned>(loaded.status), static_cast<unsigned>(loaded.decoderStatus),
                static_cast<int>(loaded.assetPath.size()), loaded.assetPath.data());
            return 0;
        }
        music->session = std::make_unique<MusicPlaybackSession>(music->bank.Clips());
        if (!music->session->IsValid()) return 0;
        return static_cast<jlong>(reinterpret_cast<std::uintptr_t>(music.release()));
    }
    catch (...) { __android_log_print(ANDROID_LOG_ERROR, "HordeLanternMusic", "Native music initialization failed"); return 0; }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_HordeMusicPlayback_nativePoll(
    JNIEnv* env, jclass, jlong handle, jboolean suspended, jlongArray control)
{
    try
    {
        if (musicPublicationFailed.load(std::memory_order_acquire)) return JNI_FALSE;
        auto* music = Get(handle);
        if (!music || !music->session || !control || env->GetArrayLength(control) != 4)
            return JNI_FALSE;
        music->input = inbox.Take(); // Mutex is outside Core Render and JNI array access.
        music->input.externallySuspended = music->input.externallySuspended || suspended == JNI_TRUE;
        if (music->input.available)
        {
            const auto status = music->session->Observe(music->input);
            if (status != MusicPcmStatus::Ok && status != MusicPcmStatus::Suspended) return JNI_FALSE;
            music->input.eventCount = 0u; // Newly copied edges already observed exactly once.
        }
        const std::array<jlong, 4u> state{
            music->input.available ? 1 : 0,
            static_cast<jlong>(music->input.restartEpoch),
            (music->input.externallySuspended || music->input.snapshot.paused) ? 1 : 0,
            static_cast<jlong>(music->session->GeneratedFrames())};
        env->SetLongArrayRegion(control, 0, static_cast<jsize>(state.size()), state.data());
        return env->ExceptionCheck() ? JNI_FALSE : JNI_TRUE;
    }
    catch (...)
    {
        __android_log_print(ANDROID_LOG_ERROR, "HordeLanternMusic",
                            "Music poll failed; audio worker will disable playback");
        return JNI_FALSE;
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samfa12_hordelanternrt_HordeMusicPlayback_nativeRender(
    JNIEnv* env, jclass, jlong handle, jfloatArray output)
{
    auto* music = Get(handle);
    if (!music || !output || env->GetArrayLength(output) != 960) return JNI_FALSE;
    jfloat* samples = env->GetFloatArrayElements(output, nullptr);
    if (!samples) return JNI_FALSE;
    const auto status = music->session->Render(music->input, std::span(samples, 960u));
    env->ReleaseFloatArrayElements(output, samples, 0);
    return status == MusicPcmStatus::Ok && !env->ExceptionCheck() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samfa12_hordelanternrt_HordeMusicPlayback_nativeDestroy(JNIEnv*, jclass, jlong handle)
{
    delete Get(handle); // Worker has stopped device output; session dies before bank.
}
