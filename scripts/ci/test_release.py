import copy
from pathlib import Path
import shutil
import tempfile
import unittest
from unittest.mock import patch

import packages
import release
import version


def published(tag='v1.0.3', draft=False, sha='old', author='Steesha'):
    return dict(tag_name=tag, draft=draft, prerelease=False,
                target_commitish=sha, author={'login': author})


class ReleaseTests(unittest.TestCase):
    def test_existing_release_is_never_modified(self):
        existing = [published()]
        before = copy.deepcopy(existing)
        self.assertEqual(release.decision('1.0.3', existing, 'new'), 'skip')
        self.assertEqual(existing, before)
        with patch.dict('os.environ', {'GH_REPO': 'test/repo', 'GITHUB_SHA': 'new'}), \
             patch.object(release, 'read_version', return_value='1.0.3'), \
             patch.object(release, 'releases', return_value=existing), \
             patch.object(release, 'gh') as gh:
            release.main()
            gh.assert_not_called()

    def test_new_version_and_rollbacks(self):
        self.assertEqual(release.decision('1.0.4', [published()], 'new'), 'create')
        self.assertEqual(release.decision('1.0.3', [], 'new'), 'skip')
        self.assertEqual(release.decision('1.0.2', [published()], 'new'), 'skip')
        self.assertEqual(release.decision('1.0.9', [published('v1.0.10')], 'new'), 'skip')
        self.assertEqual(release.decision('1.1.0', [published('v1.0.10')], 'new'), 'create')

    def test_draft_retry_is_limited_to_same_bot_and_commit(self):
        draft = published('v1.0.4', True, 'new', 'github-actions[bot]')
        self.assertEqual(release.decision('1.0.4', [draft], 'new'), 'resume')
        with self.assertRaises(ValueError):
            release.decision('1.0.4', [draft], 'another')
        draft['author']['login'] = 'Steesha'
        with self.assertRaises(ValueError):
            release.decision('1.0.4', [draft], 'new')

    def test_package_inventory_and_corruption(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)
            with self.assertRaises(ValueError):
                packages.assemble(path, '1.0.3')
            for name in packages.PACKAGES:
                (path / name).write_bytes(name.encode())
            packages.assemble(path, '1.0.3')
            self.assertEqual({p.name for p in path.iterdir()}, set(packages.ASSETS))
            packages.verify(path)
            (path / packages.PACKAGES[0]).write_bytes(b'corrupt')
            with self.assertRaises(ValueError):
                packages.verify(path)

    def test_versions_and_native_metadata(self):
        self.assertEqual(version.version_code('1.0.3'), 4)
        self.assertGreater(version.version_code('1.1.0'), version.version_code('1.0.999'))
        self.assertGreater(version.version_code('2.0.0'), version.version_code('1.999.999'))
        for value in ['v1.0.4', '1.0.04', '1.0.4\ncommand', '1.0.4-rc1', '0.1.0']:
            with self.assertRaises(ValueError):
                version.parse_version(value)
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            shutil.copytree(version.ROOT / 'platforms', root / 'platforms')
            (root / 'VERSION').write_text('1.2.3\n')
            version.stamp(root)
            self.assertIn('versionCode 2004', (root / 'platforms/android/app/build.gradle').read_text())
            self.assertIn('FILEVERSION 1,2,3,4', (root / 'platforms/windows/game.rc').read_text())
            self.assertIn('<string>1.2.3</string>', (root / 'platforms/apple/mac/Info.plist').read_text())


if __name__ == '__main__':
    unittest.main()
