package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

import android.content.res.AssetFileDescriptor;
import android.media.SoundPool;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.util.Map;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadow.api.Shadow;
import org.robolectric.shadows.ShadowSoundPool;
import org.robolectric.util.ReflectionHelpers;
import org.robolectric.util.ReflectionHelpers.ClassParameter;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34, shadows = PackagedSoundLoadTest.Pool.class)
public final class PackagedSoundLoadTest {
    private static final String CUE = "audio/filmcow/ui_select.wav";
    private static final String KEY = "test_cue";
    private MainActivity activity;
    private SoundPool soundPool;
    private Pool pool;
    private File stagedSound;

    @Implements(SoundPool.class)
    public static class Pool extends ShadowSoundPool {
        int directId = 12;
        int directLoads;
        int pathLoads;
        AssetFileDescriptor descriptor;
        String path;

        @Implementation
        protected int load(AssetFileDescriptor asset, int priority) {
            assertEquals(1, priority);
            descriptor = asset;
            ++directLoads;
            return directId;
        }

        @Implementation
        protected int load(String filename, int priority) {
            assertEquals(1, priority);
            path = filename;
            ++pathLoads;
            return 23;
        }
    }

    @Before public void setUp() {
        activity = Robolectric.buildActivity(MainActivity.class).get();
        soundPool = new SoundPool.Builder().setMaxStreams(5).build();
        pool = Shadow.extract(soundPool);
        ReflectionHelpers.setField(activity, "soundPool", soundPool);
        stagedSound = new File(activity.getCacheDir(), "alpha_sfx/" + KEY + ".wav");
    }

    @After public void releasePool() {
        soundPool.release();
    }

    private Map<String, Integer> load(String path) {
        ReflectionHelpers.callInstanceMethod(activity, "loadSound",
                ClassParameter.from(String.class, KEY), ClassParameter.from(String.class, path));
        return ReflectionHelpers.getField(activity, "sounds");
    }

    @Test public void storedAssetClosesItsDescriptorAndRegistersWithoutACacheCopy() {
        assertEquals(Integer.valueOf(12), load(CUE).get(KEY));
        assertEquals(1, pool.directLoads);
        assertEquals(0, pool.pathLoads);
        assertFalse(pool.descriptor.getFileDescriptor().valid());
        assertFalse(stagedSound.exists());
    }

    @Test public void rejectedDescriptorUsesTheExistingStagedAudioBytes() throws IOException {
        pool.directId = 0;
        assertEquals(Integer.valueOf(23), load(CUE).get(KEY));
        assertEquals(1, pool.directLoads);
        assertEquals(1, pool.pathLoads);
        assertEquals(stagedSound.getAbsolutePath(), pool.path);
        try (InputStream source = activity.getAssets().open(CUE)) {
            assertArrayEquals(source.readAllBytes(), Files.readAllBytes(stagedSound.toPath()));
        }
    }

    @Test public void missingAssetLeavesTheSoundUnregistered() {
        assertFalse(load("audio/missing.wav").containsKey(KEY));
        assertEquals(0, pool.directLoads);
        assertEquals(0, pool.pathLoads);
        assertFalse(stagedSound.exists());
    }
}
