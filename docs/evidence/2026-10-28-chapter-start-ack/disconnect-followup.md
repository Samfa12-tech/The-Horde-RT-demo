# Interrupted phone preflight and verified recovery

Candidate source e2addc7795dc3fc7d687b9fd0aa089fb2c4cd1b7; all six jobs in
CI40662079547 passed before installation. SM-S948B / Android17 API37, isolated
Debug APK SHA2565704832437d098211882e3a40a39a7940c37d78af02235a0ee4b7bb7f696b91f.
Both development APK signatures verified with matching signer. Only the isolated
primary-user package was updated; no uninstall/data clear or owner package change.
Its189 bundled assets match the958f5182 baseline byte-for-byte, including shaders.

The first checkpoint207 request was queued. Its ACK read failed JSON decoding
at line1/column1 before the raw payload was retained. The subsequent raw-byte
inspection never executed: exec-server transport disconnected. No timed motion
run started. No fresh RT-presented ACK, actual mode preparation, renderer fault
or performance result is established for this build. Do not call this native
JSON corruption or a failed device compatibility verdict.

After the environment reconnected, ONLY recovery cleanup ran. Release was
verified at2026-10-28T09:27:50.091467UTC, before the09:39:40 deadline: isolated
experiment stopped/no PID, launcher foreground, original automatic brightness
mode1 restored and numeric readback73 matched the initial73. Rotation0, auto-
rotation0 and font1.5 were unchanged; both owner APK identities were unchanged.
The isolated e2 candidate remains installed and stopped. No owned monitor,
transfer or tracing remains, and no device operation followed release.

## Offline consumer finding, not diagnosis of unseen device bytes

The local consumer checked remote read errors only when adb returned nonzero.
An explicit missing-file message in stdout with exit0 was therefore passed to
JSON decoding. Synthetic tests reproduce that weakness; the actual failed bytes
are unavailable, so this remains a possible explanation rather than the proven
cause of this phone observation.

The private consumer now records raw ACK stdout/stderr before classification,
treats explicit missing-file responses as pending for either exit0 or1, and
stops on access denial, malformed UTF-8/JSON, wrong schema/type or excessive size.
Twelve offline tests pass with zero device calls. All synthetic raw fixtures are
isolated under offline-synthetic-consumer and labelled synthetic; none is actual
phone evidence or an accepted native ACK. Production source/APK was not changed.

Next device preflight needs a new allocation, fresh raw capture and matching
current-process checkpoint generation/surface/RT identities. Only after repeated
start acknowledgement is genuinely witnessed should a new bounded comparison
begin. Existing inconclusive958 timings, normal worker OFF and all other feature
admission gates remain unchanged. No publication or report transmission occurred.
