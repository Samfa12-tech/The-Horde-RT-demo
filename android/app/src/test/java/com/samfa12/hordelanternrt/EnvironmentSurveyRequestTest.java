package com.samfa12.hordelanternrt;

import static org.junit.Assert.*;
import android.content.Intent;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34)
public final class EnvironmentSurveyRequestTest {
    static Intent request() {
        return new Intent(EnvironmentSurveyRequest.ACTION).putExtra("request_id", "river-A_1")
                .putExtra("profile", "river-bend").putExtra("water_disabled", false);
    }

    @Test public void acceptsOnlyExplicitDebugColdStartWithTypedClosedExtras() {
        EnvironmentSurveyRequest parsed = EnvironmentSurveyRequest.parse(request(), true, true);
        assertNotNull(parsed);
        assertEquals("river-A_1", parsed.requestId);
        assertFalse(parsed.waterDisabled);
        assertTrue(EnvironmentSurveyRequest.parse(request().putExtra("water_disabled", true), true, true).waterDisabled);
        assertNull(EnvironmentSurveyRequest.parse(request(), false, true));
        assertNull(EnvironmentSurveyRequest.parse(request(), true, false));
        assertNull(EnvironmentSurveyRequest.parse(request().setAction(Intent.ACTION_MAIN), true, true));
        assertNull(EnvironmentSurveyRequest.parse(null, true, true));
    }

    @Test public void rejectsMalformedIdsAndTypesAndUnrelatedAutomation() {
        for (String id : new String[]{"", "../owner", "a/b", "a\\b", "a.b", "a b", "é", "x".repeat(65)})
            assertNull(id, EnvironmentSurveyRequest.parse(request().putExtra("request_id", id), true, true));
        assertNotNull(EnvironmentSurveyRequest.parse(request().putExtra("request_id", "x".repeat(64)), true, true));
        assertNull(EnvironmentSurveyRequest.parse(request().putExtra("request_id", 1), true, true));
        assertNull(EnvironmentSurveyRequest.parse(request().putExtra("profile", "River-bend"), true, true));
        assertNull(EnvironmentSurveyRequest.parse(request().putExtra("water_disabled", "false"), true, true));
        Intent missing = request(); missing.removeExtra("water_disabled");
        assertNull(EnvironmentSurveyRequest.parse(missing, true, true));
        for (String extra : new String[]{"horde.debug.checkpoint", "horde.debug.capture", "horde.benchmark.run_id",
                "entryPlay", "campaign_test_namespace", "motion", "replay"})
            assertNull(extra, EnvironmentSurveyRequest.parse(request().putExtra(extra, "ignored"), true, true));
    }

    @Test public void keeperFixtureRequiresItsOwnTypedClosedInputsAndColdDebugAdmission() {
        Intent fixture=new Intent(EnvironmentSurveyRequest.ACTION).putExtra("request_id","keeper-A1")
                .putExtra("profile",EnvironmentSurveyRequest.KEEPER_PROFILE).putExtra("mist_enabled",true);
        EnvironmentSurveyRequest parsed=EnvironmentSurveyRequest.parse(fixture,true,true);
        assertNotNull(parsed);assertTrue(parsed.keeperMist);assertTrue(parsed.mistEnabled);assertFalse(parsed.waterDisabled);
        assertFalse(EnvironmentSurveyRequest.parse(fixture.putExtra("mist_enabled",false),true,true).mistEnabled);
        assertNull(EnvironmentSurveyRequest.parse(fixture,false,true));
        assertNull(EnvironmentSurveyRequest.parse(fixture,true,false));
        assertNull(EnvironmentSurveyRequest.parse(new Intent(fixture).putExtra("mist_enabled","false"),true,true));
        for(String key:new String[]{"water_disabled","horde.debug.motion","horde.debug.checkpoint","horde.debug.performance_profile","campaign_test_namespace"})
            assertNull(EnvironmentSurveyRequest.parse(new Intent(fixture).putExtra(key,false),true,true));
    }

