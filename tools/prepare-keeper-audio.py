"""Prepare the owner's licensed local cues without copying MP3s into Git.

Only these runtime derivatives, their integration and provenance belong to the
game. Do not distribute the originals or a standalone sound library.
"""
import argparse
import array
import hashlib
import json
import math
import subprocess
import wave
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--inventory", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
inventory = json.loads(args.inventory.read_text(encoding="utf-8-sig"))
mapping = {
    "skeleton_falling_bones": ("skeleton_falling_bones.wav", "existing delayed EnemyDefeated fall; replaces enemy_fall, never stacks", None),
    "skeleton_rattling_bones": ("skeleton_idle_rattle.wav", "sparse SkeletonIncidental; authoritative eligibility and cooldown", 1.25),
    "lich_i_sense_you": ("keeper_i_sense_you.wav", "KeeperRevealStarted; first entry only", None),
    "lich_come_closer": ("keeper_come_closer.wav", "KeeperWarning; first reveal only", None),
}
rate = 48000
args.output.mkdir(parents=True, exist_ok=True)
records = []
for record in inventory["assets"]:
    original = args.inventory.parent / record["original_file"]
    source = original.read_bytes()
    expected = record["media"]["sha256"].lower()
    if hashlib.sha256(source).hexdigest() != expected:
        raise ValueError("Owner source hash mismatch: " + record["id"])
    command = ["ffmpeg", "-v", "error", "-i", str(original), "-ac", "1", "-ar", str(rate), "-f", "s16le", "pipe:1"]
    pcm = array.array("h", subprocess.check_output(command))
    # Remove only edge windows below -60dBFS, keeping25ms around audible content.
    window = rate // 100
    rms = [math.sqrt(sum(float(v)*v for v in pcm[i:i+window]) / max(1,len(pcm[i:i+window]))) for i in range(0,len(pcm),window)]
    audible = [i for i,v in enumerate(rms) if v >= 32767*0.001]
    if not audible:
        raise ValueError("Cue has no audible content: " + record["id"])
    begin = max(0, audible[0]*window - rate//40)
    end = min(len(pcm), (audible[-1]+1)*window + rate//40)
    filename, trigger, maximum = mapping[record["id"]]
    if maximum:
        end = min(end, begin + int(maximum*rate))
    selected = pcm[begin:end]
    # Short endpoint ramps eliminate hard PCM truncation; no loudness retune.
    ramp_in, ramp_out = rate//500, rate//100
    for i in range(min(ramp_in,len(selected))):
        selected[i] = round(selected[i] * i / ramp_in)
    for i in range(min(ramp_out,len(selected))):
        selected[-1-i] = round(selected[-1-i] * i / ramp_out)
    path = args.output / filename
    with wave.open(str(path),"wb") as output:
        output.setnchannels(1); output.setsampwidth(2); output.setframerate(rate)
        output.writeframes(selected.tobytes())
    records.append({"id":record["id"],"sourceUrl":record["source_url"],"artist":record["artist"],
                    "sourceSha256":expected,"sourceBytes":len(source),"runtimePath":filename,
                    "sha256":hashlib.sha256(path.read_bytes()).hexdigest(),"bytes":path.stat().st_size,
                    "sampleRate":rate,"channels":1,"bitsPerSample":16,"frames":len(selected),
                    "durationSeconds":len(selected)/rate,"peakDbfs":20*math.log10(max(1,max(abs(v) for v in selected))/32767),
                    "decodedTrimFrames":[begin,end],"endpointRampsFrames":[ramp_in,ramp_out],
                    "gainChangeDb":0,"trigger":trigger,"listeningAcceptance":"pending owner exact-candidate audition"})
manifest={"schema":1,"target":"1.6.2","license":"Pixabay Content License; no CC0 claim",
          "licenseSummary":"https://pixabay.com/service/license-summary/",
          "licenseTerms":"https://pixabay.com/service/terms/","verifiedAt":"2026-10-03",
          "sourcePolicy":"Owner originals retained locally and excluded from public Git and packages. Runtime cues integrated in game only; no standalone sound distribution.",
          "processing":"FFmpeg9 decode to monoPCM16/48k; edge<-60dBFS windows trimmed with25ms reserve;2ms/10ms endpoint ramps; idle excerpt max1.25s; no gain normalization.",
          "assets":records}
(args.output/"keeper-asset.manifest.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
print(json.dumps(records,indent=2))
