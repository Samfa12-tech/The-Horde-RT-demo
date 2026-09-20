package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

import org.json.JSONArray;
import org.json.JSONObject;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34)
public final class BenchmarkAutomationExportTest {
    private static final String RUN_ID = "automation-abc_1";

    @Rule
    public final TemporaryFolder temporaryFolder = new TemporaryFolder();

    @Test
    public void validReportsAreCopiedAndCompleteMarkerIsWrittenLast() throws Exception {
        final File privateReports = temporaryFolder.newFolder("private");
        final File externalRoot = temporaryFolder.newFolder("external");
        final String json = validJson(RUN_ID, 3, 3, 3, 3, 3);
        final String text = validText(RUN_ID);
        write(privateReports, "HordeLanternRT-benchmark-latest.json", json);
        write(privateReports, "HordeLanternRT-benchmark-latest.txt", text);

        final BenchmarkAutomationExport.Result result = BenchmarkAutomationExport.export(
                privateReports, externalRoot, RUN_ID, 2);

        assertTrue(result.successful);
        assertEquals("complete", new JSONObject(read(result.directory, "result.json"))
                .getString("status"));
        assertEquals(json, read(result.directory, "benchmark.json"));
        assertEquals(text, read(result.directory, "benchmark.txt"));
        assertTrue(result.directory.getPath().endsWith("benchmarks" + File.separator + RUN_ID));
    }

    @Test
    public void staleJsonRunIdIsRejectedWithoutCopyingPayload() throws Exception {
        final File privateReports = temporaryFolder.newFolder("private");
        final File externalRoot = temporaryFolder.newFolder("external");
        write(privateReports, "HordeLanternRT-benchmark-latest.json", validJson("old-run", 2, 2, 2, 2, 2));
        write(privateReports, "HordeLanternRT-benchmark-latest.txt", validText("old-run"));

        final BenchmarkAutomationExport.Result result = BenchmarkAutomationExport.export(
                privateReports, externalRoot, RUN_ID, 2);

        assertFalse(result.successful);
        assertFalse(new File(result.directory, "benchmark.json").exists());
        assertFalse(new File(result.directory, "benchmark.txt").exists());
        assertEquals("invalid", new JSONObject(read(result.directory, "result.json"))
                .getString("status"));
    }

    @Test
    public void nativeFailureStatusCannotClaimSuccess() throws Exception {
        final File privateReports = temporaryFolder.newFolder("private");
        final File externalRoot = temporaryFolder.newFolder("external");
        write(privateReports, "HordeLanternRT-benchmark-latest.json", validJson(RUN_ID, 2, 2, 2, 2, 2));
        write(privateReports, "HordeLanternRT-benchmark-latest.txt", validText(RUN_ID));

        final BenchmarkAutomationExport.Result result = BenchmarkAutomationExport.export(
                privateReports, externalRoot, RUN_ID, 3);

        assertFalse(result.successful);
        assertEquals("invalid", new JSONObject(read(result.directory, "result.json"))
                .getString("status"));
        assertFalse(new File(result.directory, "benchmark.json").exists());
    }

    @Test
    public void missingMalformedAndOversizedReportsAreInvalid() throws Exception {
        final File privateReports = temporaryFolder.newFolder("private");
        final File externalRoot = temporaryFolder.newFolder("external");

        BenchmarkAutomationExport.Result missing = BenchmarkAutomationExport.export(
                privateReports, externalRoot, "missing", 2);
        assertFalse(missing.successful);

        write(privateReports, "HordeLanternRT-benchmark-latest.json", "not-json");
        write(privateReports, "HordeLanternRT-benchmark-latest.txt", validText("malformed"));
        BenchmarkAutomationExport.Result malformed = BenchmarkAutomationExport.export(
                privateReports, externalRoot, "malformed", 2);
        assertFalse(malformed.successful);

        write(privateReports, "HordeLanternRT-benchmark-latest.json", validJson("oversized", 1, 1, 1, 1, 1));
        write(privateReports, "HordeLanternRT-benchmark-latest.txt", repeat('x', 1024 * 1024 + 1));
        BenchmarkAutomationExport.Result oversized = BenchmarkAutomationExport.export(
                privateReports, externalRoot, "oversized", 2);
        assertFalse(oversized.successful);
        assertFalse(new File(oversized.directory, "benchmark.txt").exists());
    }

    @Test
    public void invalidRunIdsAndRepeatedDestinationsAreRejected() throws Exception {
        final File privateReports = temporaryFolder.newFolder("private");
        final File externalRoot = temporaryFolder.newFolder("external");
        final String[] invalidIds = {"../escape", "a/b", "", "a.b", repeat('a', 65)};
        for (String invalidId : invalidIds) {
            try {
                BenchmarkAutomationExport.export(privateReports, externalRoot, invalidId, 2);
                fail("invalid run id must be rejected: " + invalidId);
            } catch (IllegalArgumentException expected) {
                // expected
            }
        }

        write(privateReports, "HordeLanternRT-benchmark-latest.json", validJson(RUN_ID, 1, 1, 1, 1, 1));
        write(privateReports, "HordeLanternRT-benchmark-latest.txt", validText(RUN_ID));
        BenchmarkAutomationExport.export(privateReports, externalRoot, RUN_ID, 2);
        try {
            BenchmarkAutomationExport.export(privateReports, externalRoot, RUN_ID, 2);
            fail("a run directory must not be reused");
        } catch (IOException expected) {
            // expected
        }
    }

