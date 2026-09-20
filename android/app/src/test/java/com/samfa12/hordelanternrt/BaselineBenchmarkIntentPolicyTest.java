package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import android.content.Intent;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34)
public final class BaselineBenchmarkIntentPolicyTest {
    private static final String ACTION =
            "com.samfa12.hordelanternrt.action.BASELINE_BENCHMARK";
    private static final String WORKLOAD = "horde.benchmark.workload";

    private static Intent request(final String workload) {
        return new Intent(ACTION).putExtra(WORKLOAD, workload);
    }

    @Test
    public void exactSixWorkloadsAreAccepted() {
        final String[] workloads = {
                "showcase-route-v1", "lantern-held-high-v1", "lantern-held-low-v1",
                "lantern-grazing-v1", "lantern-motion-extreme-v1",
                "lantern-reveal-sequence-v1"
        };
        for (final String workload : workloads) {
            assertTrue(MainActivity.isAllowedBaselineWorkload(workload));
            assertEquals(workload, MainActivity.baselineWorkloadFromIntent(request(workload), true));
        }
    }

    @Test
    public void disabledBuildRejectsBaselineActionButLeavesUnrelatedActionAlone() {
        assertThrows(IllegalArgumentException.class,
                () -> MainActivity.baselineWorkloadFromIntent(request("showcase-route-v1"), false));
        assertNull(MainActivity.baselineWorkloadFromIntent(new Intent(Intent.ACTION_MAIN), false));
    }

    @Test
    public void missingUnknownMalformedAndMixedDebugWorkloadsAreRejected() {
        assertThrows(IllegalArgumentException.class,
                () -> MainActivity.baselineWorkloadFromIntent(new Intent(ACTION), true));
        for (final String workload : new String[]{"", "showcase-route", "../escape", "unknown-v1"}) {
            assertThrows(IllegalArgumentException.class,
                    () -> MainActivity.baselineWorkloadFromIntent(request(workload), true));
        }
        assertThrows(IllegalArgumentException.class,
                () -> MainActivity.baselineWorkloadFromIntent(
                        request("lantern-held-high-v1").putExtra("horde.debug.scale", 76), true));
        assertThrows(IllegalArgumentException.class,
                () -> MainActivity.baselineWorkloadFromIntent(
                        request("lantern-held-high-v1").putExtra(WORKLOAD, 7), true));
    }
}
