package com.samfa12.hordelanternrt;

import java.util.Arrays;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

/** Finite mapping from the shared chapter line IDs to admitted runtime speech assets. */
final class ChapterVoiceAssets {
    static final class Asset {
        final int line;
        final String stableId;
        final String soundKey;
        final String assetPath;

        private Asset(int line, String stableId, String soundKey, String assetPath) {
            this.line = line;
            this.stableId = stableId;
            this.soundKey = soundKey;
            this.assetPath = assetPath;
        }
    }

    private static final Asset[] ASSETS = {
            asset(1, "keeper.sense", "keeper_i_sense_you", "audio/pixabay/keeper_i_sense_you.wav"),
            asset(2, "keeper.closer", "keeper_come_closer", "audio/pixabay/keeper_come_closer.wav"),
            kit(3, "prologue.kit_grate"),
            kit(4, "rescue.found"),
            kit(5, "rescue.rope"),
            kit(6, "reunion.question"),
            kit(7, "reunion.hint"),
            kit(8, "reunion.proof"),
            kit(9, "reunion.first_piece"),
            kit(10, "reunion.depart"),
            kit(11, "forest.night"),
            kit(12, "forest.waystone"),
            kit(13, "forest.clue"),
            kit(14, "forest.wait"),
            kit(15, "forest.village"),
            kit(16, "flask.empty"),
            kit(17, "flask.topup")
    };
    private static final List<Asset> ALL = admittedAssets();

    private static List<Asset> admittedAssets() {
        final List<Asset> assets = new ArrayList<>(Arrays.asList(ASSETS));
        if (BuildConfig.BELLWETHER_VOICE_CANDIDATE) {
            assets.add(bellwether(18, "kit.accept_base.sit", "kit_"));
            assets.add(bellwether(19, "kit.accept_base.limit", "kit_"));
            assets.add(bellwether(20, "kit.accept_base.work", "kit_"));
            assets.add(bellwether(24, "hub.kit_introduce", "kit_"));
            assets.add(bellwether(26, "hub.hoard_answer", "kit_"));
        }
        if (BuildConfig.BELLWETHER_ARRIVAL_AUDITION) {
            assets.add(bellwether(21, "kit.approach_injury.stop", "kit_"));
            assets.add(bellwether(22, "kit.approach_injury.ankle", "kit_"));
            assets.add(bellwether(23, "kit.approach_injury.slow", "kit_"));
            assets.add(bellwether(25, "hub.horde_army", "keeper_"));
            assets.add(bellwether(27, "hub.arrival_chair", "keeper_"));
        }
        if (BuildConfig.BELLWETHER_SALVAGER_AUDITION) {
            assets.add(bellwether(52, "access.body_task", "salvager_"));
            assets.add(bellwether(53, "access.body_progress", "salvager_"));
            assets.add(bellwether(54, "hub.passing.learned_retreat", "salvager_"));
        }
        return Collections.unmodifiableList(assets);
    }

    private ChapterVoiceAssets() {}

    static List<Asset> all() { return ALL; }

    /** Returns null for None, out-of-range lines, and every unadmitted ID. */
    static Asset forLine(int line) {
        for (Asset asset : ALL) if (asset.line == line) return asset;
        return null;
    }

    /** A missing or unadmitted cue returns null so callers can keep subtitle fallback. */
    static Integer soundIdForLine(int line, Map<String, Integer> sounds) {
        final Asset asset = forLine(line);
        return asset == null || sounds == null ? null : sounds.get(asset.soundKey);
    }

    private static Asset kit(int line, String stableId) {
        final String key = "kit_" + stableId.replace('.', '_');
        return asset(line, stableId, key, "audio/kit/runtime/" + stableId + ".wav");
    }

    private static Asset bellwether(int line, String stableId, String keyPrefix) {
        return asset(line, stableId, keyPrefix + stableId.replace('.', '_'),
                "private_bellwether_voice/" + stableId + ".wav");
    }

    private static Asset asset(int line, String stableId, String key, String path) {
        return new Asset(line, stableId, key, path);
    }
}
