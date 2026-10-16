#!/usr/bin/env python3
"""Continuous 30-route demo: title, menu, intros, gameplay, clear, endings, next stage."""
import argparse
import datetime
import hashlib
import json
import os
import pathlib
import re
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('--binary', required=True, type=pathlib.Path)
parser.add_argument('--output', required=True, type=pathlib.Path)
args = parser.parse_args()
binary = args.binary.resolve()
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=False)

env = {
    k: v for k, v in os.environ.items()
    if not k.startswith('RB_PROBE') and k not in ('RB_VISUAL_PROBE_OUTPUT', 'RB_ROUTE_DEMO')
}
env.update(RB_ROUTE_DEMO='1', RB_VISUAL_PROBE_OUTPUT=str(out), RB_PROBE_SAVE=str(out / 'progress-save.json'))
(out / 'run-info.json').write_text(json.dumps(dict(
    started=datetime.datetime.now().astimezone().isoformat(),
    binary=str(binary),
    sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
    scope='Continuous in-engine menu actions and route inputs from title, shared fresh save, '
          'normal intro/ending playback',
), ensure_ascii=False, indent=2))

results = []
seen = set()


def record(kind, episode, stage):
    """Tracks one START/PASS marker and republishes the progress files."""
    print(f'{kind} 第{episode}章 第{stage}关', flush=True)
    if kind == 'START':
        results.append(dict(episode=episode, stage=stage, status='RUNNING'))
    else:
        row = next(r for r in results if r['episode'] == episode and r['stage'] == stage)
        row['status'] = 'PASS'
    (out / 'results.json').write_text(json.dumps(results, ensure_ascii=False, indent=2))
    (out / 'progress.txt').write_text(
        '\n'.join(f"第{r['episode']}章第{r['stage']}关 {r['status']}" for r in results) + '\n')


with (out / 'run.log').open('w') as log:
    process = subprocess.Popen([str(binary)], env=env, stdout=log, stderr=subprocess.STDOUT)
    print(f'从主页面开始，游戏进程 {process.pid}', flush=True)
    try:
        while True:
            text = (out / 'run.log').read_text(errors='replace')
            for kind, episode, stage in re.findall(r'RB_MENU_DEMO_(START|PASS) episode=(\d+) stage=(\d+)', text):
                key = (kind, episode, stage)
                if key in seen:
                    continue
                seen.add(key)
                record(kind, int(episode), int(stage))
            if process.poll() is not None:
                break
            time.sleep(1)
    except BaseException:
        process.terminate()
        process.wait(timeout=10)
        raise

passed = (process.returncode == 0 and len(results) == 30 and all(r['status'] == 'PASS' for r in results)
          and 'RB_ROUTE_DEMO_COMPLETE 30/30' in text)
print('30/30 PASS' if passed else f'演示未完成，退出码 {process.returncode}；日志：{out / "run.log"}', flush=True)
raise SystemExit(0 if passed else 1)
