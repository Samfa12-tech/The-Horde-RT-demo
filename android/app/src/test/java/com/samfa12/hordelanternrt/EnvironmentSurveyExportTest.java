package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
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
public final class EnvironmentSurveyExportTest {
    private static final String ID = "river-A_1";
    @Rule public TemporaryFolder temporary = new TemporaryFolder();

    @Test public void exportsExactlyTwelveImagesAndUnchangedCanonicalEvidenceAndCannotReplace() throws Exception {
        File source = temporary.newFolder("private"), external = temporary.newFolder("external");
        JSONObject manifest = manifest(source);
        String original = manifest.toString();
        write(source, name(), original.getBytes(StandardCharsets.UTF_8));
        EnvironmentSurveyExport.Result result = EnvironmentSurveyExport.export(source, external, ID, false, 2);
        assertTrue(result.successful);
        assertEquals(original, new String(Files.readAllBytes(new File(result.directory, name()).toPath()), StandardCharsets.UTF_8));
        assertEquals(14, result.directory.list().length);
        assertEquals("complete", resultJson(result).getString("status"));
        assertEquals(4, new File(result.directory, image(11)).length());
        try { EnvironmentSurveyExport.export(source, external, ID, false, 2); fail("replace admitted"); }
        catch (IOException expected) { assertTrue(expected.getMessage().contains("exists")); }
        assertEquals(original, new String(Files.readAllBytes(new File(result.directory, name()).toPath()), StandardCharsets.UTF_8));
        assertTrue(new File(source, image(0)).exists());
    }

    @Test public void rejectsWrongIdentityProfileClassWaterResultAndSchemaWithoutCopyingPayload() throws Exception {
        String[] fields = {"requestId", "profile", "diagnosticClass", "waterDisabled", "result", "schema"};
        Object[] bad = {"stale", "lake", "benchmark", "false", "failed", 2};
        for (int i = 0; i < fields.length; ++i) {
            File source = temporary.newFolder("source" + i), external = temporary.newFolder("ext" + i);
            JSONObject report = manifest(source).put(fields[i], bad[i]);
            reject(source, external, report);
        }
    }

    @Test public void rejectsTraversalDuplicateMissingAndOversizedRosters() throws Exception {
        for (int variant = 0; variant < 8; ++variant) {
            File source = temporary.newFolder("source" + variant), external = temporary.newFolder("ext" + variant);
            JSONObject report = manifest(source);
            JSONArray rows = report.getJSONArray("images");
            switch (variant) {
                case 0: rows.getJSONObject(0).put("file", "../owner"); break;
                case 1: rows.getJSONObject(1).put("file", image(0)); break;
                case 2: rows.remove(11); break;
                case 3: rows.put(rows.getJSONObject(0)); break;
                case 4: rows.getJSONObject(0).put("width", 8193); break;
                case 5: rows.getJSONObject(0).put("bytes", Long.MAX_VALUE); break;
                case 6: rows.getJSONObject(0).put("width", 1.5); break;
                default: Files.write(new File(source, image(11)).toPath(), new byte[3]); break;
            }
            reject(source, external, report);
        }
    }

    @Test public void rejectsMissingImagesAndMalformedUtf8ManifestAndNonterminalStatus() throws Exception {
        for (int variant = 0; variant < 3; ++variant) {
            File source = temporary.newFolder("source" + variant), external = temporary.newFolder("ext" + variant);
            JSONObject report = manifest(source);
            if (variant == 0) Files.delete(new File(source, image(5)).toPath());
            write(source, name(), variant == 1 ? new byte[]{(byte)0xff} : report.toString().getBytes(StandardCharsets.UTF_8));
            EnvironmentSurveyExport.Result result = EnvironmentSurveyExport.export(source, external, ID, false, variant == 2 ? 1 : 2);
            assertFalse(result.successful);
            assertEquals("invalid", resultJson(result).getString("status"));
            assertEquals(1, result.directory.list().length);
        }
    }

    @Test public void preservesFailedNativeManifestAsFailedRatherThanSuccessfulReceipt() throws Exception {
        File source = temporary.newFolder("private"), external = temporary.newFolder("external");
        JSONObject report = manifest(source).put("result", "failed").put("images", new JSONArray());
        write(source, name(), report.toString().getBytes(StandardCharsets.UTF_8));
        EnvironmentSurveyExport.Result result = EnvironmentSurveyExport.export(source, external, ID, false, 3);
        assertFalse(result.successful);
        assertEquals("failed", resultJson(result).getString("status"));
        assertTrue(new File(result.directory, name()).isFile());
    }

