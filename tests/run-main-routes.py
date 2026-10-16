#!/usr/bin/env python3
"""Run the existing 30 in-engine route probes serially, preserving fresh evidence."""
import argparse
import datetime
import hashlib
import json
import os
import pathlib
import subprocess
import time

# Displayed episode -> internal database episode.
EPISODES = [(1, 3), (2, 1), (3, 2), (4, 4), (5, 5), (6, 6)]
STAGE_TIMEOUT = 720

parser = argparse.ArgumentParser()
parser.add_argument('--binary', required=True, type=pathlib.Path)
parser.add_argument('--output', required=True, type=pathlib.Path)
args = parser.parse_args()
binary = args.binary.resolve()
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=False)
results = []


def publish():
    (out / 'results.json').write_text(json.dumps(results, ensure_ascii=False, indent=2))
    lines = ['RoomBreak 30关复测（本次运行）', '显示章节 / 内部编号 / 状态 / 用时（秒）']
    lines += [
        f"第{r['episode']}章第{r['stage']}关 / {r['probe']} / {r['status']} / {r.get('seconds', '-')}"
        for r in results
    ]
    (out / 'progress.txt').write_text('\n'.join(lines) + '\n')


def run_stage(probe, folder, env):
    """Runs one probe process and returns its exit code, or 'timeout'."""
    with (folder / 'run.log').open('w') as log:
        process = subprocess.Popen([str(binary)], stdout=log, stderr=subprocess.STDOUT, env=env)
        results[-1]['pid'] = process.pid
        publish()
        try:
            return process.wait(timeout=STAGE_TIMEOUT)
        except subprocess.TimeoutExpired:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            return 'timeout'


(out / 'run-info.json').write_text(json.dumps({
    'started': datetime.datetime.now().astimezone().isoformat(),
    'binary': str(binary),
    'sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
    'language': 'Chinese; episode 6 stage 3 English, matching prior validated route',
    'scope': 'existing in-engine route probes; individual fresh sessions, not OS-level mouse automation',
}, ensure_ascii=False, indent=2))

for episode, internal in EPISODES:
    for stage in range(1, 6):
        probe = f'RB_PROBE_EPISODE{internal}_STAGE{stage}'
        folder = out / f'episode{episode}-stage{stage}'
        folder.mkdir()
        result = {'episode': episode, 'stage': stage, 'probe': probe, 'status': 'RUNNING'}
        results.append(result)
        publish()
        print(f'开始 第{episode}章 第{stage}关 ({probe})', flush=True)
        env = {k: v for k, v in os.environ.items() if not k.startswith('RB_PROBE') and k != 'RB_VISUAL_PROBE_OUTPUT'}
        env.update({
            probe: '1',
            'RB_VISUAL_PROBE_OUTPUT': str(folder),
            'RB_PROBE_LANGUAGE': '2' if (episode, stage) == (6, 3) else '0',
        })
        started = time.monotonic()
        code = run_stage(probe, folder, env)
        text = (folder / 'run.log').read_text(errors='replace')
        shots = list(folder.glob('*clear.png'))
        passed = code == 0 and probe + ' PASS' in text and shots
        result.update(
            status='PASS' if passed else 'FAIL',
            seconds=round(time.monotonic() - started, 1),
            exit_code=code,
            screenshots=[str(s.relative_to(out)) for s in shots],
        )
        publish()
        print(f"{result['status']} 第{episode}章 第{stage}关 {result['seconds']} 秒；截图 {len(shots)}", flush=True)
        if result['status'] != 'PASS':
            print(text[-4000:], flush=True)
            raise SystemExit(1)
print('30/30 PASS', flush=True)
