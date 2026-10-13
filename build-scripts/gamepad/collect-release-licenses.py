import concurrent.futures
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

root = Path('/work')
licenses = root / 'licenses'
sources = root / 'release-runtime-sources'
licenses.mkdir(exist_ok=True)
sources.mkdir(exist_ok=True)
packages = {}
libraries = []
for library in sorted((root / 'runtime/lib').iterdir()):
    record = {'file': library.name, 'sha256': hashlib.sha256(library.read_bytes()).hexdigest()}
    if library.name == 'libliblua.so':
        record['source'] = 'src/lua in the corresponding CBN source archive'
        shutil.copy2(root / 'source/src/lua/LICENSE.md', licenses / 'Lua-LICENSE.md')
    elif library.name == 'libSDL3_mixer.so.0':
        source = root / 'source/out/gamepad/_deps/sdl3_mixer-src'
        record['source_revision'] = subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
        record['source'] = 'https://github.com/libsdl-org/SDL_mixer'
        shutil.copy2(source / 'LICENSE.txt', licenses / 'SDL3_mixer-LICENSE.txt')
        subprocess.run(['tar', '--exclude=.git', '-czf', str(sources / 'SDL3_mixer-source.tar.gz'), '-C', str(source), '.'], check=True)
    else:
        # Match bundled bytes to the actual build-container package, not a guessed version.
        candidates = [Path('/usr/lib64') / library.name, Path('/lib64') / library.name]
        system = next((p.resolve() for p in candidates if p.exists() and hashlib.sha256(p.read_bytes()).hexdigest() == record['sha256']), None)
        if system is None:
            raise RuntimeError(f'Cannot identify bundled library: {library.name}')
        package, srpm = subprocess.check_output(['rpm', '-qf', '--qf', '%{NAME} %{SOURCERPM}', str(system)], text=True).split()
        packages[package] = srpm
        record['package'] = package
        record['source_rpm'] = srpm
        for file in subprocess.check_output(['rpm', '-ql', package], text=True).splitlines():
            path = Path(file)
            if file.startswith('/usr/share/licenses/') and path.is_file():
                target = licenses / package / path.relative_to('/usr/share/licenses')
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(path, target)
    libraries.append(record)

def download(srpm):
    name, version, release = srpm.removesuffix('.src.rpm').rsplit('-', 2)
    url = f'https://kojipkgs.fedoraproject.org/packages/{name}/{version}/{release}/src/{srpm}'
    target = sources / srpm
    # Fedora's archive is slow per connection; validate and assemble byte ranges.
    header = subprocess.check_output(['curl', '-fsIL', '--max-time', '30', url], text=True)
    import re
    size = int(re.findall(r'(?im)^content-length: (\d+)', header)[-1])
    if target.exists() and target.stat().st_size == size:
        return {'file': srpm, 'source_url': url, 'sha256': hashlib.sha256(target.read_bytes()).hexdigest()}
    count = 32 if size > 30_000_000 else 8
    def part(index):
        begin = size * index // count
        end = size * (index + 1) // count - 1
        output = sources / f'{srpm}.part{index}'
        subprocess.run(['curl', '-fL', '--retry', '3', '--max-time', '240', '-r', f'{begin}-{end}', '-o', str(output), url], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if output.stat().st_size != end - begin + 1:
            raise RuntimeError(f'Wrong range size: {output}')
        return output
    with concurrent.futures.ThreadPoolExecutor(max_workers=count) as pool:
        parts = list(pool.map(part, range(count)))
    with target.open('wb') as output:
        for piece in parts:
            with piece.open('rb') as source:
                shutil.copyfileobj(source, output)
            piece.unlink()
    subprocess.run(['rpm', '-K', str(target)], check=True, stdout=subprocess.DEVNULL)

    return {'file': srpm, 'source_url': url, 'sha256': hashlib.sha256(target.read_bytes()).hexdigest()}

with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    source_files = list(pool.map(download, sorted(set(packages.values()))))
manifest = {'libraries': libraries, 'source_packages': source_files}
(licenses / 'runtime-provenance.json').write_text(json.dumps(manifest, indent=2) + '\n')
shutil.copy2(licenses / 'runtime-provenance.json', sources / 'runtime-provenance.json')
print(f'Collected {len(libraries)} libraries and {len(source_files)} source RPMs')
