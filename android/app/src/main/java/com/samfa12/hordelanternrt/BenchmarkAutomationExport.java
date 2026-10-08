package com.samfa12.hordelanternrt;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.util.regex.Pattern;

/** Copies one native benchmark report into an immutable, run-id-owned export. */
public final class BenchmarkAutomationExport {
    private static final String JSON_NAME = "HordeLanternRT-benchmark-latest.json";
    private static final String TEXT_NAME = "HordeLanternRT-benchmark-latest.txt";
    private static final int MAX_JSON_BYTES = 16 * 1024 * 1024;
    private static final int MAX_TEXT_BYTES = 1 * 1024 * 1024;
    private static final int MAX_MOTION_CAPTURE_COUNT = 64;
    private static final int MAX_MOTION_CAPTURE_DIMENSION = 8192;
    private static final long MAX_MOTION_CAPTURE_BYTES = 64L * 1024L * 1024L;
    private static final long MAX_MOTION_CAPTURE_TOTAL_BYTES = 128L * 1024L * 1024L;
    private static final Pattern RUN_ID_PATTERN = Pattern.compile("[A-Za-z0-9_-]{1,64}");

    private BenchmarkAutomationExport() {
    }

    public static final class Result {
        public final File directory;
        public final boolean successful;
        public final String detail;

        private Result(final File directory, final boolean successful, final String detail) {
            this.directory = directory;
            this.successful = successful;
            this.detail = detail;
        }
    }

    public static Result export(final File privateReports,
                                final File externalFilesRoot,
                                final String runId,
                                final int nativeStatus) throws IOException {
        return export(privateReports, externalFilesRoot, runId,
                "showcase-route-v1", nativeStatus);
    }

    public static Result export(final File privateReports,
                                final File externalFilesRoot,
                                final String runId,
                                final String expectedWorkload,
                                final int nativeStatus) throws IOException {
        validateRunId(runId);
        validateWorkload(expectedWorkload);
        if (privateReports == null || externalFilesRoot == null) {
            throw new IllegalArgumentException("report and external roots are required");
        }

        final File benchmarks = new File(externalFilesRoot, "benchmarks");
        ensureDirectory(externalFilesRoot, "external files root");
        ensureDirectory(benchmarks, "benchmark export root");
        final File destination = new File(benchmarks, runId);
        if (destination.exists()) {
            throw new IOException("benchmark run directory already exists: " + runId);
        }
        if (!destination.mkdir()) {
            throw new IOException("could not create benchmark run directory: " + runId);
        }

        try {
            final File jsonFile = new File(privateReports, JSON_NAME);
            final File textFile = new File(privateReports, TEXT_NAME);
            final byte[] jsonBytes = readBounded(jsonFile, MAX_JSON_BYTES);
            final byte[] textBytes = readBounded(textFile, MAX_TEXT_BYTES);
            final String jsonText = decodeUtf8(jsonBytes, JSON_NAME);
            final String reportText = decodeUtf8(textBytes, TEXT_NAME);
            validateReport(jsonText, reportText, runId, expectedWorkload, nativeStatus);

            if (expectedWorkload.startsWith("motion-")) {
                copyMotionImages(privateReports, destination, runId,
                        new JSONObject(jsonText).getJSONArray("motionImageCaptures"));
            }

            writeBytes(new File(destination, "benchmark.json"), jsonBytes);
            writeBytes(new File(destination, "benchmark.txt"), textBytes);
            writeResult(destination, runId, "complete", "benchmark export complete");
            return new Result(destination, true, "benchmark export complete");
        } catch (final InvalidReportException invalid) {
            writeResult(destination, runId, "invalid", invalid.getMessage());
            return new Result(destination, false, invalid.getMessage());
        } catch (final JSONException malformed) {
            final String detail = "malformed benchmark JSON: " + safeMessage(malformed);
            writeResult(destination, runId, "invalid", detail);
            return new Result(destination, false, detail);
        }
    }

