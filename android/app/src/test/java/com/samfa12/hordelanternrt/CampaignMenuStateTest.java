package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import org.junit.Test;

public final class CampaignMenuStateTest {
    static long[] ready() {
        long[] v=new long[33]; v[0]=1; v[1]=31; v[2]=1; v[4]=1; v[6]=-1;
        for(int i=0;i<6;i++) v[12+3*i]=CampaignMenuState.EMPTY;
        return v;
    }
    @Test public void rejectsUnsupportedMalformedAndNonfinitePublications() {
        assertNull(CampaignMenuState.decode(null));
        assertNull(CampaignMenuState.decode(new long[32]));
        for(int index:new int[]{0,4,6,7,11,12,30,31,32}) {
            long[] v=ready(); v[index]=999; assertNull("index "+index,CampaignMenuState.decode(v));
        }
        for(float value:new float[]{Float.NaN,Float.POSITIVE_INFINITY,Float.NEGATIVE_INFINITY}) {
            long[] v=ready(); v[9]=Float.floatToIntBits(value); assertNull(CampaignMenuState.decode(v));
        }
        long[] v=ready(); v[8]=-1; assertNull(CampaignMenuState.decode(v));
        v=ready(); v[10]=Float.floatToIntBits(.3f); assertNull(CampaignMenuState.decode(v));
        v=ready(); v[9]=1L<<40; assertNull(CampaignMenuState.decode(v));
        v=ready(); v[4]=CampaignMenuState.REBASE; assertNull(CampaignMenuState.decode(v));
        v=ready(); v[12]=CampaignMenuState.VALID; assertNull(CampaignMenuState.decode(v));
    }
    @Test public void immutableCatalogueKeepsManualAndAutosaveDistinct() {
        long[] v=ready(); v[12]=3; v[13]=1; v[14]=2; v[15]=3; v[16]=2; v[17]=3;
        CampaignMenuState s=CampaignMenuState.decode(v); assertNotNull(s);
        v[12]=2; assertEquals(3,s.status(0,0));
        assertEquals("Manual save: Lower chamber · 2 / 3 hearts",s.description(0,0));
        assertEquals("Autosave: Safe exit · 3 / 3 hearts",s.description(0,1));
        assertTrue(s.occupied(0)); assertFalse(s.occupied(1));
    }
    @Test public void onlyMatchingRequestSurfaceAndPresentedLoadCanComplete() {
        long[] v=ready(); v[3]=42; v[4]=7; v[30]=3;
        CampaignMenuState s=CampaignMenuState.decode(v);
        assertFalse(s.completes(42,3,31));
        v[11]=1; s=CampaignMenuState.decode(v);
        assertTrue(s.completes(42,3,31)); assertFalse(s.completes(41,3,31));
        assertFalse(s.completes(42,5,31)); assertFalse(s.completes(42,3,32));
        v[5]=10; s=CampaignMenuState.decode(v); assertFalse(s.completes(42,3,31));
        assertFalse(s.message().contains("saved")); assertTrue(s.message().contains("could not confirm"));
    }
    @Test public void forestEndpointIsNamedAndUnknownCheckpointIsRejected() {
        long[] v=ready();v[15]=CampaignMenuState.VALID;v[16]=3;v[17]=2;
        CampaignMenuState s=CampaignMenuState.decode(v);assertNotNull(s);
        assertEquals("Autosave: Forest lookout · 2 / 3 hearts",s.description(0,1));
        v[16]=4;assertNull(CampaignMenuState.decode(v));
    }
    @Test public void unknownErrorNeverBecomesSuccessOrSpecificCorruptionClaim() {
        long[] v=ready(); v[4]=8; v[5]=99;
        CampaignMenuState s=CampaignMenuState.decode(v); assertNotNull(s);
        assertTrue(s.message().contains("Check your saves")); assertFalse(s.message().contains("saved"));
    }
    @Test public void signedAndUnsignedFloatBitsRemainExact() {
        long[] v=ready(); v[9]=Integer.toUnsignedLong(Float.floatToIntBits(-12.5f));
        v[10]=Float.floatToIntBits(-.2f);
        CampaignMenuState s=CampaignMenuState.decode(v); assertNotNull(s);
        assertEquals(-12.5f,s.yaw,0); assertEquals(-.2f,s.pitch,0);
    }
    @Test public void debugNamespaceIsBoundedAndNeverAdmitsPathComponents() {
        assertEquals("trial_7-a",CampaignMenuState.testNamespace(true,"trial_7-a"));
        for(String v:new String[]{"","../owner","a/b","a\\b","a.b","a b","x".repeat(49)})
            assertNull(CampaignMenuState.testNamespace(true,v));
        assertNull(CampaignMenuState.testNamespace(false,"trial"));
        assertNull(CampaignMenuState.testNamespace(true,null));
    }
    @Test public void terminalReceiptsKeepOperationIdentityBehindPlainSaveMessage() {
        long[] v=ready();v[4]=7;v[30]=0;
        assertEquals(0,CampaignMenuState.decode(v).lastOperation);assertEquals("Saved.",CampaignMenuState.decode(v).message());
        v[30]=2;assertEquals(CampaignMenuState.SAVE_MANUAL,CampaignMenuState.decode(v).lastOperation);assertEquals("Saved.",CampaignMenuState.decode(v).message());
        v[30]=0;v[5]=10;assertTrue(CampaignMenuState.decode(v).message().contains("could not confirm"));
        assertFalse(CampaignMenuState.decode(v).message().contains("saved"));
    }
    @Test public void confirmedRetryExtensionIsOptionalAndStrictlyBoolean() {
        assertFalse(CampaignMenuState.decode(ready()).confirmedRetry);
        long[] v=java.util.Arrays.copyOf(ready(),34);v[33]=1;
        assertTrue(CampaignMenuState.decode(v).confirmedRetry);
        v[33]=2;assertNull(CampaignMenuState.decode(v));
    }

