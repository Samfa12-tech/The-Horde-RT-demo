package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.junit.Test;

/** Packaging alone does not make a dependency available to the filesystem importer. */
public final class WorldAssetStagingTest {
    private static Path appRoot() {
        return Files.exists(Paths.get("build.gradle")) && Files.exists(Paths.get("src/main"))
                ? Paths.get("") : Paths.get("android/app");
    }

    @Test public void everyPackagedWorldDependencyIsStagedBeforeNativeStartup() throws Exception {
        final Path root = appRoot();
        final String gradle = new String(Files.readAllBytes(root.resolve("build.gradle")), StandardCharsets.UTF_8);
        final String activity = new String(Files.readAllBytes(root.resolve(
                "src/main/java/com/samfa12/hordelanternrt/MainActivity.java")), StandardCharsets.UTF_8);
        final int start = activity.indexOf("private void collectInitialDiagnostics()");
        final int publish = activity.indexOf("final boolean written = ProbeBridge.writeReports(filesRoot);", start);
        assertTrue("Native startup publication must follow asset staging", start >= 0 && publish > start);
        final String startup = activity.substring(start, publish);
        final Matcher included = Pattern.compile("(?m)^\\s*include '(models/world/runtime/[^']+)'\\s*$").matcher(gradle);
        int count = 0;
        while (included.find()) {
            final String path = included.group(1);
            final Matcher staged = Pattern.compile("stageAsset\\(\\s*\"" + Pattern.quote(path)
                    + "\",\\s*\"" + Pattern.quote(path) + "\"\\s*\\)").matcher(startup);
            int copies = 0;
            while (staged.find()) ++copies;
            assertEquals("Filesystem importer dependency must be staged once at its exact path: " + path,
                    1, copies);
            ++count;
        }
        assertTrue("World package roster must not be empty", count > 0);
    }
}
