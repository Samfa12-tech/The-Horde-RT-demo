import sys,subprocess
exe='build/presets/windows-x64-debug/Debug/horde_rt_sword_target_contact_region_witness.exe'
for label,args in [('final-real-mesh-contact-witness',[]),('final-thin-joint-diagnostic',['--thin-joint-active-window']),
 ('final-contact-callback',['--contact-callback']),('final-contact-proxy-invalid',['--contact-proxy-invalid-inputs'])]:
 code=subprocess.call([sys.executable,'reports/wp2a-combat/run.py',label,exe,*args])
 if code:sys.exit(code)