    @Test public void playerProgressAndErrorLanguageHidesImplementationWithoutClaimingSuccess() {
        for(int phase:new int[]{CampaignMenuState.READING,CampaignMenuState.PREPARING,CampaignMenuState.REBASE,CampaignMenuState.PRESENTING}) {
            long[] v=ready();v[4]=phase;if(phase==CampaignMenuState.REBASE)v[8]=1;
            String text=CampaignMenuState.decode(v).message();
            for(String internal:new String[]{"record", "private", "RT", "resource", "rebase"})assertFalse(text.contains(internal));
            assertFalse(text.contains("Loaded."));
        }
        long[] v=ready();v[5]=10;
        assertTrue(CampaignMenuState.decode(v).message().contains("could not confirm the save"));
        v[5]=5;assertTrue(CampaignMenuState.decode(v).message().contains("kept"));
        v[5]=7;assertTrue(CampaignMenuState.decode(v).message().contains("still paused"));
    }

    @Test public void datedPublicationChoosesExactlyTheVisibleRecordAndRejectsBadMetadata() {
        long[] v=java.util.Arrays.copyOf(ready(),40);v[0]=2;
        v[12]=v[15]=3;v[13]=1;v[14]=2;v[16]=2;v[17]=3;
        v[34]=1767225600L;v[35]=1767225660L;
        CampaignMenuState state=CampaignMenuState.decode(v);assertNotNull(state);
        assertEquals(CampaignMenuState.AUTO,state.selectedKind(0));
        assertTrue(state.slotDescription(0).contains("Safe exit"));
        v[34]+=120;state=CampaignMenuState.decode(v);
        assertEquals(CampaignMenuState.MANUAL,state.selectedKind(0));
        assertTrue(state.slotDescription(0).contains("Lower chamber"));
        v[35]=v[34];assertEquals(CampaignMenuState.MANUAL,CampaignMenuState.decode(v).selectedKind(0));
        v[34]=v[35]=0;state=CampaignMenuState.decode(v);
        assertEquals(CampaignMenuState.AUTO,state.selectedKind(0));assertTrue(state.slotDescription(0).contains("Date unavailable"));
        assertEquals("Empty",state.slotDescription(1));assertEquals(-1,state.selectedKind(1));
        v[15]=CampaignMenuState.CHECKING;assertEquals(-1,CampaignMenuState.decode(v).selectedKind(0));
        v[35]=-1;assertNull(CampaignMenuState.decode(v));v[35]=253402300800L;assertNull(CampaignMenuState.decode(v));
        assertNull(CampaignMenuState.decode(java.util.Arrays.copyOf(v,39)));
    }
    @Test public void saveDateUsesTheSavedInstantAndPlayerLocaleAndTimezone() {
        long utcNewYear=1767225600L;
        String utc=CampaignMenuState.formatSavedTime(utcNewYear,java.util.Locale.US,java.util.TimeZone.getTimeZone("UTC"));
        String west=CampaignMenuState.formatSavedTime(utcNewYear,java.util.Locale.US,java.util.TimeZone.getTimeZone("America/Los_Angeles"));
        String east=CampaignMenuState.formatSavedTime(utcNewYear,java.util.Locale.FRANCE,java.util.TimeZone.getTimeZone("Australia/Sydney"));
        assertTrue(utc.contains("2026"));assertTrue(west.contains("2025"));assertNotEquals(utc,west);assertNotEquals(utc,east);
        assertEquals("Date unavailable",CampaignMenuState.formatSavedTime(0,java.util.Locale.US,java.util.TimeZone.getTimeZone("UTC")));
    }

}
