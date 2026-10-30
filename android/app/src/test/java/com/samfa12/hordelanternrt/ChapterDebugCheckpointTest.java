package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

/** A capture intent must select the same immutable checkpoint and camera as native. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34)
public final class ChapterDebugCheckpointTest {
    @Test public void allAuthoredChapterCapturesResolveWithTheirNativeCamera() throws Exception {
        Path root = Paths.get("../../src/gameplay/DevelopmentCheckpoints.h");
        if (!Files.exists(root)) root = Paths.get("src/gameplay/DevelopmentCheckpoints.h");
        final String registry = new String(Files.readAllBytes(root), StandardCharsets.UTF_8);
        final Matcher rows = Pattern.compile("\\{\\.id\\s*=\\s*(\\d+)\\s*,\\s*\\.name\\s*=\\s*\"(chapter-[^\"]+)\"([^}]+)\\}")
                .matcher(registry);
        int count = 0;
        while (rows.find()) {
            final int id = Integer.parseInt(rows.group(1));
            final String name = rows.group(2);
            assertEquals("Capture intent must reach native chapter checkpoint: " + name,
                    id, MainActivity.checkpointId(name));
            final Matcher yaw = Pattern.compile("\\.yaw\\s*=\\s*([-+.0-9]+)f?").matcher(rows.group(3));
            final Matcher pitch = Pattern.compile("\\.pitch\\s*=\\s*([-+.0-9]+)f?").matcher(rows.group(3));
            assertTrue(name + " has an authored yaw", yaw.find());
            assertTrue(name + " has an authored pitch", pitch.find());
            assertArrayEquals(name + " retains the native frozen view",
                    new float[]{Float.parseFloat(yaw.group(1)), Float.parseFloat(pitch.group(1))},
                    MainActivity.developmentCheckpointViewPose(id), 0.000001f);
            ++count;
        }
        assertTrue("The chapter roster must include landing, town, Kit and traversal views", count >= 22);
        assertEquals(-1, MainActivity.checkpointId("chapter-unknown"));
        assertNull(MainActivity.developmentCheckpointViewPose(999));
    }
}
