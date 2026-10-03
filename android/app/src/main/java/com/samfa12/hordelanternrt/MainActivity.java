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
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.ColorDrawable;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.text.method.LinkMovementMethod;
import android.text.InputType;
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
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.HorizontalScrollView;
import android.widget.PopupMenu;
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
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Locale;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.RejectedExecutionException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;
import java.util.UUID;

import org.json.JSONObject;

public class MainActivity extends Activity {
    private static final String TAG = "HordeLanternAudio";
    private static final String PREFS = "horde_lantern_alpha_settings";
    private static final String PREF_MUSIC_VOLUME = "music_volume";
    static final String PREF_RENDER_SCALE = "render_scale";
    static final int DEFAULT_ANDROID_RT_RENDER_SCALE_PERCENT = 75;
    private static final String PREF_RT_LAB_UNLOCKED = "rt_lab_unlocked";
    private static final String REPORT_DIRECTORY = "reports";
    private static final String ACTION_BENCHMARK = "com.samfa12.hordelanternrt.action.BENCHMARK";
    private static final String EXTRA_BENCHMARK_RUN_ID = "horde.benchmark.run_id";
    private static final String EXTRA_BENCHMARK_WORKLOAD = "horde.benchmark.workload";
    private static final String DEFAULT_BENCHMARK_WORKLOAD = "showcase-route-v1";
    private static final long BENCHMARK_AUTOMATION_TIMEOUT_MS = 15L * 60L * 1000L;
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
    private static final String EXTRA_REQUIRE_RAYQUERY_COMPUTE = "horde_require_rayquery_compute";
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
    private static final int REQUEST_SAVE_PLAYTEST = 7102;
    private final ThreadPoolExecutor reportExecutor = new ThreadPoolExecutor(1, 1, 0L,
            TimeUnit.MILLISECONDS, new ArrayBlockingQueue<>(2), runnable -> {
                Thread thread = new Thread(runnable, "HordePlaytestReport");
                thread.setDaemon(true);
                return thread;
            }, new ThreadPoolExecutor.AbortPolicy());
    private PlaytestReportExport playtestExport;
    private PlaytestReportSubmission playtestSubmission;
    private AlertDialog playtestDecisionDialog;
    private boolean playtestReportVisible;
    private long playtestPickerToken;
    private long playtestFormGeneration;
    private long playtestCaptureToken;
    private long playtestCaptureStartedAt;
    private PlaytestReportVerification.Handle playtestVerification;
    private byte[] playtestPreparedPng;
    private Bitmap playtestPreviewBitmap;
    private String playtestRemoteReportId;
    private String playtestRemoteCapturedAt;
    private TextView playtestStatus;
    private Button playtestSave, playtestEdit, playtestRemotePrepare, playtestRemoteSend;
    private EditText playtestNote;
    private Spinner playtestCategory, playtestImpact;
    private CheckBox playtestConsent, playtestContext, playtestRemoteConsent,
            playtestRemoteContext, playtestRemoteScreenshot;
    private ImageView playtestPreview;
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
    private static final int PLATFORM_EVENT_KEEPER_REVEAL_STARTED = 17;
    private static final int PLATFORM_EVENT_KEEPER_WARNING = 18;
    private static final int PLATFORM_EVENT_KEEPER_COMBAT_READY = 19;
    private static final int PLATFORM_EVENT_SKELETON_INCIDENTAL = 20;
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
    private static final int CHEST_PROMPT_UNLOCKING = 5;

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
    private TextView keeperRevealTitle;
    private Button attackButton;
    private Button parryButton;
    private Button interactButton;
    private Button toggleHeldLightPoseButton;
    private boolean parryRequestedOnTouchDown;
    private SoundPool soundPool;
    private HordeAmbiencePlayback waterfallPlayback;
    private volatile HordeMusicPlayback musicPlayback;
    private Vibrator vibrator;
    private String reportText = "";
    private boolean resumed;
    private boolean surfaceAvailable;
    private boolean surfaceStarted;
    private long surfaceRequestGeneration;
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
    private boolean benchmarkStatusExpanded;
    private boolean benchmarkReportVisible;
    private String latestBenchmarkReport = "";
    private String benchmarkAutomationId;
    private String benchmarkAutomationWorkload = DEFAULT_BENCHMARK_WORKLOAD;
    private boolean benchmarkAutomationPending;
    private boolean benchmarkAutomationFinishing;
    private long benchmarkAutomationStartedAt;
    private BroadcastReceiver debugRetryReceiver;
    private boolean deathOverlayVisible;
    private boolean endingOverlayVisible;
    private boolean endingOverlayDismissed;
    private boolean rtLabUnlocked;
    private boolean rtLabNewlyUnlocked;
    private boolean debugRtLabAccess;
    private boolean rtLabVisible;
    private boolean graphicsVisible, graphicsCloseAfterRevert, graphicsRecovering, graphicsAwaitingRestore;
    private GraphicsPreferences.Values graphicsConfirmed, graphicsDraft, graphicsSubmitted;
    private boolean graphicsBusy;
    private boolean interfaceVisible;
    private String graphicsRecoveryNotice;
    private boolean graphicsPreviewWanted, graphicsPreviewPaused, graphicsPreviewMotion, graphicsSceneRestoring;
    private int graphicsPreviewCamera;
    private GraphicsPreferences.Values graphicsOriginalPreviewDraft;
    private GraphicsPreferences.Values graphicsRestoreDraftAfterPreview;
    private int graphicsPreviewChoice = GraphicsPreviewOptions.RESOLUTION;
    private boolean graphicsPreviewImageOnly;
    private final Button[] graphicsOptionButtons = new Button[5];
    private Button graphicsDetailsButton, graphicsImageButton, graphicsControlsButton;
    private double graphicsPreviewPerformanceEpochFloor;
    private long graphicsPreviewPerformanceGeneration, graphicsNextFpsUpdate;
    private double graphicsDisplayedFps = Double.NaN, graphicsDisplayedFpsEpoch;
    private PopupMenu graphicsOptionsPopup;
    private double graphicsPreviewModalEpoch;
    private String graphicsLiveChoiceError;
    private AlertDialog graphicsDetailsDialog;
    private TextView graphicsDetailsTelemetry;
    private LinearLayout graphicsDetailsPanel;
    private String graphicsPreviewDetailsText = "Waiting for current RT preview presentation.";
    private double[] graphicsPreviewDetailsSamples = new double[9];
    private PreviewTimingGraphView graphicsGraph;
    private LinearLayout graphicsPanel;
    private long graphicsRequestSerial, graphicsPollTime, graphicsConfirmationStarted;
    private TextView graphicsTelemetry;
    private Button graphicsApply, graphicsConfirm, graphicsRevert, graphicsBack;
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
    static int renderScalePercent(final SharedPreferences preferences) {
        return preferences.getInt(PREF_RENDER_SCALE,
                DEFAULT_ANDROID_RT_RENDER_SCALE_PERCENT);
    }

    static void persistRenderScaleSelection(final SharedPreferences preferences,
                                            final int percentage) {
        preferences.edit().putInt(PREF_RENDER_SCALE, percentage).apply();
    }

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
        graphicsConfirmed = GraphicsPreferences.confirmed(preferences);
        graphicsRecovering = GraphicsPreferences.hasPending(preferences);
        if (graphicsRecovering) {
            final GraphicsPreferences.Values retained = GraphicsPreferences.retainedCandidate(preferences);
            graphicsRecoveryNotice = "An unconfirmed graphics change was interrupted. Restoring " + graphicsConfirmed.scale +
                    "% / water " + waterName(graphicsConfirmed.water) + ". Retained request: " + retained.scale +
                    "% / water " + waterName(retained.water) + "; it has not been confirmed.";
        }
        setNativeGraphics(graphicsConfirmed);
        if (!consumeBenchmarkAutomationIntent(getIntent(), true)) consumeDebugAutomationIntent(getIntent());

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
        keeperRevealTitle = findViewById(R.id.keeper_reveal_title);
        final Button diagnosticsBack = findViewById(R.id.diagnostics_back);

        styleActionButton(menuButton, 0xCC1A1713, 0xFFFFD28A);
        styleActionButton(attackButton, 0xDD5B210D, 0xFFFFE0A3);
        styleActionButton(parryButton, 0xDD263B42, 0xFFE5F7FF);
        styleActionButton(interactButton, 0xDD5C4216, 0xFFFFE5A8);
        styleActionButton(toggleHeldLightPoseButton, 0xDD173E34, 0xFFE2FFF0);
        applyInterfacePresentation();
        findViewById(R.id.root).setOnApplyWindowInsetsListener((view,insets) -> {
            if(menuVisible) applyInterfacePresentation(); // Never move an action under a held finger.
            return insets;
        });
        styleActionButton(diagnosticsBack, 0xCC211B15, 0xFFFFD28A);
        menuButton.setContentDescription(getString(R.string.menu));
        attackButton.setContentDescription(getString(R.string.swing));
        parryButton.setContentDescription(getString(R.string.parry));
        interactButton.setContentDescription(getString(R.string.interact));
        toggleHeldLightPoseButton.setContentDescription(getString(R.string.lower_lantern));
        updateVitalityHud(3);
        rtStatus.setOnClickListener(view -> { if(!benchmarkRunning) showDiagnostics(false); });
        if (isDebuggableApp()) {
            rtStatus.setOnLongClickListener(view -> {
                developerOverlayVisible = !developerOverlayVisible;
                if (!developerOverlayVisible) developerOverlay.setVisibility(View.GONE);
                return true;
            });
            registerDebugRetryReceiver();
        }

