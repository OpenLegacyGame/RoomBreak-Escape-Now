"""Create a new version release; never modify an already published release."""
import json
import os
from pathlib import Path
import subprocess
from packages import ASSETS, digest, verify
from version import read_version, parse_version

BASELINE = (1, 0, 3)


def gh(*args):
    return subprocess.check_output(['gh', *args], text=True)


def releases(repo):
    pages = json.loads(gh('api', '--paginate', '--slurp', f'repos/{repo}/releases?per_page=100'))
    return [release for page in pages for release in page]


def decision(version, existing, sha):
    tag = f'v{version}'
    matching = next((r for r in existing if r['tag_name'] == tag), None)
    if matching and not matching['draft']:
        return 'skip'
    latest = BASELINE
    for release in existing:
        if release['draft'] or release['prerelease']:
            continue
        try:
            latest = max(latest, parse_version(release['tag_name'].removeprefix('v')))
        except ValueError:
            pass
    if parse_version(version) <= latest:
        return 'skip'
    if matching:
        # Retry only our own unfinished draft for this exact commit.
        if matching['author']['login'] != 'github-actions[bot]' or matching['target_commitish'] != sha:
            raise ValueError('Existing draft belongs to another author/commit; leaving it untouched')
        return 'resume'
    return 'create'


def main():
    repo, sha = os.environ['GH_REPO'], os.environ['GITHUB_SHA']
    version = read_version()
    tag = f'v{version}'
    existing = releases(repo)
    action = decision(version, existing, sha)
    if action == 'skip':
        print(f'{tag}: already released or not newer than the published version; no changes.')
        return
    directory = Path('dist/release')
    verify(directory)
    notes = gh('api', f'repos/{repo}/commits/{sha}', '--jq', '.commit.message').rstrip('\n')
    notes_path = Path(os.environ.get('RUNNER_TEMP', '.')) / 'roombreak-release-notes.txt'
    notes_path.write_text(notes, encoding='utf-8')
    # If a tag was created manually, it must point at the exact built commit.
    refs = json.loads(gh('api', f'repos/{repo}/git/matching-refs/tags/{tag}'))
    if any(ref['ref'] == f'refs/tags/{tag}' for ref in refs):
        tag_sha = gh('api', f'repos/{repo}/commits/{tag}', '--jq', '.sha').strip()
        if tag_sha != sha:
            raise ValueError('Existing tag does not point at this build; leaving it untouched')
    if action == 'create':
        gh('release', 'create', tag, '--draft', '--target', sha,
           '--title', f'RoomBreak {tag}', '--notes-file', str(notes_path))
    release = next(r for r in releases(repo) if r['tag_name'] == tag)
    assets = {asset['name']: asset for asset in release['assets']}
    if set(assets) - set(ASSETS):
        raise ValueError('Unexpected draft assets; leaving them untouched')
    # Never overwrite assets, including when retrying an interrupted upload.
    for name in ASSETS:
        path = directory / name
        if name in assets:
            asset = assets[name]
            if asset['state'] != 'uploaded' or asset['digest'] != 'sha256:' + digest(path):
                raise ValueError(f'Existing draft asset differs: {name}')
        else:
            gh('release', 'upload', tag, str(path))
    release = next(r for r in releases(repo) if r['tag_name'] == tag)
    if not release['draft'] or release['body'] != notes:
        raise ValueError('Release draft state or commit notes changed unexpectedly')
    if {a['name'] for a in release['assets']} != set(ASSETS):
        raise ValueError('Release assets are incomplete')
    for asset in release['assets']:
        if asset['state'] != 'uploaded' or asset['digest'] != 'sha256:' + digest(directory / asset['name']):
            raise ValueError('Uploaded asset failed checksum verification')
    gh('release', 'edit', tag, '--draft=false', '--latest')
    print(f'Published {tag} with six verified assets and the complete commit message.')


if __name__ == '__main__':
    main()
