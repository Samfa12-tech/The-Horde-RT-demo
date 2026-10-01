package com.samfa12.hordelanternrt;

import android.app.Activity;
import android.app.Dialog;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Base64;
import android.util.JsonReader;
import android.util.JsonToken;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.CookieManager;
import android.webkit.JsPromptResult;
import android.webkit.JsResult;
import android.webkit.PermissionRequest;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebMessage;
import android.webkit.WebMessagePort;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceResponse;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.GeolocationPermissions;
import android.webkit.SslErrorHandler;
import android.net.http.SslError;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.ByteArrayInputStream;
import java.io.StringReader;
import java.nio.charset.StandardCharsets;
import java.security.SecureRandom;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.function.LongPredicate;

/**
 * Foreground-only Turnstile verification UI. Call {@link #show} only after the
 * player has explicitly consented to remote submission. This class receives no
 * report or screenshot data and never submits a report itself.
 */
final class PlaytestReportVerification {
    static final String PAGE_URL = "https://briarhold-signal.samfa12.com/horde-report/verify";
    static final String PAGE_ORIGIN = "https://briarhold-signal.samfa12.com";
    static final String CHALLENGE_HOST = "challenges.cloudflare.com";
    static final int MAX_BRIDGE_MESSAGE_BYTES = 4 * 1024;
    static final int MAX_TOKEN_CHARS = 2048;
    static final long DEADLINE_MILLIS = 20_000L;
    private static final SecureRandom RANDOM = new SecureRandom();

    enum Failure { PAGE_LOAD_FAILED, VERIFICATION_FAILED, DEADLINE, UNAVAILABLE }

    interface Callback {
        void onVerified(long ownerGeneration, String token);
        void onFailure(long ownerGeneration, Failure failure);
        void onCancelled(long ownerGeneration);
    }

    interface Handle { void cancel(); }

    private PlaytestReportVerification() {}

    /**
     * Starts a new one-shot verification dialog. The caller owns generation
     * cancellation and must recheck ownerGeneration before using the token.
     */
    static Handle show(Activity activity, long ownerGeneration, LongPredicate isOwnerGenerationCurrent,
            Callback callback) {
        if (activity == null || isOwnerGenerationCurrent == null || callback == null)
            throw new IllegalArgumentException("verification dependencies required");
        if (Looper.myLooper() != Looper.getMainLooper())
            throw new IllegalStateException("verification dialog must start on the UI thread");
        Session session = new Session(activity, ownerGeneration, isOwnerGenerationCurrent, callback);
        session.start();
        return session;
    }

    static String newNonce(SecureRandom random) {
        byte[] bytes = new byte[24];
        random.nextBytes(bytes);
        return Base64.encodeToString(bytes, Base64.URL_SAFE | Base64.NO_PADDING | Base64.NO_WRAP);
    }

    static boolean isAllowedTopLevelUrl(String url) {
        return PAGE_URL.equals(url);
    }

    static boolean isAllowedChallengeUrl(String value) {
        if (value == null) return false;
        try {
            Uri uri = Uri.parse(value);
            return "https".equalsIgnoreCase(uri.getScheme()) &&
                    CHALLENGE_HOST.equalsIgnoreCase(uri.getHost()) &&
                    (uri.getPort() == -1 || uri.getPort() == 443) &&
                    uri.getUserInfo() == null && uri.getFragment() == null;
        } catch (RuntimeException malformed) { return false; }
    }

