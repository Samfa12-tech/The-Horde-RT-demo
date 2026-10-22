package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.system.StructStat;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.NoSuchFileException;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.HashSet;
import java.util.IdentityHashMap;
import java.util.Map;
import java.util.Set;
import org.json.JSONArray;
import org.json.JSONObject;
import org.junit.After;
import org.junit.Assume;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = {24, 34}, shadows = EnvironmentSurveyExportTest.SurveyOs.class)
public final class EnvironmentSurveyExportTest {
    private static final String ID = "river-A_1";
    @Rule public TemporaryFolder temporary = new TemporaryFolder();

    @Before public void resetSyscallContract() throws IOException { SurveyOs.reset(); }
    @After public void closeSyscallFixtures() throws IOException { SurveyOs.reset(); }

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

    @Test public void exclusiveOutputDoesNotReplaceExistingBytesAndClosesItsDescriptor() throws Exception {
        File file = temporary.newFile("owned-output");
        byte[] sentinel = {42, 12};
        Files.write(file.toPath(), sentinel);
        try { EnvironmentSurveyExport.createNewOutput(file); fail("existing output replaced"); }
        catch (IOException expected) {
            assertEquals(OsConstants.EEXIST, ((ErrnoException)expected.getCause()).errno);
        }
        assertArrayEquals(sentinel, Files.readAllBytes(file.toPath()));
        File fresh = new File(temporary.getRoot(), "new-output");
        try (OutputStream output = EnvironmentSurveyExport.createNewOutput(fresh)) {
            assertTrue(SurveyOs.lastDescriptor.valid());
            output.write(sentinel);
        }
        assertFalse(SurveyOs.lastDescriptor.valid());
        assertArrayEquals(sentinel, Files.readAllBytes(fresh.toPath()));
        assertEquals(OsConstants.O_WRONLY | OsConstants.O_CREAT | OsConstants.O_EXCL | OsConstants.O_NOFOLLOW,
                SurveyOs.lastFlags);
        assertEquals(0666, SurveyOs.lastMode);
    }

    @Test public void nonMissingLstatErrorsFailClosedRatherThanTreatingPathAsAbsent() throws Exception {
        File source = temporary.newFolder("private"), external = temporary.newFolder("external");
        SurveyOs.failLstatPath = external.getPath();
        SurveyOs.failLstatErrno = OsConstants.EACCES;
        try { EnvironmentSurveyExport.export(source, external, ID, false, 3); fail("uninspectable root admitted"); }
        catch (IOException expected) {
            assertEquals(OsConstants.EACCES, ((ErrnoException)expected.getCause()).errno);
        }
        assertEquals(0, external.list().length);
    }

    @Test public void symbolicSourceMarkerIsRejectedBeforeAnyPayloadCopy() throws Exception {
        File source = temporary.newFolder("private"), external = temporary.newFolder("external");
        JSONObject report = manifest(source);
        write(source, name(), report.toString().getBytes(StandardCharsets.UTF_8));
        // This contract does not depend on Windows symbolic-link privileges.
        SurveyOs.symbolicPaths.add(new File(source, image(0)).getPath());
        EnvironmentSurveyExport.Result result = EnvironmentSurveyExport.export(source, external, ID, false, 2);
        assertFalse(result.successful);
        assertEquals("survey path escapes its root", result.detail);
        assertEquals(1, result.directory.list().length);
        assertEquals("invalid", resultJson(result).getString("status"));
        assertArrayEquals(new byte[]{1, 2, 3, (byte)255}, Files.readAllBytes(new File(source, image(0)).toPath()));
    }

    @Test public void actualSameParentSourceLinkIsRejectedWhereHostSupportsLinks() throws Exception {
        File source = temporary.newFolder("private"), external = temporary.newFolder("external");
        JSONObject report = manifest(source);
        File target = new File(source, "owner.rgba");
        byte[] sentinel = {9, 8, 7, 6};
        Files.write(target.toPath(), sentinel);
        File alias = new File(source, image(0));
        Files.delete(alias.toPath());
        symbolicLinkOrSkip(alias, target);
        write(source, name(), report.toString().getBytes(StandardCharsets.UTF_8));
        EnvironmentSurveyExport.Result result = EnvironmentSurveyExport.export(source, external, ID, false, 2);
        assertFalse(result.successful);
        assertEquals("survey path escapes its root", result.detail);
        assertEquals(1, result.directory.list().length);
        assertArrayEquals(sentinel, Files.readAllBytes(target.toPath()));
        assertTrue(Files.isSymbolicLink(alias.toPath()));
    }

