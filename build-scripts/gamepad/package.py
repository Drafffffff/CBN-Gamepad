#!/usr/bin/env python3
"""Package an installed, verified CBN runtime without player data or backups."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--runtime-root', type=Path, required=True)
parser.add_argument('--source-commit', required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--version', default='0.1.0')
args = parser.parse_args()
runtime = args.runtime_root.resolve()
args.output.mkdir(parents=True, exist_ok=True)
name = f'cbn-gamepad-{args.version}-linux-x86_64'
with tempfile.TemporaryDirectory(prefix='cbn-release-') as temporary:
    root = Path(temporary) / 'cbn-gamepad-stable'
    root.mkdir()
    for directory in ['data', 'gfx', 'lang']:
        shutil.copytree(runtime / 'app' / directory, root / 'app' / directory,
                        ignore=shutil.ignore_patterns('.git', '__pycache__'))
    shutil.copy2(runtime / 'app/cataclysm-bn-tiles', root / 'app/cataclysm-bn-tiles')
    for directory in ['runtime', 'presets', 'licenses']:
        shutil.copytree(runtime / directory, root / directory)
    source = runtime / 'source'
    for script in ['launch.sh', 'install-device.sh', 'README.zh-CN.md']:
        shutil.copy2(source / 'build-scripts/gamepad' / script, root / script)
    for document in ['LICENSE.txt', 'LICENSE-OFL-Terminus-Font.txt']:
        shutil.copy2(source / document, root / document)
    manifest = {
        'version': args.version, 'upstream_release': 'v0.13.0',
        'source_commit': args.source_commit,
        'compiled_cpp_commit': '5425140a26749f7ab68ded5734806df81eacc85b',
        'binary_sha256': hashlib.sha256((root / 'app/cataclysm-bn-tiles').read_bytes()).hexdigest(),
        'controller_tests': {'cases': 10, 'assertions': 2231},
        'tested_on': ['Bazzite, Steam, 3840x2160'],
        'small_screen_preset': 'Included; not physically retested on Steam Deck for this release',
        'requirements': 'Linux x86_64, glibc >= 2.38 and compatible host system libraries',
        'notes': 'Packaging/docs changes after compiled_cpp_commit do not change the tested game binary.',
    }
    (root / 'release-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    for script in ['launch.sh', 'install-device.sh']:
        (root / script).chmod(0o755)
    with tarfile.open(args.output / f'{name}.tar.gz', 'w:gz') as archive:
        archive.add(root, arcname=root.name)
print(args.output / f'{name}.tar.gz')
