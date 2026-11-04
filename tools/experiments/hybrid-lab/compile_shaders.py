"""Compile and validate every native Vulkan lab module, then embed exact bytes."""
import hashlib,json,pathlib,subprocess,os
root=pathlib.Path(__file__).resolve().parent
sdk=pathlib.Path(os.environ.get('VULKAN_SDK',r'C:\VulkanSDK\1.4.350.0'))/'Bin'
out=root/'generated';out.mkdir(exist_ok=True)
rows=[];header=['#pragma once','#include <cstdint>']
for name in ['forward.vert','forward.frag','full.comp','full.rgen','full.rchit','full.rmiss']:
    dest=out/(name+'.spv')
    subprocess.run([str(sdk/'glslangValidator.exe'),'-V','--target-env','vulkan1.2','-I'+str(root/'shaders'),str(root/'shaders'/name),'-o',str(dest)],check=True)
    subprocess.run([str(sdk/'spirv-val.exe'),'--target-env','vulkan1.2',str(dest)],check=True)
    b=dest.read_bytes();sym=name.replace('.','_')
    header.append('inline constexpr uint32_t '+sym+'[]={'+','.join(hex(int.from_bytes(b[i:i+4],'little')) for i in range(0,len(b),4))+'};')
    rows.append({'name':name,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()})
(out/'Shaders.h').write_text('\n'.join(header)+'\n')
(out/'shaders.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(rows,indent=2))
