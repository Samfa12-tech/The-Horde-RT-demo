package com.samfa12.hordelanternrt;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ClipData;
import android.content.BroadcastReceiver;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.media.AudioAttributes;
import android.media.MediaPlayer;
import android.media.SoundPool;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.text.method.LinkMovementMethod;
import android.text.util.Linkify;
import android.util.Log;
import android.util.TypedValue;
import android.view.HapticFeedbackConstants;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import javax.net.ssl.HttpsURLConnection;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Locale;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.json.JSONObject;

public class MainActivity extends Activity {
    private static final String TAG = "HordeLanternAudio";
    private static final String PREFS = "horde_lantern_alpha_settings";
    private static final String PREF_RT_LAB_UNLOCKED = "rt_lab_unlocked";
    private static final String REPORT_DIRECTORY = "reports";
    private static final String TEXT_REPORT_FILE = "vulkan_capability_report.txt";
    private static final String JSON_REPORT_FILE = "vulkan_capability_report.json";
    private static final String GITHUB_RELEASE_PAGE_PREFIX =
            "https://github.com/Samfa12-tech/The-Horde-RT-demo/releases/tag/";
    private static final String SKELETON_ASSET = "models/enemies/meshy/skeleton_biped_merged_animations_v01.glb";
    private static final String SKELETON_FILE = "skeleton_biped_merged_animations_v01.glb";
    private static final String LICH_ASSET = "models/enemies/meshy/lich_placeholder_merged_animations_v01.glb";
    private static final String LICH_FILE = "lich_placeholder_merged_animations_v01.glb";
    private static final String EXTRA_DEBUG_CHECKPOINT = "horde.debug.checkpoint";
    private static final String EXTRA_DEBUG_CAPTURE = "horde.debug.capture";
    private static final String EXTRA_DEBUG_REPLAY = "horde.debug.replay";
    private static final String EXTRA_DEBUG_SCALE = "horde.debug.scale";
    private static final String EXTRA_DEBUG_AUTOSTART = "horde.debug.autostart";
    private static final String EXTRA_DEBUG_OVERLAY = "horde.debug.overlay";
    private static final String EXTRA_DEBUG_GPU_TIMING = "horde.debug.gpu_timing";
    private static final String EXTRA_DEBUG_RT_LAB = "horde.debug.rt_lab";
    private static final String EXTRA_DEBUG_RT_WATERFALL = "horde.debug.rt_waterfall_width";
    private static final String EXTRA_DEBUG_RT_ROOF = "horde.debug.rt_roof_open";
    private static final String EXTRA_DEBUG_RT_DAWN = "horde.debug.rt_dawn_reveal";
    private static final String EXTRA_DEBUG_RT_FOG = "horde.debug.rt_fog_density";
    private static final String EXTRA_DEBUG_RT_LIGHT_GROUP = "horde.debug.rt_light_group";
    private static final String EXTRA_DEBUG_RT_LIGHT_HUE = "horde.debug.rt_light_hue";
    private static final String EXTRA_DEBUG_RT_LIGHT_INTENSITY = "horde.debug.rt_light_intensity";
    private static final String EXTRA_DEBUG_RT_FIRE_STRENGTH = "horde.debug.rt_fire_strength";
    private static final String EXTRA_DEBUG_RT_FIRE_TURBULENCE = "horde.debug.rt_fire_turbulence";
    private static final String EXTRA_DEBUG_RT_FIRE_SMOKE = "horde.debug.rt_fire_smoke";
    private static final String EXTRA_DEBUG_RT_WORKLOAD = "horde.debug.rt_workload";
    private static final String DEBUG_RETRY_ACTION =
            "com.samfa12.hordelanternrt.DEBUG_RETRY_ENCOUNTER";
    private static final int REQUEST_SAVE_BENCHMARK = 7101;
    private static final int PLATFORM_EVENT_PLAYER_FOOTSTEP = 0;
    private static final int PLATFORM_EVENT_PLAYER_SWING = 1;
    private static final int PLATFORM_EVENT_PLAYER_DAMAGED = 2;
    private static final int PLATFORM_EVENT_PLAYER_KILLED = 3;
    private static final int PLATFORM_EVENT_ENEMY_FOOTSTEP = 4;
    private static final int PLATFORM_EVENT_ENEMY_ATTACK_STARTED = 5;
    private static final int PLATFORM_EVENT_ENEMY_HIT = 6;
    private static final int PLATFORM_EVENT_ENEMY_DEFEATED = 7;
    private static final int PLATFORM_EVENT_LICH_CHARGE_STARTED = 8;
    private static final int PLATFORM_EVENT_LICH_IMPACT = 9;
    private static final int PLATFORM_EVENT_LICH_DEFEATED = 10;
    private static final int PLATFORM_EVENT_PLAYER_PARRY_SUCCEEDED = 12;
    private static final int PLATFORM_EVENT_CHEST_UNLOCKED = 13;
    private static final int PLATFORM_EVENT_CHEST_OPENED = 14;
    private static final int PLATFORM_EVENT_TORCH_EXTINGUISHED = 16;
    private static final int ENTITY_LICH = 3;
    private static final int PLAYER_ALIVE = 0;
    private static final int PLAYER_DYING = 1;
    private static final int PLAYER_DEAD = 2;
    private static final int FINALE_ENDING_COMPLETE = 4;
    private static final int WATER_QUALITY_OFF = 0;
    private static final int WATER_QUALITY_MOBILE = 1;
    private static final int WATER_QUALITY_HIGH = 2;
    private static final int HAPTIC_SWING = 0;
    private static final int HAPTIC_DAMAGE = 1;
    private static final int HAPTIC_FATAL = 2;
    private static final int HAPTIC_PARRY = 3;
    private static final long ENEMY_IMPACT_FALL_DELAY_MILLISECONDS = 140L;
    private static final int CONTEXTUAL_INTERACT = 1;
    private static final int CONTEXTUAL_RAISE = 2;
    private static final int CONTEXTUAL_LOWER = 4;
    private static final int CHEST_PROMPT_SHIFT = 3;
    private static final int CHEST_PROMPT_MASK = 7 << CHEST_PROMPT_SHIFT;
    private static final int CHEST_PROMPT_NONE = 0;
    private static final int CHEST_PROMPT_LOCKED = 1;
    private static final int CHEST_PROMPT_OPEN = 2;
    private static final int CHEST_PROMPT_OPENING = 3;
    private static final int CHEST_PROMPT_CLAIM = 4;

    private final Handler handler = new Handler(Looper.getMainLooper());
    private final ExecutorService updateExecutor = Executors.newSingleThreadExecutor();
    private final float[] viewControls = {0.0f, 0.0f, 1.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    private final int[] activePointers = {-1, -1};
    private final Map<String, Integer> sounds = new HashMap<>();
    private final Set<Integer> loadedSounds = new HashSet<>();

    private SharedPreferences preferences;
    private SurfaceView surfaceView;
    private Surface currentSurface;
    private FrameLayout menuScrim;
    private ScrollView diagnosticsPanel;
    private TextView reportTextView;
    private TextView rtStatus;
    private TextView developerOverlay;
    private Button menuButton;
    private TextView vitalityStatus;
    private Button attackButton;
    private Button parryButton;
    private Button interactButton;
    private Button toggleHeldLightPoseButton;
    private boolean parryRequestedOnTouchDown;
    private SoundPool soundPool;
    private MediaPlayer waterfallPlayer;
    private Vibrator vibrator;
    private String reportText = "";
    private boolean resumed;
    private boolean surfaceAvailable;
    private boolean surfaceStarted;
    private boolean menuVisible = true;
    private boolean diagnosticsVisible;
    private boolean diagnosticsErrorState;
    private boolean autoDiagnosticsShown;
    private boolean firstMenu = true;
    private int swingVariant;
    private int playerStepVariant;
    private int enemyStepVariant;
    private int diagnosticsRefreshTick;
    private int pendingDebugCheckpoint = -1;
    private boolean pendingDebugCapture;
    private boolean pendingDebugReplay;
    private boolean debugAutomationAutostart;
    private boolean developerOverlayVisible;
    private boolean debugCaptureUiSuppressed;
    private boolean benchmarkRunning;
    private boolean benchmarkReportVisible;
    private String latestBenchmarkReport = "";
    private BroadcastReceiver debugRetryReceiver;
    private boolean deathOverlayVisible;
    private boolean endingOverlayVisible;
    private boolean endingOverlayDismissed;
    private boolean rtLabUnlocked;
    private boolean rtLabNewlyUnlocked;
    private boolean debugRtLabAccess;
    private boolean rtLabVisible;
    private boolean rtLabReturnToEnding;
    private TextView rtLabTelemetry;
    private int rtWaterfallWidthPercent = 100;
    private boolean rtRoofOverrideEnabled;
    private int rtRoofOpenPercent;
    private boolean rtDawnOverrideEnabled;
    private int rtDawnRevealPercent;
    private int rtFogDensityPercent = 100;
    private int rtFireStrengthPercent = 100;
    private int rtFireTurbulencePercent = 100;
    private int rtFireSmokePercent = 100;
    private int rtGlassVisibilityPercent;
    private int rtGlassTransmissionPercent = 94;
    private int rtGlassIorHundredths = 152;
    private int rtGlassRoughnessPercent = 12;
    private int rtLightGroup;
    private final int[] rtLightHueDegrees = {0, 0, 0, 0};
    private final int[] rtLightIntensityPercent = {100, 100, 100, 100};
    private int rtWorkloadPreset = 1;
    private boolean retryPending;
    private boolean updateCheckInFlight;
    private boolean updatePromptShown;
    private boolean startupUpdateCheckScheduled;
    private boolean startupUpdateCheckCompleted;
    private String pendingUpdateDecision;
    private boolean pendingUpdateManualRequest;
    private int lastPlayerLifePhase = PLAYER_ALIVE;
    private int lastPlayerVitality = 3;
    private long delayedGameplayFeedbackGeneration;
    private final Runnable applyPendingRenderScale = () ->
            ProbeBridge.setRenderScale(preferences.getInt("render_scale", 100) / 100.0f);
    private final Runnable runStartupUpdateCheck = () -> {
        startupUpdateCheckScheduled = false;
        if (!resumed || startupUpdateCheckCompleted) return;
        startupUpdateCheckCompleted = true;
        checkForUpdates(false);
    };
    private final Runnable refreshRtLabTelemetry = new Runnable() {
        @Override public void run() {
            if (!rtLabVisible || rtLabTelemetry == null) return;
            final float gpuMs = ProbeBridge.getRtGpuFrameTimeMilliseconds();
            final long samples = ProbeBridge.getRtGpuSampleCount();
            final int renderScale = ProbeBridge.getCurrentRenderScalePercent();
            final int waterQuality = ProbeBridge.getCurrentWaterQuality();
            final String waterName = waterQuality == WATER_QUALITY_HIGH ? getString(R.string.water_quality_high) :
                    (waterQuality == WATER_QUALITY_MOBILE ? getString(R.string.water_quality_mobile) :
                            getString(R.string.water_quality_off));
            rtLabTelemetry.setText(samples > 0
                    ? getString(R.string.rt_lab_telemetry,
                            String.format(Locale.US, "%.2f", gpuMs), samples, renderScale, waterName)
                    : getString(R.string.rt_lab_telemetry_warming, renderScale, waterName));
            handler.postDelayed(this, 250L);
        }
    };

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.activity_main);
        preferences = getSharedPreferences(PREFS, MODE_PRIVATE);
        rtLabUnlocked = preferences.getBoolean(PREF_RT_LAB_UNLOCKED, false);
        vibrator = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
        ProbeBridge.resetRtSceneTuning();
        ProbeBridge.setRenderScale(preferences.getInt("render_scale", 100) / 100.0f);
        ProbeBridge.setWaterQuality(preferences.getInt("water_quality", WATER_QUALITY_MOBILE));
        consumeDebugAutomationIntent(getIntent());

        surfaceView = findViewById(R.id.scene_surface);
        surfaceView.setHapticFeedbackEnabled(true);
        menuScrim = findViewById(R.id.menu_scrim);
        diagnosticsPanel = findViewById(R.id.diagnostics_panel);
        reportTextView = findViewById(R.id.report_text);
        rtStatus = findViewById(R.id.rt_status);
        developerOverlay = findViewById(R.id.developer_overlay);
        menuButton = findViewById(R.id.menu_button);
        attackButton = findViewById(R.id.attack_button);
        parryButton = findViewById(R.id.parry_button);
        interactButton = findViewById(R.id.interact_button);
        toggleHeldLightPoseButton = findViewById(R.id.toggle_held_light_pose_button);
        vitalityStatus = findViewById(R.id.vitality_status);
        final Button diagnosticsBack = findViewById(R.id.diagnostics_back);