    @Test
    public void countMismatchIsInvalid() throws Exception {
        final File privateReports = temporaryFolder.newFolder("private");
        final File externalRoot = temporaryFolder.newFolder("external");
        write(privateReports, "HordeLanternRT-benchmark-latest.json", validJson(RUN_ID, 3, 3, 2, 3, 3));
        write(privateReports, "HordeLanternRT-benchmark-latest.txt", validText(RUN_ID));

        final BenchmarkAutomationExport.Result result = BenchmarkAutomationExport.export(
                privateReports, externalRoot, RUN_ID, 2);

        assertFalse(result.successful);
        assertTrue(result.detail.contains("count"));
        assertFalse(new File(result.directory, "benchmark.json").exists());
    }

    @Test
    public void workloadMarkerMustMatchRequestedAllowlistedCase() throws Exception {
        final File privateReports = temporaryFolder.newFolder("private");
        final File externalRoot = temporaryFolder.newFolder("external");
        write(privateReports, "HordeLanternRT-benchmark-latest.json", validJson(RUN_ID, 1, 1, 1, 1, 1));
        write(privateReports, "HordeLanternRT-benchmark-latest.txt", validText(RUN_ID));

        final BenchmarkAutomationExport.Result mismatch = BenchmarkAutomationExport.export(
                privateReports, externalRoot, RUN_ID, "lantern-held-high-v1", 2);

        assertFalse(mismatch.successful);
        assertTrue(mismatch.detail.contains("workload"));
        try {
            BenchmarkAutomationExport.export(privateReports, externalRoot,
                    "invalid-workload", "unknown-case", 2);
            fail("unknown workload must be rejected before export");
        } catch (IllegalArgumentException expected) {
            // expected
        }
    }

    @Test
    public void numericFieldsMustBeIntegralAndWithinIntRange() throws Exception {
        final File privateReports = temporaryFolder.newFolder("private");
        final File externalRoot = temporaryFolder.newFolder("external");

        write(privateReports, "HordeLanternRT-benchmark-latest.json",
                validJson("fractional", 1, 1, 1, 1, 1).replace("\"expected\":1", "\"expected\":1.5"));
        write(privateReports, "HordeLanternRT-benchmark-latest.txt", validText("fractional"));
        assertFalse(BenchmarkAutomationExport.export(privateReports, externalRoot,
                "fractional", 2).successful);

        write(privateReports, "HordeLanternRT-benchmark-latest.json",
                validJson("large-number", 1, 1, 1, 1, 1)
                        .replace("\"expected\":1", "\"expected\":4294967297"));
        write(privateReports, "HordeLanternRT-benchmark-latest.txt", validText("large-number"));
        assertFalse(BenchmarkAutomationExport.export(privateReports, externalRoot,
                "large-number", 2).successful);

        write(privateReports, "HordeLanternRT-benchmark-latest.json",
                validJson("fractional-schema", 1, 1, 1, 1, 1)
                        .replace("\"schema\":2", "\"schema\":2.5"));
        write(privateReports, "HordeLanternRT-benchmark-latest.txt", validText("fractional-schema"));
        assertFalse(BenchmarkAutomationExport.export(privateReports, externalRoot,
                "fractional-schema", 2).successful);
    }

    private static String validJson(final String runId, final int expected, final int completed,
                                    final int measuredFrames, final int cpuAccepted,
                                    final int rows) throws Exception {
        final JSONObject evidence = new JSONObject()
                .put("status", "complete")
                .put("counts", new JSONObject()
                        .put("expected", expected)
                        .put("completed", completed)
                        .put("rejected", 0)
                        .put("cancelled", 0)
                        .put("cpuAccepted", cpuAccepted)
                        .put("cpuRejected", 0)
                        .put("outstanding", 0))
                .put("rows", new JSONArray());
        for (int index = 0; index < rows; ++index) {
            evidence.getJSONArray("rows").put(new JSONObject().put("index", index));
        }
        return new JSONObject()
                .put("schema", 2)
                .put("runId", runId)
                .put("result", "complete")
                .put("workload", "showcase-route-v1")
                .put("measuredFrames", measuredFrames)
                .put("completedFrameEvidence", evidence)
                .toString();
    }

    private static String validText(final String runId) {
        return "HORDE LANTERN RT - IN-APP BENCHMARK\nRun ID: " + runId +
                "\nPreset: showcase-route-v1\n";
    }

    private static void write(final File directory, final String name, final String value)
            throws IOException {
        try (FileOutputStream output = new FileOutputStream(new File(directory, name))) {
            output.write(value.getBytes(StandardCharsets.UTF_8));
        }
    }

    private static String read(final File directory, final String name) throws IOException {
        try (FileInputStream input = new FileInputStream(new File(directory, name));
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            final byte[] buffer = new byte[1024];
            int read;
            while ((read = input.read(buffer)) >= 0) output.write(buffer, 0, read);
            return new String(output.toByteArray(), StandardCharsets.UTF_8);
        }
    }

    private static String repeat(final char value, final int count) {
        final StringBuilder result = new StringBuilder(count);
        for (int index = 0; index < count; ++index) result.append(value);
        return result.toString();
    }
}
