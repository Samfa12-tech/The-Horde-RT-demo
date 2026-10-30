# Horde 1.7 typed codec and isolated storage

Source parent: `3badb64652e3df0244cc3d4df5fbc0e70a8b3572`. Existing branch
`codex/horde-1.7-wp2-vertical-proof`, existing working directory
`C:\Users\sam_s\Documents\the Horde RT Demo\.worktrees\horde-1.7-wp2-vertical-proof`.
The folder date uses local18 October; receipt timestamps use UTC17 October.
This is an unpushed development checkpoint, separate from the preceding
[logical recovery](../2026-10-17-logical-save/README.md).

The application does not call the adapter or choose an owner-save location.
Every filesystem test exclusively creates a disposable direct child of the
system temp directory. Its cleanup verifies that location and its ownership
marker. No actual owner saves, settings, credentials, device or release data
were read, migrated, replaced or cleared. Existing unrelated work is preserved.

## Closed typed format

`CampaignCheckpointCodec` serializes every logical field explicitly; it never
dumps C++ object memory, padding, pointers or runtime generations. Format1 is
exactly **162 bytes**: a20-byte header and142-byte payload. The maximum admitted
file is4096bytes, including future length checks before allocation/decoding.

| Byte offset | Field |
| --- | --- |
| 0 | Eight-byte `HRDCPKT\0` magic |
| 8 | LE32 envelope version1 |
| 12 | LE32 payload byte count142 |
| 16 | IEEE CRC32 of the payload |
| 20 | Explicit checkpoint schema/content, IDs, pose, health, victory/equipment, torch/guard and dialogue values |

Integer/enum fields are LE32; floats preserve IEEE754 binary32 bits; bools admit
only0/1. Dialogue uses a fixed-width count and all eight bounded continuation
IDs. A literal golden fixture has payload CRC **`0x01703382`**. Decode checks
size, magic, version, exact length, checksum, typed representation and existing
logical cross-field validation into a local candidate before assigning output.
Unknown envelope/schema/content revisions are typed failures with no automatic
migration. Invalid encode/decode leaves the caller's destination unchanged.

## Exact storage contract

`CampaignCheckpointStorage` takes an explicit existing directory and a plain
basename; it creates no directories and has no default application path.
It rejects traversal/device names, final symlink/reparse/nonregular targets and
oversized records. Read never opens a target for writing. Invalid, truncated,
corrupt and unsupported existing records retain their exact original bytes.
Write validates the new checkpoint before filesystem work; it never repairs or
replaces an unreadable/unsupported record automatically. An empty sidecar lock
may be created by an otherwise rejected valid write; it is not a save record.

| Result/boundary | Guarantee |
| --- | --- |
| Create-only (`expected == null`) | Existing target is never clobbered. POSIX uses same-directory `linkat` without overwrite; Windows native rename has no replace flag. |
| Expected update | Requires the validated exact encoded bytes and normalized absolute target returned by Read. A cooperative nonblocking OS lock serializes participating writers; current bytes are rechecked after preparation. Another target's receipt or different newer content rejects. |
| `AlreadyCurrent` | Exact candidate already matches the admitted target. No temporary/write/rename, and no new flush/durability claim. |
| Before publication | A fresh exclusive sibling temporary receives all bytes, including short-write retries, then file flush and close. Open/write/flush/close/read/conflict/injected-publish failure preserves the target. Cleanup attempts only that attempt's owned temporary; failed cleanup can leave an ignored orphan. |
| Published | One same-filesystem name operation exposes complete new bytes. Existing readers retain a complete old file; later opens get the new file. No truncate-in-place, delete-first or copy/delete fallback is used. |
| Windows `Committed / FileFlushed` | Complete file flush and successful Windows10+ `FileRenameInfoEx` name cutover. **No containing-directory flush or power-loss durability is claimed.** Unsupported OS/filesystem rename semantics fail closed. |
| POSIX `Committed / FileAndDirectoryFlushed` | Complete file `fsync`, atomic name publication and successful containing-directory `fsync`. This still relies on the OS/filesystem/device honoring those calls; no physical power-cut result is inferred. |
| `PublishedDurabilityUnknown` | The new complete record is visible, but the postpublication boundary/directory flush failed. Read back before retry or advancing a durable receipt. Do not report old-file preservation. |
| `PublishOutcomeUnknown` | Native failure/readback cannot establish unchanged versus published state. Preserve the uncertainty and read back; never blindly retry or advance a save coordinator. |

