"""Read VERSION and stamp native package metadata before building."""
import argparse
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]


def parse_version(value):
    if not re.fullmatch(r"[1-9][0-9]{0,2}\.(0|[1-9][0-9]{0,2})\.(0|[1-9][0-9]{0,2})", value):
        raise ValueError(f"Expected MAJOR.MINOR.PATCH (major 1..999, minor/patch 0..999): {value!r}")
    return tuple(map(int, value.split('.')))


def read_version(root=ROOT):
    value = (root / 'VERSION').read_text(encoding='utf-8').strip()
    parse_version(value)
    return value


def version_code(version):
    major, minor, patch = parse_version(version)
    # Preserve the existing 1.0.3 -> 4 mapping, and increase across minor/major bumps.
    return (major - 1) * 1_000_000 + minor * 1000 + patch + 1


def stamp(root=ROOT):
    version = read_version(root)
    code = version_code(version)
    def replace(path, pattern, replacement, count=1):
        path = root / path
        text, found = re.subn(pattern, lambda _: replacement, path.read_text(encoding='utf-8'))
        if found != count:
            raise ValueError(f'{path}: expected {count} version fields, found {found}')
        path.write_text(text, encoding='utf-8')

    for path in ['platforms/apple/mac/Info.plist',
                 'platforms/apple/ios/targets/ios/Info.plist',
                 'platforms/apple/ios/targets/tvos/Info.plist']:
        for key, value in [('CFBundleShortVersionString', version), ('CFBundleVersion', str(code))]:
            replace(path, rf'<key>{key}</key>\s*<string>[^<]+</string>',
                    f'<key>{key}</key>\n\t<string>{value}</string>')
    replace('platforms/android/app/build.gradle', r'versionCode \d+', f'versionCode {code}')
    replace('platforms/android/app/build.gradle', r'versionName "[^"]+"', f'versionName "{version}"')
    parts = [*parse_version(version), parse_version(version)[2] + 1]
    for key in ['FILEVERSION', 'PRODUCTVERSION']:
        replace('platforms/windows/game.rc', rf'{key} [\d,]+', f'{key} ' + ','.join(map(str, parts)))
    for key in ['FileVersion', 'ProductVersion']:
        replace('platforms/windows/game.rc', rf'VALUE "{key}", "[\d, ]+\\0"',
                f'VALUE "{key}", "' + ', '.join(map(str, parts)) + '\\0"')
    replace('platforms/winrt/Package.appxmanifest', r'(?<![A-Za-z])Version="[\d.]+"', f'Version="{version}.0"')
    print(f'Package version {version} (Android/Apple build {code})')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--stamp', action='store_true')
    args = parser.parse_args()
    if args.stamp:
        stamp()
    else:
        print(read_version())
