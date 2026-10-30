"""Read APK layout without extracting, modifying, signing or running it.

Usage: python inspect-apk-layout.py prior.apk current.apk
JSON goes to stdout. ZipFile's central-directory start is used only for this
diagnostic; it does not validate signatures or establish installability.
"""
import json
from pathlib import Path
import struct
import sys
import zipfile


def inspect(path):
    with path.open("rb") as stream, zipfile.ZipFile(path) as archive:
        previous_end = payload = base_headers = extras = 0
        records = []
        for entry in sorted(archive.infolist(), key=lambda item: item.header_offset):
            stream.seek(entry.header_offset)
            header = struct.unpack("<IHHHHHIIIHH", stream.read(30))
            if header[0] != 0x04034B50 or header[2] & 8:
                raise ValueError("Unexpected local header or data descriptor")
            name_bytes, extra_bytes = header[-2:]
            end = entry.header_offset + 30 + name_bytes + extra_bytes + entry.compress_size
            records.append({"name": entry.filename, "offset": entry.header_offset,
                            "gapBefore": entry.header_offset - previous_end,
                            "localExtraBytes": extra_bytes, "end": end})
            previous_end = end
            payload += entry.compress_size
            base_headers += 30 + name_bytes
            extras += extra_bytes
        return {"path": str(path), "bytes": path.stat().st_size,
                "entries": len(records), "compressedPayload": payload,
                "localBaseBytes": base_headers, "localExtraBytes": extras,
                "interRecordGaps": sum(record["gapBefore"] for record in records),
                "preCentralGap": archive.start_dir - previous_end,
                "centralAndEndBytes": path.stat().st_size - archive.start_dir,
                "nonzeroGaps": [record for record in records if record["gapBefore"]]}


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit("Pass one or more APK paths")
    print(json.dumps([inspect(Path(argument)) for argument in sys.argv[1:]], indent=2))