Abandoned partial temporaries are ignored by Read and never swept or promoted.
They can remain after process interruption. Later updates use different exclusive
names and leave abandoned files untouched. No retention/cleanup policy is chosen.

Expected identity is **content comparison**, not an inode/history version or a
defense against A→B→A history. It assumes cooperating writers and a caller-owned
directory. Ancestor aliases, hostile/uncoordinated directory mutation, network
filesystem semantics and protection against another process deliberately
rewriting data without the lock are outside this contract. A future asynchronous
coordinator must separately bind session/request/target identities; this adapter
does not advance an active gameplay checkpoint or accept completion callbacks.

The Windows rename semantics follow Microsoft's
[FILE_RENAME_INFO](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_rename_info)
and [POSIX rename flags](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_rename_information).
The POSIX file/directory distinction follows the Linux
[fsync contract](https://man7.org/linux/man-pages/man2/fsync.2.html) and
[rename contract](https://man7.org/linux/man-pages/man2/rename.2.html).
API contracts explain the implementation; they do not certify an untested device.

## Validation and rejected development results

[validation.json](validation.json) binds commands, source and exact artifacts/logs.
Windows tests execute on **Windows10.0.26300.0, NTFS**, in Debug and Release.
The selected matrix is codec, storage, logical checkpoint and chapter dialogue.
Final Debug **4/4**, Release **4/4**; each includes **1,970 codec**, **1,178
storage** and **722 logical checkpoint** checks with zero failures. Both Windows
applications and all four Android Debug ABIs compile successfully.
The storage fixture covers all162 existing-file truncations, all161 nonempty
partial-write boundaries, unknown format/schema/content, malformed expectations,
target isolation, locks, real OS sharing/read-only replacement denial, bounded
short writes, postpublication uncertainty and active readers across30 updates.
Four separately launched owned children exit themselves before/after publication;
fresh reads prove old-target preservation before publication and complete new
visibility afterward. These are process interruptions, **not power cuts**.
If a child times out, the harness stops without further root mutation/cleanup and
reports the pending owned process/root rather than killing an owner process.

The codec additionally tests24 safe-side/health/equipment combinations, ordered
dialogue variants, every truncation, checksum/payload byte flips, strict bools,
finite values, bad enums/counts/queue order, oversize/trailing input and unchanged
destination outputs. The original722 logical recovery checks remain selected.

The first storage run rejected12 active-reader writer attempts; diagnostic retry
rejected6 with `ERROR_ACCESS_DENIED`. Both retained previous bytes and all reader
observations were complete. This exposed `MoveFileEx` open-reader restrictions;
the assertion was preserved and the implementation changed to handle-based
`FileRenameInfoEx` with `REPLACE_IF_EXISTS | POSIX_SEMANTICS`. An intermediate
relative-directory form returned `ERROR_INVALID_PARAMETER(87)`; setup assertions
failed and the early test harness also crashed after using absent fixture data.
The harness now stops safely on setup failure. The documented full-path form
passes the same preservation, real-denial and30-update concurrent-reader checks.
Rejected receipts/log hashes are retained; they are not acceptance runs.

## Platform limits and next review boundary

Android's four Debug ABIs compile the real codec/POSIX adapter. No installation
or device interaction occurs. There is no Linux runtime here: WSL is unavailable,
and none was installed. POSIX runtime/interruption/directory-fsync acceptance is
therefore **NOT RUN**, rather than inferred from Windows or compilation. New
portable/sanitizer CI registrations are present; current-source cloud results
remain pending a future authorized push. Windows could not create the optional
symlink fixture without extra privilege; reparse runtime coverage is explicitly
NOT RUN, while directory/nonregular rejection and source guards are checked.

No platform load/camera adapter, active-slot/session coordinator, menu/autosave
policy, actual save location, automatic migration, forest checkpoint or owner
data overwrite is connected. Three slots remain the direction; manual snapshot
retention remains unresolved. Next review boundary: obtain POSIX runtime evidence
in a disposable location, then add an independently tested asynchronous
session/request receipt coordinator. Do not connect destructive UI overwrite
semantics or count uncertain publication as a durable checkpoint success.

Audio/haptic manual revalidation required: **NO** for this isolated adapter;
it does not alter simulation, events or playback. The preceding logical-load
integration's later audio/camera/physical acceptance requirements remain open.
