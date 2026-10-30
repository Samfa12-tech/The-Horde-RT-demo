"""Prepare pinned free nature recordings locally. No network or overwrite."""
import argparse
import hashlib
import json
import math
import struct
import subprocess
import wave
from pathlib import Path

SOURCES = [
    dict(key="tawny_owl", source="tawny-owl.ogg", sha256="c3752121e30b9a481d602cc2a959962b713fb19ac6582605b8096bc3fce279fb",
         title="Tawny owl calling at night in Tuntorp, Brastad, Sweden", creator="W.carter",
         page="https://commons.wikimedia.org/wiki/File:Tawny_owl_calling_at_night_in_Tuntorp,_Brastad,_Sweden.ogg",
         license="CC-BY-4.0", licenseUrl="https://creativecommons.org/licenses/by/4.0/",
         metadata="Strix aluco, night calls 23:00-02:00; recorded 2026-05-04 00:47. Author cut two locations and reduced background noise.",
         start=0, seconds=8, gainDb=-12, admitted=True),
    dict(key="evening_cricket", source="evening-cricket.wav", sha256="8beca03203ddc3c4c9d341167eaef4f9af568ccb15712da2972069d4c6289baa",
         title="Oecanthus pellucens song 23C", creator="Gunther Tschuch",
         page="https://commons.wikimedia.org/wiki/File:Oecanthus_pellucens_song_23C.wav",
         license="CC-BY-SA-4.0", licenseUrl="https://creativecommons.org/licenses/by-sa/4.0/",
         metadata="Male Italian tree cricket, Kamenz, Germany, 2021-08-14 21:04, 23 C. Darkness not independently established.",
         start=2, seconds=8, gainDb=-18, admitted=True),
    dict(key="pond_frogs_crickets", source="pond-dusk.ogg", sha256="ebc99f767a1832608ef5a8e2fd170a19c01720a8ba957fd7521f291bba57b21f",
         title="Nature sounds ambience in a Dordogne pond", creator="Glaneur de sons",
         page="https://commons.wikimedia.org/wiki/File:Nature_sounds_ambience_in_a_Dordogne_pond.ogg",
         license="CC-BY-3.0", licenseUrl="https://creativecommons.org/licenses/by/3.0/",
         metadata="Mixed frogs/crickets at dusk, Dordogne, 2007-05-02. Exact species unspecified.",
         runtimePolicy="Finite cue only within 18m Euclidean distance of actual ChapterRegionalWater faces, in ready same-night grounded exterior without dialogue/pause; mix/listening pending.",
         start=2, seconds=8, gainDb=-12, admitted=True),
    dict(key="periodical_cicada", source="periodical-cicada.oga", sha256="3a0cf77a2f6034e602d61da75b180838cd1a68c998c5a4fe026479312012262a",
         title="Calling song of Magicicada septendecim - Audio S1, pone.0000892", creator="Kathryn M. Fontaine, John R. Cooley, Chris Simon",
         page="https://commons.wikimedia.org/wiki/File:Calling_song_of_Magicicada_septendecim_-_pone.0000892.s001.oga",
         license="CC-BY-2.5", licenseUrl="https://creativecommons.org/licenses/by/2.5/",
         metadata="PLOS ONE 2007 Audio S1, doi:10.1371/journal.pone.0000892. North American species; no time/night metadata. Not suitable as verified nocturnal European ambience.",
         start=0, seconds=5.8, gainDb=-15, admitted=False),
]

def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def prepare(source_directory, output_directory):
    if not output_directory.is_dir() or any(output_directory.iterdir()):
        raise SystemExit("Provide an existing empty output directory; no files overwritten")
    for entry in SOURCES:
        if digest(source_directory / entry['source']) != entry['sha256']:
            raise SystemExit("Unrecognized source bytes: " + entry['source'])
    records = []
    for entry in SOURCES:
        relative = ('runtime' if entry['admitted'] else 'candidates') + '/' + entry['key'] + '.wav'
        target = output_directory / relative
        target.parent.mkdir(exist_ok=True)
        fade_start = entry['seconds'] - .15
        filters = (f"aformat=channel_layouts=mono,aresample=24000,"
                   f"atrim=start={entry['start']}:duration={entry['seconds']},asetpts=PTS-STARTPTS,"
                   f"volume={entry['gainDb']}dB,afade=t=in:d=0.15,afade=t=out:st={fade_start}:d=0.15")
        command = ['ffmpeg','-v','error','-nostdin','-n','-i',str(source_directory / entry['source']),
                   '-map_metadata','-1',
                   '-ac','1','-ar','24000','-af',filters,'-c:a','pcm_s16le',str(target)]
        subprocess.run(command, check=True)
        with wave.open(str(target),'rb') as wav:
            frames=wav.getnframes(); samples=struct.unpack('<'+'h'*frames,wav.readframes(frames))
        peak=max(abs(s) for s in samples)/32768
        rms=math.sqrt(sum(s*s for s in samples)/frames)/32768
        record = dict(entry, file=relative, bytes=target.stat().st_size, runtimeSha256=digest(target),
                      channels=1, sampleRate=24000, bitsPerSample=16, frames=frames,
                      durationSeconds=frames/24000, peakDbfs=20*math.log10(peak), rmsDbfs=20*math.log10(rms),
                      changes=f"Excerpt from {entry['start']}s for {entry['seconds']}s; mono downmix, 24kHz resample, PCM16, {entry['gainDb']}dB gain, 150ms fades; no loop/pitch/noise filtering",
                      acceptance="Unheard candidate; source metadata only. Native mix/listening acceptance pending.")
        record['sourceSha256']=record.pop('sha256')
        records.append(record)
    (output_directory/'manifest.json').write_text(json.dumps(dict(schema=1, assets=records),indent=2)+'\n',encoding='utf-8')
    notice=['# Nature audio attribution', '', 'All four recordings remain unheard here; only runtime/ is packaged.',
            'The pond cut is admitted only beside actual shared regional water surfaces;',
            'the periodical cicada remains excluded. Native mix/listening acceptance is pending.',
            'Cricket derivative remains CC BY-SA 4.0. No endorsement is implied.', '']
    for record in records:
        notice.extend([record['title']+' — '+record['creator'],record['page'],record['license']+' '+record['licenseUrl'],
                       record['changes'],record['metadata'],''])
    (output_directory/'NOTICE.md').write_text('\n'.join(notice),encoding='utf-8')
    print(json.dumps([dict(file=r['file'],bytes=r['bytes'],peakDbfs=r['peakDbfs'],rmsDbfs=r['rmsDbfs']) for r in records]))

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--source-directory',type=Path,required=True)
    parser.add_argument('--output-directory',type=Path,required=True)
    args=parser.parse_args()
    prepare(args.source_directory,args.output_directory)
