package com.samfa12.hordelanternrt;
import static org.junit.Assert.*;
import org.junit.Test;
public final class HealingFlaskUiStateTest {
    @Test public void currentOwnerFlagsKeepDrinkAndRestDistinctAndDefaultOff() {
        long[] v={1,31,0,1,2,1,2,0};
        HealingFlaskUiState s=HealingFlaskUiState.decode(v,31);assertNotNull(s);
        assertFalse(s.allows(false));assertFalse(s.allows(true));
        v[2]=1;s=HealingFlaskUiState.decode(v,31);assertTrue(s.allows(false));assertTrue(s.allows(true));
        v[3]=0;s=HealingFlaskUiState.decode(v,31);assertFalse(s.allows(false));assertTrue(s.allows(true));
        v[4]=0;s=HealingFlaskUiState.decode(v,31);assertFalse(s.allows(true));
        v[3]=1;v[7]=1;s=HealingFlaskUiState.decode(v,31);assertFalse(s.allows(false));assertFalse(s.allows(true));
    }
    @Test public void staleMalformedOrUnavailablePublicationCannotExposeOrAdmitControls() {
        assertNull(HealingFlaskUiState.decode(null,31));assertNull(HealingFlaskUiState.decode(new long[7],31));
        long[] v={1,31,1,1,1,2,2,0};assertNull(HealingFlaskUiState.decode(v,32));
        for(int index:new int[]{0,2,3,4,5,6,7}) {long[] bad=v.clone();bad[index]=99;assertNull(HealingFlaskUiState.decode(bad,31));}
        assertNull(HealingFlaskUiState.decode(v,0));
    }
}
