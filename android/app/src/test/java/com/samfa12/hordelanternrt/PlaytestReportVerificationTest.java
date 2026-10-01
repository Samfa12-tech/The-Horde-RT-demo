package com.samfa12.hordelanternrt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.os.Looper;

import java.lang.reflect.Method;
import java.security.SecureRandom;
import java.time.Duration;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.Shadows;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 28)
public final class PlaytestReportVerificationTest {
    private static final String NONCE = "0123456789abcdefghijklmnopqrstuv";

    @Test public void createsHighEntropyUrlSafeNonce() {
        String first = PlaytestReportVerification.newNonce(new SecureRandom());
        String second = PlaytestReportVerification.newNonce(new SecureRandom());
        assertTrue(first.matches("[A-Za-z0-9_-]{32}"));
        assertTrue(second.matches("[A-Za-z0-9_-]{32}"));
        assertFalse(first.equals(second));
    }

    @Test public void navigationAllowsOnlyVerificationMainPageAndCloudflareHttpsResources() {
        assertTrue(PlaytestReportVerification.isAllowedTopLevelUrl(
                "https://briarhold-signal.samfa12.com/horde-report/verify"));
        assertFalse(PlaytestReportVerification.isAllowedTopLevelUrl(
                "https://briarhold-signal.samfa12.com/horde-report/verify?report=private"));
        assertFalse(PlaytestReportVerification.isAllowedTopLevelUrl(
                "https://briarhold-signal.samfa12.com.evil.invalid/horde-report/verify"));
        assertTrue(PlaytestReportVerification.isAllowedChallengeUrl(
                "https://challenges.cloudflare.com/turnstile/v0/api.js?render=explicit"));
        assertTrue(PlaytestReportVerification.isAllowedChallengeUrl(
                "https://challenges.cloudflare.com:443/cdn-cgi/challenge-platform/frame"));
        assertFalse(PlaytestReportVerification.isAllowedChallengeUrl(
                "http://challenges.cloudflare.com/turnstile/v0/api.js"));
        assertFalse(PlaytestReportVerification.isAllowedChallengeUrl(
                "https://challenges.cloudflare.com.evil.invalid/frame"));
        assertFalse(PlaytestReportVerification.isAllowedChallengeUrl(
                "https://attacker@challenges.cloudflare.com/frame"));
        assertFalse(PlaytestReportVerification.isAllowedChallengeUrl(
                "https://challenges.cloudflare.com:444/frame"));
        assertFalse(PlaytestReportVerification.isAllowedChallengeUrl(
                "https://challenges.cloudflare.com/frame#fragment"));
        assertFalse(PlaytestReportVerification.isAllowedChallengeUrl("file:///sdcard/report.json"));
    }

    @Test public void admitsOnlyExactLocalTurnstileChildDocuments() {
        assertTrue(PlaytestReportVerification.isAllowedChildDocumentUrl("about:blank"));
        assertTrue(PlaytestReportVerification.isAllowedChildDocumentUrl("about:srcdoc"));
        assertTrue(PlaytestReportVerification.isAllowedWebViewRequest(false, "about:blank"));
        assertTrue(PlaytestReportVerification.isAllowedWebViewRequest(false, "about:srcdoc"));
        assertTrue(PlaytestReportVerification.isAllowedWebViewRequest(false,
                "https://challenges.cloudflare.com/turnstile/v0/api.js"));

        for (String variation : new String[] {
                "ABOUT:blank", "about:blank?x=1", "about:blank#fragment", "about:blank/evil",
                "about:srcdoc?x=1", "about:srcdoc#fragment", "about:srcdoc/evil",
                "about:", "data:text/html,frame", "file:///sdcard/report.json",
                "javascript:alert(1)", "https://attacker.invalid/frame"
        }) {
            assertFalse("child URL must stay exact: " + variation,
                    PlaytestReportVerification.isAllowedChildDocumentUrl(variation));
            assertFalse("child request must stay allowlisted: " + variation,
                    PlaytestReportVerification.isAllowedWebViewRequest(false, variation));
        }
        assertFalse(PlaytestReportVerification.isAllowedWebViewRequest(true, "about:blank"));
        assertFalse(PlaytestReportVerification.isAllowedWebViewRequest(true, "about:srcdoc"));
        assertTrue(PlaytestReportVerification.isAllowedWebViewRequest(true,
                PlaytestReportVerification.PAGE_URL));
        assertFalse(PlaytestReportVerification.isAllowedWebViewRequest(false,
                PlaytestReportVerification.PAGE_URL));
    }

    @Test public void acceptsOnlyExactBoundedNonceMatchedBridgeReplies() {
        PlaytestReportVerification.BridgeReply verified = reply(
                "{\"type\":\"horde-report-verification\",\"nonce\":\"" + NONCE +
                        "\",\"status\":\"verified\",\"token\":\"abc123._- =\"}");
        assertNotNull(verified);
        assertTrue(verified.verified);
        assertEquals("abc123._- =", verified.token);

        PlaytestReportVerification.BridgeReply failed = reply(
                "{\"type\":\"horde-report-verification\",\"nonce\":\"" + NONCE +
                        "\",\"status\":\"failed\"}");
        assertNotNull(failed);
        assertFalse(failed.verified);
        assertNull(failed.token);

        assertNull(reply("{\"type\":\"horde-report-verification\",\"nonce\":\"wrong_nonce_123456\",\"status\":\"verified\",\"token\":\"t\"}"));
        assertNull(reply("{\"type\":\"other\",\"nonce\":\"" + NONCE + "\",\"status\":\"verified\",\"token\":\"t\"}"));
        assertNull(reply("{\"type\":\"horde-report-verification\",\"nonce\":\"" + NONCE + "\",\"status\":\"verified\",\"token\":\"t\",\"extra\":true}"));
        assertNull(reply("{\"type\":\"horde-report-verification\",\"type\":\"horde-report-verification\",\"nonce\":\"" + NONCE + "\",\"status\":\"verified\",\"token\":\"t\"}"));
        assertNull(reply("{\"type\":\"horde-report-verification\",\"nonce\":\"" + NONCE + "\",\"status\":\"failed\",\"token\":\"unexpected\"}"));
        assertNull(reply("{\"type\":\"horde-report-verification\",\"nonce\":\"" + NONCE + "\",\"status\":\"verified\",\"token\":\"bad\\nvalue\"}"));
        assertNull(reply("{\"type\":\"horde-report-verification\",\"nonce\":\"" + NONCE + "\",\"status\":\"verified\",\"token\":\"\u007f\"}"));
        assertNull(reply("not json"));
    }