    private static void validateRunId(final String runId) {
        if (runId == null || !RUN_ID_PATTERN.matcher(runId).matches()) {
            throw new IllegalArgumentException("runId must match [A-Za-z0-9_-]{1,64}");
        }
    }

    private static void validateWorkload(final String workload) {
        if (!isAllowedWorkload(workload)) {
            throw new IllegalArgumentException("workload is not an allowlisted benchmark: " + workload);
        }
    }

    private static boolean isAllowedWorkload(final String workload) {
        return "showcase-route-v1".equals(workload) ||
                "lantern-held-high-v1".equals(workload) ||
                "lantern-held-low-v1".equals(workload) ||
                "lantern-grazing-v1".equals(workload) ||
                "lantern-motion-extreme-v1".equals(workload) ||
                "lantern-reveal-sequence-v1".equals(workload) ||
                workload != null && workload.matches("motion-(keeper-retry-reward|keeper-first-entry|torch-low-opening|shaft-up|waterfall-equipment|torch-drench)-v1");
    }

    private static void ensureDirectory(final File directory, final String label)
            throws IOException {
        if (directory.exists()) {
            if (!directory.isDirectory()) {
                throw new IOException(label + " is not a directory");
            }
            return;
        }
        if (!directory.mkdirs() && !directory.isDirectory()) {
            throw new IOException("could not create " + label);
        }
    }

    private static byte[] readBounded(final File file, final int maximumBytes)
            throws IOException, InvalidReportException {
        if (!file.isFile()) {
            throw new InvalidReportException("missing benchmark report: " + file.getName());
        }
        try (FileInputStream input = new FileInputStream(file)) {
            final byte[] buffer = new byte[8192];
            byte[] result = new byte[Math.min(8192, maximumBytes + 1)];
            int length = 0;
            while (true) {
                final int read = input.read(buffer);
                if (read < 0) break;
                if (length + read > maximumBytes) {
                    throw new InvalidReportException("benchmark report exceeds size limit: " +
                            file.getName());
                }
                if (length + read > result.length) {
                    final int expanded = Math.min(maximumBytes,
                            Math.max(length + read, result.length * 2));
                    final byte[] replacement = new byte[expanded];
                    System.arraycopy(result, 0, replacement, 0, length);
                    result = replacement;
                }
                System.arraycopy(buffer, 0, result, length, read);
                length += read;
            }
            final byte[] exact = new byte[length];
            System.arraycopy(result, 0, exact, 0, length);
            return exact;
        } catch (final FileNotFoundException missing) {
            throw new InvalidReportException("missing benchmark report: " + file.getName());
        }
    }

