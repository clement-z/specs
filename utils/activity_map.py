#!/usr/bin/env python3
"""Static activity map: the circuit graph coloured by the events each device processed in a run.

Inputs are the two files written by one SPECS run:

    specs -f net.cir -o traces/run --json traces/run.json --stats=ports,devices

Usage:

    activity_map.py traces/run.json traces/run.stats.json -o traces/map
    activity_map.py CIRCUIT_JSON STATS_JSON -o OUT [--format svg,png] [--suppression] [--no-field]
                    [--dot-only] [--title TEXT] [--net-labels]

Writes OUT.dot always, and OUT.svg / OUT.png unless --dot-only is given. The core (reading the files and
writing the DOT graph) uses the Python standard library only. Rendering needs numpy, matplotlib and the
Graphviz `dot` binary (used for the layout only).

Activity of a device = sum of `processed` over the stats ports it writes (devices without output ports,
such as probes, have 0). Node colour: linear scale 0..max activity. Suppression ratio of a device =
(abstol + reltol + snap suppressed) / processed. See doc/activity_map.md.

Exit codes: 0 ok, 2 bad input, 3 rendering dependency missing.
"""

import argparse
import json
import math
import os
import shutil
import subprocess
import sys

STATS_FORMAT = "specs-run-stats"
STATS_FORMAT_VERSION = 1

# Sequential single-hue ramp (blue) for activity, white at zero; interpolated linearly in RGB.
ACTIVITY_STOPS = ["#ffffff", "#cde2fb", "#9ec5f4", "#6da7ec", "#3987e5", "#256abf", "#184f95", "#0d366b"]
# Second ramp (orange) for the suppression ratio, fixed scale 0..1.
SUPPRESSION_STOPS = ["#ffffff", "#fbd3c0", "#f4a07c", "#eb6834", "#b8481d", "#7a2c0f"]
INK = "#1a1a1a"
EDGE_COLOR = "#555555"
CLUSTER_COLOR = "#8a8a8a"
FIELD_K0 = math.exp(-2.0)  # background field offset, see field_grid()
FIELD_PIXELS = 400  # field grid resolution on the longest side
CHAR_WIDTH_PT = 6.0  # estimated text width per character at NODE_FONT_PT, for the node box width
NODE_FONT_PT = 9
NODE_HEIGHT_PT = 38.0


class InputError(Exception):
    pass


# ---- Colours (stdlib) ------------------------------------------------------------------------------


def hex_to_rgb(h):
    h = h.lstrip("#")
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def ramp_color(stops, t):
    """Colour of the ramp at t in [0, 1] (clamped), linear interpolation in RGB between the stops."""
    t = min(1.0, max(0.0, t))
    pos = t * (len(stops) - 1)
    i = min(int(pos), len(stops) - 2)
    f = pos - i
    a, b = hex_to_rgb(stops[i]), hex_to_rgb(stops[i + 1])
    return tuple(round(a[k] + f * (b[k] - a[k])) for k in range(3))


def rgb_to_hex(rgb):
    return "#%02x%02x%02x" % tuple(rgb)


def luminance(rgb):
    return 0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2]


def text_color(fill_rgb):
    """Dark ink on light fills, white on fills darker than the midpoint of the ramp."""
    mid = luminance(ramp_color(ACTIVITY_STOPS, 0.5))
    return "#ffffff" if luminance(fill_rgb) < mid else INK


# ---- Input -----------------------------------------------------------------------------------------


def load_json(path, what):
    try:
        with open(path) as f:
            return json.load(f)
    except OSError as e:
        raise InputError("cannot read %s file %s: %s" % (what, path, e.strerror))
    except ValueError as e:
        raise InputError("%s file %s is not valid JSON: %s" % (what, path, e))


def check_inputs(circuit, stats):
    if not isinstance(circuit, dict) or not isinstance(circuit.get("elements"), list):
        raise InputError("circuit file has no 'elements' list (expected the output of specs --json)")
    if not isinstance(stats, dict) or stats.get("format") != STATS_FORMAT:
        raise InputError("stats file is not a SPECS run-statistics file (format != '%s')" % STATS_FORMAT)
    if stats.get("format_version") != STATS_FORMAT_VERSION:
        raise InputError("unsupported stats format_version %r (this tool reads %d)"
                         % (stats.get("format_version"), STATS_FORMAT_VERSION))
    if not isinstance(stats.get("ports"), list):
        raise InputError("stats file has no 'ports' section: rerun specs with --stats=ports,devices")


