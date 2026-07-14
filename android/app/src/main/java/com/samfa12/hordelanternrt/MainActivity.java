package com.samfa12.hordelanternrt;

import android.graphics.Typeface;
import android.graphics.Color;
import android.os.Bundle;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import android.app.Activity;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class MainActivity extends Activity {
    private static final String REPORT_DIRECTORY = "reports";
    private static final String TEXT_REPORT_FILE = "vulkan_capability_report.txt";
    private static final String JSON_REPORT_FILE = "vulkan_capability_report.json";
    private static final String SKELETON_ASSET = "models/enemies/meshy/skeleton_biped_merged_animations_v01.glb";
    private static final String SKELETON_FILE = "skeleton_biped_merged_animations_v01.glb";

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

    private boolean stageSkeletonAsset() {
        return stageAsset(SKELETON_ASSET, SKELETON_FILE);
    }

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final TextView reportTextView = new TextView(this);
        reportTextView.setTextIsSelectable(true);
        reportTextView.setTypeface(Typeface.MONOSPACE);
        reportTextView.setTextSize(12);
        reportTextView.setTextColor(Color.GREEN);
        reportTextView.setBackgroundColor(0xDD000000);
        reportTextView.setPadding(16, 16, 16, 16);

        final ScrollView container = new ScrollView(this);
        container.addView(reportTextView);
        container.setBackgroundColor(0x00000000);
        container.setVisibility(View.GONE);

        final String textReport;
        final String jsonReport;
        final SurfaceView surfaceView = new SurfaceView(this);
        surfaceView.setLayoutParams(new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
        ));

        final FrameLayout root = new FrameLayout(this);
        root.setBackgroundColor(Color.BLACK);
        root.addView(surfaceView);

        final FrameLayout.LayoutParams overlayLayoutParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
        );
        root.addView(container, overlayLayoutParams);

        final TextView sceneHud = new TextView(this);
        sceneHud.setText("HORDE RT\nMOVE | LOOK");
        sceneHud.setTextColor(0xFFFFC46B);
        sceneHud.setTextSize(11);
        sceneHud.setTypeface(Typeface.create(Typeface.SERIF, Typeface.BOLD));
        sceneHud.setShadowLayer(8.0f, 0.0f, 2.0f, Color.BLACK);
        sceneHud.setBackgroundColor(0x33000000);
        sceneHud.setPadding(18, 14, 18, 14);
        sceneHud.setMaxLines(2);
        final FrameLayout.LayoutParams hudLayoutParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT
        );
        hudLayoutParams.gravity = Gravity.TOP | Gravity.START;
        hudLayoutParams.setMargins(18, 18, 18, 18);
        root.addView(sceneHud, hudLayoutParams);
        sceneHud.setOnClickListener(view -> {
            final boolean showingDiagnostics = container.getVisibility() == View.VISIBLE;
            container.setVisibility(showingDiagnostics ? View.GONE : View.VISIBLE);
            sceneHud.setText(showingDiagnostics
                    ? "HORDE RT\nMOVE | LOOK"
                    : "HORDE RT\nLOGS OPEN");
        });

        final TextView attackButton = new TextView(this);
        attackButton.setText("SWING");
        attackButton.setTextColor(0xFFFFE0A3);
        attackButton.setTextSize(17);
        attackButton.setTypeface(Typeface.create(Typeface.SERIF, Typeface.BOLD));
        attackButton.setGravity(Gravity.CENTER);
        attackButton.setBackgroundColor(0x883D1608);
        attackButton.setPadding(34, 24, 34, 24);
        attackButton.setOnClickListener(view -> ProbeBridge.requestAttack());
        final FrameLayout.LayoutParams attackLayoutParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT
        );
        attackLayoutParams.gravity = Gravity.BOTTOM | Gravity.END;
        attackLayoutParams.setMargins(24, 24, 28, 42);
        root.addView(attackButton, attackLayoutParams);
        final float[] viewControls = {0.0f, 0.0f, 1.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        final int[] activePointers = {-1, -1};
        surfaceView.setOnTouchListener((view, event) -> {
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
                for (int i = 0; i < event.getPointerCount(); ++i) {
                    final int pointerId = event.getPointerId(i);
                    if (pointerId == activePointers[0]) {
                        final float dx = event.getX(i) - viewControls[5];
                        final float dy = event.getY(i) - viewControls[6];
                        viewControls[7] = Math.max(-1.0f, Math.min(1.0f, dx / Math.max(view.getWidth() * 0.16f, 1.0f)));
                        viewControls[8] = Math.max(-1.0f, Math.min(1.0f, -dy / Math.max(view.getHeight() * 0.16f, 1.0f)));
                    } else if (pointerId == activePointers[1]) {
                        final float dx = event.getX(i) - viewControls[3];
                        final float dy = event.getY(i) - viewControls[4];
                        viewControls[3] = event.getX(i);
                        viewControls[4] = event.getY(i);
                        viewControls[0] = viewControls[0] + dx * 0.0036f;
                        viewControls[1] = Math.max(-0.32f, Math.min(0.28f, viewControls[1] - dy * 0.0028f));
                    }
                }
                ProbeBridge.setViewControls(viewControls[0], viewControls[1], viewControls[2], viewControls[7], viewControls[8]);
                return true;
            }
            if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP || action == MotionEvent.ACTION_CANCEL) {
                final int pointerId = event.getPointerId(event.getActionIndex());
                if (pointerId == activePointers[0]) {
                    activePointers[0] = -1;
                    viewControls[7] = 0.0f;
                    viewControls[8] = 0.0f;
                }
                if (pointerId == activePointers[1]) {
                    activePointers[1] = -1;
                }
                ProbeBridge.setViewControls(viewControls[0], viewControls[1], viewControls[2], viewControls[7], viewControls[8]);
                return true;
            }
            return true;
        });
        setContentView(root);

        try {
            textReport = ProbeBridge.getTextReport();
            jsonReport = ProbeBridge.getJsonReport();
        } catch (final UnsatisfiedLinkError error) {
            reportTextView.setText("Unable to load native library or run probe.\n\n" + error.getMessage());
            return;
        } catch (final Exception ex) {
            reportTextView.setText("Unexpected probe failure.\n\n" + ex.getMessage());
            return;
        }

        final String filesRoot = getFilesDir().getAbsolutePath();
        final boolean skeletonStaged = stageSkeletonAsset();
        for (final String legacy : new String[]{"diff-array-512.rgba", "normal-array-512.rgba", "arm-array-512.rgba"}) {
            final File stale = new File(getFilesDir(), legacy);
            if (stale.exists()) stale.delete();
        }
        final boolean materialsStaged = stageAsset("textures/polyhaven/mobile_1k/diff-array-512-astc6x6.ktx2", "diff-array-512-astc6x6.ktx2")
                && stageAsset("textures/polyhaven/mobile_1k/normal-array-512-astc4x4.ktx2", "normal-array-512-astc4x4.ktx2")
                && stageAsset("textures/polyhaven/mobile_1k/arm-array-512-astc6x6.ktx2", "arm-array-512-astc6x6.ktx2");
        final boolean written = ProbeBridge.writeReports(filesRoot);
        final StringBuilder output = new StringBuilder();
        output.append(textReport).append('\n');
        if (textReport.contains("RT mode: Unsupported")) {
            output.append("Unsupported: fake RT fallback is disabled.\n\n");
        }
        output.append("Reports written: ").append(written ? "yes" : "no").append('\n');
        output.append("Animated skeleton staged: ").append(skeletonStaged ? "yes" : "no").append('\n');
        output.append("ASTC PBR material arrays staged: ").append(materialsStaged ? "yes" : "no").append('\n');
        output.append("Report directory: ").append(filesRoot).append('/').append(REPORT_DIRECTORY).append('\n');
        output.append("Report files: ").append(TEXT_REPORT_FILE).append(", ").append(JSON_REPORT_FILE).append('\n');
        output.append("JSON sample:\n").append(jsonReport);
        reportTextView.setText(output.toString());

        surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(final SurfaceHolder holder) {
                final boolean started = ProbeBridge.startDiagnosticSurface(holder.getSurface(), filesRoot);
                if (!started) {
                    reportTextView.append("\nDiagnostic surface failed to start.\n");
                }
            }

            @Override
            public void surfaceChanged(final SurfaceHolder holder, final int format, final int width, final int height) {
                // Surface can be recreated by the system; keep implementation minimal.
            }

            @Override
            public void surfaceDestroyed(final SurfaceHolder holder) {
                ProbeBridge.stopDiagnosticSurface();
            }
        });
    }

    @Override
    protected void onPause() {
        super.onPause();
        ProbeBridge.stopDiagnosticSurface();
    }

    @Override
    protected void onDestroy() {
        ProbeBridge.stopDiagnosticSurface();
        super.onDestroy();
    }
}
