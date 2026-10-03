package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertTrue;

import android.content.Context;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

/** Keeps the user-facing consent boundary explicit as the native flow evolves. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk = 28)
public final class PlaytestReportFlowResourceTest {
    @Test public void remoteDisclosureSeparatesVerificationFromSelectedReportData() {
        Context context = RuntimeEnvironment.getApplication();
        String disclosure = context.getString(R.string.playtest_privacy);
        assertTrue(disclosure.contains("Cloudflare verification"));
        assertTrue(disclosure.contains("does not receive your report or image"));
        assertTrue(disclosure.contains("game RT frame only—not an OS screenshot"));
        assertTrue(disclosure.contains("No logs, saves, private files"));
    }

    @Test public void remoteImageAndRemoteSubmissionRequireSeparateOptInsFromLocalExport() {
        Context context = RuntimeEnvironment.getApplication();
        String local = context.getString(R.string.playtest_export_consent);
        String remote = context.getString(R.string.playtest_remote_consent);
        String image = context.getString(R.string.playtest_remote_screenshot_consent);
        String preview = context.getString(R.string.playtest_preview_description);
        assertTrue(local.contains("exporting"));
        assertTrue(remote.contains("explicitly consent to sending"));
        assertTrue(image.contains("768 px long edge / 432 px short edge"));
        assertTrue(image.contains("512 KiB"));
        assertTrue(image.contains("never an OS screenshot"));
        assertTrue(preview.contains("768 px long edge / 432 px short edge"));
        assertTrue(preview.contains("512 KiB"));
    }
}
