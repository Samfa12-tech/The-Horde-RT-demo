"""Check an actual run's evidence coherence, without certifying speed or visuals."""
import csv,json,pathlib,sys,math
p=pathlib.Path(sys.argv[1]);r=json.loads((p/'results.json').read_text())
rows=list(csv.DictReader((p/'frames.csv').open()))
assert r['validationErrors']==0,r
assert len(r['buildIdentity'])==64
assert (p/'build-identity.txt').read_text().strip()==r['buildIdentity']
assert r['blasBuildGpuMs']>0 and r['tlasBuildGpuMs']>0
for s,scene in enumerate(r['scenes']):
    sample=[]
    for block in range(8):
        mode='hybrid' if block%4 in (1,2) else 'full-rt'
        b=[x for x in rows if int(x['scene'])==s and int(x['block'])==block]
        assert len(b)>=4
        assert {x['mode'] for x in b}=={mode}
        assert [int(x['frame']) for x in b]==list(range(len(b)))
        sample.append([x['phase'] for x in b])
        for x in b:
            for key in ['gpu_ms','cpu_cycle_ms','cpu_record_ms','cpu_submit_wait_ms']:
                assert math.isfinite(float(x[key])) and float(x[key])>0
            assert abs(float(x['cpu_cycle_ms'])-float(x['cpu_record_ms'])-float(x['cpu_submit_wait_ms']))<1e-6
    assert all(x==sample[0] for x in sample)
    for m in scene['modes']:
        subset=[float(x['gpu_ms']) for x in rows if int(x['scene'])==s and x['mode']==m['mode']]
        assert len(subset)==m['samples']
        subset.sort()
        assert abs(subset[math.ceil(.5*(len(subset)-1))]-m['gpuMedianMs'])<1e-7
        assert abs(subset[math.ceil(.95*(len(subset)-1))]-m['gpuP95Ms'])<1e-7
    for mode in ['rt','hybrid','diff']:
        bmp=(p/f'scene{s}-{mode}.bmp').read_bytes()
        assert bmp[:2]==b'BM'
        assert int.from_bytes(bmp[18:22],'little')==r['width']
        assert int.from_bytes(bmp[22:26],'little')==r['height']
print(f"PASS: {len(rows)} paired frame records, 3 scenes, 9 captures, build identity and timings coherent; visual/phone/gameplay acceptance remain separate.")