    @Test public void actualSymbolicExternalRootIsRejectedWhereHostSupportsLinks() throws Exception {
        File source = temporary.newFolder("private"), target = temporary.newFolder("owner-root");
        File alias = new File(temporary.getRoot(), "linked-root");
        write(target, "owner", new byte[]{42});
        symbolicLinkOrSkip(alias, target);
        try { EnvironmentSurveyExport.export(source, alias, ID, false, 3); fail("linked root admitted"); }
        catch (IOException expected) { assertEquals("survey root unavailable", expected.getMessage()); }
        assertEquals(1, target.list().length);
        assertArrayEquals(new byte[]{42}, Files.readAllBytes(new File(target, "owner").toPath()));
    }

    @Test public void actualExistingAndDanglingOutputLinksAreNotReplacedWhereHostSupportsLinks() throws Exception {
        File target = temporary.newFile("owner-output");
        byte[] sentinel = {42, 12};
        Files.write(target.toPath(), sentinel);
        File alias = new File(temporary.getRoot(), "linked-output");
        symbolicLinkOrSkip(alias, target);
        File dangling = new File(temporary.getRoot(), "dangling-output");
        symbolicLinkOrSkip(dangling, new File(temporary.getRoot(), "absent-target"));
        for (File link : new File[]{alias, dangling}) {
            try { EnvironmentSurveyExport.createNewOutput(link); fail("output link replaced"); }
            catch (IOException expected) {
                assertEquals(OsConstants.EEXIST, ((ErrnoException)expected.getCause()).errno);
            }
            assertTrue(Files.isSymbolicLink(link.toPath()));
        }
        assertArrayEquals(sentinel, Files.readAllBytes(target.toPath()));
        assertFalse(new File(temporary.getRoot(), "absent-target").exists());
    }

    private static void symbolicLinkOrSkip(File alias, File target) throws IOException {
        try { Files.createSymbolicLink(alias.toPath(), target.toPath()); }
        catch (IOException | UnsupportedOperationException | SecurityException unsupported) {
            Assume.assumeNoException("host cannot create symbolic-link fixture", unsupported);
        }
    }

    /**
     * Robolectric 4.14's default Linux/Posix lstat follows links and its open
     * shadow ignores O_EXCL. Model this syscall contract explicitly; these
     * tests do not certify Android kernel behavior or concurrent filesystem races.
     */
    @Implements(Os.class)
    public static final class SurveyOs {
        static final Set<String> symbolicPaths = new HashSet<>();
        static final Map<FileDescriptor, FileOutputStream> streams = new IdentityHashMap<>();
        static String failLstatPath;
        static int failLstatErrno, lastFlags, lastMode;
        static FileDescriptor lastDescriptor;

        static void reset() throws IOException {
            for (FileOutputStream stream : streams.values()) stream.close();
            streams.clear(); symbolicPaths.clear(); failLstatPath = null;
            failLstatErrno = lastFlags = lastMode = 0; lastDescriptor = null;
        }

        @Implementation protected static StructStat lstat(String path) throws ErrnoException {
            if (path.equals(failLstatPath)) throw new ErrnoException("lstat", failLstatErrno);
            try {
                BasicFileAttributes attributes = Files.readAttributes(new File(path).toPath(),
                        BasicFileAttributes.class, LinkOption.NOFOLLOW_LINKS);
                int mode = symbolicPaths.contains(path) || attributes.isSymbolicLink() ? OsConstants.S_IFLNK :
                        attributes.isDirectory() ? OsConstants.S_IFDIR : OsConstants.S_IFREG;
                return new StructStat(0, 0, mode, 0, 0, 0, 0, attributes.size(), 0, 0, 0, 0, 0);
            } catch (NoSuchFileException missing) { throw new ErrnoException("lstat", OsConstants.ENOENT); }
            catch (IOException error) { throw new ErrnoException("lstat", OsConstants.EIO, error); }
        }

        @Implementation protected static FileDescriptor open(String path, int flags, int mode) throws ErrnoException {
            lastFlags = flags; lastMode = mode;
            assertEquals(OsConstants.O_WRONLY | OsConstants.O_CREAT | OsConstants.O_EXCL | OsConstants.O_NOFOLLOW, flags);
            assertEquals(0666, mode);
            try {
                // The single test call models exclusive creation; Android's
                // actual Os.open provides atomic creation without this host split.
                Files.createFile(new File(path).toPath());
                FileOutputStream stream = new FileOutputStream(path);
                lastDescriptor = stream.getFD();
                streams.put(lastDescriptor, stream);
                return lastDescriptor;
            } catch (FileAlreadyExistsException exists) { throw new ErrnoException("open", OsConstants.EEXIST); }
            catch (IOException error) { throw new ErrnoException("open", OsConstants.EIO, error); }
        }

        @Implementation protected static void close(FileDescriptor descriptor) throws ErrnoException {
            FileOutputStream stream = streams.get(descriptor);
            if (stream == null) throw new ErrnoException("close", OsConstants.EBADF);
            try { stream.close(); }
            catch (IOException error) { throw new ErrnoException("close", OsConstants.EIO, error); }
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
