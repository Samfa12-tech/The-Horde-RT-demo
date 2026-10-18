# Real bounded campaign IO worker

Parent `0d6c22d698604d527b85fddb33f6db1fadedf2ed`, existing PR27 worktree.
This runnable slice connects immutable coordinator requests to actual storage in
exclusively created disposable host directories. No application save location,
menu or phone save/load adapter is connected.

Confirmed policy is three independent slots, each with one manual file and one
latest autosave. The six closed logical identities and basenames never alias;
autosaves do not replace manual targets and no indefinite history is created.

Storage now splits into Prepare, Publish and Finish. Preparation encodes, locks,
writes/flushes/closes and rechecks expected content; Windows rename handles and
name buffers are opened/allocated before publication. Move-only prepared work
owns its directory/lock/temp until completion or cancellation. Publish performs
one native name operation only. Cleanup, readback, fault callbacks and POSIX
directory fsync run off the coordinator gate in Finish. Synchronous Write wraps
these stages and retains its prior typed results.

The owned worker has one active request plus one queued request. Full/stopped
queues reject submission explicitly. Read receipts, prepublication failures and
postpublication results return through weak single-use tickets; only owner Poll
changes coordinator state. Revocation before the gate prevents publication;
revocation after cutover drops its late receipt while worker cleanup finishes.
The owner closes coordinators before shutdown joins the owned thread. OS IO and
thread joining have no promised wall-clock deadline; no thread is detached or
killed. The publication gate contains one OS operation, not a real-time latency
certificate.

The worker fixture exercises all six actual files, preflush cancellation,
postpublication cancellation while Finish is blocked, a bounded queue and
shutdown. The retained storage suite adds prepared ownership/move/cancel,
held-lock lifetime, one-use publication and deferred fault callbacks. Existing
codec and coordinator assertions remain in the selection.

An initial new storage assertion used a generic stream while the retained
Windows rename handle was still open; its sharing mode could not read the file.
The failure is preserved. The corrected fixture uses the application's
sharing-compatible storage Read and still requires exact visible bytes with no
publication-gate callbacks. No assertion was skipped or weakened.

See validation.json for exact source/log hashes and actual results. This does not
establish Android/POSIX filesystem runtime, power-loss durability, RTX frames,
phone lifecycle or gameplay UI acceptance. Windows retains FileFlushed only.
The separately recorded phone cold-checkpoint evidence belongs to sealed
0d6c22d6 and does not test this worker.
