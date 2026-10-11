#!/usr/bin/env python3
"""Run in builder container. Bundle the dependency closure without replacing glibc."""
import os,pathlib,re,shutil,subprocess
root=pathlib.Path('/work');lib=root/'runtime/lib';lib.mkdir(parents=True,exist_ok=True)
base={'libc.so.6','libm.so.6','libpthread.so.0','libdl.so.2','librt.so.1','libresolv.so.2','libutil.so.1','ld-linux-x86-64.so.2'}
for name in ['libSDL3.so.0','libSDL3_image.so.0','libSDL3_ttf.so.0','libstdc++.so.6','libgcc_s.so.1','libsqlite3.so.0']:
 p=subprocess.check_output(['bash','-lc',f'ldconfig -p | awk \'$1 == "{name}" && /x86-64/ {{print $NF;exit}}\''],text=True).strip()
 if not p:raise RuntimeError(name)
 shutil.copyfile(p,lib/name)
for _ in range(10):
 added=[]
 for target in [root/'app/cataclysm-bn-tiles',*lib.glob('*.so*')]:
  result=subprocess.run(['ldd',str(target)],text=True,capture_output=True,env=dict(os.environ,LD_LIBRARY_PATH=str(lib)))
  if 'not found' in result.stdout+result.stderr:raise RuntimeError(result.stdout+result.stderr)
  for name,path in re.findall(r'\s*(\S+) => (/\S+) ',result.stdout):
   if name not in base and not (lib/name).exists():
    shutil.copyfile(path,lib/name);added.append(name)
 if not added:break
 print('Bundled:',', '.join(added))
else:raise RuntimeError('Dependencies did not converge')
print('Runtime libraries:',len(list(lib.glob('*.so*'))))