    static BridgeReply parseBridgeMessage(String raw, String expectedNonce) {
        if (raw == null || expectedNonce == null || !validNonce(expectedNonce) ||
                strictUtf8ByteLength(raw, MAX_BRIDGE_MESSAGE_BYTES) < 0) return null;
        Set<String> keys = new HashSet<>();
        String type = null, nonce = null, status = null, token = null;
        try (JsonReader reader = new JsonReader(new StringReader(raw))) {
            reader.setLenient(false);
            reader.beginObject();
            while (reader.hasNext()) {
                String key = reader.nextName();
                if (!keys.add(key) || reader.peek() != JsonToken.STRING) return null;
                String value = reader.nextString();
                if ("type".equals(key)) type = value;
                else if ("nonce".equals(key)) nonce = value;
                else if ("status".equals(key)) status = value;
                else if ("token".equals(key)) token = value;
                else return null;
            }
            reader.endObject();
            if (reader.peek() != JsonToken.END_DOCUMENT) return null;
        } catch (Exception malformed) { return null; }
        if (!"horde-report-verification".equals(type) || !expectedNonce.equals(nonce)) return null;
        if ("verified".equals(status) && keys.size() == 4 && validToken(token))
            return new BridgeReply(true, token);
        if ("failed".equals(status) && keys.size() == 3 && token == null)
            return new BridgeReply(false, null);
        return null;
    }

