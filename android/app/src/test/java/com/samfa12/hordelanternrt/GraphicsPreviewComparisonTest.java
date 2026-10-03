package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import org.junit.Test;

public final class GraphicsPreviewComparisonTest {
    private final GraphicsPreferences.Values confirmed = new GraphicsPreferences.Values(68, 0, 1, 15);
    private final GraphicsPreferences.Values draft = new GraphicsPreferences.Values(100, 2, 0, 60);

    @Test public void afterChangesOnlySelectedDraftFieldAndBeforePreservesCustomConfirmedTuple() {
        int[][] expected = {{100,0,1,15}, {68,2,1,15}, {68,0,0,15}, {68,0,1,60}};
        for (int choice = 0; choice < 4; ++choice) {
            assertSame(confirmed, GraphicsPreviewComparison.selection(confirmed, draft, choice, false));
            GraphicsPreferences.Values after = GraphicsPreviewComparison.selection(confirmed, draft, choice, true);
            assertTrue(after.same(new GraphicsPreferences.Values(expected[choice][0], expected[choice][1],
                    expected[choice][2], expected[choice][3])));
        }
        assertTrue(confirmed.same(new GraphicsPreferences.Values(68,0,1,15)));
        assertTrue(draft.same(new GraphicsPreferences.Values(100,2,0,60)));
    }

    @Test public void eachChoiceUsesProductionCameraWithWaterAndSkeletonDedicatedViews() {
        assertEquals(1, GraphicsPreviewComparison.camera(GraphicsPreviewComparison.RESOLUTION));
        assertEquals(3, GraphicsPreviewComparison.camera(GraphicsPreviewComparison.WATER));
        assertEquals(0, GraphicsPreviewComparison.camera(GraphicsPreviewComparison.FIRE));
        assertEquals(4, GraphicsPreviewComparison.camera(GraphicsPreviewComparison.CAP));
    }

    @Test(expected=IllegalArgumentException.class) public void unknownChoiceCannotConstructAfterTuple() {
        GraphicsPreviewComparison.selection(confirmed, draft, 4, true);
    }

    private long[] snapshot() {
        // Current-generation real RT preview with acknowledged requested/effective custom tuple.
        return new long[]{42,7,0,68,0,1,15,245,435,360,640,0,1,1,0,68,0,1,15,1};
    }

    @Test public void comparisonRequiresCurrentGenerationSerialPreviewAndExactEffectiveTuple() {
        assertTrue(GraphicsPreviewComparison.presented(snapshot(),7,42,confirmed));
        assertTrue(GraphicsPreviewComparison.presented(snapshot(),7,0,confirmed));
        assertFalse(GraphicsPreviewComparison.presented(snapshot(),8,42,confirmed));
        assertFalse(GraphicsPreviewComparison.presented(snapshot(),7,43,confirmed));
        assertFalse(GraphicsPreviewComparison.presented(snapshot(),7,42,draft));
        for (int index : new int[]{3,4,5,6,15,16,17,18}) {
            long[] changed = snapshot(); ++changed[index];
            assertFalse("tuple index " + index, GraphicsPreviewComparison.presented(changed,7,42,confirmed));
        }
        for (int index : new int[]{1,7,8,9,10,12,13,19}) {
            long[] changed = snapshot(); changed[index] = 0;
            assertFalse("presentation index " + index, GraphicsPreviewComparison.presented(changed,7,42,confirmed));
        }
        long[] compute = snapshot(); compute[12] = 2;
        assertTrue(GraphicsPreviewComparison.presented(compute,7,42,confirmed));
        assertFalse(GraphicsPreviewComparison.presented(null,7,42,confirmed));
        assertFalse(GraphicsPreviewComparison.presented(new long[19],7,42,confirmed));
    }

    @Test public void initialBeforeStillRequiresEffectiveTupleWhileOverlayBudgetIncludesBottomGap() {
        long[] changed = snapshot(); changed[3] = 75;
        assertFalse(GraphicsPreviewComparison.presented(changed,7,0,confirmed));
        for (int height : new int[]{568,640,915,100000}) for (int gap : new int[]{8,24,48}) {
            assertTrue(GraphicsPreviewComparison.maximumOverlayHeight(height,gap) + gap <= height * .35);
        }
        assertEquals(1, GraphicsPreviewComparison.maximumOverlayHeight(0,0));
    }
}
