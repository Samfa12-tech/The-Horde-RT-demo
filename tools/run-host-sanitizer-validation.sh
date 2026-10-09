#!/usr/bin/env bash
# Selected CPU contracts only; never hardware RT or playback acceptance.
set -euo pipefail

hordeBuild=build/host-sanitizer-ci
hordeFlags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer'
hordeTargets=(
  horde_rt_retirement_owner_tests
  horde_rt_pipeline_cache_seed_tests
  pocket_audio_native_pcm_tests
  horde_rt_simulation_timing_tests
  horde_rt_simulation_gameplay_tests
  horde_rt_player_animation_tests
  horde_rt_held_item_socket_tests
  horde_rt_dielectric_math_tests
  horde_rt_music_director_tests
  horde_rt_music_pcm_stream_tests
  horde_rt_music_pcm_wave_tests
  horde_rt_music_pcm_asset_bank_tests
  horde_rt_music_playback_session_tests
  horde_rt_surface_session_mailbox_tests
  horde_rt_input_mailbox_stress
  horde_rt_playtest_report_tests
  horde_rt_playtest_submission_tests
)

cmake -S . -B "$hordeBuild" \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DHORDE_RT_BUILD_VULKAN_TARGETS=OFF \
  "-DCMAKE_C_FLAGS=$hordeFlags" "-DCMAKE_CXX_FLAGS=$hordeFlags" \
  '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined'
cmake --build "$hordeBuild" --parallel 2 --target "${hordeTargets[@]}"

export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1
export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
# Run each required registration explicitly: a missing test is a failure, not
# a silently smaller passing subset. A diagnostic exits nonzero without retry.
for hordeTarget in "${hordeTargets[@]}"; do
  ctest --test-dir "$hordeBuild" --output-on-failure --no-tests=error \
    -R "^${hordeTarget}$"
done