    @Test public void existingDirectoryWithOwnerSentinelIsNeverTouched() throws Exception {
        File source = temporary.newFolder("private"), external = temporary.newFolder("external");
        File destination = new File(external, "environment-surveys/" + ID);
        assertTrue(destination.mkdirs());
        write(destination, "owner", new byte[]{42});
        try { EnvironmentSurveyExport.export(source, external, ID, false, 3); fail(); }
        catch (IOException expected) { /* Immutable request id, even for failed attempts. */ }
        assertArrayEquals(new byte[]{42}, Files.readAllBytes(new File(destination, "owner").toPath()));
        assertEquals(1, destination.list().length);
    }

    @Test public void rejectsTotalBudgetBeforeReadingAnyLargeImagePayload() throws Exception {
        File source = temporary.newFolder("private"), external = temporary.newFolder("external");
        JSONObject report = manifest(source);
        for (int i = 0; i < 12; ++i) {
            report.getJSONArray("images").getJSONObject(i)
                    .put("width", 4096).put("height", 4096).put("bytes", 64L * 1024 * 1024);
        }
        write(source, name(), report.toString().getBytes(StandardCharsets.UTF_8));
        EnvironmentSurveyExport.Result result = EnvironmentSurveyExport.export(source, external, ID, false, 2);
        assertFalse(result.successful);
        assertTrue(result.detail.contains("total byte cap"));
        assertEquals(1, result.directory.list().length);
        // Fixtures stay four bytes; this exercises the cap without allocating large test artifacts.
        assertEquals(4, new File(source, image(0)).length());
    }

    @Test public void timeoutOrUnavailableWritesOnlyInvalidMarkerWithoutReadingNativePayload() throws Exception {
        for (int phase : new int[]{0, 1}) {
            File source = temporary.newFolder("nonterminal" + phase), external = temporary.newFolder("extNonterminal" + phase);
            // This would fail JSON parsing if export reached the native manifest.
            write(source, name(), "still being written".getBytes(StandardCharsets.UTF_8));
            EnvironmentSurveyExport.Result result = EnvironmentSurveyExport.export(source, external, ID, false, phase);
            assertFalse(result.successful);
            assertEquals("native survey is not terminal", result.detail);
            assertEquals(1, result.directory.list().length);
            assertEquals("invalid", resultJson(result).getString("status"));
            assertFalse(new File(result.directory, name()).exists());
            assertEquals("still being written", new String(Files.readAllBytes(new File(source, name()).toPath()), StandardCharsets.UTF_8));
        }
    }

    private void reject(File source, File external, JSONObject report) throws Exception {
        write(source, name(), report.toString().getBytes(StandardCharsets.UTF_8));
        EnvironmentSurveyExport.Result result = EnvironmentSurveyExport.export(source, external, ID, false, 2);
        assertFalse(result.successful);
        assertEquals("invalid", resultJson(result).getString("status"));
        assertEquals(1, result.directory.list().length);
    }

    private static JSONObject manifest(File source) throws Exception {
        JSONArray images = new JSONArray();
        for (int i = 0; i < 12; ++i) {
            write(source, image(i), new byte[]{1, 2, 3, (byte)255});
            images.put(new JSONObject().put("file", image(i)).put("width", 1).put("height", 1).put("bytes", 4));
        }
        return new JSONObject().put("schema", 1).put("requestId", ID).put("profile", "river-bend")
                .put("diagnosticClass", "environment-survey").put("waterDisabled", false).put("result", "complete")
                .put("images", images).put("canonicalFrameEvidence", new JSONObject().put("diagnosticOnly", true));
    }
    private static JSONObject resultJson(EnvironmentSurveyExport.Result result) throws Exception {
        return new JSONObject(new String(Files.readAllBytes(new File(result.directory, "result.json").toPath()), StandardCharsets.UTF_8));
    }
    private static String name() { return "environment-survey-" + ID + ".json"; }
    private static String image(int i) { return "environment-survey-" + ID + "-" + i + ".rgba"; }
    private static void write(File root, String name, byte[] bytes) throws IOException {
        Files.write(new File(root, name).toPath(), bytes);
    }
}