        initialiseAudio();
        musicPlayback = new HordeMusicPlayback(this, musicVolumePercent());
        menuButton.setOnClickListener(view -> {
            playSound("menu_toggle", 0.20f);
            showMainMenu(false);
        });
        attackButton.setOnClickListener(view -> {
            if (menuVisible || diagnosticsVisible || deathOverlayVisible || ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) != 1) return;
            ProbeBridge.requestAttack();
        });
        interactButton.setOnClickListener(view -> {
            if (menuVisible || diagnosticsVisible || deathOverlayVisible ||
                    endingOverlayVisible || ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) != 1) return;
            ProbeBridge.requestInteract();
        });
        toggleHeldLightPoseButton.setOnClickListener(view -> {
            if (menuVisible || diagnosticsVisible || deathOverlayVisible ||
                    endingOverlayVisible || ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) != 1) return;
            ProbeBridge.requestToggleHeldLightPose();
        });
        parryButton.setOnClickListener(view -> {
            if (parryRequestedOnTouchDown) {
                parryRequestedOnTouchDown = false;
                return;
            }
            if (menuVisible || diagnosticsVisible || deathOverlayVisible || ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) != 1) return;
            ProbeBridge.requestParry();
        });
        parryButton.setOnTouchListener((view, event) -> {
            switch (event.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    parryRequestedOnTouchDown = true;
                    view.setPressed(true);
                    if (!menuVisible && !diagnosticsVisible && !deathOverlayVisible && ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) == 1) {
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
            if (!stageAsset("textures/environment/runtime/night-storm.android.ktx2", "night-storm.android.ktx2") ||
                    !stageAsset("textures/environment/runtime/asset.manifest.json", "textures/environment/runtime/asset.manifest.json"))
                throw new IllegalStateException("Required native environment runtime assets could not be staged.");
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
                    && stageAsset("models/player/viewmodel/runtime/asset.manifest.json", "models/player/viewmodel/runtime/asset.manifest.json")
                    && stageAsset("models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb", "models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb")
                    && stageAsset("textures/props/runtime/asset.manifest.json", "textures/props/runtime/asset.manifest.json")
                    && stageAsset("textures/props/runtime/base-color.android.ktx2", "textures/props/runtime/base-color.android.ktx2")
                    && stageAsset("textures/props/runtime/normal.android.ktx2", "textures/props/runtime/normal.android.ktx2")
                    && stageAsset("textures/props/runtime/orm.android.ktx2", "textures/props/runtime/orm.android.ktx2")
                    && stageAsset("textures/props/runtime/emissive.android.ktx2", "textures/props/runtime/emissive.android.ktx2");
            if (!heldItemsStaged) throw new IllegalStateException("Required production held-item/player assets could not be staged.");
            final boolean collapseStaged = stageAsset("models/world/runtime/collapsed-entry/asset.manifest.json",
                    "models/world/runtime/collapsed-entry/asset.manifest.json") &&
                    stageAsset("models/world/runtime/collapsed-entry/collapsed-entry-lod0.runtime.glb",
                    "models/world/runtime/collapsed-entry/collapsed-entry-lod0.runtime.glb");
            if (!collapseStaged) throw new IllegalStateException("Required collapsed-entry runtime assets could not be staged.");
            if (BuildConfig.VIEWMODEL_CANDIDATE && !stageAsset(
                    "models/player/viewmodel/runtime/candidate-receipt.json",
                    "models/player/viewmodel/runtime/candidate-receipt.json")) {
                throw new IllegalStateException("Opt-in viewmodel candidate receipt could not be staged.");
            }
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
        if (!resumed || !surfaceAvailable || surfaceRequestGeneration != 0 || currentSurface == null) return;
        try {
            surfaceRequestGeneration = ProbeBridge.startDiagnosticSurface(currentSurface, getFilesDir().getAbsolutePath());
            surfaceStarted = false; // Accepted/pending is distinct from an RT-presented frame.
            setGameplayPaused(menuVisible || diagnosticsVisible);
            if (surfaceRequestGeneration == 0) {
                reportTextView.append("\n\nRenderer surface failed to start.");
                showDiagnostics(true);
            }
        } catch (final Throwable error) {
            reportTextView.append("\n\nRenderer surface failure: " + error.getMessage());
            showDiagnostics(true);
        }
    }

    private void stopSurface() {
        setBenchmarkStatusExpanded(false);
        if (musicPlayback != null) musicPlayback.setSuspended(true);
        final long generation = surfaceRequestGeneration;
        surfaceRequestGeneration = 0;
        surfaceStarted = false;
        if (generation != 0) ProbeBridge.stopDiagnosticSurface(generation); // Cancels pending starts too; no join.
    }

    private void configureTouchControls() {
        surfaceView.setOnTouchListener((view, event) -> {
            if (menuVisible || diagnosticsVisible || deathOverlayVisible || ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) != 1) return true;
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
        interfaceVisible=false;
        setBenchmarkStatusExpanded(false);
        rtLabVisible = false;
        rtLabTelemetry = null;
        handler.removeCallbacks(refreshRtLabTelemetry);
        benchmarkReportVisible = false;
        diagnosticsVisible = false;
        diagnosticsPanel.setVisibility(View.GONE);
        menuVisible = true;
        setGameplayPaused(true);
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
        addMenuButton(panel, getString(R.string.playtest_report), this::showPlaytestReport);
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
        if (ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) != 1 || !(benchmarkAutomationId == null
                ? ProbeBridge.requestBenchmark()
                : ProbeBridge.requestBenchmarkWithIdAndWorkload(
                        benchmarkAutomationId, benchmarkAutomationWorkload))) {
            Toast.makeText(this, R.string.benchmark_unavailable, Toast.LENGTH_LONG).show();
            return;
        }
        benchmarkRunning = true;
        benchmarkAutomationPending = false;
        latestBenchmarkReport = "";
        firstMenu = false;
        hideMenu();
        setBenchmarkStatusExpanded(benchmarkAutomationId == null);
        menuButton.setVisibility(View.GONE);
        attackButton.setVisibility(View.GONE);
        parryButton.setVisibility(View.GONE);
        rtStatus.setVisibility(View.VISIBLE);
        vitalityStatus.setVisibility(View.GONE);
        rtStatus.setText(R.string.benchmark_starting);
    }

    // This Release-safe route does not grant any Debug checkpoint or quality control.
    static String benchmarkAutomationRequestId(final Intent intent) {
        if (intent == null || !ACTION_BENCHMARK.equals(intent.getAction())) return null;
        final String id = intent.getStringExtra(EXTRA_BENCHMARK_RUN_ID);
        if (id == null || !id.matches("[A-Za-z0-9_-]{1,64}")) {
            throw new IllegalArgumentException("Benchmark requires a safe, unique run ID.");
        }
        if (intent.getExtras() != null) {
            for (final String key : intent.getExtras().keySet()) {
                if (key != null && (key.startsWith("horde.debug.") || EXTRA_REQUIRE_RAYQUERY_COMPUTE.equals(key))) {
                    throw new IllegalArgumentException("Benchmark and Debug automation cannot be combined.");
                }
            }
        }
        return id;
    }

    static String benchmarkAutomationWorkload(final Intent intent) {
        if (intent == null || !ACTION_BENCHMARK.equals(intent.getAction())) return null;
        benchmarkAutomationRequestId(intent);
        if (!intent.hasExtra(EXTRA_BENCHMARK_WORKLOAD)) return DEFAULT_BENCHMARK_WORKLOAD;
        final String workload;
        try {
            workload = intent.getStringExtra(EXTRA_BENCHMARK_WORKLOAD);
        } catch (final ClassCastException error) {
            throw new IllegalArgumentException("Benchmark workload must be a string.", error);
        }
        if (!isAllowedBenchmarkWorkload(workload)) {
            throw new IllegalArgumentException("Benchmark workload is not allowlisted.");
        }
        return workload;
    }

    private static boolean isAllowedBenchmarkWorkload(final String workload) {
        return DEFAULT_BENCHMARK_WORKLOAD.equals(workload) ||
                "lantern-held-high-v1".equals(workload) ||
                "lantern-held-low-v1".equals(workload) ||
                "lantern-grazing-v1".equals(workload) ||
                "lantern-motion-extreme-v1".equals(workload) ||
                "lantern-reveal-sequence-v1".equals(workload);
    }

    private boolean consumeBenchmarkAutomationIntent(final Intent intent, final boolean freshLaunch) {
        if (intent == null || !ACTION_BENCHMARK.equals(intent.getAction())) return false;
        try {
            final String id = benchmarkAutomationRequestId(intent);
            final String workload = benchmarkAutomationWorkload(intent);
            // Consume even a duplicate request; recreation must not replay it.
            intent.setAction(Intent.ACTION_MAIN);
            intent.removeExtra(EXTRA_BENCHMARK_RUN_ID);
            intent.removeExtra(EXTRA_BENCHMARK_WORKLOAD);
            if (benchmarkAutomationId != null || benchmarkRunning) {
                Log.w(TAG, "Rejected benchmark automation while another run is active.");
                return true;
            }
            benchmarkAutomationId = id;
            benchmarkAutomationWorkload = workload;
            benchmarkAutomationPending = true;
            benchmarkAutomationStartedAt = SystemClock.elapsedRealtime();
            ProbeBridge.setRequiredRayQueryCompute(false);
            handler.removeCallbacks(runStartupUpdateCheck);
            startupUpdateCheckScheduled = false;
        } catch (final IllegalArgumentException error) {
            Log.e(TAG, "HORDE_BENCHMARK_EXPORT status=rejected " + error.getMessage());
            if (freshLaunch) handler.post(this::finishAndRemoveTask);
        }
        return true;
    }

    private void finishBenchmarkAutomation(final int nativeStatus) {
        if (benchmarkAutomationId == null || benchmarkAutomationFinishing) return;
        benchmarkAutomationFinishing = true;
        benchmarkAutomationPending = false;
        benchmarkRunning = false;
        setBenchmarkStatusExpanded(false);
        if (nativeStatus != 2) ProbeBridge.cancelBenchmark();
        final String runId = benchmarkAutomationId;
        final String workload = benchmarkAutomationWorkload;
        final File privateReports = new File(getFilesDir(), REPORT_DIRECTORY);
        final File externalFiles = getExternalFilesDir(null);
        new Thread(() -> {
            try {
                final BenchmarkAutomationExport.Result result = BenchmarkAutomationExport.export(
                        privateReports, externalFiles, runId, workload, nativeStatus);
                Log.i(TAG, "HORDE_BENCHMARK_EXPORT run_id=" + runId +
                        " status=" + (result.successful ? "complete" : "invalid") +
                        " directory=" + result.directory.getAbsolutePath() +
                        " detail=" + result.detail);
            } catch (final Exception error) {
                Log.e(TAG, "HORDE_BENCHMARK_EXPORT run_id=" + runId + " status=export-failed", error);
            } finally {
                handler.post(() -> {
                    if (!isDestroyed() && !isFinishing()) finishAndRemoveTask();
                });
            }
        }, "horde-benchmark-export").start();
    }

    private void showBenchmarkReport(final boolean completed) {
        benchmarkRunning = false;
        setBenchmarkStatusExpanded(false);
        benchmarkReportVisible = true;
        menuVisible = true;
        diagnosticsVisible = false;
        setGameplayPaused(true);
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

    private void showPlaytestReport() {
        // Entry is only from the existing paused menu; no gameplay/sound authority changes.
        playtestReportVisible = true;
        ++playtestFormGeneration;
        playtestExport = new PlaytestReportExport();
        playtestSubmission = newPlaytestSubmission();
        menuScrim.removeAllViews();
        final LinearLayout panel = createPanel(getString(R.string.playtest_report),
                getString(R.string.playtest_local_only));
        addBody(panel, getString(R.string.playtest_privacy));
        addBody(panel, getString(R.string.playtest_category));
        playtestCategory = createPlaytestChoice(getResources().getStringArray(R.array.playtest_categories));
        panel.addView(playtestCategory);
        addBody(panel, getString(R.string.playtest_impact));
        playtestImpact = createPlaytestChoice(getResources().getStringArray(R.array.playtest_impacts));
        playtestImpact.setSelection(2);
        panel.addView(playtestImpact);
        addBody(panel, getString(R.string.playtest_note_help));
        playtestNote = new EditText(this);
        playtestNote.setHint(R.string.playtest_note_hint);
        playtestNote.setTextColor(Color.WHITE);
        playtestNote.setHintTextColor(0xFFADADAD);
        playtestNote.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_MULTI_LINE |
                InputType.TYPE_TEXT_FLAG_CAP_SENTENCES);
        playtestNote.setMinLines(3);
        playtestNote.setMaxLines(8);
        panel.addView(playtestNote);
        addBody(panel, getString(R.string.playtest_remote_section));
        playtestRemoteConsent = new CheckBox(this);
        playtestRemoteConsent.setText(R.string.playtest_remote_consent);
        stylePlaytestConsent(playtestRemoteConsent);
        panel.addView(playtestRemoteConsent);
        playtestRemoteContext = new CheckBox(this);
        playtestRemoteContext.setText(R.string.playtest_remote_context_consent);
        stylePlaytestConsent(playtestRemoteContext);
        panel.addView(playtestRemoteContext);
        playtestRemoteScreenshot = new CheckBox(this);
        playtestRemoteScreenshot.setText(R.string.playtest_remote_screenshot_consent);
        stylePlaytestConsent(playtestRemoteScreenshot);
        panel.addView(playtestRemoteScreenshot);
        playtestPreview = new ImageView(this);
        playtestPreview.setAdjustViewBounds(true);
        playtestPreview.setScaleType(ImageView.ScaleType.FIT_CENTER);
        playtestPreview.setContentDescription(getString(R.string.playtest_preview_description));
        playtestPreview.setVisibility(View.GONE);
        panel.addView(playtestPreview, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(220)));
        playtestStatus = new TextView(this);
        playtestStatus.setTextColor(0xFFFFDEAD);
        playtestStatus.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);
        panel.addView(playtestStatus);
        playtestRemotePrepare = new Button(this);
        playtestRemotePrepare.setText(R.string.playtest_prepare_remote);
        playtestRemotePrepare.setOnClickListener(view -> preparePlaytestSubmission());
        panel.addView(playtestRemotePrepare);
        playtestRemoteSend = new Button(this);
        playtestRemoteSend.setText(R.string.playtest_verify_send);
        playtestRemoteSend.setVisibility(View.GONE);
        playtestRemoteSend.setOnClickListener(view -> verifyAndSendPlaytestReport());
        panel.addView(playtestRemoteSend);
        playtestEdit = new Button(this);
        playtestEdit.setText(R.string.playtest_edit);
        playtestEdit.setVisibility(View.GONE);
        playtestEdit.setOnClickListener(view -> requestPlaytestEdit());
        panel.addView(playtestEdit);
        addBody(panel, getString(R.string.playtest_local_section));
        playtestContext = new CheckBox(this);
        playtestContext.setText(R.string.playtest_context_consent);
        stylePlaytestConsent(playtestContext);
        panel.addView(playtestContext);
        playtestConsent = new CheckBox(this);
        playtestConsent.setText(R.string.playtest_export_consent);
        stylePlaytestConsent(playtestConsent);
        panel.addView(playtestConsent);
        playtestSave = new Button(this);
        playtestSave.setText(R.string.playtest_save_json);
        playtestSave.setOnClickListener(view -> preparePlaytestExport());
        panel.addView(playtestSave);
        addMenuButton(panel, getString(R.string.back), this::requestClosePlaytestReport);
        attachPanel(panel);
    }

    private void setPlaytestFieldsEnabled(final boolean enabled) {
        playtestNote.setEnabled(enabled);
        playtestCategory.setEnabled(enabled);
        playtestImpact.setEnabled(enabled);
        playtestConsent.setEnabled(enabled);
        playtestContext.setEnabled(enabled);
        playtestRemoteConsent.setEnabled(enabled);
        playtestRemoteContext.setEnabled(enabled);
        playtestRemoteScreenshot.setEnabled(enabled);
    }

    private void stylePlaytestConsent(final CheckBox box) {
        box.setTextColor(Color.WHITE);
        box.setButtonTintList(android.content.res.ColorStateList.valueOf(0xFFFFDEAD));
        box.setChecked(false);
    }

    /** Guard is independent of View.enabled so stale or accessibility-triggered clicks cannot abandon a send. */
    void requestPlaytestEdit() {
        final PlaytestReportSubmission.State state = playtestSubmission == null
                ? PlaytestReportSubmission.State.DRAFT : playtestSubmission.state();
        if (state == PlaytestReportSubmission.State.IN_FLIGHT) return;
        if (state == PlaytestReportSubmission.State.RETRYABLE) {
            showPlaytestDecision(R.string.playtest_edit_uncertain, state, this::beginPlaytestEdit);
        } else if (state == PlaytestReportSubmission.State.QUEUED ||
                state == PlaytestReportSubmission.State.SENT) {
            showPlaytestDecision(R.string.playtest_edit_accepted, state, this::beginPlaytestEdit);
        } else beginPlaytestEdit();
    }

    /** A decision only applies to the exact visible form and submission state that opened it. */
    private void showPlaytestDecision(final int messageResource,
                                     final PlaytestReportSubmission.State requiredState,
                                     final Runnable confirmed) {
        dismissPlaytestDecisionDialog();
        final long generation = playtestFormGeneration;
        final PlaytestReportSubmission owner = playtestSubmission;
        final AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle(R.string.playtest_report)
                .setMessage(messageResource)
                .setNegativeButton(R.string.playtest_stay_here, null)
                .setPositiveButton(R.string.playtest_edit_continue, null)
                .create();
        playtestDecisionDialog = dialog;
        dialog.setOnDismissListener(ignored -> {
            if (playtestDecisionDialog == dialog) playtestDecisionDialog = null;
        });
        dialog.setOnShowListener(ignored -> dialog.getButton(AlertDialog.BUTTON_POSITIVE)
                .setOnClickListener(view -> {
                    final boolean stillCurrent = playtestDecisionDialog == dialog &&
                            playtestReportVisible && playtestFormGeneration == generation &&
                            playtestSubmission == owner && owner != null && owner.state() == requiredState;
                    dialog.dismiss();
                    if (stillCurrent) confirmed.run();
                }));
        dialog.show();
    }

    private void dismissPlaytestDecisionDialog() {
        final AlertDialog dialog = playtestDecisionDialog;
        playtestDecisionDialog = null;
        if (dialog != null && dialog.isShowing()) dialog.dismiss();
    }

    private void beginPlaytestEdit() {
        invalidateRemotePlaytest(true);
        playtestSubmission = newPlaytestSubmission();
        playtestExport.cancel();
        playtestExport = new PlaytestReportExport();
        setPlaytestFieldsEnabled(true);
        playtestConsent.setChecked(false);
        playtestRemoteConsent.setChecked(false);
        playtestRemoteContext.setChecked(false);
        playtestRemoteScreenshot.setChecked(false);
        playtestSave.setEnabled(true);
        playtestSave.setText(R.string.playtest_save_json);
        playtestRemotePrepare.setEnabled(true);
        playtestRemotePrepare.setVisibility(View.VISIBLE);
        playtestRemoteSend.setVisibility(View.GONE);
        playtestRemoteSend.setText(R.string.playtest_verify_send);
        playtestRemoteSend.setEnabled(true);
        playtestEdit.setEnabled(true);
        playtestEdit.setVisibility(View.GONE);
        playtestStatus.setText("");
    }

    private void requestClosePlaytestReport() {
        final PlaytestReportSubmission.State state = playtestSubmission == null
                ? PlaytestReportSubmission.State.DRAFT : playtestSubmission.state();
        if (state == PlaytestReportSubmission.State.IN_FLIGHT) {
            showPlaytestDecision(R.string.playtest_close_inflight, state, this::closePlaytestReport);
        } else if (state == PlaytestReportSubmission.State.RETRYABLE) {
            showPlaytestDecision(R.string.playtest_close_uncertain, state, this::closePlaytestReport);
        } else closePlaytestReport();
    }

    private PlaytestReportSubmission newPlaytestSubmission() {
        return new PlaytestReportSubmission(reportExecutor::execute,
                new PlaytestReportSubmission.HttpsTransport());
    }

    private static final class CaptureOutcome {
        final boolean pending;
        final byte[] png;
        CaptureOutcome(boolean pending, byte[] png) { this.pending = pending; this.png = png; }
    }

    private void preparePlaytestSubmission() {
        if (!playtestRemoteConsent.isChecked()) {
            playtestStatus.setText(R.string.playtest_consent_remote_required);
            return;
        }
        final long generation = ++playtestFormGeneration;
        final String id = UUID.randomUUID().toString();
        final SimpleDateFormat utc = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'", Locale.ROOT);
        utc.setTimeZone(TimeZone.getTimeZone("UTC"));
        final String capturedAt = utc.format(new Date());
        final boolean includeContext = playtestRemoteContext.isChecked();
        final boolean includeScreenshot = playtestRemoteScreenshot.isChecked();
        playtestRemoteReportId = id;
        playtestRemoteCapturedAt = capturedAt;

        // Native validation happens before requesting an RT-frame capture. This
        // validation envelope is discarded; only the final native bytes are frozen.
        final byte[] validation;
        try {
            validation = ProbeBridge.preparePlaytestSubmission(id, capturedAt,
                    playtestCategory.getSelectedItemPosition(), playtestImpact.getSelectedItemPosition(),
                    playtestNote.getText().toString(), true, includeContext, Build.MODEL,
                    false, 0L, null);
        } catch (RuntimeException | LinkageError unavailable) {
            playtestStatus.setText(R.string.playtest_prepare_failed);
            return;
        }
        if (!isPreparedEnvelope(validation)) {
            wipe(validation);
            playtestStatus.setText(R.string.playtest_prepare_failed);
            return;
        }
        wipe(validation);
        setPlaytestFieldsEnabled(false);
        playtestSave.setEnabled(false);
        playtestRemotePrepare.setEnabled(false);
        playtestEdit.setVisibility(View.GONE);
        playtestRemoteSend.setVisibility(View.GONE);
        if (!includeScreenshot) {
            final byte[] prepared = prepareFinalRemoteEnvelope(id, capturedAt, includeContext, false, 0L, null);
            finishRemotePreparation(generation, prepared, null);
            return;
        }

        final long token;
        try { token = ProbeBridge.requestPlaytestCapture(true); }
        catch (RuntimeException | LinkageError unavailable) {
            remoteCaptureFailed(generation);
            return;
        }
        if (token <= 0L) {
            remoteCaptureFailed(generation);
            return;
        }
        playtestCaptureToken = token;
        playtestCaptureStartedAt = SystemClock.elapsedRealtime();
        playtestStatus.setText(R.string.playtest_capture_wait);
        pollPlaytestCapture(generation, token);
    }

    private static boolean isPreparedEnvelope(final byte[] value) {
        return value != null && value.length > 1 && value[0] == 0;
    }

    private byte[] prepareFinalRemoteEnvelope(final String id, final String capturedAt,
            final boolean includeContext, final boolean includeScreenshot,
            final long captureToken, final byte[] png) {
        try {
            return ProbeBridge.preparePlaytestSubmission(id, capturedAt,
                    playtestCategory.getSelectedItemPosition(), playtestImpact.getSelectedItemPosition(),
                    playtestNote.getText().toString(), true, includeContext, Build.MODEL,
                    includeScreenshot, captureToken, png);
        } catch (RuntimeException | LinkageError unavailable) { return null; }
    }

    private void pollPlaytestCapture(final long generation, final long token) {
        if (!playtestReportVisible || generation != playtestFormGeneration || token != playtestCaptureToken) return;
        if (SystemClock.elapsedRealtime() - playtestCaptureStartedAt >= 10_000L) {
            remoteCaptureFailed(generation);
            return;
        }
        try {
            reportExecutor.execute(() -> {
                final CaptureOutcome outcome = takeAndEncodePlaytestCapture(token);
                handler.post(() -> {
                    if (!playtestReportVisible || generation != playtestFormGeneration ||
                            token != playtestCaptureToken) {
                        wipe(outcome.png);
                        return;
                    }
                    if (SystemClock.elapsedRealtime() - playtestCaptureStartedAt >= 10_000L) {
                        wipe(outcome.png);
                        remoteCaptureFailed(generation);
                        return;
                    }
                    if (outcome.pending) {
                        handler.postDelayed(() -> pollPlaytestCapture(generation, token), 100L);
                        return;
                    }
                    if (outcome.png == null) {
                        remoteCaptureFailed(generation);
                        return;
                    }
                    playtestPreparedPng = outcome.png;
                    final byte[] prepared = prepareFinalRemoteEnvelope(playtestRemoteReportId,
                            playtestRemoteCapturedAt, playtestRemoteContext.isChecked(), true,
                            token, playtestPreparedPng);
                    finishRemotePreparation(generation, prepared, playtestPreparedPng);
                });
            });
        } catch (RejectedExecutionException saturated) { remoteCaptureFailed(generation); }
    }

    private CaptureOutcome takeAndEncodePlaytestCapture(final long token) {
        final int maxOwnedCaptureBytes = 32 * 1024 * 1024 + 9;
        byte[] raw = null;
        try {
            raw = ProbeBridge.takePlaytestCapture(token);
            if (raw == null || raw.length == 0 || raw.length > maxOwnedCaptureBytes)
                return new CaptureOutcome(false, null);
            if (raw[0] == 1) return new CaptureOutcome(true, null);
            if (raw[0] != 0 || raw.length < 9) return new CaptureOutcome(false, null);
            final ByteBuffer header = ByteBuffer.wrap(raw).order(ByteOrder.LITTLE_ENDIAN);
            final long widthLong = Integer.toUnsignedLong(header.getInt(1));
            final long heightLong = Integer.toUnsignedLong(header.getInt(5));
            if (widthLong < 1 || heightLong < 1 || widthLong > PlaytestReportScreenshot.MAX_LONG_EDGE ||
                    heightLong > PlaytestReportScreenshot.MAX_LONG_EDGE ||
                    Math.min(widthLong, heightLong) > PlaytestReportScreenshot.MAX_SHORT_EDGE ||
                    9L + widthLong * heightLong * 4L != raw.length) return new CaptureOutcome(false, null);
            byte[] rgba = Arrays.copyOfRange(raw, 9, raw.length);
            try {
                PlaytestReportScreenshot.Result result = PlaytestReportScreenshot.encodeRgba8(
                        rgba, (int) widthLong, (int) heightLong);
                return new CaptureOutcome(false, result.isEncoded() ? result.png : null);
            } finally { wipe(rgba); }
        } catch (RuntimeException | LinkageError unavailable) {
            return new CaptureOutcome(false, null);
        } finally { wipe(raw); }
    }

    private void finishRemotePreparation(final long generation, final byte[] prepared, final byte[] png) {
        if (!playtestReportVisible || generation != playtestFormGeneration) {
            wipe(prepared);
            wipe(png);
            return;
        }
        if (!isPreparedEnvelope(prepared) || playtestSubmission == null ||
                !playtestSubmission.begin(prepared, true)) {
            cancelPlaytestCapture();
            playtestSubmission = newPlaytestSubmission();
            wipe(prepared);
            wipe(png);
            clearPlaytestPng();
            setPlaytestFieldsEnabled(true);
            playtestSave.setEnabled(true);
            playtestRemotePrepare.setEnabled(true);
            playtestEdit.setVisibility(View.VISIBLE);
            playtestStatus.setText(R.string.playtest_prepare_failed);
            return;
        }
        playtestCaptureToken = 0L; // Native accepted and consumed any one-use screenshot ticket.
        wipe(prepared);
        if (png != null && !showPreparedPlaytestPreview(png)) {
            if (playtestCaptureToken > 0L) cancelPlaytestCapture();
            playtestSubmission.cancel();
            playtestSubmission = newPlaytestSubmission();
            clearPlaytestPng();
            setPlaytestFieldsEnabled(true);
            playtestSave.setEnabled(true);
            playtestRemotePrepare.setEnabled(true);
            playtestEdit.setVisibility(View.VISIBLE);
            playtestStatus.setText(R.string.playtest_capture_failed);
            return;
        }
        clearPlaytestPng();
        playtestRemotePrepare.setVisibility(View.GONE);
        playtestRemoteSend.setVisibility(View.VISIBLE);
        playtestRemoteSend.setEnabled(true);
        playtestEdit.setVisibility(View.VISIBLE);
        playtestStatus.setText(R.string.playtest_review_ready);
    }

    private boolean showPreparedPlaytestPreview(final byte[] png) {
        Bitmap decoded = BitmapFactory.decodeByteArray(png, 0, png.length);
        if (decoded == null || decoded.getWidth() > PlaytestReportScreenshot.MAX_LONG_EDGE ||
                decoded.getHeight() > PlaytestReportScreenshot.MAX_LONG_EDGE ||
                Math.min(decoded.getWidth(), decoded.getHeight()) > PlaytestReportScreenshot.MAX_SHORT_EDGE) {
            if (decoded != null) decoded.recycle();
            return false;
        }
        releasePlaytestPreview();
        playtestPreviewBitmap = decoded;
        playtestPreview.setImageBitmap(decoded);
        playtestPreview.setVisibility(View.VISIBLE);
        return true;
    }

    private void remoteCaptureFailed(final long generation) {
        if (generation != playtestFormGeneration || !playtestReportVisible) return;
        cancelPlaytestCapture();
        clearPlaytestPng();
        setPlaytestFieldsEnabled(true);
        playtestSave.setEnabled(true);
        playtestRemotePrepare.setEnabled(true);
        playtestEdit.setVisibility(View.VISIBLE);
        playtestStatus.setText(R.string.playtest_capture_failed);
    }

    private void verifyAndSendPlaytestReport() {
        if (!playtestReportVisible || playtestSubmission == null) return;
        if (playtestSubmission.state() == PlaytestReportSubmission.State.RETRYABLE) {
            if (playtestSubmission.retry() < 0L) return;
        }
        if (playtestSubmission.state() != PlaytestReportSubmission.State.READY) return;
        final long attempt = playtestSubmission.attempt();
        final long formGeneration = playtestFormGeneration;
        playtestRemoteSend.setEnabled(false);
        playtestStatus.setText(R.string.playtest_verification_open);
        try {
            playtestVerification = PlaytestReportVerification.show(this, attempt,
                    candidate -> playtestReportVisible && playtestSubmission != null &&
                            playtestFormGeneration == formGeneration &&
                            playtestSubmission.attempt() == candidate &&
                            playtestSubmission.state() == PlaytestReportSubmission.State.READY,
                    new PlaytestReportVerification.Callback() {
                        @Override public void onVerified(long ownerGeneration, String token) {
                            if (!isCurrentRemoteAttempt(formGeneration, ownerGeneration)) return;
                            playtestVerification = null;
                            playtestEdit.setEnabled(false);
                            playtestStatus.setText(R.string.playtest_sending);
                            final boolean accepted = playtestSubmission.submit(ownerGeneration, token,
                                    (completedAttempt, result) -> handler.post(() ->
                                            showSubmissionResult(formGeneration, completedAttempt, result)));
                            if (!accepted) {
                                playtestEdit.setEnabled(true);
                                playtestRemoteSend.setEnabled(true);
                                playtestStatus.setText(R.string.playtest_verification_failed);
                            }
                        }
                        @Override public void onFailure(long ownerGeneration,
                                PlaytestReportVerification.Failure failure) {
                            if (!isCurrentRemoteAttempt(formGeneration, ownerGeneration)) return;
                            playtestVerification = null;
                            playtestEdit.setEnabled(true);
                            playtestRemoteSend.setEnabled(true);
                            playtestStatus.setText(R.string.playtest_verification_failed);
                        }
                        @Override public void onCancelled(long ownerGeneration) {
                            if (!isCurrentRemoteAttempt(formGeneration, ownerGeneration)) return;
                            playtestVerification = null;
                            playtestEdit.setEnabled(true);
                            playtestRemoteSend.setEnabled(true);
                            playtestStatus.setText(R.string.playtest_verification_cancelled);
                        }
                    });
        } catch (RuntimeException unavailable) {
            playtestRemoteSend.setEnabled(true);
            playtestStatus.setText(R.string.playtest_verification_failed);
        }
    }

    private boolean isCurrentRemoteAttempt(final long formGeneration, final long attempt) {
        return playtestReportVisible && playtestSubmission != null &&
                playtestFormGeneration == formGeneration && playtestSubmission.attempt() == attempt;
    }

    private void showSubmissionResult(final long formGeneration, final long attempt,
            final PlaytestReportSubmission.Result result) {
        if (result == null || !isCurrentRemoteAttempt(formGeneration, attempt)) return;
        playtestRemoteSend.setEnabled(false);
        playtestEdit.setEnabled(true);
        switch (result.code) {
            case QUEUED:
                showSubmissionState(PlaytestReportSubmission.State.QUEUED);
                break;
            case SENT:
                showSubmissionState(PlaytestReportSubmission.State.SENT);
                break;
            case CONTENT_CONFLICT:
                showSubmissionState(PlaytestReportSubmission.State.CONFLICT);
                break;
            case REJECTED:
                showSubmissionState(PlaytestReportSubmission.State.REJECTED);
                break;
            case UNCERTAIN:
                playtestStatus.setText(R.string.playtest_uncertain);
                playtestRemoteSend.setText(R.string.playtest_retry_submission);
                playtestRemoteSend.setVisibility(View.VISIBLE);
                playtestRemoteSend.setEnabled(true);
                break;
            case VERIFICATION_EXPIRED:
                playtestStatus.setText(R.string.playtest_verification_expired);
                playtestRemoteSend.setEnabled(true);
                break;
            case RATE_LIMITED:
                playtestStatus.setText(R.string.playtest_rate_limited);
                playtestRemoteSend.setEnabled(true);
                break;
            default:
                playtestStatus.setText(R.string.playtest_uncertain);
                playtestRemoteSend.setText(R.string.playtest_retry_submission);
                playtestRemoteSend.setVisibility(View.VISIBLE);
                playtestRemoteSend.setEnabled(true);
                break;
        }
    }

    private void showSubmissionState(final PlaytestReportSubmission.State state) {
        playtestRemoteSend.setEnabled(false);
        playtestRemoteSend.setVisibility(View.GONE);
        playtestEdit.setEnabled(true);
        if (state == PlaytestReportSubmission.State.QUEUED) playtestStatus.setText(R.string.playtest_queued);
        else if (state == PlaytestReportSubmission.State.SENT) playtestStatus.setText(R.string.playtest_sent);
        else if (state == PlaytestReportSubmission.State.CONFLICT) playtestStatus.setText(R.string.playtest_conflict);
        else if (state == PlaytestReportSubmission.State.REJECTED) playtestStatus.setText(R.string.playtest_remote_rejected);
    }

    private void cancelPlaytestCapture() {
        final long token = playtestCaptureToken;
        playtestCaptureToken = 0L;
        if (token > 0L) {
            try { ProbeBridge.cancelPlaytestCapture(token); }
            catch (RuntimeException | LinkageError ignored) { /* Capture teardown is best effort. */ }
        }
    }

    private void clearPlaytestPng() {
        wipe(playtestPreparedPng);
        playtestPreparedPng = null;
    }

    private void releasePlaytestPreview() {
        if (playtestPreview != null) playtestPreview.setImageDrawable(null);
        if (playtestPreviewBitmap != null && !playtestPreviewBitmap.isRecycled()) playtestPreviewBitmap.recycle();
        playtestPreviewBitmap = null;
        if (playtestPreview != null) playtestPreview.setVisibility(View.GONE);
    }

    private static void wipe(final byte[] bytes) {
        if (bytes != null) Arrays.fill(bytes, (byte) 0);
    }

    private void invalidateRemotePlaytest(final boolean closing) {
        ++playtestFormGeneration;
        dismissPlaytestDecisionDialog();
        if (playtestVerification != null) {
            playtestVerification.cancel();
            playtestVerification = null;
        }
        cancelPlaytestCapture();
        clearPlaytestPng();
        if (playtestSubmission != null) playtestSubmission.cancel();
        if (closing) releasePlaytestPreview();
    }

    /** UI-owned pause reconciliation seam; kept package-visible for no-JNI Robolectric coverage. */
    void reconcilePlaytestReportForPause() {
        if (!playtestReportVisible || playtestSubmission == null) return;
        final boolean sendWasInFlight = playtestSubmission.state() == PlaytestReportSubmission.State.IN_FLIGHT;
        final boolean interrupted = sendWasInFlight && playtestSubmission.interruptInFlight();
        final PlaytestReportSubmission.State state = playtestSubmission.state();
        final boolean capturePending = playtestCaptureToken > 0L;
        ++playtestFormGeneration;
        dismissPlaytestDecisionDialog();
        if (playtestVerification != null) {
            playtestVerification.cancel();
            playtestVerification = null;
        }
        cancelPlaytestCapture();
        if (capturePending) {
            playtestSubmission = newPlaytestSubmission();
            clearPlaytestPng();
            releasePlaytestPreview();
            setPlaytestFieldsEnabled(true);
            playtestSave.setEnabled(true);
            playtestRemotePrepare.setEnabled(true);
            playtestRemotePrepare.setVisibility(View.VISIBLE);
            playtestRemoteSend.setVisibility(View.GONE);
            playtestRemoteSend.setText(R.string.playtest_verify_send);
            playtestEdit.setEnabled(true);
            playtestEdit.setVisibility(View.VISIBLE);
            playtestRemoteConsent.setChecked(false);
            playtestRemoteContext.setChecked(false);
            playtestRemoteScreenshot.setChecked(false);
            playtestStatus.setText(R.string.playtest_lifecycle_cancelled);
        } else if (state == PlaytestReportSubmission.State.IN_FLIGHT) {
            // Do not offer a second send while an attempt still owns the controller.
            playtestRemoteSend.setEnabled(false);
            playtestEdit.setEnabled(false);
            playtestStatus.setText(R.string.playtest_sending);
        } else if (state == PlaytestReportSubmission.State.RETRYABLE || interrupted) {
            playtestRemoteSend.setText(R.string.playtest_retry_submission);
            playtestRemoteSend.setEnabled(true);
            playtestRemoteSend.setVisibility(View.VISIBLE);
            playtestEdit.setEnabled(true);
            playtestStatus.setText(interrupted || sendWasInFlight ? R.string.playtest_lifecycle_uncertain :
                    R.string.playtest_retry_ready);
        } else if (state == PlaytestReportSubmission.State.READY) {
            playtestRemoteSend.setText(R.string.playtest_verify_send);
            playtestRemoteSend.setEnabled(true);
            playtestRemoteSend.setVisibility(View.VISIBLE);
            playtestEdit.setEnabled(true);
            playtestStatus.setText(R.string.playtest_verification_cancelled);
        } else if (state == PlaytestReportSubmission.State.QUEUED) {
            showSubmissionState(PlaytestReportSubmission.State.QUEUED);
        } else if (state == PlaytestReportSubmission.State.SENT) {
            showSubmissionState(PlaytestReportSubmission.State.SENT);
        } else if (state == PlaytestReportSubmission.State.CONFLICT) {
            showSubmissionState(PlaytestReportSubmission.State.CONFLICT);
        } else if (state == PlaytestReportSubmission.State.REJECTED) {
            showSubmissionState(PlaytestReportSubmission.State.REJECTED);
        }
    }

    private Spinner createPlaytestChoice(final String[] choices) {
        final Spinner spinner = new Spinner(this);
        final ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, choices) {
            @Override public View getView(final int position, final View convertView, final ViewGroup parent) {
                final TextView selected = (TextView) super.getView(position, convertView, parent);
                selected.setTextColor(Color.WHITE); // Selected native item must contrast with the game panel.
                selected.setTextSize(15);
                return selected;
            }
        };
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);
        return spinner;
    }

    private void preparePlaytestExport() {
        if (playtestExport.state() == PlaytestReportExport.State.RETRYABLE) {
            if (playtestExport.retry()) choosePlaytestDestination();
            return;
        }
        if (!playtestConsent.isChecked()) {
            playtestStatus.setText(R.string.playtest_consent_required);
            return;
        }
        final SimpleDateFormat utc = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'", Locale.ROOT);
        utc.setTimeZone(TimeZone.getTimeZone("UTC"));
        final byte[] prepared;
        try {
            prepared = ProbeBridge.preparePlaytestReport(UUID.randomUUID().toString(), utc.format(new Date()),
                    playtestCategory.getSelectedItemPosition(), playtestImpact.getSelectedItemPosition(),
                    playtestNote.getText().toString(), true, playtestContext.isChecked(), Build.MODEL);
        } catch (final RuntimeException | LinkageError unavailable) {
            playtestStatus.setText(R.string.playtest_prepare_failed);
            return;
        }
        if (!playtestExport.begin(prepared, true)) {
            playtestStatus.setText(PlaytestReportExport.preparationError(prepared));
            return;
        }
        setPlaytestFieldsEnabled(false); // Retry owns these exact approved bytes, not later edits.
        playtestRemotePrepare.setEnabled(false);
        choosePlaytestDestination();
    }

    private void choosePlaytestDestination() {
        playtestPickerToken = playtestExport.token();
        playtestSave.setEnabled(false);
        playtestEdit.setVisibility(View.GONE);
        playtestStatus.setText(R.string.playtest_choose_destination);
        final Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/json");
        intent.putExtra(Intent.EXTRA_TITLE, "HordeLanternRT-player-report.json");
        try { startActivityForResult(intent, REQUEST_SAVE_PLAYTEST); }
        catch (final RuntimeException unavailable) {
            playtestExport.pickerCancelled(playtestPickerToken);
            playtestExportFailed(R.string.playtest_picker_failed);
        }
    }

    private void playtestExportFailed(final int message) {
        playtestStatus.setText(message);
        playtestSave.setText(R.string.playtest_retry);
        playtestSave.setEnabled(true);
        playtestEdit.setVisibility(View.VISIBLE);
    }

    private void closePlaytestReport() {
        invalidateRemotePlaytest(true);
        if (playtestExport != null) playtestExport.cancel();
        playtestReportVisible = false;
        showMainMenu(false); // Back never resumes gameplay or submits anything.
    }

    private void finishPlaytestPicker(final int resultCode, final Intent data) {
        if (!playtestReportVisible || playtestExport == null) return;
        final PlaytestReportExport owner = playtestExport;
        final long attempt = playtestPickerToken;
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            if (owner.pickerCancelled(attempt)) playtestExportFailed(R.string.playtest_cancelled);
            return;
        }
        final byte[] bytes = owner.startWrite(attempt);
        if (bytes == null) return;
        playtestStatus.setText(R.string.playtest_saving);
        final android.content.ContentResolver resolver = getApplicationContext().getContentResolver();
        final Uri destination = data.getData();
        try {
            reportExecutor.execute(() -> {
                // No note/URI/private provider exception enters logs or payload.
                final boolean completed = PlaytestReportExport.writeApproved(owner, attempt, bytes,
                        () -> resolver.openOutputStream(destination, "wt"));
                handler.post(() -> {
                    if (!playtestReportVisible || playtestExport != owner ||
                            !owner.complete(attempt, completed)) return;
                    if (completed) {
                        playtestStatus.setText(R.string.playtest_saved);
                        playtestSave.setText(R.string.playtest_save_json);
                        playtestSave.setEnabled(false);
                    } else playtestExportFailed(R.string.playtest_save_failed);
                });
            });
        } catch (RejectedExecutionException saturated) {
            owner.complete(attempt, false);
            playtestExportFailed(R.string.playtest_save_failed);
        }
    }

    private void hideMenu() {
        if (graphicsSceneRestoring) {
            Toast.makeText(this, "Restoring the game RT scene. Resume is available after it presents.", Toast.LENGTH_SHORT).show();
            return;
        }
        if (!benchmarkRunning) setBenchmarkStatusExpanded(false);
        if (deathOverlayVisible) {
            return;
        }
        applyInterfacePresentation();
        menuVisible = false;
        menuScrim.setVisibility(View.GONE);
        final boolean showHud = preferences.getBoolean("show_hud", true);
        menuButton.setVisibility(showHud ? View.VISIBLE : View.GONE);
        attackButton.setVisibility(showHud && ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) == 1 &&
                lastPlayerLifePhase == PLAYER_ALIVE
                ? View.VISIBLE : View.GONE);
        parryButton.setVisibility(showHud && ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) == 1 &&
                lastPlayerLifePhase == PLAYER_ALIVE
                ? View.VISIBLE : View.GONE);
        rtStatus.setVisibility(showHud && (InterfacePreferences.read(preferences).routineStatus ||
                ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) != 1) ? View.VISIBLE : View.GONE);
        vitalityStatus.setVisibility(showHud && lastPlayerLifePhase == PLAYER_ALIVE ? View.VISIBLE : View.GONE);
        setGameplayPaused(false);
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
        interfaceVisible=false;
        playSound("ui_select", 0.18f);
        menuScrim.removeAllViews();
        final LinearLayout panel = createPanel(getString(R.string.settings), "SAVED ON THIS DEVICE");
        addMenuButton(panel, getString(R.string.graphics_settings), this::openGraphics);
        addBody(panel, "Audio");

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
        addSlider(panel, getString(R.string.music_volume), musicVolumePercent(), 0, 100,
                value -> {
                    final int clamped = Math.max(0, Math.min(100, value));
                    preferences.edit().putInt(PREF_MUSIC_VOLUME, clamped).apply();
                    if (musicPlayback != null) musicPlayback.setVolumePercent(clamped);
                });
        addBody(panel, "Controls");
        addSlider(panel, getString(R.string.look_sensitivity), preferences.getInt("look_sensitivity", 100), 50, 175,
                value -> preferences.edit().putInt("look_sensitivity", value).apply());

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
        addMenuButton(panel, "Interface / HUD", this::showInterfaceSettings);

        addMenuButtonRow(panel,
                getString(R.string.reset_non_graphics), () -> {
                    preferences.edit().putBoolean("sfx_enabled", true).putInt("sfx_volume", 70)
                            .putInt(PREF_MUSIC_VOLUME, 70).putInt("look_sensitivity", 100)
                            .putBoolean("haptics_enabled", true).putBoolean("show_hud", true).apply();
                    if (musicPlayback != null) musicPlayback.setVolumePercent(70);
                    InterfacePreferences.reset(preferences);
                    applyInterfacePresentation();
                    showSettings();
                },
                getString(R.string.back), () -> showMainMenu(false));
        attachPanel(panel);
    }

    private void setNativeGraphics(GraphicsPreferences.Values values) {
        ProbeBridge.setGraphicsSettings(values.scale, values.water, values.fire, values.cap, values.glassEnabled);
    }

    private void openGraphics() {
        setGameplayPaused(true);
        clearTouchState();
        graphicsConfirmed = GraphicsPreferences.confirmed(preferences);
        graphicsDraft = graphicsConfirmed;
        graphicsRestoreDraftAfterPreview = null;
        graphicsPreviewImageOnly = false;
        graphicsBusy = false;
        ProbeBridge.beginGraphicsEdit(graphicsConfirmed.scale, graphicsConfirmed.water,
                graphicsConfirmed.fire, graphicsConfirmed.cap, graphicsConfirmed.glassEnabled);
        graphicsVisible = true;
        graphicsSceneRestoring = false;
        graphicsPreviewWanted = false;
        graphicsCloseAfterRevert = false;
        graphicsAwaitingRestore = false;
        graphicsRequestSerial = 0;
        graphicsPollTime = SystemClock.elapsedRealtime();
        graphicsConfirmationStarted = 0;
        showGraphicsPage();
        handler.removeCallbacks(refreshGraphics);
        handler.post(refreshGraphics);
    }

    private void showGraphicsPage() {
        if (graphicsPreviewWanted) { showGraphicsPreviewPage(); return; }
        dismissGraphicsPreviewDetails();
        menuScrim.setBackgroundColor(0xC7080706);
        graphicsGraph = null;
        menuScrim.removeAllViews();
        final LinearLayout panel = createPanel(getString(R.string.graphics_settings), "GRAPHICS / DEVICE");
        graphicsPanel = panel;
        final GradientDrawable background = new GradientDrawable();
        background.setColor(0xF2151719); background.setCornerRadius(dp(4));
        background.setStroke(dp(1), 0xFFCFA96A); panel.setBackground(background);
        graphicsTelemetry = new TextView(this);
        graphicsTelemetry.setTextColor(0xFFF2E9D8); graphicsTelemetry.setTextSize(14);
        graphicsTelemetry.setPadding(0, 0, 0, dp(12)); panel.addView(graphicsTelemetry, matchWrap());
        addBody(panel, getString(R.string.graphics_help));
        if (graphicsRecoveryNotice != null) addBody(panel, graphicsRecoveryNotice);
        if (graphicsDraft.scale >= 50) {
            addSlider(panel, getString(R.string.render_scale), graphicsDraft.scale, 50, 100, value -> {
                graphicsDraft = GraphicsPreviewOptions.withChoice(graphicsDraft, GraphicsPreviewOptions.RESOLUTION, value);
            });
        } else {
            addBody(panel, "Resolution: " + GraphicsPreviewOptions.resolutionLabel(graphicsDraft.scale));
            addGraphicsButton(panel, "Return to 50% resolution", () -> {
                graphicsDraft = GraphicsPreviewOptions.withChoice(graphicsDraft, GraphicsPreviewOptions.RESOLUTION, 50);
                showGraphicsPage();
            });
        }
        if (GraphicsPreferences.MIN_RENDER_SCALE_PERCENT == 33) {
            for (int scale : new int[]{33,40}) {
                addGraphicsButton(panel, "Resolution " + GraphicsPreviewOptions.resolutionLabel(scale), () -> {
                    graphicsDraft = GraphicsPreviewOptions.withChoice(graphicsDraft, GraphicsPreviewOptions.RESOLUTION, scale);
                    showGraphicsPage();
                });
            }
        }
        addGraphicsButton(panel, "Preview resolution", () -> openGraphicsPreview(GraphicsPreviewOptions.RESOLUTION));
        addGraphicsButton(panel, "Water: " + waterName(graphicsDraft.water), () -> {
            graphicsDraft = new GraphicsPreferences.Values(graphicsDraft.scale, (graphicsDraft.water + 1) % 3,
                    graphicsDraft.fire, graphicsDraft.cap, graphicsDraft.glassEnabled); showGraphicsPage();
        });
        addBody(panel, getString(R.string.graphics_water_help));
        addGraphicsButton(panel, "Preview water", () -> openGraphicsPreview(GraphicsPreviewOptions.WATER));
        addGraphicsButton(panel, "Fire detail: " + (graphicsDraft.fire == 1 ? "High" : "Mobile"), () -> {
            graphicsDraft = new GraphicsPreferences.Values(graphicsDraft.scale, graphicsDraft.water,
                    1 - graphicsDraft.fire, graphicsDraft.cap, graphicsDraft.glassEnabled); showGraphicsPage();
        });
        addBody(panel, getString(R.string.graphics_fire_help));
        addGraphicsButton(panel, "Preview fire", () -> openGraphicsPreview(GraphicsPreviewOptions.FIRE));
        addGraphicsButton(panel, "Menu / preview cap: " + graphicsDraft.cap + " Hz", () -> {
            final int next = graphicsDraft.cap == 30 ? 60 : graphicsDraft.cap == 60 ? 15 : 30;
            graphicsDraft = new GraphicsPreferences.Values(graphicsDraft.scale, graphicsDraft.water, graphicsDraft.fire, next, graphicsDraft.glassEnabled);
            showGraphicsPage();
        });
        addGraphicsButton(panel, "Preview frame cap", () -> openGraphicsPreview(GraphicsPreviewOptions.CAP));
        addGraphicsButton(panel, "Glass: " + (graphicsDraft.glassEnabled ? "On" : "Off"), () -> {
            graphicsDraft = GraphicsPreviewOptions.withChoice(graphicsDraft, GraphicsPreviewOptions.GLASS, graphicsDraft.glassEnabled ? 0 : 1);
            showGraphicsPage();
        });
        addBody(panel, "On enables glass supported by this build. Mobile lantern panes remain absent. Off removes glass from primary, reflection and shadow rays.");
        addGraphicsButton(panel, "Preview glass", () -> openGraphicsPreview(GraphicsPreviewOptions.GLASS));
        addGraphicsButton(panel, getString(R.string.graphics_baseline), () -> {
            graphicsDraft = GraphicsPreferences.baseline(); showGraphicsPage();
        });
        addGraphicsButton(panel, "Open authored RT preview", () -> {
            openGraphicsPreview(GraphicsPreviewOptions.RESOLUTION);
        });
        graphicsApply = addGraphicsButton(panel, getString(R.string.graphics_apply), () -> {
            graphicsSubmitted = graphicsDraft;
            if (!GraphicsPreferences.markPending(preferences, graphicsSubmitted)) {
                graphicsTelemetry.setText(R.string.graphics_storage_failed); return;
            }
            graphicsRequestSerial = ProbeBridge.applyGraphicsSettings(graphicsDraft.scale, graphicsDraft.water,
                    graphicsDraft.fire, graphicsDraft.cap, graphicsDraft.glassEnabled, surfaceRequestGeneration);
            if (graphicsRequestSerial == 0) {
                graphicsTelemetry.setText(R.string.graphics_not_ready);
                return; // Keep recovery marker until an acknowledged restore.
            }
            graphicsConfirmationStarted = 0;
            graphicsBusy = graphicsRequestSerial != 0;
            setGraphicsEditorsEnabled(graphicsPanel, !graphicsBusy);
        });
        graphicsConfirm = addGraphicsButton(panel, getString(R.string.graphics_keep), this::confirmGraphicsSelection);
        graphicsConfirm.setEnabled(false);
        graphicsRevert = addGraphicsButton(panel, getString(R.string.graphics_revert), () -> requestGraphicsRevert(false));
        graphicsBack = addGraphicsButton(panel, getString(R.string.back), () -> requestGraphicsRevert(true));
        // Scroll/reflow at system font scales. Native buttons retain minimum 48dp hit areas.
        final ScrollView scroller = new ScrollView(this);
        scroller.addView(panel, new ScrollView.LayoutParams(-1, -2));
        final WindowInsets insets = menuScrim.getRootWindowInsets();
        int left = dp(16), right = dp(16), top = dp(20), bottom = dp(20);
        if (insets != null) {
            left = Math.max(left, insets.getStableInsetLeft() + dp(8));
            right = Math.max(right, insets.getStableInsetRight() + dp(8));
            top = Math.max(top, insets.getStableInsetTop() + dp(8));
            bottom = Math.max(bottom, insets.getStableInsetBottom() + dp(8));
            if (Build.VERSION.SDK_INT >= 28 && insets.getDisplayCutout() != null) {
                left = Math.max(left, insets.getDisplayCutout().getSafeInsetLeft() + dp(8));
                right = Math.max(right, insets.getDisplayCutout().getSafeInsetRight() + dp(8));
                top = Math.max(top, insets.getDisplayCutout().getSafeInsetTop() + dp(8));
                bottom = Math.max(bottom, insets.getDisplayCutout().getSafeInsetBottom() + dp(8));
            }
        }
        final int available = getResources().getDisplayMetrics().widthPixels - left - right;
        final FrameLayout.LayoutParams layout = new FrameLayout.LayoutParams(Math.min(dp(520), Math.max(dp(48), available)), -1);
        layout.gravity = Gravity.CENTER_HORIZONTAL;
        layout.setMargins(left, top, right, bottom);
        menuScrim.addView(scroller, layout);
        setGraphicsEditorsEnabled(panel, !graphicsBusy);
    }

    private void setGraphicsEditorsEnabled(View view, boolean enabled) {
        if (view instanceof SeekBar) view.setMinimumHeight(dp(48));
        if (view instanceof ViewGroup) {
            final ViewGroup group = (ViewGroup)view;
            for (int i = 0; i < group.getChildCount(); ++i) setGraphicsEditorsEnabled(group.getChildAt(i), enabled);
        } else if ((view instanceof SeekBar || view instanceof Button) && view != graphicsConfirm &&
                view != graphicsRevert && view != graphicsBack && view != graphicsDetailsButton &&
                view != graphicsImageButton && view != graphicsControlsButton) view.setEnabled(enabled);
    }

    private void publishGraphicsPreview(boolean reset) {
        ProbeBridge.setGraphicsPreview(graphicsPreviewWanted, graphicsPreviewPaused, graphicsPreviewMotion,
                graphicsPreviewCamera, reset, surfaceRequestGeneration);
    }

    private void openGraphicsPreview(int choice) {
        graphicsOriginalPreviewDraft = graphicsDraft;
        graphicsDraft = graphicsConfirmed;
        graphicsRestoreDraftAfterPreview = null;
        graphicsPreviewChoice = choice;
        graphicsPreviewImageOnly = false;
        graphicsPreviewWanted = true;
        graphicsPreviewPaused = false;
        graphicsPreviewMotion = choice == GraphicsPreviewOptions.CAP;
        graphicsPreviewCamera = GraphicsPreviewOptions.camera(choice);
        graphicsRequestSerial = 0;
        graphicsLiveChoiceError = null;
        graphicsPreviewDetailsText = "Waiting for current RT preview presentation.";
        resetGraphicsPreviewTimeline(); showGraphicsPreviewPage();
        compareGraphicsPreview(GraphicsPreviewOptions.withChoice(graphicsDraft, choice,
                GraphicsPreviewOptions.value(graphicsOriginalPreviewDraft, choice)));
    }

    private GraphicsPreferences.Values previewSelection() {
        return graphicsDraft;
    }

    private void beginGraphicsPreviewPerformanceScope() {
        final double[] old = ProbeBridge.getGraphicsPreviewPerformance();
        graphicsPreviewPerformanceEpochFloor = GraphicsPreviewOptions.performanceEpochFloor(
                graphicsPreviewPerformanceGeneration, surfaceRequestGeneration, old);
        graphicsPreviewPerformanceGeneration = surfaceRequestGeneration;
        graphicsDisplayedFps = Double.NaN; graphicsDisplayedFpsEpoch = 0; graphicsNextFpsUpdate = 0;
        graphicsPreviewDetailsText = "Waiting for current RT preview presentation.";
        graphicsPreviewDetailsSamples = new double[9];
    }

    private void resetGraphicsPreviewTimeline() {
        beginGraphicsPreviewPerformanceScope();
        publishGraphicsPreview(true);
    }

    private void compareGraphicsPreview(GraphicsPreferences.Values values) {
        graphicsPreviewImageOnly = false;
        if (!GraphicsPreferences.markPending(preferences, values)) {
            graphicsLiveChoiceError = getString(R.string.graphics_storage_failed); return;
        }
        final long serial = ProbeBridge.compareGraphicsPreview(values.scale, values.water, values.fire,
                values.cap, values.glassEnabled, surfaceRequestGeneration);
        if (serial == 0) {
            graphicsLiveChoiceError = "Choice unavailable; use Revert to restore confirmed settings."; return;
        }
        graphicsLiveChoiceError = null;
        graphicsSubmitted = values; graphicsDraft = values; graphicsRequestSerial = serial;
        graphicsBusy = false;
        graphicsConfirmationStarted = 0;
        resetGraphicsPreviewTimeline();
        showGraphicsPreviewPage();
    }

    private String graphicsOptionLabel(int choice) {
        if (graphicsDraft == null) return GraphicsPreviewOptions.name(choice);
        switch (choice) {
            case GraphicsPreviewOptions.RESOLUTION: return "Resolution " + GraphicsPreviewOptions.resolutionLabel(graphicsDraft.scale);
            case GraphicsPreviewOptions.WATER: return "Water " + waterName(graphicsDraft.water);
            case GraphicsPreviewOptions.FIRE: return "Fire " + (graphicsDraft.fire == 1 ? "High" : "Mobile");
            case GraphicsPreviewOptions.GLASS: return "Glass " + (graphicsDraft.glassEnabled ? "On" : "Off");
            default: return "Cap " + graphicsDraft.cap + " Hz";
        }
    }

    private PopupMenu createGraphicsPreviewOptionMenu(Button anchor, int choice) {
        final PopupMenu popup = new PopupMenu(this, anchor);
        final int[] choices = GraphicsPreviewOptions.choices(graphicsDraft, choice);
        for (int value : choices) {
            final String label = choice == GraphicsPreviewOptions.RESOLUTION ? GraphicsPreviewOptions.resolutionLabel(value) :
                    choice == GraphicsPreviewOptions.WATER ? waterName(value) :
                    choice == GraphicsPreviewOptions.FIRE ? (value == 1 ? "High" : "Mobile") :
                    choice == GraphicsPreviewOptions.GLASS ? (value == 1 ? "On" : "Off") : value + " Hz";
            popup.getMenu().add(0,value,0,label).setCheckable(true)
                    .setChecked(value == GraphicsPreviewOptions.value(graphicsDraft,choice));
        }
        popup.setOnMenuItemClickListener(item -> {
            if (graphicsBusy || !graphicsPreviewWanted || !GraphicsPreviewOptions.presented(
                    ProbeBridge.getGraphicsSnapshot(), surfaceRequestGeneration, graphicsRequestSerial, previewSelection())) return true;
            graphicsPreviewChoice = choice;
            graphicsPreviewCamera = GraphicsPreviewOptions.camera(choice);
            graphicsPreviewMotion = choice == GraphicsPreviewOptions.CAP;
            compareGraphicsPreview(GraphicsPreviewOptions.withChoice(graphicsDraft,choice,item.getItemId()));
            return true;
        });
        return popup;
    }

    private void dismissGraphicsPreviewOptions() {
        if (graphicsOptionsPopup != null) graphicsOptionsPopup.dismiss();
        graphicsOptionsPopup = null;
    }

    private void showGraphicsPreviewOptionMenu(Button anchor, int choice) {
        dismissGraphicsPreviewOptions();
        final PopupMenu popup = createGraphicsPreviewOptionMenu(anchor, choice);
        graphicsOptionsPopup = popup;
        graphicsPreviewModalEpoch = graphicsPreviewDetailsSamples[0];
        popup.setOnDismissListener(ignored -> {
            if (graphicsOptionsPopup == popup) {
                graphicsOptionsPopup = null;
                requestGraphicsPreviewPresentationRefresh();
            }
        });
        popup.show();
    }

    private void requestGraphicsPreviewPresentationRefresh() {
        graphicsDisplayedFps = Double.NaN; graphicsNextFpsUpdate = 0;
        if (resumed && graphicsVisible && graphicsPreviewWanted) {
            handler.removeCallbacks(refreshGraphics);
            handler.post(refreshGraphics); // Fetch a current ACK and generation-filtered sample; never replay frozen text.
        }
    }

    private boolean graphicsPreviewModalOpen() {
        return graphicsOptionsPopup != null || graphicsDetailsDialog != null;
    }

    private void showGraphicsPreviewPage() {
        dismissGraphicsPreviewOptions();
        dismissGraphicsPreviewDetails();
        menuScrim.setBackgroundColor(0x00000000);
        // The transparent inspection overlay still owns input; no touch reaches gameplay.
        menuScrim.setClickable(true);
        menuScrim.removeAllViews();
        if (graphicsPreviewImageOnly) { showGraphicsPreviewImageOnly(); return; }
        graphicsControlsButton = null;
        final LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL); panel.setPadding(dp(8), dp(4), dp(8), dp(4));
        panel.setBackground(HordeUiTokens.plate(this, 0xEB151719, HordeUiTokens.BRASS, 1));
        graphicsPanel = panel;
        graphicsTelemetry = new TextView(this);
        graphicsTelemetry.setTextColor(HordeUiTokens.PARCHMENT); graphicsTelemetry.setTextSize(14);
        graphicsTelemetry.setMaxLines(2); graphicsTelemetry.setEllipsize(android.text.TextUtils.TruncateAt.END);
        graphicsTelemetry.setText("Preview FPS: waiting for RT");
        panel.addView(graphicsTelemetry, matchWrap());
        final LinearLayout optionsRow = addPreviewControlRow(panel);
        for (int choice = 0; choice < graphicsOptionButtons.length; ++choice) {
            final int selectedChoice = choice;
            graphicsOptionButtons[choice] = addPreviewControl(optionsRow, graphicsOptionLabel(choice),
                    () -> showGraphicsPreviewOptionMenu(graphicsOptionButtons[selectedChoice],selectedChoice));
            graphicsOptionButtons[choice].setContentDescription(graphicsOptionLabel(choice) + "; choose live preview setting");
            graphicsOptionButtons[choice].setEnabled(false); // A matching current RT frame enables editing.
        }
        final LinearLayout actionRow = addPreviewControlRow(panel);
        final String[] cameras = {"Overview", "Materials", "Glass", "Water", "Skeleton", "Mirror"};
        final Button view = addPreviewControl(actionRow, "View", () -> {
            graphicsPreviewCamera = (graphicsPreviewCamera + 1) % cameras.length;
            resetGraphicsPreviewTimeline(); showGraphicsPreviewPage();
        });
        view.setContentDescription("View: " + cameras[graphicsPreviewCamera] + "; change preview camera");
        graphicsImageButton = addPreviewControl(actionRow, "Image", () -> {
            graphicsPreviewImageOnly = true; showGraphicsPreviewPage();
        });
        graphicsImageButton.setContentDescription("Show image without the control strip; Controls restores it. Unconfirmed settings still revert after 15 visible seconds.");
        graphicsDetailsButton = addPreviewControl(actionRow, "Details", this::showGraphicsPreviewDetails);
        graphicsConfirm = addPreviewControl(actionRow, "Apply", () -> {
            final long[] applied = ProbeBridge.getGraphicsSnapshot();
            if (!GraphicsPreviewOptions.presented(applied, surfaceRequestGeneration, graphicsRequestSerial, previewSelection())) return;
            if (applied[2] == 2) {
                confirmGraphicsSelection();
            } else if (GraphicsPreferences.markPending(preferences, previewSelection())) {
                resetGraphicsPreviewTimeline();
                graphicsSubmitted = previewSelection(); graphicsDraft = graphicsSubmitted;
                graphicsRequestSerial = ProbeBridge.applyGraphicsSettings(graphicsSubmitted.scale, graphicsSubmitted.water,
                        graphicsSubmitted.fire, graphicsSubmitted.cap, graphicsSubmitted.glassEnabled, surfaceRequestGeneration);
                graphicsBusy = graphicsRequestSerial != 0; graphicsConfirmationStarted = 0;
            }
        });
        graphicsApply = graphicsConfirm;
        graphicsConfirm.setEnabled(false);
        graphicsRevert = addPreviewControl(actionRow, "Revert", () -> {
            requestGraphicsRevert(false);
        });
        graphicsRevert.setContentDescription(getString(R.string.graphics_revert));
        graphicsBack = addPreviewControl(actionRow, "Back", this::returnFromGraphicsPreview);
        graphicsBack.setContentDescription("Revert live settings and return to Graphics");
        int left = dp(8), right = dp(8), bottom = dp(8);
        final WindowInsets insets = menuScrim.getRootWindowInsets();
        if (insets != null) {
            left = Math.max(left, insets.getStableInsetLeft() + dp(8));
            right = Math.max(right, insets.getStableInsetRight() + dp(8));
            bottom = Math.max(bottom, insets.getStableInsetBottom() + dp(8));
            if (Build.VERSION.SDK_INT >= 28 && insets.getDisplayCutout() != null) {
                left = Math.max(left, insets.getDisplayCutout().getSafeInsetLeft() + dp(8));
                right = Math.max(right, insets.getDisplayCutout().getSafeInsetRight() + dp(8));
                bottom = Math.max(bottom, insets.getDisplayCutout().getSafeInsetBottom() + dp(8));
            }
        }
        final int viewportHeight = menuScrim.getHeight() > 0 ? menuScrim.getHeight() : getResources().getDisplayMetrics().heightPixels;
        final ScrollView scroller = new GraphicsPreviewControlsScrollView(this,
                GraphicsPreviewOptions.maximumOverlayHeight(viewportHeight, bottom));
        scroller.addView(panel, new ScrollView.LayoutParams(-1, -2));
        final FrameLayout.LayoutParams layout = new FrameLayout.LayoutParams(-1, -2);
        layout.gravity = Gravity.BOTTOM; layout.setMargins(left, 0, right, bottom);
        menuScrim.addView(scroller, layout);
        setGraphicsEditorsEnabled(panel, !graphicsBusy);
        for (Button option : graphicsOptionButtons) option.setEnabled(false);
    }

    private LinearLayout addPreviewControlRow(LinearLayout panel) {
        final HorizontalScrollView scroll = new HorizontalScrollView(this);
        scroll.setHorizontalScrollBarEnabled(true);
        scroll.setVerticalScrollBarEnabled(false);
        scroll.setScrollbarFadingEnabled(false);
        scroll.setScrollBarStyle(View.SCROLLBARS_OUTSIDE_INSET);
        scroll.setScrollBarSize(dp(4));
        scroll.setPadding(0, 0, 0, dp(6)); // Separate the native bar from the full-size button targets.
        if (Build.VERSION.SDK_INT >= 29) {
            scroll.setHorizontalScrollbarThumbDrawable(new ColorDrawable(HordeUiTokens.BRASS));
            scroll.setHorizontalScrollbarTrackDrawable(new ColorDrawable(HordeUiTokens.IRON));
        }
        scroll.setContentDescription("Preview actions; scroll horizontally for more controls");
        final LinearLayout row = new LinearLayout(this); row.setOrientation(LinearLayout.HORIZONTAL);
        scroll.addView(row, new HorizontalScrollView.LayoutParams(-2, -2));
        final LinearLayout.LayoutParams layout = new LinearLayout.LayoutParams(-1, -2);
        layout.topMargin = dp(4); panel.addView(scroll, layout); return row;
    }

    private void returnFromGraphicsPreview() {
        graphicsPreviewImageOnly = false;
        graphicsRestoreDraftAfterPreview = graphicsOriginalPreviewDraft;
        requestGraphicsRevert(false);
        graphicsPreviewWanted = false; graphicsSceneRestoring = true;
        publishGraphicsPreview(false); showGraphicsPage();
    }

    private void showGraphicsPreviewImageOnly() {
        final LinearLayout row = new LinearLayout(this); row.setOrientation(LinearLayout.HORIZONTAL);
        graphicsPanel = row;
        graphicsControlsButton = addPreviewControl(row, "Controls", () -> {
            graphicsPreviewImageOnly = false; showGraphicsPreviewPage();
        });
        graphicsControlsButton.setContentDescription("Show graphics controls. Image inspection does not save; unconfirmed settings still revert after 15 visible seconds.");
        int left = dp(8), top = dp(8);
        final WindowInsets insets = menuScrim.getRootWindowInsets();
        if (insets != null) {
            left = Math.max(left, insets.getStableInsetLeft() + dp(8));
            top = Math.max(top, insets.getStableInsetTop() + dp(8));
            if (Build.VERSION.SDK_INT >= 28 && insets.getDisplayCutout() != null) {
                left = Math.max(left, insets.getDisplayCutout().getSafeInsetLeft() + dp(8));
                top = Math.max(top, insets.getDisplayCutout().getSafeInsetTop() + dp(8));
            }
        }
        final FrameLayout.LayoutParams layout = new FrameLayout.LayoutParams(-2, -2);
        layout.gravity = Gravity.TOP | Gravity.START; layout.setMargins(left, top, dp(8), dp(8));
        menuScrim.addView(row, layout); graphicsControlsButton.requestFocus();
    }

    private Button addPreviewControl(LinearLayout row, String text, Runnable action) {
        final Button button = createMenuButton(text, action);
        button.setSingleLine(true); button.setMinWidth(dp(48)); button.setMinHeight(dp(48));
        button.setMinimumWidth(dp(48)); button.setMinimumHeight(dp(48));
        button.setPadding(dp(12), dp(8), dp(12), dp(8));
        final LinearLayout.LayoutParams layout = new LinearLayout.LayoutParams(-2, -2);
        if (row.getChildCount() != 0) layout.leftMargin = dp(8);
        row.addView(button, layout); return button;
    }

    private void dismissGraphicsPreviewDetails() {
        dismissGraphicsPreviewOptions();
        if (graphicsDetailsDialog != null) graphicsDetailsDialog.dismiss();
        graphicsDetailsDialog = null; graphicsDetailsTelemetry = null; graphicsDetailsPanel = null; graphicsGraph = null;
    }

    private void showGraphicsPreviewDetails() {
        graphicsPreviewImageOnly = false;
        dismissGraphicsPreviewDetails();
        final LinearLayout panel = new LinearLayout(this); panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(dp(16), dp(12), dp(16), dp(12));
        panel.setBackgroundColor(HordeUiTokens.CHARCOAL); graphicsDetailsPanel = panel;
        graphicsDetailsTelemetry = new TextView(this);
        graphicsDetailsTelemetry.setTextColor(HordeUiTokens.PARCHMENT); graphicsDetailsTelemetry.setTextSize(15);
        graphicsDetailsTelemetry.setText(graphicsPreviewDetailsText); panel.addView(graphicsDetailsTelemetry, matchWrap());
        if (graphicsRecoveryNotice != null) addBody(panel, graphicsRecoveryNotice);
        graphicsGraph = new PreviewTimingGraphView(this);
        graphicsGraph.setSamples(graphicsPreviewDetailsSamples);
        panel.addView(graphicsGraph, new LinearLayout.LayoutParams(-1, dp(56)));
        addBody(panel, "Choose settings directly in this scene. Each choice retains the other live settings. Preview changes do not save; Apply then Keep is required. " +
                "Preview FPS counts successful RT swapchain presents per second, not display scanout FPS or full-game performance. Image hides the strip; Controls restores it.");
        addBody(panel, "Timing display is a snapshot while Details is open. Rendering and measurement continue; closing Details refreshes the display.");
        final Button pause = addGraphicsButton(panel, graphicsPreviewPaused ? "Resume preview animation" : "Pause preview animation", () -> {
            graphicsPreviewPaused = !graphicsPreviewPaused; publishGraphicsPreview(false);
            dismissGraphicsPreviewDetails(); showGraphicsPreviewDetails();
        });
        pause.setEnabled(!graphicsBusy);
        addGraphicsButton(panel, "Reset timeline", this::resetGraphicsPreviewTimeline);
        addGraphicsButton(panel, graphicsPreviewMotion ? "Stop motion test" : "Start motion test", () -> {
            graphicsPreviewMotion = !graphicsPreviewMotion; resetGraphicsPreviewTimeline();
            dismissGraphicsPreviewDetails(); showGraphicsPreviewDetails();
        });
        final ScrollView scroller = new ScrollView(this); scroller.addView(panel);
        final AlertDialog dialog = new AlertDialog.Builder(this).setTitle("Preview details")
                .setView(scroller).setPositiveButton("Close details", null).create();
        graphicsDetailsDialog = dialog;
        graphicsPreviewModalEpoch = graphicsPreviewDetailsSamples[0];
        dialog.setOnDismissListener(ignored -> {
            if (graphicsDetailsDialog == dialog) {
                graphicsDetailsDialog = null; graphicsDetailsTelemetry = null; graphicsDetailsPanel = null; graphicsGraph = null;
                requestGraphicsPreviewPresentationRefresh();
            }
        });
        dialog.show(); setGraphicsEditorsEnabled(panel, !graphicsBusy);
    }

    private void refreshGraphicsPreviewTelemetry(long[] applied, String settingsText) {
        if ((applied[14] & 64) != 0 && GraphicsPreferences.presented(applied, surfaceRequestGeneration) && applied[19] == 0) {
            graphicsPreviewWanted = false;
            graphicsPreviewDetailsSamples = new double[9];
            graphicsPreviewDetailsText = "Preview unavailable; the game scene was restored.";
            graphicsRecoveryNotice = "Preview could not be created; the previous game scene was restored.";
            showGraphicsPage(); return;
        }
        if ((applied[14] & 64) != 0 && GraphicsPreferences.presented(applied, surfaceRequestGeneration) &&
                applied[19] == 1 && applied[0] == graphicsRequestSerial &&
                !GraphicsPreferences.matchesEffective(applied, previewSelection())) {
            dismissGraphicsPreviewDetails();
            graphicsLiveChoiceError = "Choice could not be applied. The previous scene was restored; use Revert to restore confirmed settings.";
            graphicsPreviewDetailsSamples = new double[9];
            graphicsDisplayedFps = Double.NaN;
            setGraphicsPreviewStatus("Preview FPS: unavailable\nChoice failed; Revert");
            graphicsPreviewDetailsText = graphicsLiveChoiceError + "\n" + settingsText;
            return; // The restored tuple is honest evidence, not an ACK for the failed candidate.
        }
        // Only presentation updates freeze: ACK/failure/lifecycle handling remains operational.
        if (graphicsPreviewModalOpen()) {
            final double[] current = ProbeBridge.getGraphicsPreviewPerformance();
            final boolean changedScope = graphicsPreviewModalEpoch > 0 && current != null && current.length >= 9 &&
                    current[0] > 0 && current[0] != graphicsPreviewModalEpoch;
            if (!GraphicsPreviewOptions.presented(applied, surfaceRequestGeneration, graphicsRequestSerial, previewSelection()) || changedScope) {
                dismissGraphicsPreviewDetails();
                graphicsPreviewDetailsSamples = new double[9];
                graphicsPreviewDetailsText = "Waiting for current RT preview presentation.";
            } else return; // Native measurements and visible confirmation timeout still advance.
        }
        if (!GraphicsPreferences.presented(applied, surfaceRequestGeneration) || applied[19] != 1) {
            setGraphicsPreviewStatus("Preview FPS: waiting for RT");
            graphicsPreviewDetailsText = "Loading actual RT preview...\n" + settingsText;
            graphicsPreviewDetailsSamples = new double[9];
            if (graphicsDetailsTelemetry != null) graphicsDetailsTelemetry.setText(graphicsPreviewDetailsText);
            if (graphicsGraph != null) graphicsGraph.setSamples(new double[9]);
            return;
        }
        final boolean matching = GraphicsPreviewOptions.presented(applied, surfaceRequestGeneration,
                graphicsRequestSerial, previewSelection());
        final String state = graphicsLiveChoiceError != null ? "choice unavailable; Revert" : applied[2] == 2 && matching ? "Keep? " + Math.max(0, 15 -
                (SystemClock.elapsedRealtime() - graphicsConfirmationStarted) / 1000) + "s" : matching ? "RT ready" :
                (applied[14] & 64) != 0 ? "unavailable; previous output retained" : "waiting for RT";
        final String[] cameras = {"Overview", "Materials", "Glass", "Water", "Skeleton", "Mirror"};
        final double[] p = matching ? ProbeBridge.getGraphicsPreviewPerformance() : null;
        final boolean currentTiming = matching && GraphicsPreviewOptions.currentPerformance(p, graphicsPreviewPerformanceEpochFloor);
        final long now = SystemClock.elapsedRealtime();
        if (currentTiming && (Double.isNaN(graphicsDisplayedFps) || graphicsDisplayedFpsEpoch != p[0] || now >= graphicsNextFpsUpdate)) {
            graphicsDisplayedFps = p[1]; graphicsDisplayedFpsEpoch = p[0]; graphicsNextFpsUpdate = now + 1000;
        }
        final String fps = currentTiming ? String.format(Locale.US,"%.1f",graphicsDisplayedFps) : "waiting for RT";
        setGraphicsPreviewStatus("Preview FPS: " + fps + "\n" + cameras[graphicsPreviewCamera] + " · " + state);
        if (!matching) {
            graphicsPreviewDetailsText = "Waiting for these settings' current RT presentation.\n" + settingsText;
            graphicsPreviewDetailsSamples = new double[9];
            if (graphicsLiveChoiceError != null) graphicsPreviewDetailsText += "\n" + graphicsLiveChoiceError;
            if (graphicsDetailsTelemetry != null) graphicsDetailsTelemetry.setText(graphicsPreviewDetailsText);
            if (graphicsGraph != null) graphicsGraph.setSamples(new double[9]);
            return;
        }
        if (!currentTiming) {
            graphicsPreviewDetailsText = settingsText + "\nWaiting for current-epoch preview timing and allocations.";
            graphicsPreviewDetailsSamples = new double[9];
            if (graphicsDetailsTelemetry != null) graphicsDetailsTelemetry.setText(graphicsPreviewDetailsText);
            if (graphicsGraph != null) graphicsGraph.setSamples(new double[9]);
            return;
        }
        final String gpu = p[4] < 0 ? "unavailable" : String.format(Locale.US, "%.2f ms", p[4]);
        final String deviceLocal = p[5] < 0 ? "unavailable" : String.format(Locale.US, "%.2f MiB", p[5]/1048576.0);
        final String hostVisible = p[6] < 0 ? "unavailable" : String.format(Locale.US, "%.2f MiB", p[6]/1048576.0);
        graphicsPreviewDetailsText = "Preview scene performance\n" + settingsText + String.format(Locale.US,
                "\nRT successful presents/s: %.1f\nStable loop: %.2f ms / CPU render call: %.2f ms / GPU: %s" +
                "\nCap: %d Hz / transitions in graph: %.0f\nTracked scene device-local: %s; host-visible: %s" +
                "\nAllocation classifications may overlap. GPU budget/residency unavailable." +
                "\nSuccessful swapchain presents are not display scanout FPS. Preview is not a full-game benchmark.",
                p[1], p[2], p[3], gpu, applied[6], p[7], deviceLocal, hostVisible);
        if (graphicsLiveChoiceError != null) graphicsPreviewDetailsText += "\n" + graphicsLiveChoiceError;
        graphicsPreviewDetailsSamples = p;
        if (graphicsDetailsTelemetry != null) graphicsDetailsTelemetry.setText(graphicsPreviewDetailsText);
        if (graphicsGraph != null) graphicsGraph.setSamples(p);
    }

    private void setGraphicsPreviewStatus(String status) {
        // Keep native 4 Hz sampling without repeated accessibility/text events or FPS label flicker.
        if (!graphicsPreviewModalOpen() && !status.contentEquals(graphicsTelemetry.getText())) graphicsTelemetry.setText(status);
    }

    private Button addGraphicsButton(LinearLayout panel, String text, Runnable action) {
        final Button button = createMenuButton(text, action);
        button.setMinHeight(dp(48)); button.setTextColor(0xFFF2E9D8);
        final LinearLayout.LayoutParams layout = new LinearLayout.LayoutParams(-1, -2);
        layout.topMargin = dp(8); panel.addView(button, layout); return button;
    }

    private String waterName(int quality) { return quality == 2 ? "High" : quality == 1 ? "Mobile" : "Off"; }

    private void confirmGraphicsSelection() {
        if (!ProbeBridge.confirmGraphicsSettings(graphicsRequestSerial, surfaceRequestGeneration)) return;
        if (GraphicsPreferences.confirm(preferences, graphicsSubmitted)) {
            graphicsConfirmed = graphicsSubmitted; graphicsDraft = graphicsSubmitted;
        } else {
            // A storage failure must also rebase the native session: otherwise
            // a later Back/Revert would restore its unsaved committed candidate.
            ProbeBridge.beginGraphicsEdit(graphicsConfirmed.scale, graphicsConfirmed.water,
                    graphicsConfirmed.fire, graphicsConfirmed.cap, graphicsConfirmed.glassEnabled);
            if (graphicsPreviewWanted) {
                // Match the actual restored tuple; the original outside draft remains separate.
                graphicsDraft = graphicsConfirmed;
                resetGraphicsPreviewTimeline();
            }
            graphicsAwaitingRestore = true;
            graphicsRequestSerial = ProbeBridge.revertGraphicsSettings(surfaceRequestGeneration);
            if (graphicsRequestSerial == 0) { setNativeGraphics(graphicsConfirmed); graphicsRecovering = true; }
            Toast.makeText(this, R.string.graphics_save_failed, Toast.LENGTH_LONG).show();
        }
        graphicsConfirmationStarted = 0;
    }

    private void saveInterface(InterfacePreferences.Values values) {
        if(!InterfacePreferences.save(preferences,values)) {
            Toast.makeText(this,"Interface settings could not be saved.",Toast.LENGTH_LONG).show(); return;
        }
        applyInterfacePresentation(); showInterfaceSettings();
    }

    private void showInterfaceSettings() {
        interfaceVisible=true;
        menuScrim.removeAllViews();
        final InterfacePreferences.Values v=InterfacePreferences.read(preferences);
        final LinearLayout panel=createPanel("Interface / HUD","PRESENTATION ONLY");
        addBody(panel,"Preview the paused action cluster below. Move/look sides and action timing stay the same.");
        final FrameLayout preview=new FrameLayout(this);
        final Button swing=new Button(this), parry=new Button(this), interact=new Button(this), light=new Button(this);
        swing.setText(R.string.swing); parry.setText(R.string.parry); interact.setText(R.string.interact); light.setText(R.string.raise_lantern);
        for(Button b:new Button[]{swing,parry,interact,light}) {
            b.setAllCaps(false); b.setTextSize(17); b.setClickable(false); b.setFocusable(false);
            b.setMaxLines(b==interact?2:1); b.setIncludeFontPadding(false);
            b.setPadding(b.getPaddingLeft(),0,b.getPaddingRight(),0);
            if(Build.VERSION.SDK_INT>=26) b.setAutoSizeTextTypeUniformWithConfiguration(
                    b==swing || b==parry?10:9,17,1,android.util.TypedValue.COMPLEX_UNIT_SP);
            b.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO); preview.addView(b);
            styleActionButton(b,interfaceBacking(v,HordeUiTokens.SLATE),HordeUiTokens.PARCHMENT);
        }
        final android.widget.HorizontalScrollView previewScroll=new android.widget.HorizontalScrollView(this);
        previewScroll.setContentDescription("Paused control layout preview");
        final int previewWidth=dp(360),previewHeight=dp(230);
        UiControlLayout.apply(this,v,swing,parry,interact,light,0,0,previewWidth);
        previewScroll.addView(preview,new ViewGroup.LayoutParams(previewWidth,previewHeight));
        panel.addView(previewScroll,new LinearLayout.LayoutParams(-1,previewHeight));
        addMenuButton(panel,"Presentation: "+(v.compact?"Compact":"Comfortable"),() -> {
            InterfacePreferences.Values current=InterfacePreferences.read(preferences);
            saveInterface(new InterfacePreferences.Values(!current.compact,current.scale,current.opacity,current.strongerBacking,current.routineStatus));
        });
        addSlider(panel,"Control scale",v.scale,85,110,value -> {
            InterfacePreferences.Values current=InterfacePreferences.read(preferences);
            InterfacePreferences.Values next=new InterfacePreferences.Values(current.compact,value,current.opacity,current.strongerBacking,current.routineStatus);
            InterfacePreferences.saveLive(preferences,next);
            applyInterfacePresentation(); UiControlLayout.apply(this,next,swing,parry,interact,light,0,0,previewWidth);
        });
        addSlider(panel,"Backing opacity",v.opacity,55,95,value -> {
            InterfacePreferences.Values current=InterfacePreferences.read(preferences);
            InterfacePreferences.Values next=new InterfacePreferences.Values(current.compact,current.scale,value,current.strongerBacking,current.routineStatus);
            InterfacePreferences.saveLive(preferences,next);
            applyInterfacePresentation();
            for(Button b:new Button[]{swing,parry,interact,light}) styleActionButton(b,interfaceBacking(next,HordeUiTokens.SLATE),HordeUiTokens.PARCHMENT);
        });
        addMenuButton(panel,"Stronger backing: "+(v.strongerBacking?"On":"Off"),() -> {
            InterfacePreferences.Values current=InterfacePreferences.read(preferences);
            saveInterface(new InterfacePreferences.Values(current.compact,current.scale,current.opacity,!current.strongerBacking,current.routineStatus));
        });
        addMenuButton(panel,"Routine RT status: "+(v.routineStatus?"Shown":"Hidden"),() -> {
            InterfacePreferences.Values current=InterfacePreferences.read(preferences);
            saveInterface(new InterfacePreferences.Values(current.compact,current.scale,current.opacity,current.strongerBacking,!current.routineStatus));
        });
        addBody(panel,"Labels and focus outlines remain opaque. Startup, unsupported-device and error diagnostics remain available.");
        addMenuButtonRow(panel,"Reset Interface only",() -> {
            if(InterfacePreferences.reset(preferences)) { applyInterfacePresentation(); showInterfaceSettings(); }
        },getString(R.string.back),this::showSettings);
        attachPanel(panel);
    }

    private int interfaceBacking(InterfacePreferences.Values v,int color) {
        int alpha=Math.round((v.strongerBacking?95:v.opacity)*255/100f);
        return (alpha<<24)|(color&0x00ffffff);
    }

    private void applyInterfacePresentation() {
        if(preferences==null || attackButton==null || parryButton==null || interactButton==null || toggleHeldLightPoseButton==null) return;
        final InterfacePreferences.Values v=InterfacePreferences.read(preferences);
        final WindowInsets insets=menuScrim==null?null:menuScrim.getRootWindowInsets();
        int safeLeft=0,safeRight=0,safeTop=0,safeBottom=0;
        if(insets!=null) {
            safeLeft=insets.getStableInsetLeft(); safeRight=insets.getStableInsetRight();
            safeTop=insets.getStableInsetTop(); safeBottom=insets.getStableInsetBottom();
            if(Build.VERSION.SDK_INT>=28 && insets.getDisplayCutout()!=null) {
                safeLeft=Math.max(safeLeft,insets.getDisplayCutout().getSafeInsetLeft());
                safeRight=Math.max(safeRight,insets.getDisplayCutout().getSafeInsetRight());
                safeTop=Math.max(safeTop,insets.getDisplayCutout().getSafeInsetTop());
                safeBottom=Math.max(safeBottom,insets.getDisplayCutout().getSafeInsetBottom());
            }
        }
        UiControlLayout.apply(this,v,attackButton,parryButton,interactButton,toggleHeldLightPoseButton,
                safeRight,safeBottom,getResources().getDisplayMetrics().widthPixels-safeLeft);
        styleActionButton(attackButton,interfaceBacking(v,HordeUiTokens.CHARCOAL),HordeUiTokens.PARCHMENT);
        for(Button b:new Button[]{parryButton,interactButton,toggleHeldLightPoseButton})
            styleActionButton(b,interfaceBacking(v,HordeUiTokens.SLATE),HordeUiTokens.PARCHMENT);
        if(menuButton!=null) {
            styleActionButton(menuButton,interfaceBacking(v,HordeUiTokens.SLATE),HordeUiTokens.PARCHMENT);
            FrameLayout.LayoutParams p=(FrameLayout.LayoutParams)menuButton.getLayoutParams();
            p.setMarginEnd(dp(16)+safeRight); p.topMargin=dp(16)+safeTop; menuButton.setLayoutParams(p);
        }
        if(vitalityStatus!=null) {
            FrameLayout.LayoutParams p=(FrameLayout.LayoutParams)vitalityStatus.getLayoutParams();
            p.gravity=Gravity.TOP|Gravity.START; p.setMarginStart(dp(16)+safeLeft); p.topMargin=dp(16)+safeTop;
            p.height=FrameLayout.LayoutParams.WRAP_CONTENT; vitalityStatus.setLayoutParams(p);
            vitalityStatus.setPadding(dp(10),dp(8),dp(10),dp(8)); vitalityStatus.setTextSize(12);
            vitalityStatus.setMaxWidth(Math.max(dp(120),getResources().getDisplayMetrics().widthPixels-safeLeft-safeRight-dp(128)));
            vitalityStatus.setBackground(HordeUiTokens.plate(this,interfaceBacking(v,HordeUiTokens.CHARCOAL),HordeUiTokens.IRON,1));
        }
    }

    private void layoutRtStatus(boolean compact) {
        if(rtStatus==null) return;
        FrameLayout.LayoutParams p=(FrameLayout.LayoutParams)rtStatus.getLayoutParams();
        int gravity=compact?Gravity.TOP|Gravity.END:Gravity.TOP|Gravity.CENTER_HORIZONTAL;
        if(p.gravity!=gravity || p.height!=(compact?dp(48):-2)) {
            p.gravity=gravity; p.height=compact?dp(48):-2; p.width=-2;
            int top=dp(compact?64:16),right=0;
            WindowInsets insets=rtStatus.getRootWindowInsets();
            if(insets!=null) { top+=insets.getStableInsetTop(); right=insets.getStableInsetRight(); }
            p.topMargin=top; p.setMarginEnd(compact?dp(16)+right:0); rtStatus.setLayoutParams(p);
        }
        rtStatus.setMinWidth(dp(compact?48:260)); rtStatus.setMaxWidth(dp(compact?80:360));
        rtStatus.setTextSize(compact?12:13);
    }

    private void requestGraphicsRevert(boolean close) {
        dismissGraphicsPreviewDetails();
        if (close) graphicsPreviewImageOnly = false;
        graphicsLiveChoiceError = null;
        if (graphicsPreviewWanted) { graphicsDraft = graphicsConfirmed; resetGraphicsPreviewTimeline(); }
        if (close) graphicsRestoreDraftAfterPreview = null;
        if (graphicsPreviewWanted && close) {
            graphicsPreviewWanted = false;
            graphicsSceneRestoring = true;
            publishGraphicsPreview(false);
        }
        graphicsCloseAfterRevert = close;
        graphicsAwaitingRestore = true;
        graphicsBusy = true;
        setGraphicsEditorsEnabled(graphicsPanel, false);
        graphicsRequestSerial = ProbeBridge.revertGraphicsSettings(surfaceRequestGeneration);
        graphicsConfirmationStarted = 0;
        if (graphicsRequestSerial == 0) {
            setNativeGraphics(graphicsConfirmed);
            graphicsRecovering = true;
            if (close) { graphicsVisible = false; showSettings(); }
        }
    }

    private final Runnable refreshGraphics = new Runnable() {
        @Override public void run() {
            if (!graphicsVisible && !graphicsRecovering && !graphicsSceneRestoring) return;
            if (!resumed) return;
            final long now = SystemClock.elapsedRealtime();
            final double seconds = graphicsPollTime == 0 ? 0 : Math.max(0, now - graphicsPollTime) / 1000.0;
            graphicsPollTime = now;
            final long[] a = ProbeBridge.getGraphicsSnapshot();
            if (a == null || a.length != 22) { handler.postDelayed(this, 250); return; }
            final boolean presented = GraphicsPreferences.presented(a, surfaceRequestGeneration);
            if (graphicsRecovering && presented && GraphicsPreferences.matchesEffective(a, graphicsConfirmed)) {
                if (GraphicsPreferences.clearAfterRestore(preferences)) {
                    graphicsRecovering = false;
                    Toast.makeText(MainActivity.this, R.string.graphics_restored, Toast.LENGTH_LONG).show();
                }
            }
            if (graphicsSceneRestoring && presented && a[19] == 0) graphicsSceneRestoring = false;
            if (graphicsVisible) {
                final int state = (int)a[2];
                final boolean busy = state == 1 || state == 2 || state == 4;
                graphicsBusy = busy;
                setGraphicsEditorsEnabled(graphicsPanel, !busy);
                if (graphicsDetailsPanel != null) setGraphicsEditorsEnabled(graphicsDetailsPanel, !busy);
                final GraphicsPreferences.Values expected = graphicsAwaitingRestore ? graphicsConfirmed :
                        graphicsSubmitted != null ? graphicsSubmitted : graphicsConfirmed;
                final boolean ready = presented && a[0] == graphicsRequestSerial && (graphicsPreviewWanted ?
                        GraphicsPreviewOptions.presented(a, surfaceRequestGeneration, graphicsRequestSerial, previewSelection()) :
                        GraphicsPreferences.matchesEffective(a, expected) && GraphicsPreferences.matchesRequested(a, expected));
                if (state == 2 && ready) {
                    final boolean firstConfirmationPoll = graphicsConfirmationStarted == 0;
                    if (firstConfirmationPoll) graphicsConfirmationStarted = now;
                    final long reverted = ProbeBridge.advanceGraphicsConfirmation(firstConfirmationPoll ? 0 : seconds,
                            true, surfaceRequestGeneration);
                    if (reverted != 0) { graphicsRequestSerial = reverted; graphicsConfirmationStarted = 0;
                        if (graphicsPreviewWanted) { graphicsDraft = graphicsConfirmed; resetGraphicsPreviewTimeline(); }
                        graphicsAwaitingRestore = true; }
                }
                if (graphicsSceneRestoring && ready && a[19] == 1 && (a[14] & 64) != 0) {
                    graphicsSceneRestoring = false; graphicsPreviewWanted = true; graphicsCloseAfterRevert = false;
                    graphicsRecoveryNotice = "The game scene could not be restored. The previous preview is available; try Return to Graphics again.";
                    showGraphicsPage();
                }
                if (graphicsSceneRestoring && presented && a[19] == 0) graphicsSceneRestoring = false;
                if (state == 0 && ready && graphicsAwaitingRestore && !graphicsSceneRestoring) {
                    graphicsAwaitingRestore = false;
                    GraphicsPreferences.clearAfterRestore(preferences);
                    graphicsDraft = graphicsRestoreDraftAfterPreview != null ? graphicsRestoreDraftAfterPreview : graphicsConfirmed;
                    graphicsRestoreDraftAfterPreview = null;
                    if (graphicsCloseAfterRevert && a[19] == 0) {
                        graphicsVisible = false; handler.removeCallbacks(this); showSettings(); return;
                    }
                    showGraphicsPage();
                }
                graphicsApply.setEnabled(!busy && ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) == 1);
                final boolean previewReady = graphicsPreviewWanted && GraphicsPreviewOptions.presented(a,
                        surfaceRequestGeneration, graphicsRequestSerial, previewSelection());
                graphicsConfirm.setEnabled(graphicsPreviewWanted ? previewReady && graphicsLiveChoiceError == null && ((state == 2 && ready) || !busy) : state == 2 && ready);
                if (graphicsPreviewWanted) {
                    for (int choice = 0; choice < graphicsOptionButtons.length; ++choice) {
                        final Button option = graphicsOptionButtons[choice];
                        if (option != null) {
                            final String label = graphicsOptionLabel(choice);
                            if (!label.contentEquals(option.getText())) option.setText(label);
                            option.setContentDescription(label + "; choose live preview setting");
                            option.setEnabled(!busy && previewReady && graphicsLiveChoiceError == null);
                        }
                    }
                    final String confirmLabel = state == 2 ? "Keep" : "Apply";
                    final String confirmDescription = state == 2 ? getString(R.string.graphics_keep) : "Apply live preview settings; confirmation is required";
                    if (!confirmLabel.contentEquals(graphicsConfirm.getText())) graphicsConfirm.setText(confirmLabel);
                    if (!confirmDescription.equals(graphicsConfirm.getContentDescription())) graphicsConfirm.setContentDescription(confirmDescription);
                }
                graphicsRevert.setEnabled(state != 4);
                final String preset = graphicsDraft.same(GraphicsPreferences.baseline()) ?
                        "Accepted 1.6.1 baseline" : "Custom";
                final String status = state == 1 ? "Applying - waiting for an RT frame" : state == 4 ? "Restoring confirmed settings" :
                        state == 5 ? "Apply failed; previous output retained" : state == 2 ?
                        "Keep these settings? " + Math.max(0, 15 - (now - graphicsConfirmationStarted) / 1000) + " seconds" :
                        state == 3 ? "Confirmed and saved" : "Draft changes require Apply";
                final String settingsText = preset + "\n" + status + "\nRequested: " + graphicsDraft.scale + "% / water " +
                        waterName(graphicsDraft.water) + " / fire " + (graphicsDraft.fire == 1 ? "High" : "Mobile") +
                        " / glass " + (graphicsDraft.glassEnabled ? "On" : "Off") +
                        (presented ? " / planned " + ((a[9] * graphicsDraft.scale + 50) / 100) + " x " +
                        ((a[10] * graphicsDraft.scale + 50) / 100) : " / planned extent unavailable") +
                        "\nEffective: " + (presented ? a[3] + "% / " + a[7] + " x " + a[8] + " internal / " +
                        a[9] + " x " + a[10] + " output / water " + waterName((int)a[4]) + " / fire " +
                        (a[5] == 1 ? "High" : "Mobile") + " / glass " + (a[20] == 1 ? "On" : "Off") : "not yet presented") + "\n" +
                        (presented ? (a[11] == 1 ? getString(R.string.graphics_optics_high) : getString(R.string.graphics_optics_mobile)) :
                        "Optical profile: unavailable until an RT frame presents.") + "\nBackend: " +
                        (presented ? (a[12] == 2 ? "RayQueryCompute" : "RayTracingPipeline") : "not yet presented");
                if (graphicsPreviewWanted) refreshGraphicsPreviewTelemetry(a, settingsText);
                else graphicsTelemetry.setText(settingsText);
            }
            handler.postDelayed(this, 250);
        }
    };

    private int musicVolumePercent() {
        return Math.max(0, Math.min(100, preferences.getInt(PREF_MUSIC_VOLUME, 70)));
    }

    private void setBenchmarkStatusExpanded(boolean expanded) {
        if (rtStatus == null) return;
        layoutRtStatus(false);
        benchmarkStatusExpanded = expanded;
        rtStatus.setMaxLines(expanded ? 3 : 1);
        final ViewGroup.LayoutParams layout = rtStatus.getLayoutParams();
        layout.height = expanded ? ViewGroup.LayoutParams.WRAP_CONTENT : dp(40);
        rtStatus.setLayoutParams(layout);
    }

    private void setGameplayPaused(boolean paused) {
        if (paused) suspendAndResetWaterfall(); // Same generation retains the Core cursor.
        if (musicPlayback != null) musicPlayback.setSuspended(paused || !resumed ||
                !surfaceStarted || ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) != 1);
        ProbeBridge.setSimulationPaused(paused); // Existing JNI mailbox authority unchanged.
    }

    private void showDiagnostics(final boolean errorState) {
        menuVisible = true;
        diagnosticsVisible = true;
        diagnosticsErrorState = errorState;
        diagnosticsRefreshTick = 0;
        setGameplayPaused(true);
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
        suspendAndResetWaterfall();
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
        vitalityStatus.setCompoundDrawables(new VitalitySegmentsDrawable(this,safeVitality),null,null,null);
        vitalityStatus.setCompoundDrawablePadding(dp(8));
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
        if (deathOverlayVisible || graphicsVisible || playtestReportVisible || benchmarkRunning || debugCaptureUiSuppressed) return;
        deathOverlayVisible = true;
        menuVisible = true;
        setGameplayPaused(true);
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
                menuVisible || graphicsVisible || playtestReportVisible || rtLabVisible || benchmarkRunning || debugCaptureUiSuppressed) return;
        endingOverlayVisible = true;
        menuVisible = true;
        setGameplayPaused(true);
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
        setGameplayPaused(true);
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
        background.setColor(0xD9151719);
        background.setCornerRadius(dp(4));
        background.setStroke(dp(1), HordeUiTokens.BRASS);
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
        if (benchmarkAutomationId != null) return;
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
        if (benchmarkAutomationId != null || debugCaptureUiSuppressed || debugAutomationAutostart ||
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
        suspendAndResetWaterfall();
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
                if (benchmarkAutomationId != null && !benchmarkAutomationFinishing &&
                        SystemClock.elapsedRealtime() - benchmarkAutomationStartedAt >=
                            BENCHMARK_AUTOMATION_TIMEOUT_MS) {
                    finishBenchmarkAutomation(3);
                }
                final int state = ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration);
                surfaceStarted = resumed && surfaceAvailable && surfaceRequestGeneration != 0 && state == 1;
                if (musicPlayback != null) musicPlayback.setSuspended(!resumed || !surfaceStarted ||
                        state != 1 || menuVisible || diagnosticsVisible);
                if (state == 1) {
                    if(!benchmarkRunning) layoutRtStatus(true);
                    rtStatus.setText(benchmarkRunning?R.string.rt_active:R.string.rt_active_compact);
                    rtStatus.setContentDescription(getString(R.string.rt_active)+"; opens diagnostics");
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
                    if (!menuVisible && !benchmarkRunning && !debugCaptureUiSuppressed)
                        rtStatus.setVisibility(showHud && InterfacePreferences.read(preferences).routineStatus ? View.VISIBLE : View.GONE);
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
                    if (finaleEndingPhase == FINALE_ENDING_COMPLETE && !benchmarkRunning &&
                            benchmarkAutomationId == null) {
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
                    layoutRtStatus(false);
                    updateContextualControls(false);
                    rtStatus.setText(R.string.rt_unsupported);
                    rtStatus.setContentDescription(getString(R.string.rt_unsupported));
                    rtStatus.setTextColor(0xFFFF8A7A);
                    if (!autoDiagnosticsShown) {
                        autoDiagnosticsShown = true;
                        showDiagnostics(false);
                    }
                } else if (state == 3) {
                    layoutRtStatus(false);
                    updateContextualControls(false);
                    rtStatus.setText(R.string.rt_error);
                    rtStatus.setContentDescription(getString(R.string.rt_error));
                    rtStatus.setTextColor(0xFFFF8A7A);
                    if (!autoDiagnosticsShown) {
                        autoDiagnosticsShown = true;
                        showDiagnostics(true);
                    }
                } else {
                    updateContextualControls(false);
                    rtStatus.setText(R.string.rt_starting);
                }

                if (benchmarkAutomationId != null && !benchmarkAutomationFinishing) {
                    if (state == 2 || state == 3) {
                        finishBenchmarkAutomation(3);
                    } else if (benchmarkAutomationPending && !benchmarkRunning && resumed && state == 1) {
                        startBenchmark();
                        if (!benchmarkRunning) finishBenchmarkAutomation(3);
                    }
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
                        if (benchmarkAutomationId != null) finishBenchmarkAutomation(benchmarkStatus);
                        else showBenchmarkReport(benchmarkStatus == 2);
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

                if (keeperRevealTitle != null) {
                    final float titleOpacity = !menuVisible && !diagnosticsVisible && !graphicsVisible &&
                            !rtLabVisible && !playtestReportVisible && !benchmarkRunning &&
                            !debugCaptureUiSuppressed && resumed && state == 1 ?
                            ProbeBridge.getKeeperRevealTitleOpacity() : 0;
                    keeperRevealTitle.setAlpha(Math.max(0, Math.min(1, titleOpacity)));
                    keeperRevealTitle.setVisibility(titleOpacity > 0 ? View.VISIBLE : View.GONE);
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
                                    "player_step_1" : "player_step_2", 0.45f, stereoGains);
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
                                            playSpatialSound("skeleton_falling_bones", 0.24f, stereoGains);
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
                        case PLATFORM_EVENT_SKELETON_INCIDENTAL:
                            playSpatialSound("skeleton_idle_rattle", 0.10f, stereoGains);
                            break;
                        case PLATFORM_EVENT_KEEPER_REVEAL_STARTED:
                            playSpatialSound("keeper_i_sense_you", 0.36f, stereoGains);
                            break;
                        case PLATFORM_EVENT_KEEPER_WARNING:
                            playSpatialSound("keeper_come_closer", 0.36f, stereoGains);
                            break;
                        case PLATFORM_EVENT_KEEPER_COMBAT_READY:
                            // Readiness has no separate admitted sound; title polls the snapshot.
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

    static boolean shouldRequireRayQueryCompute(final boolean debugBuild, final Intent intent) {
        return debugBuild && intent != null &&
                intent.getBooleanExtra(EXTRA_REQUIRE_RAYQUERY_COMPUTE, false);
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
            case "lantern-chest-held-high": return 135;
            case "player-viewmodel-grips": return 136;
            case "player-viewmodel-forward": return 137;
            case "player-viewmodel-downward-cut": return 138;
            case "player-viewmodel-upward-slice": return 139;
            case "player-viewmodel-look-up": return 140;
            case "player-viewmodel-look-down": return 141;
            case "player-viewmodel-lantern-high": return 142;
            case "player-viewmodel-lantern-low": return 143;
            case "player-viewmodel-lantern-low-parry": return 144;
            case "player-viewmodel-lantern-low-look-down": return 145;
            case "player-viewmodel-lantern-high-look-up": return 146;
            default: return -1;
        }
    }

    private void consumeDebugAutomationIntent(final Intent intent) {
        final boolean requireRayQueryCompute =
                shouldRequireRayQueryCompute(isDebuggableApp(), intent);
        ProbeBridge.setRequiredRayQueryCompute(requireRayQueryCompute);
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
                    " requireRayQueryCompute=" + requireRayQueryCompute +
                    " rtLab=" + hasRtLabIntent);
        }
    }

    @SuppressLint("UnspecifiedRegisterReceiverFlag") // API 24-32 require the legacy overload.
    private void registerDebugRetryReceiver() {
        debugRetryReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(final Context context, final Intent intent) {
                if (intent == null || !DEBUG_RETRY_ACTION.equals(intent.getAction())) return;
                if (ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) != 1 ||
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

    static float[] developmentCheckpointViewPose(final int checkpoint) {
        switch (checkpoint) {
            case 136: return new float[]{0.0f, -0.32f};
            case 137: return new float[]{0.0f, -0.05f};
            case 138: return new float[]{0.0f, -0.28f};
            case 139: return new float[]{0.0f, -0.28f};
            case 140: return new float[]{0.0f, 0.28f};
            case 141: return new float[]{0.0f, -0.32f};
            case 142:
            case 143:
            case 144: return new float[]{-1.5707963f, -0.30f};
            case 145: return new float[]{-1.5707963f, -0.32f};
            case 146: return new float[]{-1.5707963f, 0.28f};
            default: return null;
        }
    }

    private void applyCheckpointViewPose(final int checkpoint) {
        final float[] developmentPose = developmentCheckpointViewPose(checkpoint);
        if (developmentPose != null) {
            viewControls[0] = developmentPose[0];
            viewControls[1] = developmentPose[1];
            return;
        }
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
        loadSound("keeper_i_sense_you", "audio/pixabay/keeper_i_sense_you.wav");
        loadSound("keeper_come_closer", "audio/pixabay/keeper_come_closer.wav");
        loadSound("skeleton_idle_rattle", "audio/pixabay/skeleton_idle_rattle.wav");
        loadSound("skeleton_falling_bones", "audio/pixabay/skeleton_falling_bones.wav");
        initialiseWaterfallLoop();
    }

    private void initialiseWaterfallLoop() {
        waterfallPlayback = new HordeAmbiencePlayback(this, this::isMusicAudioFocusGranted);
        suspendAndResetWaterfall();
    }

    private boolean isMusicAudioFocusGranted() {
        final HordeMusicPlayback playback = musicPlayback;
        return playback != null && playback.isAudioFocusGranted();
    }

    private void suspendAndResetWaterfall() {
        if (waterfallPlayback != null)
            waterfallPlayback.setControl(true, 0.0f, 0.0f, delayedGameplayFeedbackGeneration);
    }

    private void updateWaterfallLoop() {
        if (waterfallPlayback == null) return;
        final boolean audible = resumed && surfaceStarted && !menuVisible && !diagnosticsVisible &&
                !benchmarkRunning && preferences.getBoolean("sfx_enabled", true) &&
                ProbeBridge.getSurfaceRuntimeState(surfaceRequestGeneration) == 1;
        if (!audible) {
            suspendAndResetWaterfall();
            return;
        }

        final long packedStereoGains = ProbeBridge.getWaterfallStereoGains();
        final float leftScale = clamp(
                Float.intBitsToFloat((int) packedStereoGains), 0.0f, 1.0f);
        final float rightScale = clamp(
                Float.intBitsToFloat((int) (packedStereoGains >>> 32)), 0.0f, 1.0f);
        final float userGain = preferences.getInt("sfx_volume", 70) / 100.0f;
        waterfallPlayback.setControl(false,
                clamp(userGain * leftScale, 0.0f, 1.0f),
                clamp(userGain * rightScale, 0.0f, 1.0f), delayedGameplayFeedbackGeneration);
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
        background.setColor(0xF2151719);
        background.setCornerRadius(dp(4));
        background.setStroke(dp(1), HordeUiTokens.BRASS);
        panel.setBackground(background);

        final TextView eyebrowView = new TextView(this);
        eyebrowView.setText(eyebrow);
        eyebrowView.setTextColor(HordeUiTokens.BRASS);
        eyebrowView.setTextSize(10);
        eyebrowView.setTypeface(Typeface.SANS_SERIF, Typeface.BOLD);
        eyebrowView.setLetterSpacing(0.12f);
        panel.addView(eyebrowView, matchWrap());

        final TextView titleView = new TextView(this);
        titleView.setText(title);
        titleView.setTextColor(HordeUiTokens.PARCHMENT);
        titleView.setTextSize(22);
        titleView.setTypeface(Typeface.create(Typeface.SERIF, Typeface.BOLD));
        titleView.setPadding(0, dp(5), 0, dp(14));
        panel.addView(titleView, matchWrap());
        return panel;
    }

    private void attachPanel(final LinearLayout panel) {
        menuScrim.setBackgroundColor(0xC7080706);
        final ScrollView scroller = new ScrollView(this);
        scroller.setFillViewport(false);
        scroller.addView(panel, new ScrollView.LayoutParams(ScrollView.LayoutParams.MATCH_PARENT, ScrollView.LayoutParams.WRAP_CONTENT));
        final int screenWidth = getResources().getDisplayMetrics().widthPixels;
        int left=dp(16),right=dp(16),top=dp(16),bottom=dp(16);
        final WindowInsets insets=menuScrim.getRootWindowInsets();
        if(insets!=null) {
            left=Math.max(left,insets.getStableInsetLeft()+dp(8)); right=Math.max(right,insets.getStableInsetRight()+dp(8));
            top=Math.max(top,insets.getStableInsetTop()+dp(8)); bottom=Math.max(bottom,insets.getStableInsetBottom()+dp(8));
            if(Build.VERSION.SDK_INT>=28 && insets.getDisplayCutout()!=null) {
                left=Math.max(left,insets.getDisplayCutout().getSafeInsetLeft()+dp(8));
                right=Math.max(right,insets.getDisplayCutout().getSafeInsetRight()+dp(8));
                top=Math.max(top,insets.getDisplayCutout().getSafeInsetTop()+dp(8));
                bottom=Math.max(bottom,insets.getDisplayCutout().getSafeInsetBottom()+dp(8));
            }
        }
        final FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(Math.min(dp(520), Math.max(dp(48),screenWidth-left-right)), FrameLayout.LayoutParams.MATCH_PARENT);
        params.gravity = Gravity.CENTER_HORIZONTAL;
        params.setMargins(left,top,right,bottom);
        menuScrim.addView(scroller, params);
    }

    private void addBody(final LinearLayout panel, final String text) {
        final TextView body = new TextView(this);
        body.setText(text);
        body.setTextColor(HordeUiTokens.MUTED);
        body.setTextSize(13);
        body.setLineSpacing(0.0f, 1.15f);
        body.setPadding(0, 0, 0, dp(14));
        panel.addView(body, matchWrap());
    }

    private void addLinkedBody(final LinearLayout panel, final String text) {
        final TextView body = new TextView(this);
        body.setText(text);
        body.setTextColor(HordeUiTokens.MUTED);
        body.setLinkTextColor(HordeUiTokens.BRASS);
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
        final LinearLayout.LayoutParams left = new LinearLayout.LayoutParams(0, -2, 1.0f);
        final LinearLayout.LayoutParams right = new LinearLayout.LayoutParams(0, -2, 1.0f);
        right.leftMargin = dp(8);
        row.addView(createMenuButton(leftText, leftAction), left);
        row.addView(createMenuButton(rightText, rightAction), right);
        final LinearLayout.LayoutParams rowParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, -2);
        rowParams.topMargin = dp(7);
        panel.addView(row, rowParams);
    }

    private Button createMenuButton(final String text, final Runnable action) {
        final Button button = new Button(this);
        button.setText(text);
        button.setAllCaps(false);
        button.setSingleLine(false);
        button.setMaxLines(Integer.MAX_VALUE);
        button.setTextSize(15);
        button.setTypeface(Typeface.SANS_SERIF, Typeface.BOLD);
        button.setGravity(Gravity.CENTER_VERTICAL | Gravity.START);
        button.setPadding(dp(18), dp(12), dp(18), dp(12));
        styleActionButton(button, HordeUiTokens.SLATE, HordeUiTokens.PARCHMENT);
        button.setOnClickListener(view -> action.run());
        return button;
    }

    private LinearLayout.LayoutParams menuButtonLayoutParams() {
        final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, -2);
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
        slider.setContentDescription(title+", "+value+"%");
        slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(final SeekBar seekBar, final int progress, final boolean fromUser) {
                final int current = progress + min;
                label.setText(title + "  " + current + "%");
                slider.setContentDescription(title+", "+current+"%");
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
        button.setBackground(HordeUiTokens.button(this,fill));
        button.setStateListAnimator(null);
        button.setTextColor(HordeUiTokens.label(text));
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
            case CHEST_PROMPT_UNLOCKING:
                interactLabel = R.string.chest_unlocking;
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
        if (requestCode == REQUEST_SAVE_PLAYTEST) {
            finishPlaytestPicker(resultCode, data);
            return;
        }
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
        if (playtestReportVisible) {
            requestClosePlaytestReport();
            return;
        }
        if (graphicsVisible) {
            if (graphicsPreviewWanted) returnFromGraphicsPreview();
            else requestGraphicsRevert(true);
            return;
        }
        if (interfaceVisible) { showSettings(); return; }
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

        if (benchmarkAutomationId != null && !benchmarkAutomationFinishing) {
            finishBenchmarkAutomation(3);
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
        if (musicPlayback != null) musicPlayback.setSuspended(true); // Wait for a ready new surface.
        resumed = true;
        enterImmersiveMode();
        startSurfaceIfReady();
        if (rtLabVisible) handler.post(refreshRtLabTelemetry);
        graphicsPollTime = SystemClock.elapsedRealtime();
        if (graphicsRecovering) handler.post(refreshGraphics);
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
        if (!consumeBenchmarkAutomationIntent(intent, false)) consumeDebugAutomationIntent(intent);
        if (debugRtLabAccess && menuVisible && !deathOverlayVisible && !endingOverlayVisible) {
            showMainMenu(false);
        }
    }

    @Override
    protected void onPause() {
        graphicsPreviewImageOnly = false;
        dismissGraphicsPreviewDetails();
        if (keeperRevealTitle != null) keeperRevealTitle.setVisibility(View.GONE);
        if (graphicsVisible) {
            graphicsPreviewWanted = false;
            graphicsSceneRestoring = false;
            ProbeBridge.setGraphicsPreview(false, true, false, 0, false, 0);
            setNativeGraphics(graphicsConfirmed); // New surface starts from confirmed settings.
            graphicsRecovering = GraphicsPreferences.hasPending(preferences);
            graphicsVisible = false;
            handler.removeCallbacks(refreshGraphics);
            showSettings();
        }
        resumed = false;
        reconcilePlaytestReportForPause(); // A document-picker pause leaves local export untouched.
        if (musicPlayback != null) musicPlayback.setSuspended(true);
        if (benchmarkAutomationId != null && !benchmarkAutomationFinishing) {
            finishBenchmarkAutomation(3);
        }
        handler.removeCallbacks(runStartupUpdateCheck);
        startupUpdateCheckScheduled = false;
        handler.removeCallbacks(refreshRtLabTelemetry);
        ++delayedGameplayFeedbackGeneration;
        suspendAndResetWaterfall();
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
        setGameplayPaused(true);
        stopSurface();
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        dismissGraphicsPreviewDetails();
        invalidateRemotePlaytest(true);
        if (playtestExport != null) playtestExport.cancel();
        playtestReportVisible = false;
        reportExecutor.shutdownNow();
        if (musicPlayback != null) { musicPlayback.close(); musicPlayback = null; }
        handler.removeCallbacksAndMessages(null);
        updateExecutor.shutdownNow();
        if (vibrator != null) vibrator.cancel();
        if (debugRetryReceiver != null) {
            unregisterReceiver(debugRetryReceiver);
            debugRetryReceiver = null;
        }
        stopSurface();
        if (soundPool != null) soundPool.release();
        if (waterfallPlayback != null) { waterfallPlayback.close(); waterfallPlayback = null; }
        super.onDestroy();
    }
}
