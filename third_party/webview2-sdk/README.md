# Windows report verification SDK pin

Microsoft.Web.WebView2 1.0.4258.31, official NuGet package, BSD-3-Clause.
Only the native headers/static loader and licence are restored into ignored
build storage by `tools/restore-report-webview2.ps1`. Six entry hashes and the
9,801,922-byte archive hash are recorded in `manifest.json`; configure verifies
the restored bytes. No SDK binaries, editor framework or browser Runtime are
vendored in the source tree. No configure-time download.

The game's Win32 shell remains native. WebView2 is used only for foreground
anti-spam verification after report consent; the report/image never enters it.
Runtime uses the installed Evergreen WebView2, not a browser preview or bundled
fixed Runtime. Missing/incompatible Runtime fails clearly and retains offline
JSON export; it is not installed automatically by the game. No game publication
or successful real verification is implied by restoring/compiling this SDK.

Official package/licence:
https://www.nuget.org/packages/Microsoft.Web.WebView2/1.0.4258.31

Security/distribution reference:
https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/security
https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/distribution
