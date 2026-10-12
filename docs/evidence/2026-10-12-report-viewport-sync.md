# MSVC viewport observation correction

Run38193691355 at `b3e3a4ef9da6ec2282139301e1d117bdfeed4316` reproduced
the remote-report assertion: keyboard focus reached consent before the worker
thread observed its full viewport containment. `FocusTabControl` calls SetFocus
before EnsureControlVisible; the focus subclass also posts a visibility message.
The test was racing focus publication against UI-thread scroll completion.

Its existing WaitFor now observes focus and full containment together within
the same deadline. The final containment assertion and all navigation/consent
coverage remain. The native Release test passes locally in1721ms on a separate
non-input Windows desktop: no desktop/foreground switch or owner input.

The same CI run's Unicode editing test passed1.59s. It is not this failure.
Asset-policy test118 started at02:04:25UTC and was interrupted at02:09:21 by the
whole15-minute job deadline. Windows reruns pass all182cases in131.71/136.06s;
no local hang reproduced. Its111 temporary archives total13.0GB. A test-only
MemoryStream fixture experiment passed182cases in141.20s under concurrent
build activity, with no demonstrated benefit, and was removed. No validator,
assertion, capacity, test selection or deadline was weakened or increased.

Exact-commit CI is still required. Preserve the cancelled run as failed evidence.
This host-test fix changes no game rendering or final phone acceptance state.
