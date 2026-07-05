package com.samfa12.hordelanternrt;

import android.graphics.Typeface;
import android.graphics.Color;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.FrameLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import android.app.Activity;

public class MainActivity extends Activity {
    private static final String REPORT_DIRECTORY = "reports";
    private static final String TEXT_REPORT_FILE = "vulkan_capability_report.txt";
    private static final String JSON_REPORT_FILE = "vulkan_capability_report.json";

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final TextView reportTextView = new TextView(this);
        reportTextView.setTextIsSelectable(true);
        reportTextView.setTypeface(Typeface.MONOSPACE);
        reportTextView.setTextSize(12);
        reportTextView.setTextColor(Color.GREEN);
        reportTextView.setBackgroundColor(0x88000000);
        reportTextView.setPadding(16, 16, 16, 16);

        final ScrollView container = new ScrollView(this);
        container.addView(reportTextView);
        container.setBackgroundColor(Color.BLACK);

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
        final boolean written = ProbeBridge.writeReports(filesRoot);
        final StringBuilder output = new StringBuilder();
        output.append(textReport).append('\n');
        if (textReport.contains("RT mode: Unsupported")) {
            output.append("Unsupported: fake RT fallback is disabled.\n\n");
        }
        output.append("Reports written: ").append(written ? "yes" : "no").append('\n');
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
