"""Install Axmol's pinned shader compiler on a clean runner."""
import hashlib
import os
from pathlib import Path
import platform
import tarfile
import urllib.request
import zipfile

PACKAGES = {
    'Linux': ('linux.tar.gz', 'c8a9113f942f3025aaa5dfd46440b3256915a5b5a02fade6b2884bb4be69d7a2'),
    'Darwin': ('osx-arm64.tar.gz', '676133d2770433462391a0f982a82f078bf2fb99d62a5032e1aa592676a76336'),
    'Windows': ('win64.zip', '5ea1ae25517a14a81b578836563837fddcb3feca005581761446f3e75986e828'),
}
suffix, expected = PACKAGES[platform.system()]
name = f'axslcc-1.14.0-{suffix}'
directory = Path('external/axslcc').resolve()
directory.mkdir(parents=True, exist_ok=True)
archive = directory / name
urllib.request.urlretrieve(f'https://github.com/axmolengine/axslcc/releases/download/v1.14.0/{name}', archive)
if hashlib.sha256(archive.read_bytes()).hexdigest() != expected:
    raise SystemExit('Shader compiler checksum mismatch')
if suffix.endswith('.zip'):
    with zipfile.ZipFile(archive) as package:
        package.extractall(directory)
else:
    with tarfile.open(archive) as package:
        package.extractall(directory, filter='data')
executable = 'axslcc.exe' if platform.system() == 'Windows' else 'axslcc'
matches = [p for p in directory.rglob(executable) if p.is_file()]
if len(matches) != 1:
    raise SystemExit(f'Expected one shader compiler, found {matches}')
with open(os.environ['GITHUB_PATH'], 'a') as paths:
    paths.write(str(matches[0].parent) + '\n')
print(f'Installed verified {name}')
