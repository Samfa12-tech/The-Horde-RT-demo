package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Intent;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34)
public final class BenchmarkAutomationIntentTest {
    private Intent request(String id) {
        return new Intent("com.samfa12.hordelanternrt.action.BENCHMARK")
                .putExtra("horde.benchmark.run_id", id);
    }

    @Test public void explicitRequestPreservesIdentityWithoutDebugPrivilege() {
        assertEquals("run-123_ABC", MainActivity.benchmarkAutomationRequestId(request("run-123_ABC")));
        assertNull(MainActivity.benchmarkAutomationRequestId(new Intent(Intent.ACTION_MAIN)));
        assertNull(MainActivity.benchmarkAutomationRequestId(null));
    }

    @Test public void malformedIdentityAndMixedDebugControlsAreRejected() {
        for (String id : new String[]{"", "../escape", "with space", "x".repeat(65)}) {
            assertThrows(IllegalArgumentException.class,
                    () -> MainActivity.benchmarkAutomationRequestId(request(id)));
        }
        for (String key : new String[]{"horde.debug.scale", "horde.debug.checkpoint",
                "horde.debug.capture", "horde_require_rayquery_compute"}) {
            assertThrows(IllegalArgumentException.class,
                    () -> MainActivity.benchmarkAutomationRequestId(request("valid").putExtra(key, "value")));
        }
        assertThrows(IllegalArgumentException.class, () -> MainActivity.benchmarkAutomationRequestId(
                new Intent("com.samfa12.hordelanternrt.action.BENCHMARK")));
    }
}