        styleActionButton(menuButton, 0xCC1A1713, 0xFFFFD28A);
        styleActionButton(attackButton, 0xDD5B210D, 0xFFFFE0A3);
        styleActionButton(parryButton, 0xDD263B42, 0xFFE5F7FF);
        styleActionButton(interactButton, 0xDD5C4216, 0xFFFFE5A8);
        styleActionButton(toggleHeldLightPoseButton, 0xDD173E34, 0xFFE2FFF0);
        styleActionButton(diagnosticsBack, 0xCC211B15, 0xFFFFD28A);
        menuButton.setContentDescription(getString(R.string.menu));
        attackButton.setContentDescription(getString(R.string.swing));
        parryButton.setContentDescription(getString(R.string.parry));
        interactButton.setContentDescription(getString(R.string.interact));
        toggleHeldLightPoseButton.setContentDescription(getString(R.string.lower_lantern));
        updateVitalityHud(3);
        if (isDebuggableApp()) {
            rtStatus.setOnLongClickListener(view -> {
                developerOverlayVisible = !developerOverlayVisible;
                if (!developerOverlayVisible) developerOverlay.setVisibility(View.GONE);
                return true;
            });
            registerDebugRetryReceiver();
        }

        initialiseAudio();
        menuButton.setOnClickListener(view -> {
            playSound("menu_toggle", 0.20f);
            showMainMenu(false);
        });
        attackButton.setOnClickListener(view -> {
            if (menuVisible || diagnosticsVisible || deathOverlayVisible || ProbeBridge.getRuntimeState() != 1) return;
            ProbeBridge.requestAttack();
        });
        interactButton.setOnClickListener(view -> {
            if (menuVisible || diagnosticsVisible || deathOverlayVisible ||
                    endingOverlayVisible || ProbeBridge.getRuntimeState() != 1) return;
            ProbeBridge.requestInteract();
        });
        toggleHeldLightPoseButton.setOnClickListener(view -> {
            if (menuVisible || diagnosticsVisible || deathOverlayVisible ||
                    endingOverlayVisible || ProbeBridge.getRuntimeState() != 1) return;
            ProbeBridge.requestToggleHeldLightPose();
        });
        parryButton.setOnClickListener(view -> {
            if (parryRequestedOnTouchDown) {
                parryRequestedOnTouchDown = false;
                return;
            }
            if (menuVisible || diagnosticsVisible || deathOverlayVisible || ProbeBridge.getRuntimeState() != 1) return;
            ProbeBridge.requestParry();
        });
        parryButton.setOnTouchListener((view, event) -> {
            switch (event.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    parryRequestedOnTouchDown = true;
                    view.setPressed(true);
                    if (!menuVisible && !diagnosticsVisible && !deathOverlayVisible && ProbeBridge.getRuntimeState() == 1) {
                        ProbeBridge.requestParry();
                    }
                    return true;
                case MotionEvent.ACTION_UP:
                    view.setPressed(false);
                    view.performClick();
                    return true;
                case MotionEvent.ACTION_CANCEL:
                    parryRequestedOnTouchDown = false;
                    view.setPressed(false);
                    return true;
                default:
                    return true;
            }
        });
        diagnosticsBack.setOnClickListener(view -> {
            playSound("ui_back", 0.18f);
            diagnosticsVisible = false;
            diagnosticsPanel.setVisibility(View.GONE);
            showMainMenu(false);
        });