def short_name(name):
    base = name[5:] if name.startswith("ROOT/") else name
    return base.rsplit("/", 1)[-1]


def instance_path(name):
    """Instance path of an element, without ROOT/ and without the last component ('' at top level)."""
    base = name[5:] if name.startswith("ROOT/") else name
    return base.rsplit("/", 1)[0] if "/" in base else ""


def net_key(net):
    """Stats nets of a bidirectional net carry a :0 / :1 suffix; the circuit JSON has the bare name."""
    if net is None:
        return None
    head, sep, tail = net.rpartition(":")
    return head if sep and tail.isdigit() else net


def compute_activity(circuit, stats):
    """Per element: dict(processed, suppressed, ratio). Also the per-net writers and emitted counts."""
    names = {e["name"] for e in circuit["elements"]}
    act = {n: {"processed": 0, "suppressed": 0} for n in names}
    nets = {}  # net -> {"writers": set, "emitted": int}
    unmatched = set()
    for p in stats["ports"]:
        w = p.get("writer")
        if w in act:
            act[w]["processed"] += p.get("processed", 0)
            sup = p.get("suppressed", {})
            act[w]["suppressed"] += sum(sup.get(k, 0) for k in ("abstol", "reltol", "snap"))
        elif w is not None:
            unmatched.add(w)
        key = net_key(p.get("net"))
        if key is not None:
            d = nets.setdefault(key, {"writers": set(), "emitted": 0})
            if w in names:
                d["writers"].add(w)
            d["emitted"] += p.get("emitted", 0)
    for a in act.values():
        a["ratio"] = a["suppressed"] / a["processed"] if a["processed"] else 0.0
    if unmatched:
        print("activity_map: warning: %d writer(s) in the stats match no circuit element" % len(unmatched),
              file=sys.stderr)
    return act, nets


# ---- DOT -------------------------------------------------------------------------------------------


