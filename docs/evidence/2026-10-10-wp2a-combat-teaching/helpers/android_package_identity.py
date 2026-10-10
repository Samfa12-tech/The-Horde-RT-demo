import os,json,zipfile,hashlib,subprocess,re
from pathlib import Path
root=Path.cwd(); out=root/'reports/wp2a-combat'
sdk_text=os.environ.get('ANDROID_HOME') or os.environ.get('ANDROID_SDK_ROOT')
if not sdk_text:
 sdk_text=next(l.split('=',1)[1] for l in (root/'android/local.properties').read_text().splitlines() if l.startswith('sdk.dir='))
 sdk_text=sdk_text.replace('\\:',':').replace('\\\\','\\')
sdk=Path(sdk_text)
readelf=sdk/'ndk/26.1.10909125/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-readelf.exe'
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
expected={'arm64-v8a':('ELF64','AArch64'),'armeabi-v7a':('ELF32','ARM'),
 'x86':('ELF32','Intel 80386'),'x86_64':('ELF64','Advanced Micro Devices X86-64')}
packages=[]
for name,apk,strip in [('debug','debug/app-debug.apk','debug/stripDebugDebugSymbols'),
 ('release-unsigned','release/app-release-unsigned.apk','release/stripReleaseDebugSymbols')]:
 path=root/'android/app/build/outputs/apk'/apk; libraries=[]
 with zipfile.ZipFile(path) as z:
  for abi,(klass,machine) in expected.items():
   entry=f'lib/{abi}/libhorde_rt_probe_android.so'
   assert z.namelist().count(entry)==1
   lib=root/f'android/app/build/intermediates/stripped_native_libs/{strip}/out/lib/{abi}/libhorde_rt_probe_android.so'
   packaged=hashlib.sha256(z.read(entry)).hexdigest(); local=sha(lib); assert packaged==local
   header=subprocess.check_output([str(readelf),'-h',str(lib)],text=True)
   program=subprocess.check_output([str(readelf),'-lW',str(lib)],text=True)
   get=lambda label:re.search(r'^\s*'+label+r':\s*(.*?)\s*$',header,re.M).group(1)
   assert get('Class')==klass and get('Machine')==machine and 'little endian' in get('Data')
   loads=re.findall(r'^\s*LOAD\s+.*?\s+(0x[0-9a-fA-F]+)\s*$',program,re.M)
   assert loads and all(x.lower()=='0x4000' for x in loads)
   libraries.append(dict(abi=abi,entry=entry,bytes=len(z.read(entry)),sha256=local,
    packaged_matches_stripped=True,elf_class=klass,machine=machine,load_alignments=loads))
 packages.append(dict(configuration=name,path=path.relative_to(root).as_posix(),sha256=sha(path),
  bytes=path.stat().st_size,libraries=libraries))
result=dict(source_commit=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),
 packages=packages,readelf_sha256=sha(readelf),
 scope='All four ABIs: exact APK-entry/stripped-library identity, ELF class/machine and 16KiB LOAD alignment. This does not inspect all-ABI shader payloads; supported shader inspection is separately ARM64-only.')
(out/'android-all-abi-identities.json').write_text(json.dumps(result,indent=2)+'\n')
print('Exact package-to-library identity and ELF/LOAD checks passed for all eight Debug/unsigned-Release libraries.')