    private static void validateReport(final String jsonText,
                                       final String reportText,
                                       final String runId,
                                       final String expectedWorkload,
                                       final int nativeStatus)
            throws InvalidReportException, JSONException {
        if (nativeStatus != 2) {
            throw new InvalidReportException("native benchmark status is not complete");
        }
        if (jsonText.length() == 0 || reportText.length() == 0) {
            throw new InvalidReportException("benchmark report is empty");
        }
        final JSONObject root = new JSONObject(jsonText);
        if (expectedWorkload.startsWith("motion-")) {
            validateMotionReport(root, reportText, runId, expectedWorkload, nativeStatus);
            return;
        }
        if (requiredInteger(root, "schema") != 2) {
            throw new InvalidReportException("benchmark JSON schema is not 2");
        }
        if (!runId.equals(root.optString("runId", ""))) {
            throw new InvalidReportException("benchmark JSON runId does not match requested run");
        }
        if (!expectedWorkload.equals(root.optString("workload", ""))) {
            throw new InvalidReportException("benchmark JSON workload does not match requested workload");
        }
        if (!"complete".equals(root.optString("result", ""))) {
            throw new InvalidReportException("benchmark JSON result is not complete");
        }
        final String marker = "Run ID: " + runId + "\n";
        if (!reportText.contains(marker)) {
            throw new InvalidReportException("benchmark text Run ID marker does not match requested run");
        }
        if (!reportText.contains("Preset: " + expectedWorkload + "\n")) {
            throw new InvalidReportException("benchmark text workload marker does not match requested workload");
        }

        final JSONObject evidence = root.optJSONObject("completedFrameEvidence");
        if (evidence == null || !"complete".equals(evidence.optString("status", ""))) {
            throw new InvalidReportException("completed-frame evidence is not complete");
        }
        final JSONObject counts = evidence.optJSONObject("counts");
        final JSONArray rows = evidence.optJSONArray("rows");
        if (counts == null || rows == null) {
            throw new InvalidReportException("completed-frame evidence counts or rows are missing");
        }
        final int expected = requiredNonnegative(counts, "expected");
        final int measuredFrames = requiredNonnegative(root, "measuredFrames");
        final int completed = requiredNonnegative(counts, "completed");
        final int cpuAccepted = requiredNonnegative(counts, "cpuAccepted");
        if (expected <= 0 || expected != measuredFrames || expected != completed ||
                expected != cpuAccepted || expected != rows.length()) {
            throw new InvalidReportException("benchmark count mismatch");
        }
        if (requiredNonnegative(counts, "rejected") != 0 ||
                requiredNonnegative(counts, "cancelled") != 0 ||
                requiredNonnegative(counts, "outstanding") != 0 ||
                requiredNonnegative(counts, "cpuRejected") != 0) {
            throw new InvalidReportException("benchmark evidence contains rejected, cancelled, outstanding, or CPU-rejected rows");
        }
    }

