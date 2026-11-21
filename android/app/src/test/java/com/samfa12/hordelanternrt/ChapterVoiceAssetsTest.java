package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import java.util.HashSet;
import java.util.Set;

import org.junit.Test;

public final class ChapterVoiceAssetsTest {
    private static final String[] STABLE_IDS = {
            "keeper.sense", "keeper.closer", "prologue.kit_grate", "rescue.found",
            "rescue.rope", "reunion.question", "reunion.hint", "reunion.proof",
            "reunion.first_piece", "reunion.depart", "forest.night", "forest.waystone",
            "forest.clue", "forest.wait", "forest.village", "flask.empty", "flask.topup"
    };

    @Test public void everyAdmittedSpeechLineHasOneUniqueKeyAndExpectedRuntimePath() {
        assertEquals(STABLE_IDS.length + (BuildConfig.BELLWETHER_VOICE_CANDIDATE ? 5 : 0),
                ChapterVoiceAssets.all().size());
        final Set<String> keys = new HashSet<>();
        final Set<String> paths = new HashSet<>();
        for (int i = 0; i < STABLE_IDS.length; i++) {
            final int line = i + 1;
            final ChapterVoiceAssets.Asset asset = ChapterVoiceAssets.forLine(line);
            assertNotNull("line " + line, asset);
            assertEquals(line, asset.line);
            assertEquals(STABLE_IDS[i], asset.stableId);
            assertTrue("duplicate key " + asset.soundKey, keys.add(asset.soundKey));
            assertTrue("duplicate path " + asset.assetPath, paths.add(asset.assetPath));
            if (line <= 2) {
                assertTrue(asset.assetPath.startsWith("audio/pixabay/"));
            } else {
                assertEquals("kit_" + asset.stableId.replace('.', '_'), asset.soundKey);
                assertEquals("audio/kit/runtime/" + asset.stableId + ".wav", asset.assetPath);
            }
        }
    }

    @Test public void unknownAndBoundaryLinesHaveNoAssetAndMissingSoundDoesNotInventOne() {
        assertNull(ChapterVoiceAssets.forLine(Integer.MIN_VALUE));
        assertNull(ChapterVoiceAssets.forLine(0));
        assertNull(ChapterVoiceAssets.forLine(21));
        assertNull(ChapterVoiceAssets.forLine(Integer.MAX_VALUE));
        assertNull(ChapterVoiceAssets.soundIdForLine(3, new java.util.HashMap<String, Integer>()));
        final java.util.Map<String, Integer> sounds = new java.util.HashMap<>();
        sounds.put(ChapterVoiceAssets.forLine(3).soundKey, 27);
        assertEquals(Integer.valueOf(27), ChapterVoiceAssets.soundIdForLine(3, sounds));
        assertNull(ChapterVoiceAssets.soundIdForLine(21, sounds));
    }

    @Test public void privateBellwetherRosterMatchesApprovedKitLines() {
        final int[] lines = {18, 19, 20, 24, 26};
        final String[] ids = {"kit.accept_base.sit", "kit.accept_base.limit", "kit.accept_base.work",
                "hub.kit_introduce", "hub.hoard_answer"};
        for (int i = 0; i < lines.length; ++i) {
            final ChapterVoiceAssets.Asset asset = ChapterVoiceAssets.forLine(lines[i]);
            if (BuildConfig.BELLWETHER_VOICE_CANDIDATE) {
                assertNotNull(asset);
                assertEquals(ids[i], asset.stableId);
                assertEquals("private_bellwether_voice/" + ids[i] + ".wav", asset.assetPath);
            } else assertNull(asset);
        }
        for (int line : new int[]{21, 22, 23, 25, 27, 34}) assertNull(ChapterVoiceAssets.forLine(line));
    }
}
