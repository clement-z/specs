#!/usr/bin/env python3
"""Usage: watch_agents.py REPO ID:BRANCH:DONEFILE ...
Exit with a report when an agent (a) has pending permissions, (b) is in an error/closed state,
(c) is idle and DONEFILE is committed on BRANCH, or (d) idle without any update for 90 min."""
import json, subprocess, sys, time, datetime as dt
repo, specs = sys.argv[1], [a.split(':', 2) for a in sys.argv[2:]]
def sh(*a):
    return subprocess.run(a, capture_output=True, text=True)
while True:
    for aid, branch, done in specs:
        r = sh('paseo', 'inspect', '--json', aid)
        if r.returncode: continue
        d = json.loads(r.stdout)
        st, pp, name = d.get('Status'), d.get('PendingPermissions') or [], d.get('Name')
        upd = dt.datetime.fromisoformat(d['UpdatedAt'].replace('Z', '+00:00'))
        idle_min = (dt.datetime.now(dt.timezone.utc) - upd).total_seconds() / 60
        has_done = sh('git', '-C', repo, 'cat-file', '-e', f'{branch}:{done}').returncode == 0
        why = None
        if pp: why = f'pending_permissions={len(pp)}'
        elif st not in ('running', 'idle'): why = f'status={st}'
        elif st == 'idle' and has_done: why = 'DONE (deliverable committed)'
        elif st == 'idle' and idle_min > 90: why = f'STALLED? idle {idle_min:.0f} min, deliverable missing'
        if why:
            print(f'AGENT {name} ({aid}) on {branch}: {why}'); sys.exit(0)
    time.sleep(90)
