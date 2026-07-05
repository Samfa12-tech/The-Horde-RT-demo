package com.samfa12.hordelanternrt;

import android.graphics.Typeface;
import android.graphics.Color;
import android.os.Bundle;
import android.widget.TextView;
import android.widget.ScrollView;

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
        reportTextView.setBackgroundColor(Color.BLACK);
        reportTextView.setPadding(16, 16, 16, 16);

        final ScrollView container = new ScrollView(this);
        container.addView(reportTextView);
        setContentView(container);
        container.setBackgroundColor(Color.BLACK);

        final String textReport;
        final String jsonReport;
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
        output.append(textReport).append('\n').append('\n');
        output.append("Reports written: ").append(written ? "yes" : "no").append('\n');
        output.append("Report directory: ").append(filesRoot).append('/').append(REPORT_DIRECTORY).append('\n');
        output.append("Report files: ").append(TEXT_REPORT_FILE).append(", ").append(JSON_REPORT_FILE).append('\n');
        output.append("JSON sample:\n").append(jsonReport);
        reportTextView.setText(output.toString());
    }
}