        configureTouchControls();
        configureSurface();
        collectInitialDiagnostics();
        showMainMenu(true);
        handler.post(runtimePoll);
        scheduleStartupUpdateCheck();
    }

    private void collectInitialDiagnostics() {
        try {
            final String textReport = ProbeBridge.getTextReport();
            final String jsonReport = ProbeBridge.getJsonReport();
            final String filesRoot = getFilesDir().getAbsolutePath();
            final boolean skeletonStaged = stageAsset(SKELETON_ASSET, SKELETON_FILE);
            final boolean lichStaged = stageAsset(LICH_ASSET, LICH_FILE);
            for (final String legacy : new String[]{"diff-array-512.rgba", "normal-array-512.rgba", "arm-array-512.rgba"}) {
                final File stale = new File(getFilesDir(), legacy);
                if (stale.exists()) stale.delete();
            }
            final boolean materialsStaged = stageAsset("textures/polyhaven/mobile_1k/diff-array-512-astc6x6.ktx2", "diff-array-512-astc6x6.ktx2")
                    && stageAsset("textures/polyhaven/mobile_1k/normal-array-512-astc4x4.ktx2", "normal-array-512-astc4x4.ktx2")
                    && stageAsset("textures/polyhaven/mobile_1k/arm-array-512-astc6x6.ktx2", "arm-array-512-astc6x6.ktx2")
                    && stageAsset("textures/meshy/lich_placeholder_v01/base-color-2048-astc6x6.ktx2", "base-color-2048-astc6x6.ktx2")
                    && stageAsset("textures/meshy/lich_placeholder_v01/emissive-2048-astc6x6.ktx2", "emissive-2048-astc6x6.ktx2");
            final boolean heldItemsStaged =
                    stageAsset("models/weapons/runtime/asset.manifest.json", "models/weapons/runtime/asset.manifest.json")
                    && stageAsset("models/weapons/runtime/gothic-arming-sword-rh-lod0.runtime.glb", "models/weapons/runtime/gothic-arming-sword-rh-lod0.runtime.glb")
                    && stageAsset("models/props/runtime/asset.manifest.json", "models/props/runtime/asset.manifest.json")
                    && stageAsset("models/props/runtime/gothic-hand-torch-lod0.runtime.glb", "models/props/runtime/gothic-hand-torch-lod0.runtime.glb")
                    && stageAsset("models/props/runtime/dielectric-fixture/asset.manifest.json", "models/props/runtime/dielectric-fixture/asset.manifest.json")
                    && stageAsset("models/props/runtime/dielectric-fixture/closed-glass-lod0.runtime.glb", "models/props/runtime/dielectric-fixture/closed-glass-lod0.runtime.glb")
                    && stageAsset("models/props/runtime/gothic-chest-base/asset.manifest.json", "models/props/runtime/gothic-chest-base/asset.manifest.json")
                    && stageAsset("models/props/runtime/gothic-chest-base/gothic-chest-base-lod0.runtime.glb", "models/props/runtime/gothic-chest-base/gothic-chest-base-lod0.runtime.glb")
                    && stageAsset("models/props/runtime/gothic-chest-lid/asset.manifest.json", "models/props/runtime/gothic-chest-lid/asset.manifest.json")
                    && stageAsset("models/props/runtime/gothic-chest-lid/gothic-chest-lid-lod0.runtime.glb", "models/props/runtime/gothic-chest-lid/gothic-chest-lid-lod0.runtime.glb")
                    && stageAsset("models/props/runtime/reward-lantern-ring/asset.manifest.json", "models/props/runtime/reward-lantern-ring/asset.manifest.json")
                    && stageAsset("models/props/runtime/reward-lantern-ring/reward-lantern-ring-lod0.runtime.glb", "models/props/runtime/reward-lantern-ring/reward-lantern-ring-lod0.runtime.glb")
                    && stageAsset("models/props/runtime/reward-lantern-body/asset.manifest.json", "models/props/runtime/reward-lantern-body/asset.manifest.json")
                    && stageAsset("models/props/runtime/reward-lantern-body/reward-lantern-body-lod0.runtime.glb", "models/props/runtime/reward-lantern-body/reward-lantern-body-lod0.runtime.glb")
                    && stageAsset("models/player/runtime/asset.manifest.json", "models/player/runtime/asset.manifest.json")
                    && stageAsset("models/player/runtime/clip-manifest.json", "models/player/runtime/clip-manifest.json")
                    && stageAsset("models/player/runtime/gothic-traveller-lod0.runtime.glb", "models/player/runtime/gothic-traveller-lod0.runtime.glb")
                    && stageAsset("textures/props/runtime/asset.manifest.json", "textures/props/runtime/asset.manifest.json")
                    && stageAsset("textures/props/runtime/base-color.android.ktx2", "textures/props/runtime/base-color.android.ktx2")
                    && stageAsset("textures/props/runtime/normal.android.ktx2", "textures/props/runtime/normal.android.ktx2")
                    && stageAsset("textures/props/runtime/orm.android.ktx2", "textures/props/runtime/orm.android.ktx2")
                    && stageAsset("textures/props/runtime/emissive.android.ktx2", "textures/props/runtime/emissive.android.ktx2");
            final boolean written = ProbeBridge.writeReports(filesRoot);
            final StringBuilder output = new StringBuilder(textReport).append('\n');
            if (textReport.contains("RT mode: Unsupported")) {
                output.append("Unsupported: fake RT fallback is disabled.\n\n");
            }
            output.append("Reports written: ").append(written ? "yes" : "no").append('\n');
            output.append("Animated skeleton staged: ").append(skeletonStaged ? "yes" : "no").append('\n');
            output.append("Animated lich placeholder staged: ").append(lichStaged ? "yes" : "no").append('\n');
            output.append("ASTC PBR material arrays staged: ").append(materialsStaged ? "yes" : "no").append('\n');
            output.append("Production PBR held items/player staged: ").append(heldItemsStaged ? "yes" : "no").append('\n');
            output.append("Report directory: ").append(filesRoot).append('/').append(REPORT_DIRECTORY).append('\n');
            output.append("Report files: ").append(TEXT_REPORT_FILE).append(", ").append(JSON_REPORT_FILE).append('\n');
            output.append("JSON sample:\n").append(jsonReport);
            reportText = output.toString();
        } catch (final Throwable error) {
            reportText = "Unable to load the native Vulkan RT renderer.\n\n" + error.getMessage();
        }
        reportTextView.setText(reportText);
    }

    private void configureSurface() {
        surfaceView.setClickable(true);
        surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(final SurfaceHolder holder) {
                surfaceAvailable = true;
                currentSurface = holder.getSurface();
                startSurfaceIfReady();
            }

            @Override
            public void surfaceChanged(final SurfaceHolder holder, final int format, final int width, final int height) {
                currentSurface = holder.getSurface();
            }

            @Override
            public void surfaceDestroyed(final SurfaceHolder holder) {
                surfaceAvailable = false;
                currentSurface = null;
                stopSurface();
            }
        });
    }

    private void startSurfaceIfReady() {
        if (!resumed || !surfaceAvailable || surfaceStarted || currentSurface == null) return;
        try {
            surfaceStarted = ProbeBridge.startDiagnosticSurface(currentSurface, getFilesDir().getAbsolutePath());
            ProbeBridge.setSimulationPaused(menuVisible || diagnosticsVisible);
            if (!surfaceStarted) {
                reportTextView.append("\n\nRenderer surface failed to start.");
                showDiagnostics(true);
            }
        } catch (final Throwable error) {
            reportTextView.append("\n\nRenderer surface failure: " + error.getMessage());
            showDiagnostics(true);
        }
    }

    private void stopSurface() {
        if (!surfaceStarted) return;
        ProbeBridge.stopDiagnosticSurface();
        surfaceStarted = false;
    }

    private void configureTouchControls() {
        surfaceView.setOnTouchListener((view, event) -> {
            if (menuVisible || diagnosticsVisible || deathOverlayVisible || ProbeBridge.getRuntimeState() != 1) return true;
            final int action = event.getActionMasked();
            if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
                final int index = event.getActionIndex();
                final int pointerId = event.getPointerId(index);
                final float x = event.getX(index);
                if (x < view.getWidth() * 0.5f && activePointers[0] == -1) {
                    activePointers[0] = pointerId;
                    viewControls[5] = x;
                    viewControls[6] = event.getY(index);
                } else if (activePointers[1] == -1) {
                    activePointers[1] = pointerId;
                    viewControls[3] = x;
                    viewControls[4] = event.getY(index);
                }
                view.performClick();
                return true;
            }
            if (action == MotionEvent.ACTION_MOVE) {
                final float sensitivity = preferences.getInt("look_sensitivity", 100) / 100.0f;
                for (int i = 0; i < event.getPointerCount(); ++i) {
                    final int pointerId = event.getPointerId(i);
                    if (pointerId == activePointers[0]) {
                        final float dx = event.getX(i) - viewControls[5];
                        final float dy = event.getY(i) - viewControls[6];
                        viewControls[7] = clamp(dx / Math.max(view.getWidth() * 0.16f, 1.0f), -1.0f, 1.0f);
                        viewControls[8] = clamp(-dy / Math.max(view.getHeight() * 0.16f, 1.0f), -1.0f, 1.0f);
                    } else if (pointerId == activePointers[1]) {
                        final float dx = event.getX(i) - viewControls[3];
                        final float dy = event.getY(i) - viewControls[4];
                        viewControls[3] = event.getX(i);
                        viewControls[4] = event.getY(i);
                        viewControls[0] += dx * 0.0036f * sensitivity;
                        viewControls[1] = clamp(viewControls[1] - dy * 0.0028f * sensitivity, -0.32f, 0.28f);
                    }
                }
                pushViewControls();
                return true;
            }
            if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP || action == MotionEvent.ACTION_CANCEL) {
                final int pointerId = event.getPointerId(event.getActionIndex());
                if (pointerId == activePointers[0]) {
                    activePointers[0] = -1;
                    viewControls[7] = 0.0f;
                    viewControls[8] = 0.0f;
                }
                if (pointerId == activePointers[1]) activePointers[1] = -1;
                pushViewControls();
                return true;
            }
            return true;
        });
    }

    private void showMainMenu(final boolean firstLaunch) {
        rtLabVisible = false;
        rtLabTelemetry = null;
        handler.removeCallbacks(refreshRtLabTelemetry);
        benchmarkReportVisible = false;
        diagnosticsVisible = false;
        diagnosticsPanel.setVisibility(View.GONE);
        menuVisible = true;
        ProbeBridge.setSimulationPaused(true);
        clearTouchState();
        attackButton.setVisibility(View.GONE);
        parryButton.setVisibility(View.GONE);
        menuButton.setVisibility(View.GONE);
        rtStatus.setVisibility(View.GONE);
        vitalityStatus.setVisibility(View.GONE);
        developerOverlay.setVisibility(View.GONE);
        menuScrim.setVisibility(View.VISIBLE);
        menuScrim.removeAllViews();

        final LinearLayout panel = createPanel("HORDE LANTERN RT", getString(R.string.alpha_version));
        addBody(panel, getString(R.string.hardware_requirement));
        addMenuButton(panel, firstLaunch || firstMenu ? getString(R.string.start_demo) : getString(R.string.resume_demo), () -> {
            playSound("ui_select", 0.18f);
            firstMenu = false;
            hideMenu();
        });
        final Runnable restartAction = () -> {
            playSound("ui_select", 0.18f);
            resetRoute();
            firstMenu = false;
            hideMenu();
        };
        addMenuButtonRow(panel,
                getString(R.string.restart_route), restartAction,
                getString(R.string.controls), this::showControls);
        addMenuButtonRow(panel,
                getString(R.string.settings), this::showSettings,
                getString(R.string.technical_info), () -> showDiagnostics(false));
        if (rtLabUnlocked || debugRtLabAccess) {
            addMenuButton(panel, getString(R.string.rt_lab), () -> openRtLab(false));
        }
        addMenuButton(panel, getString(R.string.run_benchmark), this::startBenchmark);
        addMenuButtonRow(panel,
                getString(R.string.more_by_samfa12), this::openSamfa12Website,
                getString(R.string.check_for_updates), () -> checkForUpdates(true));
        addMenuButtonRow(panel,
                getString(R.string.credits), this::showCredits,
                getString(R.string.quit), this::finishAndRemoveTask);
        attachPanel(panel);
    }

    private void openSamfa12Website() {
        playSound("ui_select", 0.18f);
        try {
            startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("https://samfa12.com/")));
        } catch (final RuntimeException error) {
            Log.e(TAG, "Failed to open Samfa12.com.", error);
            Toast.makeText(this, R.string.website_open_failed, Toast.LENGTH_LONG).show();
        }
    }

    private void startBenchmark() {
        playSound("ui_select", 0.18f);
        if (ProbeBridge.getRuntimeState() != 1 || !ProbeBridge.requestBenchmark()) {
            Toast.makeText(this, R.string.benchmark_unavailable, Toast.LENGTH_LONG).show();
            return;
        }
        benchmarkRunning = true;
        latestBenchmarkReport = "";
        firstMenu = false;
        hideMenu();
        menuButton.setVisibility(View.GONE);
        attackButton.setVisibility(View.GONE);
        parryButton.setVisibility(View.GONE);
        rtStatus.setVisibility(View.VISIBLE);
        vitalityStatus.setVisibility(View.GONE);
        rtStatus.setText(R.string.benchmark_starting);
    }

    private void showBenchmarkReport(final boolean completed) {
        benchmarkRunning = false;
        benchmarkReportVisible = true;
        menuVisible = true;
        diagnosticsVisible = false;
        ProbeBridge.setSimulationPaused(true);
        clearTouchState();
        attackButton.setVisibility(View.GONE);
        parryButton.setVisibility(View.GONE);
        menuButton.setVisibility(View.GONE);
        developerOverlay.setVisibility(View.GONE);
        rtStatus.setVisibility(View.GONE);
        vitalityStatus.setVisibility(View.GONE);
        diagnosticsPanel.setVisibility(View.GONE);
        menuScrim.setVisibility(View.VISIBLE);
        menuScrim.removeAllViews();

        final LinearLayout panel = createPanel(getString(R.string.benchmark_report),
                completed ? getString(R.string.benchmark_complete) : getString(R.string.benchmark_invalid));
        final TextView report = new TextView(this);
        report.setText(latestBenchmarkReport);
        report.setTextColor(0xFFD8F0D0);
        report.setTextSize(11);
        report.setTypeface(Typeface.MONOSPACE);
        report.setTextIsSelectable(true);
        report.setPadding(0, 0, 0, dp(14));
        panel.addView(report, matchWrap());
        addMenuButtonRow(panel,
                getString(R.string.copy_report), this::copyBenchmarkReport,
                getString(R.string.save_report), this::saveBenchmarkReport);
        addMenuButton(panel, getString(R.string.back), () -> showMainMenu(false));
        attachPanel(panel);
    }

    private void copyBenchmarkReport() {
        final ClipboardManager clipboard = (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
        if (clipboard == null) {
            Toast.makeText(this, R.string.report_copy_failed, Toast.LENGTH_LONG).show();
            return;
        }
        clipboard.setPrimaryClip(ClipData.newPlainText(getString(R.string.benchmark_report), latestBenchmarkReport));
        Toast.makeText(this, R.string.report_copied, Toast.LENGTH_SHORT).show();
    }

    private void saveBenchmarkReport() {
        final Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("text/plain");
        intent.putExtra(Intent.EXTRA_TITLE, "HordeLanternRT-benchmark.txt");
        try {
            startActivityForResult(intent, REQUEST_SAVE_BENCHMARK);
        } catch (final RuntimeException error) {
            Log.e(TAG, "Failed to open benchmark document picker.", error);
            Toast.makeText(this, R.string.report_save_failed, Toast.LENGTH_LONG).show();
        }
    }

    private void hideMenu() {
        if (deathOverlayVisible) {
            return;
        }
        menuVisible = false;
        menuScrim.setVisibility(View.GONE);
        final boolean showHud = preferences.getBoolean("show_hud", true);
        menuButton.setVisibility(showHud ? View.VISIBLE : View.GONE);
        attackButton.setVisibility(showHud && ProbeBridge.getRuntimeState() == 1 &&
                lastPlayerLifePhase == PLAYER_ALIVE
                ? View.VISIBLE : View.GONE);
        parryButton.setVisibility(showHud && ProbeBridge.getRuntimeState() == 1 &&
                lastPlayerLifePhase == PLAYER_ALIVE
                ? View.VISIBLE : View.GONE);
        rtStatus.setVisibility(showHud ? View.VISIBLE : View.GONE);
        vitalityStatus.setVisibility(showHud && lastPlayerLifePhase == PLAYER_ALIVE ? View.VISIBLE : View.GONE);
        ProbeBridge.setSimulationPaused(false);
    }

    private void showControls() {
        playSound("ui_select", 0.18f);
        menuScrim.removeAllViews();
        final LinearLayout panel = createPanel(getString(R.string.controls), "PHONE CONTROLS");
        addBody(panel, getString(R.string.controls_help));
        addMenuButton(panel, getString(R.string.back), () -> showMainMenu(false));
        attachPanel(panel);
    }

    private void showCredits() {
        playSound("ui_select", 0.18f);
        menuScrim.removeAllViews();
        final LinearLayout panel = createPanel(getString(R.string.credits), "ASSET PROVENANCE");
        addLinkedBody(panel, getString(R.string.credits_body));
        addMenuButton(panel, getString(R.string.back), () -> showMainMenu(false));
        attachPanel(panel);
    }

    private void showSettings() {
        playSound("ui_select", 0.18f);
        menuScrim.removeAllViews();
        final LinearLayout panel = createPanel(getString(R.string.settings), "SAVED ON THIS DEVICE");

        final CheckBox soundEnabled = new CheckBox(this);
        soundEnabled.setText(R.string.sfx_enabled);
        soundEnabled.setTextColor(0xFFFFE5BA);
        soundEnabled.setTextSize(16);
        soundEnabled.setChecked(preferences.getBoolean("sfx_enabled", true));
        soundEnabled.setMinHeight(dp(48));
        soundEnabled.setOnCheckedChangeListener((buttonView, checked) -> preferences.edit().putBoolean("sfx_enabled", checked).apply());
        panel.addView(soundEnabled, matchWrap());

        addSlider(panel, getString(R.string.sfx_volume), preferences.getInt("sfx_volume", 70), 0, 100,
                value -> preferences.edit().putInt("sfx_volume", value).apply());
        addSlider(panel, getString(R.string.look_sensitivity), preferences.getInt("look_sensitivity", 100), 50, 175,
                value -> preferences.edit().putInt("look_sensitivity", value).apply());
        addSlider(panel, getString(R.string.render_scale), preferences.getInt("render_scale", 100), 50, 100,
                value -> {
                    preferences.edit().putInt("render_scale", value).apply();
                    handler.removeCallbacks(applyPendingRenderScale);
                    handler.postDelayed(applyPendingRenderScale, 350L);
                });

        final int waterQuality = Math.max(WATER_QUALITY_OFF, Math.min(WATER_QUALITY_HIGH,
                preferences.getInt("water_quality", WATER_QUALITY_MOBILE)));
        final String waterQualityName = waterQuality == WATER_QUALITY_HIGH ? getString(R.string.water_quality_high) :
                (waterQuality == WATER_QUALITY_MOBILE ? getString(R.string.water_quality_mobile) :
                        getString(R.string.water_quality_off));
        addMenuButton(panel, getString(R.string.water_quality, waterQualityName), () -> {
            final int nextQuality = waterQuality == WATER_QUALITY_HIGH ? WATER_QUALITY_MOBILE :
                    (waterQuality == WATER_QUALITY_MOBILE ? WATER_QUALITY_OFF : WATER_QUALITY_HIGH);
            preferences.edit().putInt("water_quality", nextQuality).apply();
            ProbeBridge.setWaterQuality(nextQuality);
            showSettings();
        });

        final CheckBox hapticsEnabled = new CheckBox(this);
        hapticsEnabled.setText(R.string.haptics_enabled);
        hapticsEnabled.setTextColor(0xFFFFE5BA);
        hapticsEnabled.setTextSize(16);
        hapticsEnabled.setChecked(preferences.getBoolean("haptics_enabled", true));
        hapticsEnabled.setMinHeight(dp(48));
        hapticsEnabled.setOnCheckedChangeListener((buttonView, checked) -> {
            preferences.edit().putBoolean("haptics_enabled", checked).apply();
            if (checked) performHaptic(HAPTIC_SWING);
        });
        panel.addView(hapticsEnabled, matchWrap());

        final CheckBox showHud = new CheckBox(this);
        showHud.setText(R.string.show_hud);
        showHud.setTextColor(0xFFFFE5BA);
        showHud.setTextSize(16);
        showHud.setChecked(preferences.getBoolean("show_hud", true));
        showHud.setMinHeight(dp(48));
        showHud.setOnCheckedChangeListener((buttonView, checked) -> preferences.edit().putBoolean("show_hud", checked).apply());
        panel.addView(showHud, matchWrap());

        addMenuButtonRow(panel,
                getString(R.string.reset_defaults), () -> {
                    preferences.edit().clear()
                            .putBoolean(PREF_RT_LAB_UNLOCKED, rtLabUnlocked)
                            .apply();
                    handler.removeCallbacks(applyPendingRenderScale);
                    ProbeBridge.setRenderScale(1.0f);
                    ProbeBridge.setWaterQuality(WATER_QUALITY_MOBILE);
                    showSettings();
                },
                getString(R.string.back), () -> showMainMenu(false));
        attachPanel(panel);
    }

    private void showDiagnostics(final boolean errorState) {
        menuVisible = true;
        diagnosticsVisible = true;
        diagnosticsErrorState = errorState;
        diagnosticsRefreshTick = 0;
        ProbeBridge.setSimulationPaused(true);
        clearTouchState();
        menuScrim.setVisibility(View.GONE);
        menuButton.setVisibility(View.GONE);
        attackButton.setVisibility(View.GONE);
        parryButton.setVisibility(View.GONE);
        rtStatus.setVisibility(View.GONE);
        vitalityStatus.setVisibility(View.GONE);
        developerOverlay.setVisibility(View.GONE);
        refreshDiagnosticsText();
        diagnosticsPanel.setVisibility(View.VISIBLE);
        diagnosticsPanel.bringToFront();
    }

    private void refreshDiagnosticsText() {
        String currentReport = reportText;
        try {
            currentReport = ProbeBridge.getTextReport()
                    + "\nReport directory: " + getFilesDir().getAbsolutePath() + "/" + REPORT_DIRECTORY
                    + "\nReport files: " + TEXT_REPORT_FILE + ", " + JSON_REPORT_FILE;
        } catch (final Throwable ignored) {
            // Retain the last readable report if the native bridge is unavailable.
        }
        reportTextView.setText((diagnosticsErrorState ? getString(R.string.rt_error) + "\n\n" : "") + currentReport);
    }

    private void resetRoute() {
        ++delayedGameplayFeedbackGeneration;
        endingOverlayVisible = false;
        endingOverlayDismissed = false;
        rtLabNewlyUnlocked = false;
        for (int i = 0; i < viewControls.length; ++i) viewControls[i] = 0.0f;
        viewControls[2] = 1.8f;
        activePointers[0] = -1;
        activePointers[1] = -1;
        restoreAuthoredRtLabTuning();
        ProbeBridge.requestRouteReset();
        pushViewControls();
    }

    private void updateVitalityHud(final int vitality) {
        final int safeVitality = Math.max(0, Math.min(3, vitality));
        lastPlayerVitality = safeVitality;
        vitalityStatus.setText("VITALITY  " + safeVitality + " / 3");
        vitalityStatus.setContentDescription(getString(R.string.vitality_accessibility, safeVitality));
        if (safeVitality >= 3) {
            vitalityStatus.setTextColor(0xFFFFD07A);
        } else if (safeVitality == 2) {
            vitalityStatus.setTextColor(0xFFFFA84F);
        } else {
            vitalityStatus.setTextColor(0xFFFF705C);
        }
    }

    private void showDeathOverlay() {
        if (deathOverlayVisible || benchmarkRunning || debugCaptureUiSuppressed) return;
        deathOverlayVisible = true;
        menuVisible = true;
        ProbeBridge.setSimulationPaused(true);
        clearTouchState();
        attackButton.setVisibility(View.GONE);
        parryButton.setVisibility(View.GONE);
        menuButton.setVisibility(View.GONE);
        rtStatus.setVisibility(View.GONE);
        vitalityStatus.setVisibility(View.GONE);
        developerOverlay.setVisibility(View.GONE);
        diagnosticsPanel.setVisibility(View.GONE);
        menuScrim.setVisibility(View.VISIBLE);
        menuScrim.removeAllViews();

        final LinearLayout panel = createPanel(getString(R.string.you_fell), getString(R.string.death_message));
        addMenuButtonRow(panel,
                getString(R.string.retry_encounter), this::retryEncounter,
                getString(R.string.restart_route), this::restartAfterDeath);
        addMenuButton(panel, getString(R.string.quit), this::finishAndRemoveTask);
        attachPanel(panel);
    }

    private void showEndingOverlay() {
        if (endingOverlayVisible || endingOverlayDismissed || deathOverlayVisible ||
                rtLabVisible || benchmarkRunning || debugCaptureUiSuppressed) return;
        endingOverlayVisible = true;
        menuVisible = true;
        ProbeBridge.setSimulationPaused(true);
        clearTouchState();
        attackButton.setVisibility(View.GONE);
        parryButton.setVisibility(View.GONE);
        menuButton.setVisibility(View.GONE);
        rtStatus.setVisibility(View.GONE);
        vitalityStatus.setVisibility(View.GONE);
        developerOverlay.setVisibility(View.GONE);
        diagnosticsPanel.setVisibility(View.GONE);
        menuScrim.setVisibility(View.VISIBLE);
        menuScrim.removeAllViews();

        final boolean labAvailable = rtLabUnlocked || debugRtLabAccess;
        final LinearLayout panel = createPanel(
                rtLabNewlyUnlocked ? getString(R.string.rt_lab_unlocked) : getString(R.string.ending_title),
                rtLabNewlyUnlocked ? getString(R.string.rt_lab_unlocked_subtitle) : getString(R.string.ending_subtitle));
        addBody(panel, getString(R.string.ending_body));
        if (labAvailable) {
            addMenuButton(panel, getString(R.string.open_rt_lab), () -> openRtLab(true));
        }
        addMenuButtonRow(panel,
                getString(R.string.continue_label), this::continueAfterEnding,
                getString(R.string.begin_again), this::restartAfterEnding);
        addMenuButton(panel, getString(R.string.quit), this::finishAndRemoveTask);
        attachPanel(panel);
    }

    private boolean persistRtLabUnlockIfEligible() {
        if (rtLabUnlocked || !ProbeBridge.isRtLabUnlockEligible()) return false;
        rtLabUnlocked = true;
        rtLabNewlyUnlocked = true;
        preferences.edit().putBoolean(PREF_RT_LAB_UNLOCKED, true).apply();
        return true;
    }

    private void openRtLab(final boolean returnToEnding) {
        rtLabReturnToEnding = returnToEnding;
        if (returnToEnding) endingOverlayVisible = false;
        showRtLab();
    }

    private void showRtLab() {
        rtLabVisible = true;
        menuVisible = true;
        diagnosticsVisible = false;
        ProbeBridge.setSimulationPaused(true);
        clearTouchState();
        attackButton.setVisibility(View.GONE);
        parryButton.setVisibility(View.GONE);
        menuButton.setVisibility(View.GONE);
        rtStatus.setVisibility(View.GONE);
        vitalityStatus.setVisibility(View.GONE);
        developerOverlay.setVisibility(View.GONE);
        diagnosticsPanel.setVisibility(View.GONE);
        menuScrim.setVisibility(View.VISIBLE);
        menuScrim.removeAllViews();

        final LinearLayout panel = createRtLabPanel();
        addBody(panel, getString(R.string.rt_lab_body));
        addRtLabSlider(panel, getString(R.string.rt_lab_waterfall_width),
                rtWaterfallWidthPercent, 25, 200, "%", value -> {
                    rtWaterfallWidthPercent = value;
                    publishRtSceneTuning();
                });
        addRtLabSlider(panel, getString(R.string.rt_lab_roof_open),
                rtRoofOpenPercent, 0, 100, "%", value -> {
                    rtRoofOverrideEnabled = true;
                    rtRoofOpenPercent = value;
                    publishRtSceneTuning();
                });
        addRtLabSlider(panel, getString(R.string.rt_lab_dawn_reveal),
                rtDawnRevealPercent, 0, 100, "%", value -> {
                    rtDawnOverrideEnabled = true;
                    rtDawnRevealPercent = value;
                    publishRtSceneTuning();
                });
        addRtLabSlider(panel, getString(R.string.rt_lab_fog_density),
                rtFogDensityPercent, 0, 200, "%", value -> {
                    rtFogDensityPercent = value;
                    publishRtSceneTuning();
                });
        addRtLabSlider(panel, getString(R.string.rt_lab_fire_strength),
                rtFireStrengthPercent, 0, 200, "%", value -> {
                    rtFireStrengthPercent = value;
                    publishRtFireTuning();
                });
        addRtLabSlider(panel, getString(R.string.rt_lab_fire_turbulence),
                rtFireTurbulencePercent, 0, 200, "%", value -> {
                    rtFireTurbulencePercent = value;
                    publishRtFireTuning();
                });
        addRtLabSlider(panel, getString(R.string.rt_lab_fire_smoke),
                rtFireSmokePercent, 0, 200, "%", value -> {
                    rtFireSmokePercent = value;
                    publishRtFireTuning();
                });
        addRtLabSlider(panel, getString(R.string.rt_lab_glass_visibility),
                rtGlassVisibilityPercent, 0, 100, "%", value -> {
                    rtGlassVisibilityPercent = value;
                    publishRtGlassTuning();
                });
        addRtLabSlider(panel, getString(R.string.rt_lab_glass_transmission),
                rtGlassTransmissionPercent, 0, 100, "%", value -> {
                    rtGlassTransmissionPercent = value;
                    publishRtGlassTuning();
                });
        addRtLabSlider(panel, getString(R.string.rt_lab_glass_ior),
                rtGlassIorHundredths, 100, 250, "", value -> {
                    rtGlassIorHundredths = value;
                    publishRtGlassTuning();
                });
        addRtLabSlider(panel, getString(R.string.rt_lab_glass_roughness),
                rtGlassRoughnessPercent, 0, 100, "%", value -> {
                    rtGlassRoughnessPercent = value;
                    publishRtGlassTuning();
                });

        addMenuButton(panel, getString(R.string.rt_lab_light_group, rtLightGroupName()), () -> {
            rtLightGroup = (rtLightGroup + 1) % rtLightHueDegrees.length;
            showRtLab();
        });
        addRtLabSlider(panel, getString(R.string.rt_lab_light_hue),
                rtLightHueDegrees[rtLightGroup], -180, 180, "°", value -> {
                    rtLightHueDegrees[rtLightGroup] = value;
                    publishRtLightTuning();
                });
        addRtLabSlider(panel, getString(R.string.rt_lab_light_intensity),
                rtLightIntensityPercent[rtLightGroup], 0, 200, "%", value -> {
                    rtLightIntensityPercent[rtLightGroup] = value;
                    publishRtLightTuning();
                });

        addWorkloadSelector(panel);

        rtLabTelemetry = new TextView(this);
        rtLabTelemetry.setTextColor(0xFFD8F0D0);
        rtLabTelemetry.setTextSize(11);
        rtLabTelemetry.setTypeface(Typeface.MONOSPACE);
        rtLabTelemetry.setMinHeight(dp(48));
        rtLabTelemetry.setGravity(Gravity.CENTER_VERTICAL);
        rtLabTelemetry.setPadding(0, dp(8), 0, dp(8));
        panel.addView(rtLabTelemetry, matchWrap());

        addMenuButtonRow(panel,
                getString(R.string.restore_authored), () -> {
                    restoreAuthoredRtLabTuning();
                    showRtLab();
                },
                getString(R.string.back), this::closeRtLab);
        attachPanel(panel);
        handler.removeCallbacks(refreshRtLabTelemetry);
        handler.post(refreshRtLabTelemetry);
    }

    private LinearLayout createRtLabPanel() {
        final LinearLayout panel = createPanel(getString(R.string.rt_lab), getString(R.string.rt_lab_eyebrow));
        final GradientDrawable background = new GradientDrawable();
        background.setColor(0xD91A1510);
        background.setCornerRadius(dp(4));
        background.setStroke(dp(1), 0xFFB17A35);
        panel.setBackground(background);
        return panel;
    }

    private void addWorkloadSelector(final LinearLayout panel) {
        final TextView label = new TextView(this);
        label.setText(getString(R.string.rt_lab_workload));
        label.setTextColor(0xFFFFE5BA);
        label.setTextSize(15);
        label.setPadding(0, dp(8), 0, 0);
        panel.addView(label, matchWrap());

        final LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.VERTICAL);
        final String[] names = {getString(R.string.rt_lab_lean), getString(R.string.rt_lab_authored),
                getString(R.string.rt_lab_max)};
        for (int preset = 0; preset < names.length; ++preset) {
            final int selectedPreset = preset;
            final Button button = createMenuButton(
                    (rtWorkloadPreset == preset ? "● " : "") + names[preset], () -> {
                        rtWorkloadPreset = selectedPreset;
                        ProbeBridge.setRtWorkloadPreset(selectedPreset);
                        showRtLab();
                    });
            button.setSingleLine(true);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                button.setAutoSizeTextTypeUniformWithConfiguration(
                        10, 15, 1, TypedValue.COMPLEX_UNIT_SP);
            } else {
                button.setTextSize(10);
            }
            final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, dp(48));
            if (preset > 0) params.topMargin = dp(6);
            row.addView(button, params);
        }
        final LinearLayout.LayoutParams rowParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        rowParams.topMargin = dp(6);
        panel.addView(row, rowParams);
    }

    private void closeRtLab() {
        rtLabVisible = false;
        rtLabTelemetry = null;
        handler.removeCallbacks(refreshRtLabTelemetry);
        if (rtLabReturnToEnding) {
            endingOverlayVisible = false;
            showEndingOverlay();
        } else {
            showMainMenu(false);
        }
    }

    private String rtLightGroupName() {
        switch (rtLightGroup) {
            case 1: return getString(R.string.rt_lab_skylight);
            case 2: return getString(R.string.rt_lab_passage);
            case 3: return getString(R.string.rt_lab_staff);
            default: return getString(R.string.rt_lab_torch);
        }
    }

    private void publishRtSceneTuning() {
        ProbeBridge.setRtSceneTuning(
                rtWaterfallWidthPercent / 100.0f,
                rtRoofOverrideEnabled, rtRoofOpenPercent / 100.0f,
                rtDawnOverrideEnabled, rtDawnRevealPercent / 100.0f,
                rtFogDensityPercent / 100.0f);
    }

    private void publishRtLightTuning() {
        ProbeBridge.setRtLightTuning(
                rtLightGroup,
                rtLightHueDegrees[rtLightGroup],
                rtLightIntensityPercent[rtLightGroup] / 100.0f);
    }

    static String installedUpdateVersion(final String packageVersion) {
        if (packageVersion == null) return "";
        return packageVersion.endsWith("-debug")
                ? packageVersion.substring(0, packageVersion.length() - "-debug".length())
                : packageVersion;
    }

    private String installedUpdateVersion() {
        try {
            return installedUpdateVersion(
                    getPackageManager().getPackageInfo(getPackageName(), 0).versionName);
        } catch (final Exception error) {
            Log.w(TAG, "Installed package version was unavailable for the update check.", error);
            return "";
        }
    }

    private void checkForUpdates(final boolean manualRequest) {
        if (manualRequest) {
            handler.removeCallbacks(runStartupUpdateCheck);
            startupUpdateCheckScheduled = false;
            startupUpdateCheckCompleted = true;
        }
        if (updateCheckInFlight) {
            if (manualRequest) {
                Toast.makeText(this, R.string.update_check_in_progress, Toast.LENGTH_SHORT).show();
            }
            return;
        }
        updateCheckInFlight = true;
        final String installedVersion = installedUpdateVersion();
        updateExecutor.execute(() -> {
            int statusCode = 0;
            byte[] body = new byte[0];
            HttpsURLConnection connection = null;
            try {
                final JSONObject request = new JSONObject(new String(
                        ProbeBridge.getGitHubReleaseRequestContract(), StandardCharsets.UTF_8));
                final URL endpoint = new URL(request.getString("url"));
                if (!"https".equalsIgnoreCase(endpoint.getProtocol())) {
                    throw new IllegalStateException("Native update request was not HTTPS.");
                }
                connection = (HttpsURLConnection) endpoint.openConnection();
                connection.setInstanceFollowRedirects(false);
                connection.setConnectTimeout(3500);
                connection.setReadTimeout(5000);
                connection.setRequestMethod("GET");
                connection.setRequestProperty("Accept", request.getString("accept"));
                connection.setRequestProperty("X-GitHub-Api-Version", request.getString("apiVersion"));
                connection.setRequestProperty("User-Agent", request.getString("userAgent"));
                statusCode = connection.getResponseCode();
                final InputStream response = statusCode >= 200 && statusCode < 300
                        ? connection.getInputStream() : connection.getErrorStream();
                body = readBoundedUpdateResponse(
                        response, request.getInt("maximumResponseBytes"));
            } catch (final Exception error) {
                Log.w(TAG, "GitHub Releases update check failed.", error);
            } finally {
                if (connection != null) connection.disconnect();
            }

            final String decision = new String(ProbeBridge.evaluateGitHubReleaseUpdate(
                    installedVersion, statusCode, body), StandardCharsets.UTF_8);
            runOnUiThread(() -> presentUpdateDecision(decision, manualRequest));
        });
    }

    private static byte[] readBoundedUpdateResponse(final InputStream response,
                                                     final int maximumBytes) throws Exception {
        if (response == null) return new byte[0];
        if (maximumBytes <= 0 || maximumBytes > 1024 * 1024) {
            throw new IllegalArgumentException("Native update response limit is invalid.");
        }
        try (InputStream input = response;
             ByteArrayOutputStream output = new ByteArrayOutputStream(8192)) {
            final byte[] buffer = new byte[8192];
            int total = 0;
            while (true) {
                final int read = input.read(buffer);
                if (read < 0) break;
                final int accepted = Math.min(read, maximumBytes + 1 - total);
                if (accepted > 0) output.write(buffer, 0, accepted);
                total += accepted;
                if (total > maximumBytes) break;
            }
            return output.toByteArray();
        }
    }

    private void presentUpdateDecision(final String decisionJson, final boolean manualRequest) {
        updateCheckInFlight = false;
        if (isFinishing() || (Build.VERSION.SDK_INT >= 17 && isDestroyed())) return;
        if (!resumed) {
            pendingUpdateDecision = decisionJson;
            pendingUpdateManualRequest = manualRequest;
            return;
        }
        pendingUpdateDecision = null;
        try {
            final JSONObject decision = new JSONObject(decisionJson);
            final String status = decision.optString("status", "error");
            if ("update-available".equals(status)) {
                final JSONObject update = decision.optJSONObject("update");
                if (update == null || updatePromptShown) return;
                final String releaseUrl = update.optString("releasePageUrl", "");
                if (!releaseUrl.startsWith(GITHUB_RELEASE_PAGE_PREFIX)) return;
                updatePromptShown = true;
                final String version = update.optString("version", "new");
                final String title = update.optString("title", "");
                final String notes = update.optString("notes", "");
                final StringBuilder message = new StringBuilder(
                        getString(R.string.update_available_body, version));
                if (!title.isEmpty()) message.append("\n\n").append(title);
                if (!notes.isEmpty()) message.append("\n\n").append(notes);
                new AlertDialog.Builder(this)
                        .setTitle(R.string.update_available_title)
                        .setMessage(message.toString())
                        .setPositiveButton(R.string.update_now, (dialog, which) ->
                                openVerifiedReleasePage(releaseUrl))
                        .setNegativeButton(R.string.later, null)
                        .show();
            } else if (manualRequest && "up-to-date".equals(status)) {
                Toast.makeText(this, R.string.up_to_date, Toast.LENGTH_LONG).show();
            } else if (manualRequest) {
                Toast.makeText(this, R.string.update_check_failed, Toast.LENGTH_LONG).show();
            }
        } catch (final Exception error) {
            Log.w(TAG, "Native update decision was invalid.", error);
            if (manualRequest) {
                Toast.makeText(this, R.string.update_check_failed, Toast.LENGTH_LONG).show();
            }
        }
    }

    private void scheduleStartupUpdateCheck() {
        if (debugCaptureUiSuppressed || debugAutomationAutostart ||
                startupUpdateCheckCompleted || startupUpdateCheckScheduled) return;
        startupUpdateCheckScheduled = true;
        handler.postDelayed(runStartupUpdateCheck, 1500L);
    }

    private void openVerifiedReleasePage(final String releaseUrl) {
        if (!releaseUrl.startsWith(GITHUB_RELEASE_PAGE_PREFIX)) return;
        try {
            startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(releaseUrl)));
        } catch (final RuntimeException error) {
            Log.e(TAG, "Failed to open the verified GitHub release page.", error);
            Toast.makeText(this, R.string.update_open_failed, Toast.LENGTH_LONG).show();
        }
    }

    private void publishRtFireTuning() {
        ProbeBridge.setRtFireTuning(
                rtFireStrengthPercent / 100.0f,
                rtFireTurbulencePercent / 100.0f,
                rtFireSmokePercent / 100.0f);
    }

    private void publishRtGlassTuning() {
        ProbeBridge.setRtGlassTuning(
                rtGlassVisibilityPercent > 0,
                rtGlassTransmissionPercent / 100.0f,
                rtGlassIorHundredths / 100.0f,
                rtGlassRoughnessPercent / 100.0f);
    }

    private void restoreAuthoredRtLabTuning() {
        rtWaterfallWidthPercent = 100;
        rtRoofOverrideEnabled = false;
        rtRoofOpenPercent = 0;
        rtDawnOverrideEnabled = false;
        rtDawnRevealPercent = 0;
        rtFogDensityPercent = 100;
        rtFireStrengthPercent = 100;
        rtFireTurbulencePercent = 100;
        rtFireSmokePercent = 100;
        rtGlassVisibilityPercent = 0;
        rtGlassTransmissionPercent = 94;
        rtGlassIorHundredths = 152;
        rtGlassRoughnessPercent = 12;
        rtLightGroup = 0;
        for (int index = 0; index < rtLightHueDegrees.length; ++index) {
            rtLightHueDegrees[index] = 0;
            rtLightIntensityPercent[index] = 100;
        }
        rtWorkloadPreset = 1;
        ProbeBridge.resetRtSceneTuning();
    }

    private void continueAfterEnding() {
        playSound("ui_select", 0.18f);
        endingOverlayVisible = false;
        endingOverlayDismissed = true;
        rtLabNewlyUnlocked = false;
        firstMenu = false;
        hideMenu();
    }

    private void restartAfterEnding() {
        playSound("ui_select", 0.18f);
        endingOverlayVisible = false;
        resetRoute();
        firstMenu = false;
        hideMenu();
    }

    private void retryEncounter() {
        if (retryPending) {
            return;
        }
        final int checkpoint = ProbeBridge.retryEncounter();
        if (checkpoint < 0) {
            Toast.makeText(this, R.string.retry_unavailable, Toast.LENGTH_LONG).show();
            return;
        }
        ++delayedGameplayFeedbackGeneration;
        retryPending = true;
        applyCheckpointViewPose(checkpoint);
        clearTouchState();
        pushViewControls();
        Toast.makeText(this, R.string.retrying_encounter, Toast.LENGTH_SHORT).show();
    }

    private void restartAfterDeath() {
        retryPending = false;
        resetRoute();
        deathOverlayVisible = false;
        lastPlayerLifePhase = PLAYER_ALIVE;
        updateVitalityHud(3);
        firstMenu = false;
        hideMenu();
    }

    private final Runnable runtimePoll = new Runnable() {
        @Override
        public void run() {
            try {
                final int state = ProbeBridge.getRuntimeState();
                if (state == 1) {
                    rtStatus.setText(R.string.rt_active);
                    rtStatus.setTextColor(0xFFFFD07A);
                    final int vitality = ProbeBridge.getPlayerVitality();
                    final int lifePhase = ProbeBridge.getPlayerLifePhase();
                    final int finaleEndingPhase = ProbeBridge.getFinaleEndingPhase();
                    if (vitality != lastPlayerVitality) updateVitalityHud(vitality);
                    lastPlayerLifePhase = lifePhase;
                    if (deathOverlayVisible && lifePhase == PLAYER_ALIVE) {
                        deathOverlayVisible = false;
                        if (retryPending) {
                            retryPending = false;
                            firstMenu = false;
                            hideMenu();
                        } else {
                            showMainMenu(false);
                        }
                    }
                    final boolean showHud = preferences.getBoolean("show_hud", true);
                    if (!debugCaptureUiSuppressed && !menuVisible && !benchmarkRunning && showHud &&
                            lifePhase == PLAYER_ALIVE) {
                        attackButton.setVisibility(View.VISIBLE);
                        parryButton.setVisibility(View.VISIBLE);
                    }
                    updateContextualControls(!debugCaptureUiSuppressed && !menuVisible &&
                            !diagnosticsVisible && !benchmarkRunning && !endingOverlayVisible &&
                            showHud && lifePhase == PLAYER_ALIVE);
                    if (!debugCaptureUiSuppressed && !menuVisible && !benchmarkRunning && showHud &&
                            lifePhase != PLAYER_DEAD) {
                        vitalityStatus.setVisibility(View.VISIBLE);
                    }
                    if (lifePhase != PLAYER_ALIVE) {
                        clearTouchState();
                        attackButton.setVisibility(View.GONE);
                        parryButton.setVisibility(View.GONE);
                    }
                    if (lifePhase == PLAYER_DEAD) showDeathOverlay();
                    if (finaleEndingPhase == FINALE_ENDING_COMPLETE) {
                        final boolean unlockGranted = persistRtLabUnlockIfEligible();
                        if (unlockGranted && endingOverlayVisible) {
                            endingOverlayVisible = false;
                        }
                        showEndingOverlay();
                    }
                    if (debugAutomationAutostart && menuVisible && !deathOverlayVisible && !endingOverlayVisible) hideMenu();
                    if (pendingDebugCheckpoint >= 0) {
                        applyCheckpointViewPose(pendingDebugCheckpoint);
                        clearTouchState();
                        final int checkpoint = pendingDebugCheckpoint;
                        pendingDebugCheckpoint = -1;
                        if (pendingDebugCapture) {
                            suppressUiForDebugCapture();
                            pendingDebugCapture = false;
                            if (!ProbeBridge.requestDebugCaptureCheckpoint(checkpoint)) {
                                Log.e(TAG, "Debug capture checkpoint request rejected: " + checkpoint);
                            }
                        } else if (!ProbeBridge.requestDebugCheckpoint(checkpoint)) {
                            Log.e(TAG, "Debug checkpoint request rejected: " + checkpoint);
                        }
                        debugAutomationAutostart = false;
                    } else if (pendingDebugReplay) {
                        pendingDebugReplay = false;
                        viewControls[0] = 0.0f;
                        viewControls[1] = -0.04f;
                        clearTouchState();
                        if (!ProbeBridge.requestDebugRouteReplay()) {
                            Log.e(TAG, "Debug route replay request rejected.");
                        }
                        debugAutomationAutostart = false;
                    }
                } else if (state == 2) {
                    updateContextualControls(false);
                    rtStatus.setText(R.string.rt_unsupported);
                    rtStatus.setTextColor(0xFFFF8A7A);
                    if (!autoDiagnosticsShown) {
                        autoDiagnosticsShown = true;
                        showDiagnostics(false);
                    }
                } else if (state == 3) {
                    updateContextualControls(false);
                    rtStatus.setText(R.string.rt_error);
                    rtStatus.setTextColor(0xFFFF8A7A);
                    if (!autoDiagnosticsShown) {
                        autoDiagnosticsShown = true;
                        showDiagnostics(true);
                    }
                } else {
                    updateContextualControls(false);
                    rtStatus.setText(R.string.rt_starting);
                }

                if (benchmarkRunning) {
                    final int benchmarkStatus = ProbeBridge.getBenchmarkStatus();
                    if (benchmarkStatus == 1) {
                        final String progress = ProbeBridge.getBenchmarkProgress();
                        rtStatus.setText(progress.isEmpty() ? getString(R.string.benchmark_starting) : progress);
                        rtStatus.setVisibility(View.VISIBLE);
                        menuButton.setVisibility(View.GONE);
                        attackButton.setVisibility(View.GONE);
                        parryButton.setVisibility(View.GONE);
                        updateContextualControls(false);
                        vitalityStatus.setVisibility(View.GONE);
                    } else if (benchmarkStatus == 2 || benchmarkStatus == 3) {
                        latestBenchmarkReport = ProbeBridge.getBenchmarkReport();
                        if (latestBenchmarkReport.isEmpty()) {
                            latestBenchmarkReport = getString(R.string.benchmark_interrupted);
                        }
                        showBenchmarkReport(benchmarkStatus == 2);
                    }
                }

                if (isDebuggableApp() && developerOverlayVisible && state == 1 &&
                        !menuVisible && !diagnosticsVisible && !benchmarkRunning) {
                    final String overlayText = ProbeBridge.getDeveloperOverlayText();
                    developerOverlay.setText(overlayText);
                    developerOverlay.setVisibility(overlayText.isEmpty() ? View.GONE : View.VISIBLE);
                    developerOverlay.bringToFront();
                } else {
                    developerOverlay.setVisibility(View.GONE);
                }

                // Platform feedback is meaningful only while this exact RT surface is
                // active. Native teardown discards its pending transport queue, so a
                // pre-Home event can neither play in the background nor replay after
                // the new surface resumes.
                if (resumed && surfaceStarted && state == 1) {
                    final long[] platformEvents = ProbeBridge.drainPlatformEvents();
                    for (int eventIndex = 0; eventIndex + 1 < platformEvents.length; eventIndex += 2) {
                    final long metadata = platformEvents[eventIndex];
                    final long stereoGains = platformEvents[eventIndex + 1];
                    final int eventType = (int) (metadata & 0xffL);
                    final int targetEntity = (int) ((metadata >>> 16) & 0xffL);
                    final long eventSequence = (metadata >>> 32) & 0xffffffffL;
                    switch (eventType) {
                        case PLATFORM_EVENT_PLAYER_FOOTSTEP:
                            playSpatialSound((playerStepVariant++ & 1) == 0 ?
                                    "player_step_1" : "player_step_2", 0.62f, stereoGains);
                            break;
                        case PLATFORM_EVENT_PLAYER_SWING:
                            if (isDebuggableApp()) {
                                Log.i(TAG, "HORDE_PLAYER_SWING_FEEDBACK sequence=" + eventSequence +
                                        " sound=1 haptic=1");
                            }
                            playSpatialSound((swingVariant++ & 1) == 0 ?
                                    "sword_swing_1" : "sword_swing_2", 0.28f, stereoGains);
                            performHaptic(HAPTIC_SWING);
                            break;
                        case PLATFORM_EVENT_PLAYER_DAMAGED:
                            performHaptic(HAPTIC_DAMAGE);
                            break;
                        case PLATFORM_EVENT_PLAYER_KILLED:
                            performHaptic(HAPTIC_FATAL);
                            break;
                        case PLATFORM_EVENT_ENEMY_FOOTSTEP:
                            playSpatialSound((enemyStepVariant++ & 1) == 0 ?
                                    "skeleton_step_1" : "skeleton_step_2", 0.11f, stereoGains);
                            break;
                        case PLATFORM_EVENT_ENEMY_ATTACK_STARTED:
                            playSpatialSound("skeleton_attack", 0.22f, stereoGains);
                            break;
                        case PLATFORM_EVENT_ENEMY_HIT:
                            if (targetEntity == ENTITY_LICH) {
                                // The hurt source includes its own impact; layering the
                                // fencing hit masks the short vocal reaction.
                                playSpatialSound("lich_hurt", 0.82f, stereoGains);
                            } else {
                                playSpatialSound((swingVariant & 1) == 0 ?
                                        "sword_hit_1" : "sword_hit_2", 0.32f, stereoGains);
                            }
                            break;
                        case PLATFORM_EVENT_ENEMY_DEFEATED:
                            // Preserve the authored separation between the sword impact
                            // and the fall cue while retaining the event's spatial gains.
                            final long feedbackGeneration = delayedGameplayFeedbackGeneration;
                            handler.postDelayed(
                                    () -> {
                                        if (feedbackGeneration == delayedGameplayFeedbackGeneration) {
                                            playSpatialSound("enemy_fall", 0.24f, stereoGains);
                                        }
                                    },
                                    ENEMY_IMPACT_FALL_DELAY_MILLISECONDS);
                            break;
                        case PLATFORM_EVENT_LICH_CHARGE_STARTED:
                            playSpatialSound("lich_charge", 0.38f, stereoGains);
                            break;
                        case PLATFORM_EVENT_LICH_IMPACT:
                            playSpatialSound("lich_impact", 0.55f, stereoGains);
                            break;
                        case PLATFORM_EVENT_LICH_DEFEATED:
                            playSpatialSound("lich_fall", 0.28f, stereoGains);
                            break;
                        case PLATFORM_EVENT_CHEST_UNLOCKED:
                            playSpatialSound("chest_unlock", 0.82f, stereoGains);
                            break;
                        case PLATFORM_EVENT_CHEST_OPENED:
                            playSpatialSound("chest_open", 1.0f, stereoGains);
                            break;
                        case PLATFORM_EVENT_TORCH_EXTINGUISHED:
                            playSpatialSound("torch_extinguish", 0.78f, stereoGains);
                            break;
                        case PLATFORM_EVENT_PLAYER_PARRY_SUCCEEDED:
                            playSpatialSound("sword_hit_2", 0.46f, stereoGains);
                            performHaptic(HAPTIC_PARRY);
                            break;
                        default:
                            break;
                    }
                    }
                }
                updateWaterfallLoop();
                if (diagnosticsVisible && ++diagnosticsRefreshTick >= 5) {
                    diagnosticsRefreshTick = 0;
                    refreshDiagnosticsText();
                }
            } catch (final Throwable ignored) {
                // Native startup failures are already surfaced in the diagnostics panel.
            }
            handler.postDelayed(this, 180L);
        }
    };

    private boolean isDebuggableApp() {
        return (getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0;
    }

    @SuppressWarnings("deprecation")
    private void performHaptic(final int cue) {
        if (!resumed || !preferences.getBoolean("haptics_enabled", true)) return;

        try {
            if (vibrator != null && vibrator.hasVibrator()) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    if (cue == HAPTIC_FATAL) {
                        vibrator.vibrate(VibrationEffect.createWaveform(
                                new long[]{0L, 70L, 55L, 120L},
                                new int[]{0, 210, 0, 255},
                                -1));
                    } else {
                        final long duration = cue == HAPTIC_DAMAGE ? 55L : cue == HAPTIC_PARRY ? 34L : 22L;
                        final int amplitude = cue == HAPTIC_DAMAGE ? 180 : cue == HAPTIC_PARRY ? 235 : 90;
                        vibrator.vibrate(VibrationEffect.createOneShot(duration, amplitude));
                    }
                } else if (cue == HAPTIC_FATAL) {
                    vibrator.vibrate(new long[]{0L, 70L, 55L, 120L}, -1);
                } else {
                    vibrator.vibrate(cue == HAPTIC_DAMAGE ? 55L : cue == HAPTIC_PARRY ? 34L : 22L);
                }
                return;
            }
        } catch (final RuntimeException error) {
            Log.w(TAG, "Direct vibration failed; using view haptic fallback.", error);
        }

        surfaceView.performHapticFeedback(
                cue == HAPTIC_FATAL ? HapticFeedbackConstants.LONG_PRESS :
                        cue == HAPTIC_PARRY ? HapticFeedbackConstants.CONTEXT_CLICK : HapticFeedbackConstants.CLOCK_TICK,
                HapticFeedbackConstants.FLAG_IGNORE_VIEW_SETTING);
    }

    static int checkpointId(final String name) {
        if (name == null) return -1;
        switch (name) {
            case "opening": return 0;
            case "skeleton": return 1;
            case "worst-bend": return 2;
            case "lantern-drop": return 3;
            case "skylight": return 4;
            case "yellow": return 5;
            case "blue": return 6;
            case "red": return 7;
            case "green": return 8;
            case "mirror": return 9;
            case "lich": return 10;
            case "finale-roof": return 11;
            case "two-enemy-combat": return 12;
            case "pbr-sword-closeup": return 100;
            case "pbr-torch-fire": return 101;
            case "player-body-grips": return 102;
            case "player-body-forward": return 103;
            case "player-fallback-forward": return 104;
            case "player-fallback-grips": return 105;
            case "player-body-owner-feedback": return 106;
            case "player-body-downward-cut": return 107;
            case "player-body-upward-slice": return 108;
            case "glass-transport": return 109;
            case "glass-fire-transport": return 110;
            case "glass-tinted-transport": return 111;
            case "glass-millimetre-closed": return 112;
            case "glass-edge-fresnel": return 113;
            case "lantern-chest-unlock": return 114;
            case "lantern-glass-production": return 115;
            case "lantern-held-high": return 116;
            case "lantern-held-low": return 117;
            case "lantern-glass-transmission": return 118;
            case "lantern-motion-extreme": return 119;
            case "lantern-sweep-high-forward": return 120;
            case "lantern-sweep-high-backward": return 121;
            case "lantern-sweep-high-left": return 122;
            case "lantern-sweep-high-right": return 123;
            case "lantern-sweep-high-diagonal": return 124;
            case "lantern-sweep-high-opposite": return 125;
            case "lantern-sweep-low-forward": return 126;
            case "lantern-sweep-low-backward": return 127;
            case "lantern-sweep-low-left": return 128;
            case "lantern-sweep-low-right": return 129;
            case "lantern-sweep-high-alt-camera": return 130;
            case "lantern-sweep-low-alt-camera": return 131;
            case "lantern-wall-high": return 132;
            case "lantern-wall-low": return 133;
            case "lantern-held-look-up": return 134;
            default: return -1;
        }
    }

    private void consumeDebugAutomationIntent(final Intent intent) {
        if (intent == null) return;
        if (!isDebuggableApp()) {
            if (intent.getBooleanExtra(EXTRA_DEBUG_CAPTURE, false)) {
                Log.w(TAG, "Rejected debug capture intent in a non-debuggable build.");
            }
            return;
        }
        final int requestedScale = intent.getIntExtra(EXTRA_DEBUG_SCALE, -1);
        if (requestedScale >= 50 && requestedScale <= 100) {
            ProbeBridge.setRenderScale(requestedScale / 100.0f);
        }
        final int requestedCheckpoint = checkpointId(intent.getStringExtra(EXTRA_DEBUG_CHECKPOINT));
        final boolean requestedReplay = intent.getBooleanExtra(EXTRA_DEBUG_REPLAY, false);
        final boolean requestedCapture = intent.getBooleanExtra(EXTRA_DEBUG_CAPTURE, false);
        if (intent.hasExtra(EXTRA_DEBUG_OVERLAY)) {
            developerOverlayVisible = intent.getBooleanExtra(EXTRA_DEBUG_OVERLAY, false);
        }
        final boolean gpuTimingEnabled = intent.getBooleanExtra(EXTRA_DEBUG_GPU_TIMING, true);
        ProbeBridge.setGpuTimingEnabled(gpuTimingEnabled);
        final boolean hasRtLabIntent = intent.getBooleanExtra(EXTRA_DEBUG_RT_LAB, false) ||
                intent.hasExtra(EXTRA_DEBUG_RT_WATERFALL) || intent.hasExtra(EXTRA_DEBUG_RT_ROOF) ||
                intent.hasExtra(EXTRA_DEBUG_RT_DAWN) || intent.hasExtra(EXTRA_DEBUG_RT_FOG) ||
                intent.hasExtra(EXTRA_DEBUG_RT_LIGHT_GROUP) || intent.hasExtra(EXTRA_DEBUG_RT_LIGHT_HUE) ||
                intent.hasExtra(EXTRA_DEBUG_RT_LIGHT_INTENSITY) ||
                intent.hasExtra(EXTRA_DEBUG_RT_FIRE_STRENGTH) ||
                intent.hasExtra(EXTRA_DEBUG_RT_FIRE_TURBULENCE) ||
                intent.hasExtra(EXTRA_DEBUG_RT_FIRE_SMOKE) || intent.hasExtra(EXTRA_DEBUG_RT_WORKLOAD);
        if (hasRtLabIntent) {
            debugRtLabAccess = true;
            rtWaterfallWidthPercent = Math.max(25, Math.min(200,
                    intent.getIntExtra(EXTRA_DEBUG_RT_WATERFALL, rtWaterfallWidthPercent)));
            if (intent.hasExtra(EXTRA_DEBUG_RT_ROOF)) {
                rtRoofOverrideEnabled = true;
                rtRoofOpenPercent = Math.max(0, Math.min(100, intent.getIntExtra(EXTRA_DEBUG_RT_ROOF, 0)));
            }
            if (intent.hasExtra(EXTRA_DEBUG_RT_DAWN)) {
                rtDawnOverrideEnabled = true;
                rtDawnRevealPercent = Math.max(0, Math.min(100, intent.getIntExtra(EXTRA_DEBUG_RT_DAWN, 0)));
            }
            rtFogDensityPercent = Math.max(0, Math.min(200,
                    intent.getIntExtra(EXTRA_DEBUG_RT_FOG, rtFogDensityPercent)));
            rtLightGroup = Math.max(0, Math.min(3,
                    intent.getIntExtra(EXTRA_DEBUG_RT_LIGHT_GROUP, rtLightGroup)));
            rtLightHueDegrees[rtLightGroup] = Math.max(-180, Math.min(180,
                    intent.getIntExtra(EXTRA_DEBUG_RT_LIGHT_HUE, rtLightHueDegrees[rtLightGroup])));
            rtLightIntensityPercent[rtLightGroup] = Math.max(0, Math.min(200,
                    intent.getIntExtra(EXTRA_DEBUG_RT_LIGHT_INTENSITY, rtLightIntensityPercent[rtLightGroup])));
            rtFireStrengthPercent = Math.max(0, Math.min(200,
                    intent.getIntExtra(EXTRA_DEBUG_RT_FIRE_STRENGTH, rtFireStrengthPercent)));
            rtFireTurbulencePercent = Math.max(0, Math.min(200,
                    intent.getIntExtra(EXTRA_DEBUG_RT_FIRE_TURBULENCE, rtFireTurbulencePercent)));
            rtFireSmokePercent = Math.max(0, Math.min(200,
                    intent.getIntExtra(EXTRA_DEBUG_RT_FIRE_SMOKE, rtFireSmokePercent)));
            rtWorkloadPreset = Math.max(0, Math.min(2,
                    intent.getIntExtra(EXTRA_DEBUG_RT_WORKLOAD, rtWorkloadPreset)));
            publishRtSceneTuning();
            publishRtLightTuning();
            publishRtFireTuning();
            ProbeBridge.setRtWorkloadPreset(rtWorkloadPreset);
        }
        if (requestedCheckpoint >= 0 || requestedReplay || hasRtLabIntent) {
            ProbeBridge.markRtLabDebugAutomation();
        }
        if (requestedCheckpoint >= 0) {
            pendingDebugCheckpoint = requestedCheckpoint;
            pendingDebugCapture = requestedCapture;
            pendingDebugReplay = false;
        } else if (requestedReplay) {
            pendingDebugReplay = true;
            pendingDebugCheckpoint = -1;
            pendingDebugCapture = false;
        }
        debugAutomationAutostart = intent.getBooleanExtra(EXTRA_DEBUG_AUTOSTART, false) ||
                requestedCheckpoint >= 0 || requestedReplay;
        if (debugAutomationAutostart) {
            Log.i(TAG, "Accepted debug automation intent: checkpoint=" + requestedCheckpoint +
                    " capture=" + requestedCapture + " replay=" + requestedReplay + " scale=" + requestedScale +
                    " gpuTiming=" + (gpuTimingEnabled ? "enabled" : "disabled") +
                    " rtLab=" + hasRtLabIntent);
        }
    }

    @SuppressLint("UnspecifiedRegisterReceiverFlag") // API 24-32 require the legacy overload.
    private void registerDebugRetryReceiver() {
        debugRetryReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(final Context context, final Intent intent) {
                if (intent == null || !DEBUG_RETRY_ACTION.equals(intent.getAction())) return;
                if (ProbeBridge.getRuntimeState() != 1 ||
                        ProbeBridge.getPlayerLifePhase() != PLAYER_DEAD) {
                    Log.w(TAG, "Rejected debug encounter-retry broadcast outside Dead state.");
                    return;
                }
                Log.i(TAG, "Accepted debug encounter-retry broadcast.");
                retryEncounter();
            }
        };
        final IntentFilter filter = new IntentFilter(DEBUG_RETRY_ACTION);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(debugRetryReceiver, filter, Context.RECEIVER_EXPORTED);
        } else {
            registerReceiver(debugRetryReceiver, filter);
        }
    }

    private void suppressUiForDebugCapture() {
        retryPending = false;
        deathOverlayVisible = false;
        endingOverlayVisible = false;
        endingOverlayDismissed = true;
        debugCaptureUiSuppressed = true;
        developerOverlayVisible = false;
        menuVisible = false;
        diagnosticsVisible = false;
        benchmarkReportVisible = false;
        menuScrim.setVisibility(View.GONE);
        diagnosticsPanel.setVisibility(View.GONE);
        menuButton.setVisibility(View.GONE);
        attackButton.setVisibility(View.GONE);
        parryButton.setVisibility(View.GONE);
        rtStatus.setVisibility(View.GONE);
        vitalityStatus.setVisibility(View.GONE);
        developerOverlay.setVisibility(View.GONE);
        clearTouchState();
    }

    private void applyCheckpointViewPose(final int checkpoint) {
        switch (checkpoint) {
            case 0: viewControls[0] = 0.0f; viewControls[1] = -0.05f; break;
            case 1: viewControls[0] = 0.0f; viewControls[1] = 0.0f; break;
            case 2: viewControls[0] = 0.0f; viewControls[1] = -0.04f; break;
            case 3: viewControls[0] = -1.5707963f; viewControls[1] = -0.08f; break;
            case 4: viewControls[0] = 0.0f; viewControls[1] = 0.22f; break;
            case 5:
            case 6:
            case 7:
            case 8: viewControls[0] = -1.5707963f; viewControls[1] = -0.02f; break;
            case 9: viewControls[0] = -1.5707963f; viewControls[1] = 0.0f; break;
            case 10: viewControls[0] = 2.52f; viewControls[1] = 0.0f; break;
            case 11: viewControls[0] = 1.5707963f; viewControls[1] = 0.28f; break;
            case 12: viewControls[0] = 0.0f; viewControls[1] = 0.0f; break;
            default: break;
        }
    }

    private void initialiseAudio() {
        final AudioAttributes attributes = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
        soundPool = new SoundPool.Builder().setMaxStreams(5).setAudioAttributes(attributes).build();
        soundPool.setOnLoadCompleteListener((pool, soundId, status) -> {
            if (status == 0) {
                synchronized (loadedSounds) {
                    loadedSounds.add(soundId);
                }
                Log.i(TAG, "SFX loaded: " + soundId);
            } else {
                Log.e(TAG, "SFX load failed for id " + soundId + " with status " + status);
            }
        });
        loadSound("ui_select", "audio/filmcow/ui_select.wav");
        loadSound("ui_back", "audio/filmcow/ui_back.wav");
        loadSound("menu_toggle", "audio/filmcow/menu_toggle.wav");
        loadSound("sword_swing_1", "audio/filmcow/sword_swing_1.wav");
        loadSound("sword_swing_2", "audio/filmcow/sword_swing_2.wav");
        loadSound("sword_hit_1", "audio/filmcow/sword_hit_1.wav");
        loadSound("sword_hit_2", "audio/filmcow/sword_hit_2.wav");
        loadSound("enemy_fall", "audio/filmcow/enemy_fall.wav");
        loadSound("player_step_1", "audio/filmcow/player_step_1.wav");
        loadSound("player_step_2", "audio/filmcow/player_step_2.wav");
        loadSound("skeleton_step_1", "audio/filmcow/skeleton_step_1.wav");
        loadSound("skeleton_step_2", "audio/filmcow/skeleton_step_2.wav");
        loadSound("skeleton_attack", "audio/filmcow/skeleton_attack.wav");
        loadSound("lich_charge", "audio/filmcow/lich_charge.wav");
        loadSound("lich_impact", "audio/filmcow/lich_impact.wav");
        loadSound("lich_fall", "audio/filmcow/lich_fall.wav");
        loadSound("lich_hurt", "audio/filmcow/lich_hurt.wav");
        loadSound("chest_unlock", "audio/pixabay/chest_unlock.wav");
        loadSound("chest_open", "audio/pixabay/chest_open.wav");
        loadSound("torch_extinguish", "audio/pixabay/torch_extinguish.wav");
        initialiseWaterfallLoop(attributes);
    }

    private void initialiseWaterfallLoop(final AudioAttributes attributes) {
        final File audioDirectory = new File(getCacheDir(), "alpha_sfx");
        final File stagedLoop = new File(audioDirectory, "waterfall_loop.wav");
        if (!audioDirectory.exists() && !audioDirectory.mkdirs()) {
            Log.e(TAG, "Could not create the waterfall audio cache directory.");
            return;
        }
        try (InputStream source = getAssets().open("audio/pixabay/waterfall_loop.wav");
             FileOutputStream output = new FileOutputStream(stagedLoop, false)) {
            final byte[] buffer = new byte[16 * 1024];
            int read;
            while ((read = source.read(buffer)) != -1) output.write(buffer, 0, read);

            waterfallPlayer = new MediaPlayer();
            waterfallPlayer.setAudioAttributes(attributes);
            waterfallPlayer.setDataSource(stagedLoop.getAbsolutePath());
            waterfallPlayer.setLooping(true);
            waterfallPlayer.setVolume(0.0f, 0.0f);
            waterfallPlayer.prepare();
        } catch (final Exception exception) {
            Log.e(TAG, "Failed to initialise the waterfall loop.", exception);
            if (waterfallPlayer != null) {
                waterfallPlayer.release();
                waterfallPlayer = null;
            }
        }
    }

    private void updateWaterfallLoop() {
        if (waterfallPlayer == null) return;
        final boolean audible = resumed && surfaceStarted && !menuVisible && !diagnosticsVisible &&
                !benchmarkRunning && preferences.getBoolean("sfx_enabled", true);
        if (!audible) {
            waterfallPlayer.setVolume(0.0f, 0.0f);
            if (waterfallPlayer.isPlaying()) waterfallPlayer.pause();
            return;
        }

        final long packedStereoGains = ProbeBridge.getWaterfallStereoGains();
        final float leftScale = clamp(
                Float.intBitsToFloat((int) packedStereoGains), 0.0f, 1.0f);
        final float rightScale = clamp(
                Float.intBitsToFloat((int) (packedStereoGains >>> 32)), 0.0f, 1.0f);
        final float userGain = preferences.getInt("sfx_volume", 70) / 100.0f;
        waterfallPlayer.setVolume(
                clamp(userGain * leftScale, 0.0f, 1.0f),
                clamp(userGain * rightScale, 0.0f, 1.0f));
        if (!waterfallPlayer.isPlaying()) waterfallPlayer.start();
    }

    private void loadSound(final String key, final String assetPath) {
        final File audioDirectory = new File(getCacheDir(), "alpha_sfx");
        final File stagedSound = new File(audioDirectory, key + ".wav");
        if (!audioDirectory.exists() && !audioDirectory.mkdirs()) {
            Log.e(TAG, "Could not create the SFX cache directory.");
            return;
        }
        try (InputStream source = getAssets().open(assetPath);
             FileOutputStream output = new FileOutputStream(stagedSound, false)) {
            final byte[] buffer = new byte[16 * 1024];
            int read;
            while ((read = source.read(buffer)) != -1) output.write(buffer, 0, read);
            final int soundId = soundPool.load(stagedSound.getAbsolutePath(), 1);
            if (soundId != 0) sounds.put(key, soundId);
            else Log.e(TAG, "SoundPool rejected " + assetPath);
        } catch (final Exception exception) {
            Log.e(TAG, "Failed to stage " + assetPath, exception);
        }
    }

    private void playSound(final String key, final float mixGain) {
        playSound(key, mixGain, 1.0f, 1.0f);
    }

    private void playSpatialSound(final String key, final float mixGain, final long packedStereoGains) {
        final float left = clamp(Float.intBitsToFloat((int) packedStereoGains), 0.0f, 1.0f);
        final float right = clamp(Float.intBitsToFloat((int) (packedStereoGains >>> 32)), 0.0f, 1.0f);
        playSound(key, mixGain, left, right);
    }

    private void playSound(final String key, final float mixGain, final float leftScale, final float rightScale) {
        if (soundPool == null || !preferences.getBoolean("sfx_enabled", true)) return;
        final Integer soundId = sounds.get(key);
        if (soundId == null) return;
        synchronized (loadedSounds) {
            if (!loadedSounds.contains(soundId)) {
                Log.w(TAG, "SFX not ready: " + key);
                return;
            }
        }
        final float userGain = preferences.getInt("sfx_volume", 70) / 100.0f;
        final float leftGain = clamp(userGain * mixGain * leftScale, 0.0f, 1.0f);
        final float rightGain = clamp(userGain * mixGain * rightScale, 0.0f, 1.0f);
        final int streamId = soundPool.play(soundId, leftGain, rightGain, 1, 0, 1.0f);
        if (streamId == 0) Log.e(TAG, "SoundPool failed to play " + key);
    }

    private LinearLayout createPanel(final String title, final String eyebrow) {
        final LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(dp(28), dp(24), dp(28), dp(28));
        final GradientDrawable background = new GradientDrawable();
        background.setColor(0xF21A1510);
        background.setCornerRadius(dp(4));
        background.setStroke(dp(1), 0xFF8A6330);
        panel.setBackground(background);

        final TextView eyebrowView = new TextView(this);
        eyebrowView.setText(eyebrow);
        eyebrowView.setTextColor(0xFFFFB84F);
        eyebrowView.setTextSize(10);
        eyebrowView.setTypeface(Typeface.SANS_SERIF, Typeface.BOLD);
        eyebrowView.setLetterSpacing(0.12f);
        panel.addView(eyebrowView, matchWrap());

        final TextView titleView = new TextView(this);
        titleView.setText(title);
        titleView.setTextColor(0xFFFFE8C3);
        titleView.setTextSize(22);
        titleView.setTypeface(Typeface.create(Typeface.SERIF, Typeface.BOLD));
        titleView.setPadding(0, dp(5), 0, dp(14));
        panel.addView(titleView, matchWrap());
        return panel;
    }

    private void attachPanel(final LinearLayout panel) {
        final ScrollView scroller = new ScrollView(this);
        scroller.setFillViewport(false);
        scroller.addView(panel, new ScrollView.LayoutParams(ScrollView.LayoutParams.MATCH_PARENT, ScrollView.LayoutParams.WRAP_CONTENT));
        final int screenWidth = getResources().getDisplayMetrics().widthPixels;
        final FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(Math.min(dp(520), screenWidth - dp(32)), FrameLayout.LayoutParams.WRAP_CONTENT);
        params.gravity = Gravity.CENTER;
        params.setMargins(0, dp(16), 0, dp(16));
        menuScrim.addView(scroller, params);
    }

    private void addBody(final LinearLayout panel, final String text) {
        final TextView body = new TextView(this);
        body.setText(text);
        body.setTextColor(0xFFD8C9B2);
        body.setTextSize(13);
        body.setLineSpacing(0.0f, 1.15f);
        body.setPadding(0, 0, 0, dp(14));
        panel.addView(body, matchWrap());
    }

    private void addLinkedBody(final LinearLayout panel, final String text) {
        final TextView body = new TextView(this);
        body.setText(text);
        body.setTextColor(0xFFD8C9B2);
        body.setLinkTextColor(0xFFFFB84F);
        body.setTextSize(13);
        body.setLineSpacing(0.0f, 1.15f);
        body.setPadding(0, 0, 0, dp(14));
        Linkify.addLinks(body, Linkify.WEB_URLS);
        body.setMovementMethod(LinkMovementMethod.getInstance());
        panel.addView(body, matchWrap());
    }

    private void addMenuButton(final LinearLayout panel, final String text, final Runnable action) {
        panel.addView(createMenuButton(text, action), menuButtonLayoutParams());
    }

    private void addMenuButtonRow(final LinearLayout panel,
                                  final String leftText, final Runnable leftAction,
                                  final String rightText, final Runnable rightAction) {
        if (getResources().getDisplayMetrics().widthPixels < getResources().getDisplayMetrics().heightPixels) {
            addMenuButton(panel, leftText, leftAction);
            addMenuButton(panel, rightText, rightAction);
            return;
        }
        final LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        final LinearLayout.LayoutParams left = new LinearLayout.LayoutParams(0, dp(50), 1.0f);
        final LinearLayout.LayoutParams right = new LinearLayout.LayoutParams(0, dp(50), 1.0f);
        right.leftMargin = dp(8);
        row.addView(createMenuButton(leftText, leftAction), left);
        row.addView(createMenuButton(rightText, rightAction), right);
        final LinearLayout.LayoutParams rowParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dp(50));
        rowParams.topMargin = dp(7);
        panel.addView(row, rowParams);
    }

    private Button createMenuButton(final String text, final Runnable action) {
        final Button button = new Button(this);
        button.setText(text);
        button.setAllCaps(false);
        button.setTextSize(15);
        button.setTypeface(Typeface.SANS_SERIF, Typeface.BOLD);
        button.setGravity(Gravity.CENTER_VERTICAL | Gravity.START);
        button.setPadding(dp(18), 0, dp(18), 0);
        styleActionButton(button, 0xFF2B2117, 0xFFFFDCA3);
        button.setOnClickListener(view -> action.run());
        return button;
    }

    private LinearLayout.LayoutParams menuButtonLayoutParams() {
        final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dp(50));
        params.topMargin = dp(7);
        return params;
    }

    private interface IntSettingListener { void onChanged(int value); }

    private void addSlider(final LinearLayout panel, final String title, final int value, final int min, final int max, final IntSettingListener listener) {
        final TextView label = new TextView(this);
        label.setText(title + "  " + value + "%");
        label.setTextColor(0xFFFFE5BA);
        label.setTextSize(15);
        label.setPadding(0, dp(8), 0, 0);
        panel.addView(label, matchWrap());
        final SeekBar slider = new SeekBar(this);
        slider.setMax(max - min);
        slider.setProgress(value - min);
        slider.setMinimumHeight(dp(48));
        slider.setContentDescription(title);
        slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(final SeekBar seekBar, final int progress, final boolean fromUser) {
                final int current = progress + min;
                label.setText(title + "  " + current + "%");
                listener.onChanged(current);
            }
            @Override public void onStartTrackingTouch(final SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(final SeekBar seekBar) {}
        });
        panel.addView(slider, matchWrap());
    }

    private void addRtLabSlider(final LinearLayout panel, final String title, final int value,
                                final int min, final int max, final String suffix,
                                final IntSettingListener listener) {
        final TextView label = new TextView(this);
        label.setText(getString(R.string.rt_lab_slider_value, title, value, suffix));
        label.setTextColor(0xFFFFE5BA);
        label.setTextSize(15);
        label.setPadding(0, dp(8), 0, 0);
        panel.addView(label, matchWrap());
        final SeekBar slider = new SeekBar(this);
        slider.setMax(max - min);
        slider.setProgress(value - min);
        slider.setMinimumHeight(dp(48));
        slider.setContentDescription(title);
        slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(final SeekBar seekBar, final int progress, final boolean fromUser) {
                final int current = progress + min;
                label.setText(getString(R.string.rt_lab_slider_value, title, current, suffix));
                if (fromUser) listener.onChanged(current);
            }
            @Override public void onStartTrackingTouch(final SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(final SeekBar seekBar) {}
        });
        panel.addView(slider, matchWrap());
    }

    private void styleActionButton(final Button button, final int fill, final int text) {
        final GradientDrawable background = new GradientDrawable();
        background.setColor(fill);
        background.setCornerRadius(dp(3));
        background.setStroke(dp(1), 0xFF8A6330);
        button.setBackground(background);
        button.setTextColor(text);
        button.setMinHeight(dp(48));
    }
    private LinearLayout.LayoutParams matchWrap() {
        return new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
    }

    private void pushViewControls() {
        ProbeBridge.setViewControls(viewControls[0], viewControls[1], viewControls[2], viewControls[7], viewControls[8]);
    }

    private void clearTouchState() {
        activePointers[0] = -1;
        activePointers[1] = -1;
        viewControls[7] = 0.0f;
        viewControls[8] = 0.0f;
        pushViewControls();
        updateContextualControls(false);
    }

    private void updateContextualControls(final boolean controlsAllowed) {
        if (!controlsAllowed) {
            interactButton.setVisibility(View.GONE);
            toggleHeldLightPoseButton.setVisibility(View.GONE);
            return;
        }
        final int contextualState = ProbeBridge.getContextualControlState();
        final int chestPrompt = (contextualState & CHEST_PROMPT_MASK) >> CHEST_PROMPT_SHIFT;
        final boolean interactEnabled = (contextualState & CONTEXTUAL_INTERACT) != 0;
        final int interactLabel;
        switch (chestPrompt) {
            case CHEST_PROMPT_LOCKED:
                interactLabel = R.string.chest_locked_until_lich_defeated;
                break;
            case CHEST_PROMPT_OPEN:
                interactLabel = R.string.open_chest;
                break;
            case CHEST_PROMPT_OPENING:
                interactLabel = R.string.chest_opening;
                break;
            case CHEST_PROMPT_CLAIM:
                interactLabel = R.string.take_lantern;
                break;
            case CHEST_PROMPT_NONE:
            default:
                interactLabel = 0;
                break;
        }
        if (interactLabel == 0) {
            interactButton.setVisibility(View.GONE);
        } else {
            interactButton.setText(interactLabel);
            interactButton.setContentDescription(getString(interactLabel));
            interactButton.setEnabled(interactEnabled);
            interactButton.setVisibility(View.VISIBLE);
        }
        final boolean showRaise = (contextualState & CONTEXTUAL_RAISE) != 0;
        final boolean showLower = (contextualState & CONTEXTUAL_LOWER) != 0;
        if (!showRaise && !showLower) {
            toggleHeldLightPoseButton.setVisibility(View.GONE);
            return;
        }
        final int label = showRaise ? R.string.raise_lantern : R.string.lower_lantern;
        toggleHeldLightPoseButton.setText(label);
        toggleHeldLightPoseButton.setContentDescription(getString(label));
        toggleHeldLightPoseButton.setVisibility(View.VISIBLE);
    }

    private boolean stageAsset(final String assetPath, final String fileName) {
        final File destination = new File(getFilesDir(), fileName);
        final File parent = destination.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) return false;
        try (InputStream source = getAssets().open(assetPath);
             FileOutputStream output = new FileOutputStream(destination, false)) {
            final byte[] buffer = new byte[64 * 1024];
            int read;
            while ((read = source.read(buffer)) != -1) output.write(buffer, 0, read);
            return true;
        } catch (final Exception ignored) {
            return false;
        }
    }

    private int dp(final int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private static float clamp(final float value, final float min, final float max) {
        return Math.max(min, Math.min(max, value));
    }

    private void enterImmersiveMode() {
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                View.SYSTEM_UI_FLAG_FULLSCREEN |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
    }

    @Override
    protected void onActivityResult(final int requestCode, final int resultCode, final Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQUEST_SAVE_BENCHMARK || resultCode != RESULT_OK ||
                data == null || data.getData() == null) {
            return;
        }
        try (OutputStream output = getContentResolver().openOutputStream(data.getData(), "wt")) {
            if (output == null) throw new IllegalStateException("Document provider returned no output stream.");
            output.write(latestBenchmarkReport.getBytes(StandardCharsets.UTF_8));
            Toast.makeText(this, R.string.report_saved, Toast.LENGTH_SHORT).show();
        } catch (final Exception error) {
            Log.e(TAG, "Failed to save benchmark report.", error);
            Toast.makeText(this, R.string.report_save_failed, Toast.LENGTH_LONG).show();
        }
    }

    @Override
    public void onBackPressed() {
        if (rtLabVisible) {
            closeRtLab();
            return;
        }
        if (deathOverlayVisible) {
            return;
        }
        if (endingOverlayVisible) {
            continueAfterEnding();
            return;
        }

        if (benchmarkRunning) {
            ProbeBridge.cancelBenchmark();
            benchmarkRunning = false;
            playSound("ui_back", 0.18f);
            showMainMenu(false);
        } else if (benchmarkReportVisible) {
            playSound("ui_back", 0.18f);
            showMainMenu(false);
        } else if (diagnosticsVisible) {
            diagnosticsVisible = false;
            diagnosticsPanel.setVisibility(View.GONE);
            showMainMenu(false);
        } else if (!menuVisible) {
            playSound("menu_toggle", 0.20f);
            showMainMenu(false);
        } else {
            if (firstMenu) {
                finishAndRemoveTask();
            } else {
                playSound("ui_back", 0.18f);
                hideMenu();
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        resumed = true;
        enterImmersiveMode();
        startSurfaceIfReady();
        if (rtLabVisible) handler.post(refreshRtLabTelemetry);
        scheduleStartupUpdateCheck();
        if (pendingUpdateDecision != null) {
            final String decision = pendingUpdateDecision;
            final boolean manualRequest = pendingUpdateManualRequest;
            pendingUpdateDecision = null;
            handler.post(() -> presentUpdateDecision(decision, manualRequest));
        }
    }

    @Override
    protected void onNewIntent(final Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        consumeDebugAutomationIntent(intent);
        if (debugRtLabAccess && menuVisible && !deathOverlayVisible && !endingOverlayVisible) {
            showMainMenu(false);
        }
    }

    @Override
    protected void onPause() {
        resumed = false;
        handler.removeCallbacks(runStartupUpdateCheck);
        startupUpdateCheckScheduled = false;
        handler.removeCallbacks(refreshRtLabTelemetry);
        if (waterfallPlayer != null) {
            waterfallPlayer.setVolume(0.0f, 0.0f);
            if (waterfallPlayer.isPlaying()) waterfallPlayer.pause();
        }
        ++delayedGameplayFeedbackGeneration;
        if (vibrator != null) vibrator.cancel();
        if (deathOverlayVisible || retryPending || endingOverlayVisible) {
            retryPending = false;
            deathOverlayVisible = false;
            endingOverlayVisible = false;
            endingOverlayDismissed = false;
            lastPlayerLifePhase = PLAYER_ALIVE;
            updateVitalityHud(3);
            showMainMenu(false);
        }
        if (benchmarkRunning) {
            ProbeBridge.cancelBenchmark();
            benchmarkRunning = false;
            showMainMenu(false);
        }
        ProbeBridge.setSimulationPaused(true);
        stopSurface();
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        handler.removeCallbacksAndMessages(null);
        updateExecutor.shutdownNow();
        if (vibrator != null) vibrator.cancel();
        if (debugRetryReceiver != null) {
            unregisterReceiver(debugRetryReceiver);
            debugRetryReceiver = null;
        }
        stopSurface();
        if (soundPool != null) soundPool.release();
        if (waterfallPlayer != null) {
            waterfallPlayer.release();
            waterfallPlayer = null;
        }
        super.onDestroy();
    }
}
