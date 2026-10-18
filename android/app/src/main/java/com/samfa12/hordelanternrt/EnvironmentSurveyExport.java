package com.samfa12.hordelanternrt;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.StandardOpenOption;

/** Bounded copy of one explicitly requested native survey; never rewrites an export. */
final class EnvironmentSurveyExport {
    static final int IMAGE_COUNT = 12;
    static final int MAX_DIMENSION = 8192;
    static final long MAX_IMAGE_BYTES = 64L * 1024 * 1024;
    static final long MAX_TOTAL_BYTES = 256L * 1024 * 1024;
    static final int MAX_MANIFEST_BYTES = 4 * 1024 * 1024;

    static final class Result {
        final File directory;
        final boolean successful;
        final String detail;
        Result(File directory, boolean successful, String detail) {
            this.directory = directory; this.successful = successful; this.detail = detail;
        }
    }

    private EnvironmentSurveyExport() {}

    static Result export(File privateReports, File externalRoot, String id,
                         boolean waterDisabled, int nativeStatus) throws IOException {
        if (!EnvironmentSurveyRequest.validId(id) || privateReports == null || externalRoot == null)
            throw new IllegalArgumentException("survey roots and valid request id are required");
        final File root = new File(externalRoot, "environment-surveys");
        ensureDirectory(externalRoot);
        ensureDirectory(root);
        final File destination = child(root, id);
        if (!destination.mkdir()) throw new IOException("survey directory exists or cannot be created: " + id);
        try {
            if (nativeStatus != 2 && nativeStatus != 3) throw new IOException("native survey is not terminal");
            final String manifestName = "environment-survey-" + id + ".json";
            final File manifest = child(privateReports, manifestName);
            final byte[] bytes = readManifest(manifest);
            final String json = StandardCharsets.UTF_8.newDecoder()
                    .onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT)
                    .decode(ByteBuffer.wrap(bytes)).toString();
            final JSONObject report = new JSONObject(json);
            if (integer(report, "schema") != 1 || !id.equals(report.get("requestId")) ||
                    !EnvironmentSurveyRequest.PROFILE.equals(report.get("profile")) ||
                    !"environment-survey".equals(report.get("diagnosticClass")) ||
                    !(report.get("waterDisabled") instanceof Boolean) ||
                    report.getBoolean("waterDisabled") != waterDisabled ||
                    !(nativeStatus == 2 ? "complete" : "failed").equals(report.get("result")))
                throw new IOException("survey identity, diagnostic class or completion mismatch");
            final JSONArray images = report.getJSONArray("images");
            if (images.length() > IMAGE_COUNT || (nativeStatus == 2 && images.length() != IMAGE_COUNT))
                throw new IOException("survey image count is invalid");
            final File[] sources = new File[images.length()];
            final long[] lengths = new long[images.length()];
            long total = bytes.length;
            // Validate the entire closed roster before copying any payload.
            for (int i = 0; i < images.length(); ++i) {
                final JSONObject row = images.getJSONObject(i);
                final String expected = "environment-survey-" + id + "-" + i + ".rgba";
                if (!expected.equals(row.get("file"))) throw new IOException("unexpected survey image name");
                final long width = integer(row, "width"), height = integer(row, "height");
                final long length = integer(row, "bytes");
                if (width < 1 || width > MAX_DIMENSION || height < 1 || height > MAX_DIMENSION ||
                        length != width * height * 4 || length > MAX_IMAGE_BYTES)
                    throw new IOException("invalid RGBA image dimensions or length");
                total += length;
                if (total > MAX_TOTAL_BYTES) throw new IOException("survey export exceeds total byte cap");
                sources[i] = child(privateReports, expected);
                lengths[i] = length;
            }
            for (int i = 0; i < sources.length; ++i) {
                if (!sources[i].isFile() || sources[i].length() != lengths[i])
                    throw new IOException("missing or mismatched survey image: " + sources[i].getName());
            }
            for (int i = 0; i < sources.length; ++i) copy(sources[i], child(destination, sources[i].getName()), lengths[i]);
            write(child(destination, manifestName), bytes);
            final boolean complete = nativeStatus == 2;
            writeResult(destination, id, complete ? "complete" : "failed", "native diagnostic survey " + report.getString("result"));
            return new Result(destination, complete, "native diagnostic survey " + report.getString("result"));
        } catch (IOException | JSONException invalid) {
            final String detail = invalid.getMessage() == null ? "survey export failed" : invalid.getMessage();
            // Keep partial files as failed evidence. No cleanup or retry can replace this id.
            writeResult(destination, id, "invalid", detail);
            return new Result(destination, false, detail);
        }
    }

    private static long integer(JSONObject row, String key) throws JSONException, IOException {
        final Object value = row.get(key);
        if (!(value instanceof Integer) && !(value instanceof Long)) throw new IOException("non-integer " + key);
        return ((Number)value).longValue();
    }

    private static File child(File root, String name) throws IOException {
        final File file = new File(root, name);
        if (Files.isSymbolicLink(file.toPath()) ||
                !root.getCanonicalFile().equals(file.getCanonicalFile().getParentFile()))
            throw new IOException("survey path escapes its root");
        return file;
    }

    private static void ensureDirectory(File directory) throws IOException {
        if (Files.isSymbolicLink(directory.toPath()) ||
                (!directory.isDirectory() && !directory.mkdirs())) throw new IOException("survey root unavailable");
    }

    private static byte[] readManifest(File file) throws IOException {
        if (!file.isFile() || file.length() < 1 || file.length() > MAX_MANIFEST_BYTES)
            throw new IOException("missing or oversized native survey manifest");
        try (FileInputStream input = new FileInputStream(file);
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[8192];
            int read;
            while ((read = input.read(buffer)) != -1) {
                if (output.size() + read > MAX_MANIFEST_BYTES) throw new IOException("native manifest grew beyond limit");
                output.write(buffer, 0, read);
            }
            return output.toByteArray();
        }
    }

    private static void copy(File source, File destination, long expected) throws IOException {
        try (FileInputStream input = new FileInputStream(source);
             OutputStream output = Files.newOutputStream(destination.toPath(), StandardOpenOption.CREATE_NEW, StandardOpenOption.WRITE)) {
            byte[] buffer = new byte[64 * 1024];
            long length = 0;
            int read;
            while ((read = input.read(buffer)) != -1) {
                length += read;
                if (length > expected) throw new IOException("native image grew while exporting");
                output.write(buffer, 0, read);
            }
            if (length != expected) throw new IOException("native image shrank while exporting");
        }
    }

    private static void write(File file, byte[] bytes) throws IOException {
        try (OutputStream output = Files.newOutputStream(file.toPath(), StandardOpenOption.CREATE_NEW, StandardOpenOption.WRITE)) {
            output.write(bytes);
        }
    }

    private static void writeResult(File directory, String id, String status, String detail) throws IOException {
        try {
            final JSONObject result = new JSONObject().put("schema", 1).put("requestId", id)
                    .put("diagnosticClass", "environment-survey").put("status", status).put("detail", detail);
            write(child(directory, "result.json"), result.toString().getBytes(StandardCharsets.UTF_8));
        } catch (JSONException impossible) { throw new IOException(impossible); }
    }
}
