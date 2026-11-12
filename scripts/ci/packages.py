"""Assemble the same six assets as the existing v1.0.3 release."""
from pathlib import Path
import argparse
import hashlib
from version import read_version, version_code

PACKAGES = (
    'RoomBreak-Android-arm64-v8a.apk',
    'RoomBreak-iOS-arm64-unsigned.ipa',
    'RoomBreak-macOS-arm64.pkg',
    'RoomBreak-Windows-amd64.exe',
)
ASSETS = (*PACKAGES, 'INSTALL.txt', 'SHA256SUMS.txt')


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def assemble(directory, version):
    if {p.name for p in directory.iterdir()} != set(PACKAGES):
        raise ValueError('Expected exactly the four platform installers')
    if any((directory / name).stat().st_size == 0 for name in PACKAGES):
        raise ValueError('An installer is empty')
    (directory / 'SHA256SUMS.txt').write_text(''.join(
        f'{digest(directory / name)}  {name}\n' for name in PACKAGES), encoding='utf-8')
    (directory / 'INSTALL.txt').write_text(f'''RoomBreak Escape Now v{version}

自适应视野与等比例缩放；桌面 F11 全屏，Esc 返回。

Windows amd64：运行 RoomBreak-Windows-amd64.exe，包含 VC++ 运行库安装程序。
macOS Apple Silicon：打开 RoomBreak-macOS-arm64.pkg，安装到 Applications。应用采用 ad hoc 签名，安装包未公证。
Android arm64：侧载 RoomBreak-Android-arm64-v8a.apk。采用 Android Debug 签名，versionCode 为 {version_code(version)}。CI 已更换固定签名密钥，无法直接覆盖旧版 v1.0.3；请先备份存档再卸载旧版（卸载通常会删除应用数据）。后续 CI APK 沿用同一新密钥。
iOS arm64：RoomBreak-iOS-arm64-unsigned.ipa 需要自行签名后安装。

SHA256SUMS.txt 提供四个平台安装包的 SHA-256 校验值。
''', encoding='utf-8')
    verify(directory)


def verify(directory):
    if {p.name for p in directory.iterdir()} != set(ASSETS):
        raise ValueError('Expected exactly six release assets')
    lines = (directory / 'SHA256SUMS.txt').read_text().splitlines()
    expected = [f'{digest(directory / name)}  {name}' for name in PACKAGES]
    if lines != expected:
        raise ValueError('Package checksums do not match')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=Path)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        verify(args.directory)
    else:
        assemble(args.directory, read_version())