    @Test public void activityAdmissionPrecedesNormalInitializationAndNeverUsesGameplayLaunch() throws Exception {
        final String source = new String(Files.readAllBytes(Path.of("src/main/java/com/samfa12/hordelanternrt/MainActivity.java")), StandardCharsets.UTF_8);
        final String create = between(source, "protected void onCreate(", "private void createEnvironmentSurvey(");
        assertTrue(create.indexOf("EnvironmentSurveyRequest.isSurvey(getIntent())") < create.indexOf("setContentView(R.layout.activity_main)"));
        assertTrue(create.indexOf("createEnvironmentSurvey(savedInstanceState)") < create.indexOf("getSharedPreferences(PREFS"));
        final String survey = between(source, "private void createEnvironmentSurvey(", "private void configureCampaignStorage()");
        for (String forbidden : new String[]{"configureCampaign(", "configureCampaignStorage(", "startDiagnosticSurface(",
                "setEntryMenu(", "consumeDebugAutomationIntent(", "consumeBenchmarkAutomationIntent(",
                "setGameplayPaused(", "clearTouchState(", "setViewControls(", "writeReports(", "collectInitialDiagnostics(",
                "resetRtSceneTuning(", ".edit()"}) assertFalse(forbidden, survey.contains(forbidden));
        assertTrue(survey.indexOf("configureEnvironmentSurvey(") < survey.indexOf("new SurfaceView(this)"));
        assertTrue(survey.contains("startEnvironmentSurveySurface(currentSurface,"));
        assertTrue(survey.contains("environmentSurveyRequest.requestId"));
        assertTrue(survey.contains("environmentSurveyFinishing = true"));
        assertTrue(survey.contains("EnvironmentSurveyExport.export("));
        final int worker = survey.indexOf("final Thread export = new Thread");
        final int await = survey.indexOf("EnvironmentSurveyTerminalAwait.await(", worker);
        final int copy = survey.indexOf("EnvironmentSurveyExport.export(", worker);
        assertTrue(worker >= 0 && await > worker && copy > await);
        assertTrue(survey.substring(copy).contains("request.waterDisabled, terminalPhase"));
        assertTrue(survey.contains("finishAndRemoveTask()"));
    }

    @Test public void lifecycleAndInputReturnBeforeNormalHandlersAndAssetCacheIsShared() throws Exception {
        final String source = new String(Files.readAllBytes(Path.of("src/main/java/com/samfa12/hordelanternrt/MainActivity.java")), StandardCharsets.UTF_8);
        for (String method : new String[]{"protected void onResume()", "protected void onPause()", "protected void onStop()",
                "protected void onDestroy()", "public void onBackPressed()", "public void onConfigurationChanged(Configuration newConfig)",
                "public boolean dispatchKeyEvent(KeyEvent event)", "public boolean dispatchGenericMotionEvent(MotionEvent event)",
                "public boolean dispatchTouchEvent(MotionEvent event)", "public void onWindowFocusChanged(boolean focused)"}) {
            final String beginning = source.substring(source.indexOf(method), source.indexOf(method) + 420);
            assertTrue(method, beginning.contains("if (environmentSurveyMode)"));
            assertTrue(method, beginning.contains("return; ") || beginning.contains("return;") || beginning.contains("return true;"));
        }
        final String assets = between(source, "private boolean[] stageRuntimeAssetCache()", "private void collectInitialDiagnostics()");
        for (String forbidden : new String[]{"delete(", "writeReports(", "preferences", "ProbeBridge.", "campaign-v1"})
            assertFalse(forbidden, assets.contains(forbidden));
        assertTrue(assets.contains("chapter-terrain-lod0.runtime.glb"));
        assertTrue(assets.contains("chapter-terrain/base-color.android.ktx2"));
        assertEquals(3, source.split("stageRuntimeAssetCache\\(\\)", -1).length - 1);
    }

    private static String between(String source, String start, String end) {
        int first = source.indexOf(start);
        assertTrue(start, first >= 0);
        int last = source.indexOf(end, first);
        assertTrue(end, last > first);
        return source.substring(first, last);
    }
}