def dot_quote(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def build_graph(circuit, act, netdata, net_labels=False):
    """Return the DOT text: one node per element, a cluster per subcircuit instance, one edge set per net."""
    elements = circuit["elements"]
    vmax = max([a["processed"] for a in act.values()] + [0])
    out = []
    out.append("digraph activity {")
    out.append('  rankdir=LR; compound=true; fontname="DejaVu Sans"; nodesep=0.35; ranksep=0.6;')
    out.append('  node [fontname="DejaVu Sans", fontsize=%d];' % NODE_FONT_PT)
    out.append('  edge [fontname="DejaVu Sans", fontsize=8, color="%s"];' % EDGE_COLOR)

    # Clusters: a tree of instance paths.
    tree = {(): {"elements": [], "children": []}}

    def ensure(path):
        if path not in tree:
            tree[path] = {"elements": [], "children": []}
            ensure(path[:-1])
            tree[path[:-1]]["children"].append(path)
        return tree[path]

    for e in elements:
        ip = instance_path(e["name"])
        ensure(tuple(ip.split("/")) if ip else ())["elements"].append(e)

    node_id = {e["name"]: "n%d" % i for i, e in enumerate(elements)}

    def emit_node(e, indent):
        a = act[e["name"]]
        fill = ramp_color(ACTIVITY_STOPS, a["processed"] / vmax if vmax else 0.0)
        lines = [short_name(e["name"]), str(e.get("type", ""))]
        w = max(76.0, max(len(s) for s in lines) * CHAR_WIDTH_PT + 30.0)
        out.append('%s%s [label=%s, shape=box, style="rounded,filled", fixedsize=true, width=%.4f, '
                   'height=%.4f, fillcolor="%s", fontcolor="%s", color="%s", tooltip=%s];'
                   % (indent, node_id[e["name"]], dot_quote("\n".join(lines)), w / 72.0,
                      NODE_HEIGHT_PT / 72.0, rgb_to_hex(fill), text_color(fill), INK,
                      dot_quote("%s: %d processed" % (e["name"], a["processed"]))))

    def emit_cluster(path, indent):
        if path:
            out.append('%ssubgraph %s {' % (indent, dot_quote("cluster_ROOT/" + "/".join(path))))
            out.append('%s  label=%s; style="rounded"; color="%s";'
                       % (indent, dot_quote("/".join(path)), CLUSTER_COLOR))
            inner = indent + "  "
        else:
            inner = indent
        for e in tree[path]["elements"]:
            emit_node(e, inner)
        for child in tree[path]["children"]:
            emit_cluster(child, inner)
        if path:
            out.append("%s}" % indent)

    emit_cluster((), "  ")

    # Nets -> edges.
    members = {}  # net -> [element names]
    for e in elements:
        for n in e.get("nets", []):
            lst = members.setdefault(n, [])
            if e["name"] not in lst:
                lst.append(e["name"])
    extra = 0
    for net in sorted(members):
        els = members[net]
        nd = netdata.get(net, {"writers": set(), "emitted": 0})
        writers = [w for w in els if w in nd["writers"]]
        nm = net[5:] if net.startswith("ROOT/") else net
        lab = ', label=%s' % dot_quote("%s\n%d" % (nm, nd["emitted"])) if net_labels else ""

        def edge(a, b, attrs):
            out.append("  %s -> %s [%s%s];" % (a, b, attrs, lab))

        if len(els) == 1:
            t = "t%d" % extra
            extra += 1
            out.append('  %s [label=%s, shape=plaintext, fontsize=8, width=0, height=0, margin=0.03, '
                       'fontcolor="%s", tooltip="terminal"];' % (t, dot_quote(nm), EDGE_COLOR))
            if writers:
                edge(node_id[els[0]], t, "dir=forward")
            elif nd["writers"] or nd["emitted"]:
                edge(t, node_id[els[0]], "dir=forward")
            else:
                edge(t, node_id[els[0]], "dir=none")
        elif len(els) == 2:
            a, b = els
            if len(writers) == 2:
                edge(node_id[a], node_id[b], "dir=both")
            elif len(writers) == 1:
                src, dst = (a, b) if writers[0] == a else (b, a)
                edge(node_id[src], node_id[dst], "dir=forward")
            else:
                edge(node_id[a], node_id[b], "dir=none")
        else:
            j = "j%d" % extra
            extra += 1
            out.append('  %s [shape=point, width=0.08, color="%s", tooltip=%s];'
                       % (j, EDGE_COLOR, dot_quote(net)))
            for el in els:
                if el in writers:
                    out.append("  %s -> %s [arrowhead=none%s];" % (node_id[el], j, lab))
                elif writers:
                    out.append("  %s -> %s [dir=forward];" % (j, node_id[el]))
                else:
                    out.append("  %s -> %s [dir=none];" % (j, node_id[el]))
    out.append("}")
    return "\n".join(out) + "\n"


# ---- Rendering (needs numpy, matplotlib, dot) ---------------------------------------------------------


def missing_render_deps():
    missing = []
    for mod in ("numpy", "matplotlib"):
        try:
            __import__(mod)
        except ImportError:
            missing.append(mod)
    if shutil.which("dot") is None:
        missing.append("the Graphviz 'dot' binary")
    return missing


def layout(dot_text):
    """Run dot -Tjson and return the parsed layout."""
    r = subprocess.run(["dot", "-Tjson"], input=dot_text, capture_output=True, text=True)
    if r.returncode != 0:
        raise InputError("dot failed: " + r.stderr.strip())
    return json.loads(r.stdout)


def parse_point(s):
    x, y = s.split(",")[:2]
    return float(x), float(y)


def parse_edge_pos(pos):
    """Graphviz spline 'e,x,y s,x,y x,y ...' -> (points, arrow_end, arrow_start)."""
    ae = as_ = None
    pts = []
    for tok in pos.split():
        if tok.startswith("e,"):
            ae = parse_point(tok[2:])
        elif tok.startswith("s,"):
            as_ = parse_point(tok[2:])
        else:
            pts.append(parse_point(tok))
    return pts, ae, as_


def field_sigma(centers, half_widths, bbox):
    """Median nearest-neighbour distance between device centres (fallback: bbox diagonal / 4), but never
    less than the median node half-width."""
    if len(centers) >= 2:
        nn = sorted(min(math.hypot(xi - xj, yi - yj) for j, (xj, yj) in enumerate(centers) if j != i)
                    for i, (xi, yi) in enumerate(centers))
        m = len(nn)
        sigma = nn[m // 2] if m % 2 else 0.5 * (nn[m // 2 - 1] + nn[m // 2])
    else:
        sigma = 0.25 * math.hypot(bbox[2] - bbox[0], bbox[3] - bbox[1])
    hw = sorted(half_widths)
    return max(sigma, hw[len(hw) // 2] if hw else 0.0, 1.0)


def field_grid(centers, acts, bbox, sigma, vmax, np):
    """Background field of the map (also documented in doc/activity_map.md):

        f(x) = sum_i a_i K_i(x) / (sum_i K_i(x) + K0),   K_i(x) = exp(-|x - x_i|^2 / (2 sigma^2))

    x_i are the device node centres, a_i their activity, K0 = exp(-2). Near a device f is close to its
    activity; between devices it interpolates smoothly; far from all devices (every K_i << K0) it goes to
    0, the colour of the surface. At 2 sigma from a lone device K = K0, so f is half its activity.
    Returns the grid divided by vmax (so 0..1 on the colour scale).
    """
    x0, y0, x1, y1 = bbox
    scale = FIELD_PIXELS / max(x1 - x0, y1 - y0)
    nx, ny = max(2, int(round((x1 - x0) * scale))), max(2, int(round((y1 - y0) * scale)))
    gx, gy = np.meshgrid(np.linspace(x0, x1, nx), np.linspace(y0, y1, ny))
    num = np.zeros_like(gx)
    den = np.zeros_like(gx)
    for (cx, cy), a in zip(centers, acts):
        k = np.exp(-((gx - cx) ** 2 + (gy - cy) ** 2) / (2.0 * sigma ** 2))
        num += a * k
        den += k
    f = num / (den + FIELD_K0)
    return f / vmax if vmax else f


def totals_text(stats, ndev):
    t = stats.get("totals", {})
    sup = t.get("suppressed", {})
    end = stats.get("end_time_s")
    return "\n".join([
        "analysis: %s" % stats.get("analysis", "?"),
        "end time: %s s" % ("%.4g" % end if isinstance(end, (int, float)) else "?"),
        "devices drawn: %d" % ndev,
        "output ports: %s" % t.get("output_ports", "?"),
        "scheduled: %s" % t.get("scheduled", "?"),
        "processed: %s" % t.get("processed", "?"),
        "emitted: %s" % t.get("emitted", "?"),
        "suppressed: %s / %s / %s" % (sup.get("abstol", "?"), sup.get("reltol", "?"), sup.get("snap", "?")),
        "  (abstol / reltol / snap)",
        "coalesced: %s" % t.get("coalesced", "?"),
        "event_cap_reached: %s" % str(stats.get("event_cap_reached", "?")).lower(),
    ])


def render(dot_text, circuit, stats, act, outbase, formats, title, suppression, field):
    import numpy as np
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.cm import ScalarMappable
    from matplotlib.colors import LinearSegmentedColormap, Normalize
    from matplotlib.patches import Circle, FancyArrowPatch, FancyBboxPatch, PathPatch
    from matplotlib.path import Path

    plt.rcParams["svg.fonttype"] = "none"
    plt.rcParams["svg.hashsalt"] = "specs-activity-map"
    plt.rcParams["font.family"] = "DejaVu Sans"

    def cmap_of(stops, name):
        return LinearSegmentedColormap.from_list(name, [tuple(c / 255.0 for c in hex_to_rgb(s)) for s in stops])

    cmap_act = cmap_of(ACTIVITY_STOPS, "specs_activity")
    cmap_sup = cmap_of(SUPPRESSION_STOPS, "specs_suppression")

    lay = layout(dot_text)
    bb = [float(v) for v in lay["bb"].split(",")]
    elem_of = {"n%d" % i: e for i, e in enumerate(circuit["elements"])}
    nodes, clusters = {}, []
    for o in lay.get("objects", []):
        if o["name"].startswith("cluster_"):
            clusters.append(o)
        elif "pos" in o:
            nodes[o["name"]] = o
    vmax = max([a["processed"] for a in act.values()] + [0])
    norm = Normalize(0, vmax if vmax else 1)

    centers, acts, halfw = [], [], []
    for nid, e in elem_of.items():
        o = nodes[nid]
        centers.append(parse_point(o["pos"]))
        acts.append(act[e["name"]]["processed"])
        halfw.append(float(o["width"]) * 72.0 / 2)

    # The field decays over about 2 sigma beyond the outermost devices: pad the view so the surface shows.
    base = (bb[0] - 24.0, bb[1] - 24.0, bb[2] + 24.0, bb[3] + 24.0)
    margin = 24.0 + (min(2.0 * field_sigma(centers, halfw, base), 250.0) if field else 0.0)
    bbox = (bb[0] - margin, bb[1] - margin, bb[2] + margin, bb[3] + margin)
    gw, gh = bbox[2] - bbox[0], bbox[3] - bbox[1]
    inch = 1.0 / 72.0  # one graph point -> one inch
    side = 3.0  # width of the legend column, inches
    fig_w = max(4.0, gw * inch) + side
    fig_h = max(7.4 if suppression else 5.2, gh * inch + 0.7)
    fig = plt.figure(figsize=(fig_w, fig_h))
    ax = fig.add_axes([0.1 / fig_w, 0.1 / fig_h, gw * inch / fig_w, gh * inch / fig_h])
    ax.set_xlim(bbox[0], bbox[2])
    ax.set_ylim(bbox[1], bbox[3])
    ax.set_aspect("equal", adjustable="box", anchor="SW")
    ax.axis("off")
    fig.text(0.1 / fig_w, 1 - 0.1 / fig_h, title, ha="left", va="top", fontsize=11)

    if field and centers:
        sigma = field_sigma(centers, halfw, bbox)
        grid = field_grid(centers, acts, bbox, sigma, vmax, np)
        ax.imshow(grid, origin="lower", extent=(bbox[0], bbox[2], bbox[1], bbox[3]), cmap=cmap_act,
                  vmin=0, vmax=1, interpolation="bilinear", aspect="auto", zorder=0)

    # Clusters, outermost first so inner ones draw over them.
    def width_of(o):
        v = [float(s) for s in o["bb"].split(",")]
        return v[2] - v[0]

    for o in sorted(clusters, key=width_of, reverse=True):
        x0, y0, x1, y1 = [float(v) for v in o["bb"].split(",")]
        ax.add_patch(FancyBboxPatch((x0, y0), x1 - x0, y1 - y0, boxstyle="round,pad=0,rounding_size=8",
                                    fc=(1, 1, 1, 0.3), ec=CLUSTER_COLOR, lw=1.0, zorder=1))
        if "lp" in o:
            lx, ly = parse_point(o["lp"])
            ax.text(lx, ly, o.get("label", ""), ha="center", va="center", fontsize=8, color="#333333",
                    zorder=2)

    # Edges.
    for o in lay.get("edges", []):
        if "pos" not in o:
            continue
        pts, ae, as_ = parse_edge_pos(o["pos"])
        if len(pts) < 4:
            continue
        verts, codes = [pts[0]], [Path.MOVETO]
        for i in range(1, len(pts) - 2, 3):
            verts += pts[i:i + 3]
            codes += [Path.CURVE4] * 3
        ax.add_patch(PathPatch(Path(verts, codes), fc="none", ec=EDGE_COLOR, lw=0.9, zorder=3))
        for tip, base in ((ae, pts[-1]), (as_, pts[0])):
            if tip is not None:
                ax.add_patch(FancyArrowPatch(base, tip, arrowstyle="-|>,head_length=4,head_width=2",
                                             mutation_scale=1, color=EDGE_COLOR, lw=0.9, zorder=3,
                                             shrinkA=0, shrinkB=0))
        if "lp" in o and o.get("label"):
            lx, ly = parse_point(o["lp"])
            ax.text(lx, ly, o["label"].replace("\\n", "\n"), ha="center", va="center", fontsize=6.5, color="#333333", zorder=4,
                    bbox=dict(fc="white", ec="none", alpha=0.75, pad=0.6))

    # Nodes, terminals and junctions.
    for nid, o in nodes.items():
        cx, cy = parse_point(o["pos"])
        if nid in elem_of:
            e = elem_of[nid]
            a = act[e["name"]]
            fill = ramp_color(ACTIVITY_STOPS, a["processed"] / vmax if vmax else 0.0)
            w, h = float(o["width"]) * 72.0, float(o["height"]) * 72.0
            ax.add_patch(FancyBboxPatch((cx - w / 2, cy - h / 2), w, h, boxstyle="round,pad=0,rounding_size=7",
                                        fc=rgb_to_hex(fill), ec=INK, lw=0.7, zorder=5))
            tc = text_color(fill)
            ax.text(cx, cy + h * 0.17, short_name(e["name"]), ha="center", va="center", fontsize=NODE_FONT_PT,
                    color=tc, fontweight="bold", zorder=6)
            ax.text(cx, cy - h * 0.22, str(e.get("type", "")), ha="center", va="center",
                    fontsize=NODE_FONT_PT - 2.5, color=tc, zorder=6)
            if suppression:
                r = 0.35 * h
                ax.add_patch(Circle((cx + w / 2 - r * 0.4, cy + h / 2 - r * 0.4), r,
                                    fc=rgb_to_hex(ramp_color(SUPPRESSION_STOPS, a["ratio"])), ec=INK, lw=0.6,
                                    zorder=7))
        elif o.get("shape") == "point":
            ax.add_patch(Circle((cx, cy), 2.5, fc=EDGE_COLOR, ec="none", zorder=5))
        else:
            ax.text(cx, cy, o.get("label", ""), ha="center", va="center", fontsize=7.5, color=EDGE_COLOR,
                    zorder=5, bbox=dict(fc="white", ec="none", alpha=0.75, pad=1.0))

    # Legend column: colourbars and the totals box.
    cb_left = 1 - (side - 0.5) / fig_w
    top = 0.5  # inches from the top
    cax = fig.add_axes([cb_left, 1 - (top + 2.0) / fig_h, 0.2 / fig_w, 2.0 / fig_h])
    cb = fig.colorbar(ScalarMappable(norm=norm, cmap=cmap_act), cax=cax)
    cb.set_label("processed events per device (linear)", fontsize=8)
    cb.ax.tick_params(labelsize=7)
    top += 2.0 + 0.3
    if suppression:
        top += 0.3
        cax2 = fig.add_axes([cb_left, 1 - (top + 1.6) / fig_h, 0.2 / fig_w, 1.6 / fig_h])
        cb2 = fig.colorbar(ScalarMappable(norm=Normalize(0, 1), cmap=cmap_sup), cax=cax2)
        cb2.set_label("suppressed / processed", fontsize=8)
        cb2.ax.tick_params(labelsize=7)
        top += 1.6 + 0.3
    fig.text(cb_left - 0.1 / fig_w, 1 - (top + 0.3) / fig_h, totals_text(stats, len(elem_of)), ha="left",
             va="top", fontsize=7, family="DejaVu Sans Mono", linespacing=1.4,
             bbox=dict(fc="white", ec=CLUSTER_COLOR, boxstyle="round,pad=0.5"))

    for fmt in formats:
        kw = {"metadata": {"Date": None}} if fmt == "svg" else {"dpi": 150}
        fig.savefig("%s.%s" % (outbase, fmt), format=fmt, **kw)
    plt.close(fig)


# ---- Main ------------------------------------------------------------------------------------------


def parse_args(argv):
    ap = argparse.ArgumentParser(description="Draw the circuit coloured by the events each device processed.")
    ap.add_argument("circuit_json", help="circuit export written by specs --json")
    ap.add_argument("stats_json", help="run statistics written by specs --stats=ports,devices")
    ap.add_argument("-o", "--output", required=True, metavar="OUT_BASENAME",
                    help="output base name (writes OUT.dot, OUT.svg, OUT.png)")
    ap.add_argument("--format", default="svg,png", help="comma-separated: svg, png (default: svg,png)")
    ap.add_argument("--suppression", action="store_true",
                    help="draw a disc per device coloured by suppressed / processed")
    ap.add_argument("--no-field", action="store_true", help="white background instead of the activity field")
    ap.add_argument("--dot-only", action="store_true", help="write only the DOT file (standard library only)")
    ap.add_argument("--title", help="figure title (default: the stats file name)")
    ap.add_argument("--net-labels", action="store_true", help="label edges with the net name and emitted count")
    return ap.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)
    try:
        formats = [f.strip().lower() for f in args.format.split(",") if f.strip()]
        for f in formats:
            if f not in ("svg", "png"):
                raise InputError("unknown format '%s' (svg, png)" % f)
        circuit = load_json(args.circuit_json, "circuit")
        stats = load_json(args.stats_json, "stats")
        check_inputs(circuit, stats)
        act, netdata = compute_activity(circuit, stats)
        dot_text = build_graph(circuit, act, netdata, args.net_labels)
        with open(args.output + ".dot", "w") as f:
            f.write(dot_text)
        if args.dot_only:
            return 0
        missing = missing_render_deps()
        if missing:
            print("activity_map: cannot render, missing: %s (%s.dot was written; --dot-only skips rendering)"
                  % (", ".join(missing), args.output), file=sys.stderr)
            return 3
        render(dot_text, circuit, stats, act, args.output, formats, args.title or os.path.basename(args.stats_json),
               args.suppression, not args.no_field)
    except InputError as e:
        print("activity_map: error: %s" % e, file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
