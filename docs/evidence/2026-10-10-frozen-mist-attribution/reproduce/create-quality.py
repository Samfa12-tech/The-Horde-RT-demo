from pathlib import Path
import shutil
root=Path.cwd()
original=root/'reports/frozen-mist'
out=original/'quality'
out.mkdir(exist_ok=True)
assert not (out/'prepare.py').exists(), 'Retain prior experiment; choose a fresh directory'
shutil.copytree(original/'shaders',out/'shaders')
shutil.copytree(original/'baseline',out/'baseline')
shutil.copy2(original/'shader-metrics.json',out/'shader-metrics.json')
s=(original/'prepare.py').read_text()
s=s.replace("out=root/'reports/frozen-mist';", "out=root/'reports/frozen-mist/quality';",1)
s=s.replace('context.shadowQuality = horde::graphics::ShadowQuality::Higher;',
 'context.shadowQuality = std::getenv(\"HORDE_MIST_PROBE_SHADOW\") != nullptr && std::string_view(std::getenv(\"HORDE_MIST_PROBE_SHADOW\")) == \"CURRENT\" ? horde::graphics::ShadowQuality::Current : horde::graphics::ShadowQuality::Higher;',1)
# stdlib includes belong outside the anonymous namespace.
s=s.replace("window='#include", "window='#include <cstdlib>\\n#include",1)
(out/'prepare.py').write_text(s,encoding='utf-8',newline='\n')
s=(original/'capture.py').read_text().replace("out=root/'reports/frozen-mist';", "out=root/'reports/frozen-mist/quality';",1)
s=s.replace("i=int(sys.argv[1]);name=rows[i]['name'];target=out/'captures'/name", "i=int(sys.argv[1]);shadow=sys.argv[2];assert shadow in ['CURRENT','HIGHER'];name=shadow.lower()+'-'+rows[i]['name'];target=out/'captures'/name",1)
s=s.replace("'HORDE_MIST_PROBE_MODE':str(i)}", "'HORDE_MIST_PROBE_MODE':str(i),'HORDE_MIST_PROBE_SHADOW':shadow}",1)
s=s.replace("mode=i,elapsed_seconds", "mode=i,shadow=shadow,cwd='<repo>',environment={'HORDE_MIST_PROBE_MODE':str(i),'HORDE_MIST_PROBE_SHADOW':shadow},elapsed_seconds",1)
(out/'capture.py').write_text(s,encoding='utf-8',newline='\n')
print('Prepared one quality-comparison executable with explicit CURRENT/HIGHER selection and identical six shader modules.')
