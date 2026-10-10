"""Verify the approved private cut package and admit only closed runtime WAVs.

No master, provider manifest, account metadata, generation or normalization is
copied. Existing differing destinations are rejected before any output write.
"""
import argparse
import array
import hashlib
import io
import json
import math
from pathlib import Path
import re
import stat
import sys
import wave

ROOT = Path(__file__).resolve().parents[1]
PINNED_CUTS = {
    "prologue.kit_grate": "2af6b1b650afb8395ced3ba2ec95beea7875a214fed26fcb5f967fcf9c1038a8",
    "rescue.found": "4e01035e42315c13012b358ba3db3438ea6b88a030c9fafaa56fc6aa78e55f9c",
    "rescue.rope": "7713bf78793f580d2a418126fd521b01ebcb309bedc3cd8b10c13b91f112a0b6",
    "reunion.question": "061f23a012cd37d8cc8158ba19df85f9fa16edff479c53aaec1fd677b400a927",
    "reunion.hint": "c5c9a012a562e900697bfdca5cd4b7dda7cb579b08d25db10bfaca5ea6fd7191",
    "reunion.proof": "f9c2db3ce4fc592d994e73ea1f82761c807b4e178012d413652cbcb70c0394b2",
    "reunion.first_piece": "8456da25e25588900c87ddfc3bba962eba6b232410e54cf284fe27b4515366d9",
    "reunion.depart": "d94158155041ab556a73d15241480b41b4305aa3c2933e8c5eced47fc980f8b4",
    "forest.night": "b063e4daa2497bb3f94d1eea388028309c835086433c8c8cfa33edd8596d84bc",
    "forest.waystone": "c00c2d6956ef6fd6c93c71092d01f2c03b125644c211c7a700a8909a40ad795a",
    "forest.clue": "cc783ba0141f6b47f67d77e197e2b60a38c69c50b79ac5b2db0d00657c47970b",
    "forest.wait": "8fb4e10dc2a596eba85a4c9ec07e111e42a336f018e824f6e942839883bdf950",
    "forest.village": "33b6b9d34dea2f6eda32ac44ceeddacfd51e3ab98dbdddafde3b65cee57d047d",
}


def linked(path):
    return path.is_symlink() or (path.exists() and
        bool(getattr(path.lstat(), "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT))


def verify_source(package):
    package = Path(package).resolve(strict=True)
    source = json.loads((package / "manifest.json").read_text(encoding="utf-8-sig"))
    clips = source["clips"]
    if [cut["id"] for cut in clips] != list(PINNED_CUTS):
        raise ValueError("source must contain exactly the ordered thirteen approved cuts")
    catalog = (ROOT / "src/gameplay/dialogue/ChapterDialogue.h").read_text()
    text_by_id = dict(re.findall(r'\{Line::\w+,"([^"]+)","Kit","([^"]+)"', catalog))
    outputs, cuts = {}, []
    for cut in clips:
        stable_id = cut["id"]
        relative = Path(cut["file"])
        path = package / relative
        if relative.is_absolute() or not path.resolve(strict=True).is_relative_to(package):
            raise ValueError("source cut path leaves its selected package")
        if any(linked(p) for p in [path, *path.parents] if p != package and p.is_relative_to(package)):
            raise ValueError("source cut has a linked/reparse ancestor")
        payload = path.read_bytes()
        sha = hashlib.sha256(payload).hexdigest()
        if sha != PINNED_CUTS[stable_id] or sha != cut["sha256"]:
            raise ValueError("approved source-cut hash mismatch: " + stable_id)
        if cut["text"] != text_by_id.get(stable_id):
            raise ValueError("spoken/subtitle text mismatch: " + stable_id)
        with wave.open(io.BytesIO(payload), "rb") as clip:
            if (clip.getnchannels(), clip.getsampwidth(), clip.getframerate(), clip.getcomptype()) != (1, 2, 24000, "NONE"):
                raise ValueError("approved cut must remain mono PCM16 24 kHz")
            frames = clip.getnframes()
            pcm = clip.readframes(frames)
        if not 0 < frames <= 156000 or len(payload) != 44 + len(pcm):
            raise ValueError("cut capacity or clean RIFF format/data layout mismatch")
        samples = array.array("h", pcm)
        if sys.byteorder != "little":
            samples.byteswap()
        filename = stable_id + ".wav"
        outputs[filename] = payload
        cuts.append({"stableId": stable_id, "text": cut["text"], "runtimeFile": filename,
                     "sourceCutSha256": sha, "sha256": sha, "bytes": len(payload),
                     "frames": frames, "sampleRate": 24000, "channels": 1, "bitsPerSample": 16,
                     "durationSeconds": frames / 24000,
                     "peak": max(abs(x) for x in samples) / 32768,
                     "rms": math.sqrt(sum(float(x) * x for x in samples) / len(samples)) / 32768})
    manifest = {"schema": 1, "package": "horde-kit-en-approved-cuts-v1", "provider": "Mureka",
                "language": "en", "ownerPerformanceAndCutsApproved": True, "mastersRemainPrivate": True,
                "rights": {"license": "LicenseRef-Mureka-Owner-Confirmed-Game-Repository",
                           "publicGameRepositoryDistributionConfirmed": True,
                           "evidenceClass": "owner-explicit-confirmation", "confirmedOn": "2026-10-11",
                           "statement": "I confirm public game/repository distribution rights",
                           "scope": ["public Horde game runtime packages", "public Horde repository"],
                           "notCovered": "No grant for unrelated assets or a blanket MIT relicensing."},
                "processing": "Byte-exact approved PCM cuts; no normalization, resampling, mask or grate filter.",
                "cuts": cuts}
    outputs["asset.manifest.json"] = (json.dumps(manifest, indent=2, ensure_ascii=False) + "\n").encode("utf-8")
    return outputs


def admit(outputs, destination):
    destination = Path(destination)
    if destination.exists() and linked(destination):
        raise ValueError("destination cannot be a linked/reparse directory")
    for parent in destination.parents:
        if linked(parent):
            raise ValueError("destination ancestor cannot be a linked/reparse directory")
    if destination.exists() and {p.name for p in destination.iterdir()} - outputs.keys():
        raise ValueError("destination contains entries outside the closed runtime roster")
    for name, payload in outputs.items():
        path = destination / name
        if path.exists() and (linked(path) or not path.is_file() or path.read_bytes() != payload):
            raise ValueError("refusing to overwrite a different destination: " + name)
    destination.mkdir(parents=True, exist_ok=True)
    for name, payload in outputs.items():
        path = destination / name
        if not path.exists():
            with path.open("xb") as stream:
                stream.write(payload)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-package", type=Path, required=True)
    parser.add_argument("--destination", type=Path, default=ROOT / "assets/audio/kit/runtime")
    parser.add_argument("--rights-confirmed", action="store_true", required=True)
    args = parser.parse_args()
    outputs = verify_source(args.source_package)
    admit(outputs, args.destination)
    print(json.dumps({"cuts": 13, "bytes": sum(len(v) for k, v in outputs.items() if k.endswith('.wav')),
                      "sampleRate": 24000, "pcmUnchanged": True, "publicRights": "owner-confirmed"}))