    @Test public void bridgeEnforcesTokenAndMessageBoundsAndStrictUtf8() {
        String largeToken = "a".repeat(PlaytestReportVerification.MAX_TOKEN_CHARS);
        assertNotNull(reply("{\"type\":\"horde-report-verification\",\"nonce\":\"" + NONCE +
                "\",\"status\":\"verified\",\"token\":\"" + largeToken + "\"}"));
        String oversizedToken = "a".repeat(PlaytestReportVerification.MAX_TOKEN_CHARS + 1);
        assertNull(reply("{\"type\":\"horde-report-verification\",\"nonce\":\"" + NONCE +
                "\",\"status\":\"verified\",\"token\":\"" + oversizedToken + "\"}"));
        assertNull(reply("{\"type\":\"horde-report-verification\",\"nonce\":\"" + NONCE +
                "\",\"status\":\"verified\",\"token\":\"" + "a".repeat(PlaytestReportVerification.MAX_BRIDGE_MESSAGE_BYTES) + "\"}"));
        assertNull(reply("{\"type\":\"horde-report-verification\",\"nonce\":\"" + NONCE +
                "\",\"status\":\"verified\",\"token\":\"\\ud800\"}"));
    }

    @Test public void cancelSilentlyTearsDownSessionWithoutCallback() {
        Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
        RecordingCallback callback = new RecordingCallback();
        PlaytestReportVerification.Handle handle = PlaytestReportVerification.show(activity, 7L,
                generation -> generation == 7L, callback);

        handle.cancel();
        Shadows.shadowOf(Looper.getMainLooper()).idle();

        assertEquals(0, callback.calls.get());
        assertNull(sessionField(handle, "webView"));
        activity.finish();
    }

    @Test public void deadlineReportsOneSafeFailureAndCompletesOnlyOnce() {
        Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
        RecordingCallback callback = new RecordingCallback();
        PlaytestReportVerification.Handle handle = PlaytestReportVerification.show(activity, 8L,
                generation -> generation == 8L, callback);

        Shadows.shadowOf(Looper.getMainLooper()).idleFor(Duration.ofMillis(
                PlaytestReportVerification.DEADLINE_MILLIS));

        assertEquals(1, callback.calls.get());
        assertEquals(PlaytestReportVerification.Failure.DEADLINE, callback.failure);
        finishAgain(handle, PlaytestReportVerification.Failure.PAGE_LOAD_FAILED);
        assertEquals(1, callback.calls.get());
        assertEquals(PlaytestReportVerification.Failure.DEADLINE, callback.failure);
        activity.finish();
    }

    @Test public void expiredOwnerGenerationDropsDeadlineCallback() {
        Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
        AtomicBoolean current = new AtomicBoolean(true);
        RecordingCallback callback = new RecordingCallback();
        PlaytestReportVerification.Handle handle = PlaytestReportVerification.show(activity, 9L,
                generation -> current.get() && generation == 9L, callback);

        current.set(false);
        Shadows.shadowOf(Looper.getMainLooper()).idleFor(Duration.ofMillis(
                PlaytestReportVerification.DEADLINE_MILLIS));

        assertEquals(0, callback.calls.get());
        assertNull(sessionField(handle, "webView"));
        activity.finish();
    }

    private static Object sessionField(Object session, String name) {
        try {
            java.lang.reflect.Field field = session.getClass().getDeclaredField(name);
            field.setAccessible(true);
            return field.get(session);
        } catch (ReflectiveOperationException error) { throw new AssertionError(error); }
    }

    private static void finishAgain(PlaytestReportVerification.Handle session,
            PlaytestReportVerification.Failure failure) {
        try {
            Method method = session.getClass().getDeclaredMethod("finish", String.class,
                    PlaytestReportVerification.Failure.class, boolean.class, boolean.class);
            method.setAccessible(true);
            method.invoke(session, null, failure, false, false);
        } catch (ReflectiveOperationException error) { throw new AssertionError(error); }
    }

    private static final class RecordingCallback implements PlaytestReportVerification.Callback {
        final AtomicInteger calls = new AtomicInteger();
        PlaytestReportVerification.Failure failure;

        @Override public void onVerified(long generation, String token) { calls.incrementAndGet(); }
        @Override public void onFailure(long generation, PlaytestReportVerification.Failure value) {
            failure = value;
            calls.incrementAndGet();
        }
        @Override public void onCancelled(long generation) { calls.incrementAndGet(); }
    }

    private static PlaytestReportVerification.BridgeReply reply(String value) {
        return PlaytestReportVerification.parseBridgeMessage(value, NONCE);
    }
}