    private static int strictUtf8ByteLength(String value, int maximum) {
        int bytes = 0;
        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt(i);
            if (c <= 0x7f) bytes += 1;
            else if (c <= 0x7ff) bytes += 2;
            else if (Character.isHighSurrogate(c)) {
                if (i + 1 >= value.length() || !Character.isLowSurrogate(value.charAt(i + 1))) return -1;
                bytes += 4;
                i++;
            } else if (Character.isLowSurrogate(c)) return -1;
            else bytes += 3;
            if (bytes > maximum) return -1;
        }
        return bytes;
    }

    private static boolean validNonce(String nonce) {
        return nonce != null && nonce.matches("[A-Za-z0-9_-]{16,96}");
    }

    private static boolean validToken(String token) {
        if (token == null || token.isEmpty() || token.length() > MAX_TOKEN_CHARS) return false;
        for (int i = 0; i < token.length(); i++) {
            char c = token.charAt(i);
            if (c < 0x20 || c > 0x7e) return false;
        }
        return true;
    }

    static final class BridgeReply {
        final boolean verified;
        final String token;
        private BridgeReply(boolean verified, String token) { this.verified = verified; this.token = token; }
    }

    private static final class Session implements Handle {
        private final Activity activity;
        private final long ownerGeneration;
        private final LongPredicate generationIsCurrent;
        private final Callback callback;
        private final Handler main = new Handler(Looper.getMainLooper());
        private final AtomicBoolean terminal = new AtomicBoolean();
        private final String nonce = newNonce(RANDOM);
        private Dialog dialog;
        private WebView webView;
        private WebMessagePort appPort;
        private WebMessagePort pagePort;
        private boolean handshakeStarted;
        private final Runnable deadline = () -> finish(null, Failure.DEADLINE, false, false);

        Session(Activity activity, long ownerGeneration, LongPredicate generationIsCurrent, Callback callback) {
            this.activity = activity;
            this.ownerGeneration = ownerGeneration;
            this.generationIsCurrent = generationIsCurrent;
            this.callback = callback;
        }

        void start() {
            if (!ownerIsCurrent() || activity.isFinishing() || (Build.VERSION.SDK_INT >= 17 && activity.isDestroyed())) {
                finish(null, null, false, true);
                return;
            }
            try {
                buildDialog();
                dialog.show();
                dialog.getWindow().setLayout(dialogWidth(), dialogHeight());
                main.postDelayed(deadline, DEADLINE_MILLIS);
                webView.loadUrl(PAGE_URL);
            } catch (RuntimeException unavailable) {
                finish(null, Failure.UNAVAILABLE, false, false);
            }
        }

        private int dialogWidth() {
            return Math.min(dp(560), (int) (activity.getResources().getDisplayMetrics().widthPixels * 0.96f));
        }

        private int dialogHeight() {
            return Math.min(dp(720), (int) (activity.getResources().getDisplayMetrics().heightPixels * 0.82f));
        }

        private int dp(int value) {
            return (int) (value * activity.getResources().getDisplayMetrics().density + 0.5f);
        }

        private void buildDialog() {
            dialog = new Dialog(activity);
            dialog.setTitle("Verify report submission");
            dialog.setCancelable(true);
            dialog.setCanceledOnTouchOutside(false);

            LinearLayout root = new LinearLayout(activity);
            root.setOrientation(LinearLayout.VERTICAL);
            root.setPadding(dp(18), dp(12), dp(18), dp(12));
            root.setBackgroundColor(Color.rgb(25, 24, 21));

            TextView explanation = new TextView(activity);
            explanation.setText("Cloudflare checks for spam. Your report and optional game screenshot stay in the game until verification succeeds.");
            explanation.setTextColor(Color.rgb(246, 234, 211));
            explanation.setTextSize(14);
            explanation.setPadding(0, 0, 0, dp(8));
            root.addView(explanation, new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

            webView = new WebView(activity);
            configureWebView(webView);
            root.addView(webView, new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f));

            Button cancel = new Button(activity);
            cancel.setText("Cancel");
            cancel.setOnClickListener(view -> dialog.cancel());
            LinearLayout.LayoutParams cancelParams = new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            cancelParams.gravity = Gravity.END;
            root.addView(cancel, cancelParams);
            dialog.setContentView(root);
            dialog.setOnCancelListener(ignored -> finish(null, null, true, false));
            dialog.setOnDismissListener(ignored -> {
                if (!terminal.get()) finish(null, null, true, false);
            });
        }

        private void configureWebView(WebView view) {
            view.setSaveEnabled(false);
            view.setDownloadListener((url, userAgent, contentDisposition, mimeType, contentLength) -> {});
            WebSettings settings = view.getSettings();
            settings.setJavaScriptEnabled(true);
            settings.setDomStorageEnabled(true);
            settings.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);
            settings.setAllowFileAccess(false);
            settings.setAllowContentAccess(false);
            settings.setAllowFileAccessFromFileURLs(false);
            settings.setAllowUniversalAccessFromFileURLs(false);
            settings.setJavaScriptCanOpenWindowsAutomatically(false);
            settings.setSupportMultipleWindows(false);
            settings.setSupportZoom(false);
            settings.setBuiltInZoomControls(false);
            if (Build.VERSION.SDK_INT >= 26) settings.setSafeBrowsingEnabled(true);
            CookieManager.getInstance().setAcceptThirdPartyCookies(view, false);
            view.setWebChromeClient(new WebChromeClient() {
                @Override public boolean onJsAlert(WebView view, String url, String message, JsResult result) {
                    result.cancel();
                    return true;
                }
                @Override public boolean onJsConfirm(WebView view, String url, String message, JsResult result) {
                    result.cancel();
                    return true;
                }
                @Override public boolean onJsPrompt(WebView view, String url, String message, String defaultValue,
                        JsPromptResult result) {
                    result.cancel();
                    return true;
                }
                @Override public boolean onJsBeforeUnload(WebView view, String url, String message, JsResult result) {
                    result.cancel();
                    return true;
                }
                @Override public void onPermissionRequest(PermissionRequest request) { request.deny(); }
                @Override public void onGeolocationPermissionsShowPrompt(String origin, GeolocationPermissions.Callback cb) {
                    cb.invoke(origin, false, false);
                }
                @Override public boolean onShowFileChooser(WebView view, ValueCallback<Uri[]> callback,
                        FileChooserParams params) { return false; }
                @Override public boolean onCreateWindow(WebView view, boolean isDialog, boolean isUserGesture,
                        android.os.Message resultMsg) { return false; }
            });
            view.setWebViewClient(new WebViewClient() {
                @Override public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
                    String url = request.getUrl().toString();
                    if (request.isForMainFrame()) return !isAllowedTopLevelUrl(url);
                    return !isAllowedChallengeUrl(url);
                }

                @Override public WebResourceResponse shouldInterceptRequest(WebView view, WebResourceRequest request) {
                    String url = request.getUrl().toString();
                    boolean allowed = request.isForMainFrame()
                            ? isAllowedTopLevelUrl(url) : isAllowedChallengeUrl(url);
                    return allowed ? null : blockedResponse();
                }

                @Override public void onPageFinished(WebView view, String url) {
                    if (!ownerIsCurrent()) { finish(null, null, false, true); return; }
                    if (!isAllowedTopLevelUrl(url) || !isAllowedTopLevelUrl(view.getUrl())) {
                        finish(null, Failure.PAGE_LOAD_FAILED, false, false);
                        return;
                    }
                    startHandshake(view);
                }

                @Override public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error) {
                    if (request.isForMainFrame()) finish(null, Failure.PAGE_LOAD_FAILED, false, false);
                }

                @Override public void onReceivedHttpError(WebView view, WebResourceRequest request,
                        android.webkit.WebResourceResponse response) {
                    if (request.isForMainFrame() && response.getStatusCode() >= 400)
                        finish(null, Failure.PAGE_LOAD_FAILED, false, false);
                }

                @Override public void onReceivedSslError(WebView view, SslErrorHandler handler, SslError error) {
                    handler.cancel();
                    finish(null, Failure.PAGE_LOAD_FAILED, false, false);
                }

            });
        }

        private WebResourceResponse blockedResponse() {
            return new WebResourceResponse("text/plain", "UTF-8", 403, "Blocked", Collections.emptyMap(),
                    new ByteArrayInputStream(new byte[0]));
        }

        private void startHandshake(WebView view) {
            if (terminal.get()) return;
            if (!ownerIsCurrent()) { finish(null, null, false, true); return; }
            if (handshakeStarted) return;
            handshakeStarted = true;
            try {
                WebMessagePort[] ports = view.createWebMessageChannel();
                if (ports == null || ports.length != 2 || ports[0] == null || ports[1] == null)
                    throw new IllegalStateException("message channel unavailable");
                appPort = ports[0];
                pagePort = ports[1];
                appPort.setWebMessageCallback(new WebMessagePort.WebMessageCallback() {
                    @Override public void onMessage(WebMessagePort port, WebMessage message) {
                        if (terminal.get()) return;
                        if (!ownerIsCurrent()) { finish(null, null, false, true); return; }
                        BridgeReply reply = parseBridgeMessage(message == null ? null : message.getData(), nonce);
                        if (reply == null) return;
                        if (reply.verified) finish(reply.token, null, false, false);
                        else finish(null, Failure.VERIFICATION_FAILED, false, false);
                    }
                }, main);
                view.postWebMessage(new WebMessage("{\"type\":\"horde-report-init\",\"nonce\":\"" + nonce + "\"}",
                        new WebMessagePort[]{pagePort}), Uri.parse(PAGE_ORIGIN));
                pagePort = null; // Ownership transferred to the page.
            } catch (RuntimeException unavailable) {
                finish(null, Failure.UNAVAILABLE, false, false);
            }
        }

        private boolean ownerIsCurrent() {
            try { return generationIsCurrent.test(ownerGeneration); }
            catch (RuntimeException unavailable) { return false; }
        }

        @Override public void cancel() {
            if (Looper.myLooper() == Looper.getMainLooper()) finish(null, null, false, true);
            else main.post(() -> finish(null, null, false, true));
        }

        private void finish(String token, Failure failure, boolean userCancelled, boolean silent) {
            if (!terminal.compareAndSet(false, true)) return;
            main.removeCallbacks(deadline);
            closePort(appPort); appPort = null;
            closePort(pagePort); pagePort = null;
            if (webView != null) {
                try { webView.stopLoading(); } catch (RuntimeException ignored) {}
                try { ((ViewGroup) webView.getParent()).removeView(webView); } catch (RuntimeException ignored) {}
                try { webView.removeAllViews(); } catch (RuntimeException ignored) {}
                try { webView.destroy(); } catch (RuntimeException ignored) {}
                webView = null;
            }
            if (dialog != null && dialog.isShowing()) {
                try { dialog.dismiss(); } catch (RuntimeException ignored) {}
            }
            if (silent || !ownerIsCurrent()) return;
            try {
                if (token != null) callback.onVerified(ownerGeneration, token);
                else if (failure != null) callback.onFailure(ownerGeneration, failure);
                else if (userCancelled) callback.onCancelled(ownerGeneration);
            } catch (RuntimeException ignored) { /* Never surface callback details to the verification UI. */ }
        }

        private void closePort(WebMessagePort port) {
            if (port == null) return;
            try { port.close(); } catch (RuntimeException ignored) {}
        }
    }
}
