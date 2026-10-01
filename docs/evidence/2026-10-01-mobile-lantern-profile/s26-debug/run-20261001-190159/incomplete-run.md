# S26 Debug visual attempt: incomplete, not accepted

Exact SM-S948B / R5GL219SZGK; candidate Debug Mobile APK
`e10b02034e0a0a78828c6ff885d03b79dd2058f77c6f20346a7c977edd794af4`.
The unchanged showcase runner used Replay + selected captures at 75%, with its
default 120-second observation deadline. It exited 1 at Wait-ForLogPattern:
`Timed out waiting for route replay completion.` No captures/lifecycle accepted.

Subsequent scoped log inspection shows generation 1 began at 19:02:51.817 and
reached waypoint 8 (yellow-torch-bay) at 19:04:25.740, with RT presentation and
timing reports continuing through 19:04:32.065. The runner's finally block
force-stops its Debug package; subsequent pidof was empty and launcher resumed.
Thus this was an incomplete observation terminated by harness cleanup, not a
completed replay that can be recovered by waiting, and not crash evidence.

Specific validity reason for one replacement attempt: the default deadline
expired during continued progress and cleanup removed the running process.
Reuse the frozen APK, increase only the existing TimeoutSeconds to 300, preserve
this failure, and do not repeat any completed Shipping timing trial.
