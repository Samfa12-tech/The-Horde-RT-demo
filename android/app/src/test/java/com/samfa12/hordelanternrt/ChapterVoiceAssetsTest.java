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
        assertEquals(STABLE_IDS.length + (BuildConfig.BELLWETHER_VOICE_CANDIDATE ? 5 : 0)
                        + (BuildConfig.BELLWETHER_ARRIVAL_AUDITION ? 5 : 0)
                        + (BuildConfig.BELLWETHER_SALVAGER_AUDITION ? 3 : 0)
                        + (BuildConfig.BELLWETHER_FOLLOW_ON_AUDITION ? 6 : 0)
                        + (BuildConfig.BELLWETHER_HEARTH_AUDITION ? 2 : 0)
                        + selectedOptionalVoiceCount(),
                ChapterVoiceAssets.all().size());
        final Set<String> keys = new HashSet<>();
        final Set<String> paths = new HashSet<>();
        for (int i = 0; i < STABLE_IDS.length; i++) {
            final int line = i + 1;
            final ChapterVoiceAssets.Asset asset = ChapterVoiceAssets.forLine(line);
            assertNotNull("line " + line, asset);
            assertEquals(line, asset.line);
            assertEquals(STABLE_IDS[i], asset.stableId);
            if (line <= 2) {
                assertTrue(asset.assetPath.startsWith("audio/pixabay/"));
            } else {
                assertEquals("kit_" + asset.stableId.replace('.', '_'), asset.soundKey);
                assertEquals("audio/kit/runtime/" + asset.stableId + ".wav", asset.assetPath);
            }
        }
        for (ChapterVoiceAssets.Asset asset : ChapterVoiceAssets.all()) {
            assertTrue("duplicate key " + asset.soundKey, keys.add(asset.soundKey));
            assertTrue("duplicate path " + asset.assetPath, paths.add(asset.assetPath));
        }
    }

    @Test public void unknownAndBoundaryLinesHaveNoAssetAndMissingSoundDoesNotInventOne() {
        assertNull(ChapterVoiceAssets.forLine(Integer.MIN_VALUE));
        assertNull(ChapterVoiceAssets.forLine(0));
        assertNull(ChapterVoiceAssets.forLine(34));
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
    }

    @Test public void privateArrivalAuditionAdmitsOnlyTheFiveReviewTakes() {
        final int[] lines = {21, 22, 23, 25, 27};
        final String[] ids = {"kit.approach_injury.stop", "kit.approach_injury.ankle",
                "kit.approach_injury.slow", "hub.horde_army", "hub.arrival_chair"};
        for (int i = 0; i < lines.length; ++i) {
            final ChapterVoiceAssets.Asset asset = ChapterVoiceAssets.forLine(lines[i]);
            if (BuildConfig.BELLWETHER_ARRIVAL_AUDITION) {
                assertNotNull(asset);
                assertEquals(ids[i], asset.stableId);
                assertEquals("private_bellwether_voice/" + ids[i] + ".wav", asset.assetPath);
                assertTrue(asset.soundKey.startsWith(i < 3 ? "kit_" : "keeper_"));
            } else assertNull(asset);
        }
    }

    @Test public void privateSalvagerAuditionAdmitsOnlyTheThreeShorePracticeTakes() {
        final String[] ids = {"access.body_task", "access.body_progress", "hub.passing.learned_retreat"};
        for (int i = 0; i < ids.length; ++i) {
            final ChapterVoiceAssets.Asset asset = ChapterVoiceAssets.forLine(52 + i);
            if (BuildConfig.BELLWETHER_SALVAGER_AUDITION) {
                assertNotNull(asset);
                assertEquals(ids[i], asset.stableId);
                assertEquals("private_bellwether_voice/" + ids[i] + ".wav", asset.assetPath);
                assertTrue(asset.soundKey.startsWith("salvager_"));
            } else assertNull(asset);
        }
    }

    @Test public void followOnVoicesKeepTheirExactLinesAndMissingSoundFallback() {
        final int[] lines = {38, 39, 44, 45, 47, 55};
        final String[] ids = {"hub.two_routes", "hub.routes_independent", "kit.voice_reaction",
                "kit.first_direct_voice.finish", "kit.royal_claim.help", "hub.ordinary_sale"};
        for (int i = 0; i < lines.length; ++i) {
            final ChapterVoiceAssets.Asset asset = ChapterVoiceAssets.forLine(lines[i]);
            if (BuildConfig.BELLWETHER_FOLLOW_ON_AUDITION) {
                assertNotNull(asset);
                assertEquals(ids[i], asset.stableId);
                assertEquals("private_bellwether_voice/" + ids[i] + ".wav", asset.assetPath);
                assertEquals(Integer.valueOf(18), ChapterVoiceAssets.soundIdForLine(
                        lines[i], java.util.Collections.singletonMap(asset.soundKey, 18)));
            } else assertNull(asset);
            assertNull(ChapterVoiceAssets.soundIdForLine(lines[i], java.util.Collections.emptyMap()));
        }
        assertNull(ChapterVoiceAssets.forLine(46));
    }

    @Test public void privateKitHearthAuditionIsIndependentAndUsesExactLineAndSoundMappings() {
        final int[] lines = {59, 60};
        final String[] ids = {"hub.kit_stake", "kit.personal_stake.key"};
        for (int i = 0; i < lines.length; ++i) {
            final ChapterVoiceAssets.Asset asset = ChapterVoiceAssets.forLine(lines[i]);
            if (BuildConfig.BELLWETHER_HEARTH_AUDITION) {
                assertNotNull(asset);
                assertEquals(ids[i], asset.stableId);
                assertEquals("kit_" + ids[i].replace('.', '_'), asset.soundKey);
                assertEquals("private_bellwether_voice/" + ids[i] + ".wav", asset.assetPath);
                assertEquals(Integer.valueOf(23), ChapterVoiceAssets.soundIdForLine(
                        lines[i], java.util.Collections.singletonMap(asset.soundKey, 23)));
            } else {
                assertNull(asset);
                assertNull(ChapterVoiceAssets.soundIdForLine(
                        lines[i], java.util.Collections.singletonMap("kit_" + ids[i].replace('.', '_'), 23)));
            }
            assertNull(ChapterVoiceAssets.soundIdForLine(lines[i], java.util.Collections.emptyMap()));
        }
    }

    @Test public void privateKeeperLeadAuditionSelectsOnlyTheTwoPinnedLines() {
        final int[] lines = {35, 36};
        final String[] ids = {"hub.first_errand", "hub.shrine_direction"};
        for (int i = 0; i < lines.length; ++i) {
            final ChapterVoiceAssets.Asset asset = ChapterVoiceAssets.forLine(lines[i]);
            if (optionalVoiceSelected(ids[i])) {
                assertNotNull(asset);
                assertEquals(ids[i], asset.stableId);
                assertEquals("record_keeper_" + ids[i].replace('.', '_'), asset.soundKey);
                assertEquals("private_bellwether_voice/" + ids[i] + ".wav", asset.assetPath);
                assertEquals(Integer.valueOf(31), ChapterVoiceAssets.soundIdForLine(
                        lines[i], java.util.Collections.singletonMap(asset.soundKey, 31)));
            } else {
                assertNull(asset);
                assertNull(ChapterVoiceAssets.soundIdForLine(
                        lines[i], java.util.Collections.singletonMap(
                                "record_keeper_" + ids[i].replace('.', '_'), 31)));
            }
            assertNull(ChapterVoiceAssets.soundIdForLine(lines[i], java.util.Collections.emptyMap()));
        }
        assertNull(ChapterVoiceAssets.forLine(34));
    }

    private static boolean optionalVoiceSelected(String stableId) {
        for (String selected : BuildConfig.BELLWETHER_OPTIONAL_VOICE_IDS.split(","))
            if (stableId.equals(selected)) return true;
        return false;
    }

    private static int selectedOptionalVoiceCount() {
        int count = 0;
        for (String selected : BuildConfig.BELLWETHER_OPTIONAL_VOICE_IDS.split(","))
            if (!selected.isEmpty()) ++count;
        return count;
    }
}
