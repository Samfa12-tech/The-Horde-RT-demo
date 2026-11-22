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
    @Test public void residentCheckpointsRequireThePrivatePackage() {
        assertEquals(BuildConfig.BELLWETHER_RESIDENT_CANDIDATE ? 329 : -1,
                MainActivity.checkpointId("bellwether-repeat-visit"));
        assertEquals(BuildConfig.BELLWETHER_RESIDENT_CANDIDATE ? 337 : -1,
                MainActivity.checkpointId("bellwether-doorway-measurement"));
        final boolean combined = BuildConfig.BELLWETHER_COMBINED_RESIDENTS;
        final boolean mill = BuildConfig.BELLWETHER_ARTIFICER_CANDIDATE && !combined;
        final boolean shore = BuildConfig.BELLWETHER_SALVAGER_CANDIDATE && !combined;
        final boolean church = BuildConfig.BELLWETHER_RESIDENT_CANDIDATE &&
                (combined || (!mill && !shore));
        assertEquals(church ? 335 : -1, MainActivity.checkpointId("bellwether-record-keeper-handoff"));
        assertEquals(mill ? 344 : -1, MainActivity.checkpointId("bellwether-mill-artificer"));
        assertEquals(mill ? 345 : -1, MainActivity.checkpointId("bellwether-mill-drive-tested"));
        assertEquals(shore ? 346 : -1, MainActivity.checkpointId("bellwether-salvager-shore"));
        if (shore)
            assertArrayEquals(new float[]{-.08f, -.17f}, MainActivity.developmentCheckpointViewPose(346), .000001f);
        else
            assertNull(MainActivity.developmentCheckpointViewPose(346));
        if (BuildConfig.BELLWETHER_RESIDENT_CANDIDATE) {
            assertArrayEquals(new float[]{-.14f, -.11f}, MainActivity.developmentCheckpointViewPose(329), .000001f);
            assertArrayEquals(MainActivity.developmentCheckpointViewPose(329),
                    MainActivity.developmentCheckpointViewPose(337), .000001f);
        } else {
            assertNull(MainActivity.developmentCheckpointViewPose(329));
            assertNull(MainActivity.developmentCheckpointViewPose(337));
        }
        if (church)
            assertArrayEquals(new float[]{1.57079632679f, -.07f}, MainActivity.developmentCheckpointViewPose(335), .000001f);
        else
            assertNull(MainActivity.developmentCheckpointViewPose(335));
        if (mill) {
            assertArrayEquals(new float[]{1.64f, -.32f}, MainActivity.developmentCheckpointViewPose(344), .000001f);
            assertArrayEquals(MainActivity.developmentCheckpointViewPose(344),
                    MainActivity.developmentCheckpointViewPose(345), .000001f);
        } else {
            assertNull(MainActivity.developmentCheckpointViewPose(344));
            assertNull(MainActivity.developmentCheckpointViewPose(345));
        }
    }

    @Test public void allAuthoredChapterCapturesResolveWithTheirNativeCamera() throws Exception {
        Path root = Paths.get("../../src/gameplay/DevelopmentCheckpoints.h");
        if (!Files.exists(root)) root = Paths.get("src/gameplay/DevelopmentCheckpoints.h");
        final String registry = new String(Files.readAllBytes(root), StandardCharsets.UTF_8);
        final Matcher rows = Pattern.compile("\\{\\.id\\s*=\\s*(\\d+)\\s*,\\s*\\.name\\s*=\\s*\"((?:chapter-|attack-ring-|enemy-hit-)[^\"]+|bellwether-(?:mill-yard|river-landing|rig-leak-trial))\"([^}]+)\\}")
                .matcher(registry);
        int count = 0;
        boolean millYard = false;
        boolean riverLanding = false;
        boolean rigLeakTrial = false;
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
            millYard |= name.equals("bellwether-mill-yard");
            riverLanding |= name.equals("bellwether-river-landing");
            rigLeakTrial |= name.equals("bellwether-rig-leak-trial");
            ++count;
        }
        assertTrue("The chapter roster must include landing, town, Kit and traversal views", count >= 22);
        assertTrue("The mill yard must have a native checkpoint and matching camera", millYard);
        assertTrue("The river landing must have a native checkpoint and matching camera", riverLanding);
        assertTrue("The rig leak trial must have a native checkpoint and matching camera", rigLeakTrial);
        assertEquals(-1, MainActivity.checkpointId("chapter-unknown"));
        assertNull(MainActivity.developmentCheckpointViewPose(999));
    }
}
