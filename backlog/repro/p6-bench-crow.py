#!/usr/bin/env python3
"""bench_f05.py REPO OLD_SPECS NEW_SPECS [N] [REPEAT]: F-05 tracing cost on the paper's CROW (crow_gen.py,
seed 1, 10 pulses, PROBE1 through + PROBE2 drop, no .options traceall). Runs each configuration REPEAT
times in a fresh directory and reports the median user time, the output sizes and the probed waveforms'
equality with the old binary's full-tracing run."""
import json, os, re, resource, statistics, subprocess, sys, tempfile
repo, old, new = sys.argv[1:4]
n = sys.argv[4] if len(sys.argv) > 4 else "30"
repeat = int(sys.argv[5]) if len(sys.argv) > 5 else 3


def probe_waveforms(path):
    """Streaming: the records of the explicit probes' variables (scopes ROOT/PROBE*), as
    {name: [(time, value), ...]} (a full-tracing VCD is about 1 GB: not loaded in memory)."""
    codes, scopes, out, t = {}, [], {}, 0
    with open(path) as f:
        for line in f:
            tok = line.split()
            if not tok:
                continue
            if tok[0] == "$scope":
                scopes.append(tok[2])
            elif tok[0] == "$upscope":
                scopes.pop()
            elif tok[0] == "$var" and scopes and scopes[-1].startswith("ROOT/PROBE"):
                codes[tok[3]] = scopes[-1] + "." + tok[4]
            elif tok[0] == "$enddefinitions":
                break
        for line in f:
            if line[0] == "#":
                t = int(line[1:])
            elif line[0] in "rR":
                val, code = line.split()
                if code in codes:
                    out.setdefault(codes[code], []).append((t, val))
    return out

gen = subprocess.run([sys.executable, "-I", repo + "/discover/scout/bench/crow_gen.py", n, "--seed", "1"],
                     capture_output=True, text=True, check=True).stdout
net_default = re.sub(r" traceall=\d+", "", gen)                  # no .options traceall: the F-05 rule decides
net_traceall = re.sub(r" traceall=\d+", " traceall=1", gen)      # explicit full tracing
configs = [
    ("old binary, default (full tracing)", old, net_default, []),
    ("new binary, .options traceall=1 (full tracing)", new, net_traceall, []),
    ("new binary, default (probes only), VCD", new, net_default, []),
    ("new binary, probes only, --format csv", new, net_default, ["--format", "csv"]),
    ("new binary, probes only, --format npz", new, net_default, ["--format", "npz"]),
    ("new binary, --no-trace", new, net_default, ["--no-trace"]),
]
env = dict(os.environ, SC_COPYRIGHT_MESSAGE="DISABLE", LC_ALL="C")
results, ref_vcd = [], None
for label, specs, net, extra in configs:
    users, sizes = [], {}
    for i in range(repeat):
        with tempfile.TemporaryDirectory(prefix="p6-bench-") as td:
            os.mkdir(os.path.join(td, "traces"))
            open(os.path.join(td, "crow.cir"), "w").write(net)
            r0 = resource.getrusage(resource.RUSAGE_CHILDREN)
            p = subprocess.run([specs, "-f", "crow.cir", "-o", "traces/crow"] + extra, cwd=td, env=env,
                               capture_output=True, text=True)
            r1 = resource.getrusage(resource.RUSAGE_CHILDREN)
            assert p.returncode == 0, (label, p.stderr[-500:])
            users.append(r1.ru_utime - r0.ru_utime)
            sizes = {f: os.path.getsize(os.path.join(td, "traces", f)) for f in os.listdir(os.path.join(td, "traces"))
                     if not f.endswith(".json")}
            simulated = re.search(r"Simulated (\S+ \S+)", p.stdout).group(1)
            if i == 0 and "crow.vcd" in sizes:
                probed = probe_waveforms(os.path.join(td, "traces", "crow.vcd"))
                assert len(probed) == 8, sorted(probed)
                if ref_vcd is None:
                    ref_vcd = probed
                    same = "reference"
                else:
                    same = "identical" if probed == ref_vcd else "DIFFERENT"
            elif i == 0:
                same = "-"
    res = dict(label=label, user_s=round(statistics.median(users), 2), runs=[round(u, 2) for u in users],
               mb={k: round(v / 1e6, 2) for k, v in sizes.items()}, simulated=simulated, probes_vs_old=same)
    results.append(res)
    print(json.dumps(res), flush=True)
print(json.dumps(results))