    private static void validateMotionReport(final JSONObject root,
                                             final String reportText,
                                             final String runId,
                                             final String expectedWorkload,
                                             final int nativeStatus)
            throws InvalidReportException, JSONException {
        if (nativeStatus != 2 || requiredInteger(root, "schema") != 1 ||
                !runId.equals(root.optString("runId", "")) ||
                !expectedWorkload.equals(root.optString("workload", "")) ||
                !"complete".equals(root.optString("result", ""))) {
            throw new InvalidReportException("motion-validation run identity or completion is invalid");
        }
        final String scenario = expectedWorkload.substring("motion-".length(), expectedWorkload.length() - "-v1".length());
        if (!scenario.equals(root.optString("scenario", ""))) {
            throw new InvalidReportException("motion-validation scenario does not match workload");
        }
        if (!reportText.contains("Run ID: " + runId + "\n") ||
                !reportText.contains("Preset: " + expectedWorkload + "\n")) {
            throw new InvalidReportException("motion-validation text identity does not match requested run");
        }
        final JSONObject manifest = root.optJSONObject("motionManifest");
        final JSONObject evidence = root.optJSONObject("motionEvidence");
        final JSONObject timing = root.optJSONObject("imagePresentationTiming");
        final JSONObject eligibility = root.optJSONObject("timingEligibility");
        final JSONArray imageCaptures = root.optJSONArray("motionImageCaptures");
        if (manifest == null || evidence == null || timing == null || eligibility == null ||
                imageCaptures == null ||
                requiredInteger(manifest, "schema") != 1 ||
                !runId.equals(manifest.optString("runId", "")) ||
                !scenario.equals(manifest.optString("scenario", "")) ||
                !requiredBoolean(manifest, "finished") || !requiredBoolean(manifest, "armed") ||
                !requiredBoolean(manifest, "complete") ||
                !requiredBoolean(evidence, "scenarioComplete") ||
                evidence.optJSONArray("completedRtFrames") == null ||
                evidence.optJSONArray("completedRtFrames").length() == 0 ||
                !requiredBoolean(eligibility, "eligible")) {
            throw new InvalidReportException("motion-validation ledger, timing, or frame joins are incomplete");
        }
        final JSONObject capture = timing.optJSONObject("capture");
        final JSONObject captureStatus = capture == null ? null : capture.optJSONObject("status");
        final JSONObject counterDelta = timing.optJSONObject("counterDelta");
        final JSONArray completedFrames = evidence.optJSONArray("completedRtFrames");
        final JSONArray states = evidence.optJSONArray("states");
        final JSONArray timingRows = capture == null ? null : capture.optJSONArray("rows");
        final JSONArray unresolvedTimings = capture == null ? null : capture.optJSONArray("unresolved");
        final JSONObject settings = root.optJSONObject("settings");
        final int scalePercent = settings == null ? 0 : requiredNonnegative(settings, "scalePercent");
        if (capture == null || requiredInteger(capture, "schemaVersion") != 1 ||
                captureStatus == null || !requiredBoolean(captureStatus, "enabled") ||
                !requiredBoolean(captureStatus, "bound") ||
                requiredNonnegative(captureStatus, "pendingCount") != 0 ||
                counterDelta == null || completedFrames == null || timingRows == null || unresolvedTimings == null ||
                unresolvedTimings.length() != 0 ||
                completedFrames.length() > 16384 || timingRows.length() > 32768 ||
                requiredNonnegative(eligibility, "expectedCompletedFrames") != completedFrames.length() ||
                requiredNonnegative(eligibility, "matchedCompletedFrames") != completedFrames.length() ||
                states == null ||
                settings == null || requiredNonnegative(settings, "width") == 0 ||
                requiredNonnegative(settings, "height") == 0 ||
                requiredNonnegative(settings, "internalWidth") == 0 ||
                requiredNonnegative(settings, "internalHeight") == 0 ||
                !(scalePercent == 33 || scalePercent == 40 || scalePercent == 50) ||
                requiredNonnegative(manifest, "scale") != scalePercent ||
                requiredString(settings, "backend").isEmpty() ||
                !requiredString(settings, "backend").equals(requiredString(manifest, "backend")) ||
                requiredNonnegative(settings, "water") != requiredNonnegative(manifest, "water") ||
                requiredNonnegative(settings, "fire") != requiredNonnegative(manifest, "fire") ||
                requiredNonnegative(settings, "cap") != requiredNonnegative(manifest, "cap") ||
                requiredBoolean(settings, "glass") != requiredBoolean(manifest, "glass") ||
                requiredNonnegative(settings, "shadow") != requiredNonnegative(manifest, "shadow") ||
                requiredBoolean(settings, "mist") != requiredBoolean(manifest, "mist") ||
                requiredNonnegative(settings, "dust") != requiredNonnegative(manifest, "dust") ||
                timingRows.length() < completedFrames.length()) {
            throw new InvalidReportException("motion-validation presentation timing has unresolved or disabled evidence");
        }
        validateMotionCaptureMetadata(imageCaptures, manifest, states.length(), completedFrames.length(), runId);
        final JSONObject startingIdentity = root.optJSONObject("startingFrameIdentity");
        final JSONObject firstFrame = completedFrames.optJSONObject(0);
        if (startingIdentity == null || firstFrame == null ||
                requiredNonnegative(startingIdentity, "surfaceGeneration") !=
                        requiredNonnegative(firstFrame, "surfaceGeneration") ||
                requiredNonnegative(startingIdentity, "measurementGeneration") !=
                        requiredNonnegative(firstFrame, "measurementGeneration") ||
                requiredNonnegative(startingIdentity, "sceneEpoch") !=
                        requiredNonnegative(firstFrame, "sceneEpoch") ||
                requiredInteger(startingIdentity, "recordSerial") != requiredInteger(firstFrame, "record") ||
                requiredInteger(startingIdentity, "submissionSerial") != requiredInteger(firstFrame, "submission") ||
                requiredNonnegative(startingIdentity, "simulationTick") != requiredNonnegative(firstFrame, "tick")) {
            throw new InvalidReportException("motion-validation starting frame identity does not match its ledger");
        }
        for (final String counter : new String[]{"rejectedPresents", "invalidRegistrations", "invalidMetadata",
                "pendingCapacityExhausted", "queryErrors", "zeroPresentTimestamps", "zeroPresentIds",
                "unknownPresentIds", "duplicateTimings", "rowCapacityExhausted", "missingOnRebind",
                "missingOnUnbind", "abandonedPreparedPresents"}) {
            if (requiredNonnegative(counterDelta, counter) != 0) {
                throw new InvalidReportException("motion-validation has an adverse presentation timing counter: " + counter);
            }
        }
    }

