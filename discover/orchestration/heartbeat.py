#!/usr/bin/env python3
"""Sprint heartbeat (run every 5 min by the coordinator's paseo heartbeat).

Usage: heartbeat.py [REGISTRY]  (default: discover/orchestration/2026-10b/agents.json)
Reads the registry ({"agents": [{"id", "phase", "branch", "done"}]}) and, for
each listed phase agent, prints one line when something needs doing; prints only "OK" otherwise:
  DONE <phase> ...       done marker committed on its branch and agent idle -> coordinator gate + merge
  RESUMED <phase> ...    agent stopped by a usage/rate limit -> a resume message was sent (max 1 per 15 min)
  ATTENTION <phase> ...  pending permission, error state, or idle > 45 min without done marker
Resume attempts are throttled through ~/.local/state/specs-sprint-heartbeat.json.
"""
import datetime as dt, json, os, re, subprocess, sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
AGENTS = os.path.join(REPO, sys.argv[1] if len(sys.argv) > 1 else 'discover/orchestration/2026-10b/agents.json')
STATE = os.path.expanduser('~/.local/state/specs-sprint-heartbeat.json')
LIMIT = re.compile(r"hit your (session|usage|weekly) limit|rate.?limit|usage limit|overloaded|API Error", re.I)
RESUME = ("The heartbeat found you stopped (usage/rate limit or error). The limit has reset: please continue where "
          "you left off. Read {progress} first; resume or relaunch any subagents "
          "that were cut off; commit progress after every item. Never git push.")

def sh(*a):
    return subprocess.run(a, capture_output=True, text=True, cwd=REPO)

def now():
    return dt.datetime.now(dt.timezone.utc)

def main():
    agents = json.load(open(AGENTS)).get('agents', [])
    state = json.load(open(STATE)) if os.path.exists(STATE) else {}
    out = []
    for a in agents:
        aid, phase, branch, done = a['id'], a['phase'], a['branch'], a['done']
        r = sh('paseo', 'inspect', '--json', aid)
        if r.returncode:
            out.append(f'ATTENTION {phase} {aid[:8]}: paseo inspect failed'); continue
        d = json.loads(r.stdout)
        st, pp = d.get('Status'), d.get('PendingPermissions') or []
        idle_min = (now() - dt.datetime.fromisoformat(d['UpdatedAt'].replace('Z', '+00:00'))).total_seconds() / 60
        has_done = sh('git', 'cat-file', '-e', f'{branch}:{done}').returncode == 0
        if pp:
            out.append(f'ATTENTION {phase} {aid[:8]}: {len(pp)} pending permission(s)'); continue
        if st == 'running':
            continue
        if has_done:
            out.append(f'DONE {phase} {aid[:8]}: {done} committed on {branch}'); continue
        tail = sh('paseo', 'logs', aid).stdout.strip().splitlines()[-6:]
        if any(LIMIT.search(l) for l in tail):
            last = state.get(aid)
            if last and (now() - dt.datetime.fromisoformat(last)).total_seconds() < 15 * 60:
                continue
            progress = os.path.join(os.path.dirname(done), f'{phase}-progress.md')
            s = sh('paseo', 'send', '--no-wait', aid, RESUME.format(progress=progress))
            state[aid] = now().isoformat()
            out.append(f'RESUMED {phase} {aid[:8]}: ' + ('message sent' if s.returncode == 0 else 'send failed: ' + s.stderr.strip()[:120]))
            continue
        if st != 'idle':
            out.append(f'ATTENTION {phase} {aid[:8]}: status={st}'); continue
        if idle_min > 45:
            out.append(f'ATTENTION {phase} {aid[:8]}: idle {idle_min:.0f} min without {done}; last: ' + (tail[-1][:120] if tail else '-'))
    os.makedirs(os.path.dirname(STATE), exist_ok=True)
    json.dump(state, open(STATE, 'w'))
    print('\n'.join(out) if out else 'OK')

if __name__ == '__main__':
    sys.exit(main())
