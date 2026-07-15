package com.samfa12.hordelanternrt;

import android.app.Activity;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.method.LinkMovementMethod;
import android.text.util.Linkify;
import android.util.Log;
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

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class MainActivity extends Activity {
    private static final String TAG = "HordeLanternAudio";
    private static final String PREFS = "horde_lantern_alpha_settings";
    private static final String REPORT_DIRECTORY = "reports";
    private static final String TEXT_REPORT_FILE = "vulkan_capability_report.txt";
    private static final String JSON_REPORT_FILE = "vulkan_capability_report.json";
    private static final String SKELETON_ASSET = "models/enemies/meshy/skeleton_biped_merged_animations_v01.glb";
    private static final String SKELETON_FILE = "skeleton_biped_merged_animations_v01.glb";
    private static final int AUDIO_EVENT_ENEMY_DEFEATED = 1;
    private static final int AUDIO_EVENT_PLAYER_FOOTSTEP = 1 << 1;
    private static final int AUDIO_EVENT_ENEMY_FOOTSTEP = 1 << 2;
    private static final int AUDIO_EVENT_ENEMY_ATTACK = 1 << 3;

    private final Handler handler = new Handler(Looper.getMainLooper());
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
    private Button menuButton;
    private Button attackButton;
    private SoundPool soundPool;
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
    private final Runnable applyPendingRenderScale = () ->
            ProbeBridge.setRenderScale(preferences.getInt("render_scale", 100) / 100.0f);

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.activity_main);
        preferences = getSharedPreferences(PREFS, MODE_PRIVATE);
        ProbeBridge.setRenderScale(preferences.getInt("render_scale", 100) / 100.0f);

        surfaceView = findViewById(R.id.scene_surface);
        menuScrim = findViewById(R.id.menu_scrim);
        diagnosticsPanel = findViewById(R.id.diagnostics_panel);
        reportTextView = findViewById(R.id.report_text);
        rtStatus = findViewById(R.id.rt_status);
        menuButton = findViewById(R.id.menu_button);
        attackButton = findViewById(R.id.attack_button);
        final Button diagnosticsBack = findViewById(R.id.diagnostics_back);

        styleActionButton(menuButton, 0xCC1A1713, 0xFFFFD28A);
        styleActionButton(attackButton, 0xDD5B210D, 0xFFFFE0A3);
        styleActionButton(diagnosticsBack, 0xCC211B15, 0xFFFFD28A);
        menuButton.setContentDescription(getString(R.string.menu));
        attackButton.setContentDescription(getString(R.string.swing));

        initialiseAudio();
        menuButton.setOnClickListener(view -> {
            playSound("menu_toggle", 0.20f);
            showMainMenu(false);
        });
        attackButton.setOnClickListener(view -> {
            if (menuVisible || diagnosticsVisible || ProbeBridge.getRuntimeState() != 1) return;
            ProbeBridge.requestAttack();
            playSound((swingVariant++ & 1) == 0 ? "sword_swing_1" : "sword_swing_2", 0.28f);
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
    }

    private void collectInitialDiagnostics() {
        try {
            final String textReport = ProbeBridge.getTextReport();
            final String jsonReport = ProbeBridge.getJsonReport();
            final String filesRoot = getFilesDir().getAbsolutePath();
            final boolean skeletonStaged = stageAsset(SKELETON_ASSET, SKELETON_FILE);
            for (final String legacy : new String[]{"diff-array-512.rgba", "normal-array-512.rgba", "arm-array-512.rgba"}) {
                final File stale = new File(getFilesDir(), legacy);
                if (stale.exists()) stale.delete();
            }
            final boolean materialsStaged = stageAsset("textures/polyhaven/mobile_1k/diff-array-512-astc6x6.ktx2", "diff-array-512-astc6x6.ktx2")
                    && stageAsset("textures/polyhaven/mobile_1k/normal-array-512-astc4x4.ktx2", "normal-array-512-astc4x4.ktx2")
                    && stageAsset("textures/polyhaven/mobile_1k/arm-array-512-astc6x6.ktx2", "arm-array-512-astc6x6.ktx2");
            final boolean written = ProbeBridge.writeReports(filesRoot);
            final StringBuilder output = new StringBuilder(textReport).append('\n');
            if (textReport.contains("RT mode: Unsupported")) {
                output.append("Unsupported: fake RT fallback is disabled.\n\n");
            }
            output.append("Reports written: ").append(written ? "yes" : "no").append('\n');
            output.append("Animated skeleton staged: ").append(skeletonStaged ? "yes" : "no").append('\n');
            output.append("ASTC PBR material arrays staged: ").append(materialsStaged ? "yes" : "no").append('\n');
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
            if (menuVisible || diagnosticsVisible || ProbeBridge.getRuntimeState() != 1) return true;
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
        diagnosticsVisible = false;
        diagnosticsPanel.setVisibility(View.GONE);
        menuVisible = true;
        ProbeBridge.setSimulationPaused(true);
        clearTouchState();
        attackButton.setVisibility(View.GONE);
        menuButton.setVisibility(View.GONE);
        rtStatus.setVisibility(View.GONE);
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
        addMenuButtonRow(panel,
                getString(R.string.credits), this::showCredits,
                getString(R.string.quit), this::finishAndRemoveTask);
        attachPanel(panel);
    }

    private void hideMenu() {
        menuVisible = false;
        menuScrim.setVisibility(View.GONE);
        final boolean showHud = preferences.getBoolean("show_hud", true);
        menuButton.setVisibility(showHud ? View.VISIBLE : View.GONE);
        attackButton.setVisibility(ProbeBridge.getRuntimeState() == 1 ? View.VISIBLE : View.GONE);
        rtStatus.setVisibility(showHud ? View.VISIBLE : View.GONE);
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
                    preferences.edit().clear().apply();
                    handler.removeCallbacks(applyPendingRenderScale);
                    ProbeBridge.setRenderScale(1.0f);
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
        for (int i = 0; i < viewControls.length; ++i) viewControls[i] = 0.0f;
        viewControls[2] = 1.8f;
        activePointers[0] = -1;
        activePointers[1] = -1;
        ProbeBridge.requestRouteReset();
        pushViewControls();
    }

    private final Runnable runtimePoll = new Runnable() {
        @Override
        public void run() {
            try {
                final int state = ProbeBridge.getRuntimeState();
                if (state == 1) {
                    rtStatus.setText(R.string.rt_active);
                    rtStatus.setTextColor(0xFFFFD07A);
                    if (!menuVisible && preferences.getBoolean("show_hud", true)) attackButton.setVisibility(View.VISIBLE);
                } else if (state == 2) {
                    rtStatus.setText(R.string.rt_unsupported);
                    rtStatus.setTextColor(0xFFFF8A7A);
                    if (!autoDiagnosticsShown) {
                        autoDiagnosticsShown = true;
                        showDiagnostics(false);
                    }
                } else if (state == 3) {
                    rtStatus.setText(R.string.rt_error);
                    rtStatus.setTextColor(0xFFFF8A7A);
                    if (!autoDiagnosticsShown) {
                        autoDiagnosticsShown = true;
                        showDiagnostics(true);
                    }
                } else {
                    rtStatus.setText(R.string.rt_starting);
                }

                final int events = ProbeBridge.consumeAudioEvents();
                if ((events & AUDIO_EVENT_ENEMY_DEFEATED) != 0) {
                    playSound((swingVariant & 1) == 0 ? "sword_hit_1" : "sword_hit_2", 0.32f);
                    handler.postDelayed(() -> playSound("enemy_fall", 0.24f), 140L);
                }
                if ((events & AUDIO_EVENT_PLAYER_FOOTSTEP) != 0) {
                    playSound((playerStepVariant++ & 1) == 0 ? "player_step_1" : "player_step_2", 0.13f);
                }
                if ((events & AUDIO_EVENT_ENEMY_FOOTSTEP) != 0) {
                    playSound((enemyStepVariant++ & 1) == 0 ? "skeleton_step_1" : "skeleton_step_2", 0.11f);
                }
                if ((events & AUDIO_EVENT_ENEMY_ATTACK) != 0) {
                    playSound("skeleton_attack", 0.22f);
                }
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
        final float gain = clamp(userGain * mixGain, 0.0f, 1.0f);
        final int streamId = soundPool.play(soundId, gain, gain, 1, 0, 1.0f);
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
    }

    private boolean stageAsset(final String assetPath, final String fileName) {
        final File destination = new File(getFilesDir(), fileName);
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
    public void onBackPressed() {
        if (diagnosticsVisible) {
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
    }

    @Override
    protected void onPause() {
        resumed = false;
        ProbeBridge.setSimulationPaused(true);
        stopSurface();
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        handler.removeCallbacksAndMessages(null);
        stopSurface();
        if (soundPool != null) soundPool.release();
        super.onDestroy();
    }
}