    private static void validateMotionCaptureMetadata(final JSONArray captures,
                                                      final JSONObject manifest,
                                                      final int stateCount,
                                                      final int rtFrameCount,
                                                      final String runId)
            throws InvalidReportException, JSONException {
        final int declaredCount = requiredNonnegative(manifest, "captures");
        if (declaredCount == 0 || declaredCount > MAX_MOTION_CAPTURE_COUNT ||
                captures.length() != declaredCount) {
            throw new InvalidReportException("motion image capture count is missing or inconsistent");
        }
        long totalBytes = 0L;
        for (int index = 0; index < captures.length(); ++index) {
            final JSONObject capture = captures.optJSONObject(index);
            if (capture == null) throw new InvalidReportException("motion image capture row is malformed");
            final String expectedName = "android-motion-" + runId + "-" + index + ".rgba";
            if (!expectedName.equals(capture.optString("file", ""))) {
                throw new InvalidReportException("motion image capture filename is not owned by this run");
            }
            final int width = requiredNonnegative(capture, "width");
            final int height = requiredNonnegative(capture, "height");
            final int bytes = requiredNonnegative(capture, "bytes");
            final int stateRow = requiredNonnegative(capture, "stateRow");
            final int rtRow = requiredNonnegative(capture, "rtRow");
            final long expectedBytes = (long) width * (long) height * 4L;
            if (width == 0 || height == 0 || width > MAX_MOTION_CAPTURE_DIMENSION ||
                    height > MAX_MOTION_CAPTURE_DIMENSION || expectedBytes != bytes ||
                    expectedBytes > MAX_MOTION_CAPTURE_BYTES || stateRow >= stateCount || rtRow >= rtFrameCount) {
                throw new InvalidReportException("motion image capture dimensions, byte count, or row binding is invalid");
            }
            totalBytes += expectedBytes;
            if (totalBytes > MAX_MOTION_CAPTURE_TOTAL_BYTES) {
                throw new InvalidReportException("motion image captures exceed the export size limit");
            }
        }
    }

    private static void copyMotionImages(final File privateReports,
                                         final File destination,
                                         final String runId,
                                         final JSONArray captures)
            throws IOException, InvalidReportException, JSONException {
        final File sourceRoot = privateReports.getCanonicalFile();
        final byte[] buffer = new byte[64 * 1024];
        for (int index = 0; index < captures.length(); ++index) {
            final JSONObject capture = captures.getJSONObject(index);
            final String name = capture.getString("file"); // Exact allowlisted name was checked above.
            final long expectedBytes = requiredInteger(capture, "bytes");
            final File source = new File(privateReports, name);
            final File canonicalSource = source.getCanonicalFile();
            final File canonicalParent = source.getAbsoluteFile().getParentFile().getCanonicalFile();
            if (!source.isFile() || !sourceRoot.equals(canonicalParent) ||
                    !sourceRoot.equals(canonicalSource.getParentFile()) ||
                    !name.equals(canonicalSource.getName()) || source.length() != expectedBytes) {
                throw new InvalidReportException("motion image capture is missing, redirected, or has the wrong size");
            }
            final File copy = new File(destination, name);
            if (copy.exists() || !copy.createNewFile()) {
                throw new IOException("could not create owned motion image export");
            }
            long copied = 0L;
            boolean complete = false;
            try (FileInputStream input = new FileInputStream(source);
                 FileOutputStream output = new FileOutputStream(copy)) {
                while (true) {
                    final int read = input.read(buffer);
                    if (read < 0) break;
                    if (read == 0) continue;
                    copied += read;
                    if (copied > expectedBytes || copied > MAX_MOTION_CAPTURE_BYTES) {
                        throw new InvalidReportException("motion image capture grew beyond its declared size");
                    }
                    output.write(buffer, 0, read);
                }
                if (copied != expectedBytes) {
                    throw new InvalidReportException("motion image capture is partial");
                }
                output.getFD().sync();
                complete = true;
            } finally {
                if (!complete && copy.exists() && !copy.delete())
                    throw new IOException("could not remove partial motion image export");
            }
        }
    }

    private static String decodeUtf8(final byte[] bytes, final String name)
            throws InvalidReportException {
        try {
            return StandardCharsets.UTF_8.newDecoder()
                    .onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT)
                    .decode(ByteBuffer.wrap(bytes))
                    .toString();
        } catch (final CharacterCodingException malformed) {
            throw new InvalidReportException("benchmark report is not valid UTF-8: " + name);
        }
    }

    private static int requiredNonnegative(final JSONObject object, final String key)
            throws InvalidReportException {
        final long value = requiredInteger(object, key);
        if (value < 0) {
            throw new InvalidReportException("benchmark field is negative: " + key);
        }
        return (int) value;
    }

    private static boolean requiredBoolean(final JSONObject object, final String key)
            throws InvalidReportException {
        final Object value = object.opt(key);
        if (!(value instanceof Boolean)) {
            throw new InvalidReportException("benchmark field is missing or not boolean: " + key);
        }
        return (Boolean) value;
    }

    private static String requiredString(final JSONObject object, final String key)
            throws InvalidReportException {
        final Object value = object.opt(key);
        if (!(value instanceof String) || ((String) value).isEmpty()) {
            throw new InvalidReportException("benchmark field is missing or not text: " + key);
        }
        return (String) value;
    }

    private static long requiredInteger(final JSONObject object, final String key)
            throws InvalidReportException {
        if (!object.has(key) || object.isNull(key)) {
            throw new InvalidReportException("benchmark field is missing: " + key);
        }
        final Object raw = object.opt(key);
        if (!(raw instanceof Integer) && !(raw instanceof Long)) {
            throw new InvalidReportException("benchmark field is not an integer: " + key);
        }
        final long value = ((Number) raw).longValue();
        if (value < 0 || value > Integer.MAX_VALUE) {
            throw new InvalidReportException("benchmark field is outside supported range: " + key);
        }
        return value;
    }

    private static void writeBytes(final File destination, final byte[] bytes) throws IOException {
        try (FileOutputStream output = new FileOutputStream(destination)) {
            output.write(bytes);
            output.getFD().sync();
        }
    }

    private static void writeResult(final File directory,
                                    final String runId,
                                    final String status,
                                    final String detail) throws IOException {
        final JSONObject result;
        try {
            result = new JSONObject()
                    .put("schema", 1)
                    .put("runId", runId)
                    .put("status", status)
                    .put("detail", detail);
        } catch (final JSONException impossible) {
            throw new IOException("could not construct benchmark result marker", impossible);
        }
        final File temporary = new File(directory, "result.json.tmp");
        writeBytes(temporary, result.toString().getBytes(StandardCharsets.UTF_8));
        final File target = new File(directory, "result.json");
        if (!temporary.renameTo(target)) {
            throw new IOException("could not commit benchmark result marker");
        }
    }

    private static String safeMessage(final Exception exception) {
        final String message = exception.getMessage();
        return message == null || message.isEmpty() ? "invalid JSON" : message;
    }

    private static final class InvalidReportException extends Exception {
        InvalidReportException(final String message) {
            super(message);
        }
    }
}
