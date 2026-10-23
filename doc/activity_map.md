# Activity map

`utils/activity_map.py` draws the flattened circuit with each device coloured by the number of events it
processed during a run, so the places where the simulation spends its events are visible at a glance. It
reads two files written by one `specs` run; nothing in the simulator changes.

## What it shows

- One node per device (name on the first line, device type on the second), filled with the activity colour.
- One cluster (rounded box, labelled with the instance path) per subcircuit instance, nested for nested
  instances. Devices are flattened in the circuit export; the instance path is the part of the name between
  `ROOT/` and the device (`ROOT/X2/X1/WG1` is device `WG1` in instance `X1` of instance `X2`).
- One edge per connection. Direction comes from the stats: from the device that writes the net to the
  others. A net with two devices that both write it (bidirectional) gets a double arrow; a net whose writer
  is unknown gets a plain line. A net with a single device (circuit port such as `in`, `out`, `drop`) ends in
  a small terminal label with the net name. A net with more than two devices goes through a junction point.
- A background field coloured with the same scale as the nodes (below).
- Legend: a colour bar "processed events per device (linear)", and a box with the run totals (analysis, end
  time, output ports, scheduled, processed, emitted, suppressed by abstol / reltol / snap, coalesced,
  `event_cap_reached`) and the number of devices drawn.

Activity of a device = sum of `processed` over the stats ports it writes. Devices without output ports
(probes, ...) have activity 0. The suppression ratio of a device is
(abstol + reltol + snap suppressed) / processed (0 when nothing was processed).

## Producing the inputs

```
specs -f net.cir -o traces/run --json traces/run.json --stats=ports,devices
```

writes `traces/run.json` (circuit) and `traces/run.stats.json` (statistics). The `ports` section is required;
`devices` is optional and not used by the map. The tool checks `format == "specs-run-stats"` and
`format_version == 1`. Writers in the stats that match no circuit element give one warning on stderr.

## Running

With pixi (the `py` environment has numpy, matplotlib and graphviz):

```
pixi run -e py python -I utils/activity_map.py traces/run.json traces/run.stats.json -o traces/map
```

Without pixi: any Python >= 3.9 with numpy and matplotlib, and the Graphviz `dot` binary on the PATH.
The core uses the standard library only.

| Option | Effect |
|---|---|
| `-o OUT` | output base name (required): writes `OUT.dot`, `OUT.svg`, `OUT.png` |
| `--format svg,png` | formats to render (default both) |
| `--suppression` | solid disc on the top-right corner of each device, coloured by suppressed / processed, with its own colour bar |
| `--field-spread S` | spread of the background field, in median node widths (default 0.5, see below) |
| `--no-field` | white background instead of the background field |
| `--dot-only` | write only `OUT.dot`; imports no third-party module and does not call `dot` |
| `--title TEXT` | figure title (default: the stats file name) |
| `--net-labels` | label each edge with the net name and its emitted count |

Exit codes: 0 ok; 2 bad input (missing file, wrong format or version, no `ports` section); 3 a rendering
dependency (numpy, matplotlib, `dot`) is missing: the message names it and `OUT.dot` is kept. `dot` is used
for the layout only; the drawing is done with matplotlib. SVG text stays text (device names are searchable)
and the SVG is reproducible (fixed hash salt, no date).

## Colour scales

- Activity: linear from 0 to the maximum device activity, white at 0, then seven blue steps
  (`#cde2fb #9ec5f4 #6da7ec #3987e5 #256abf #184f95 #0d366b`), interpolated linearly in RGB. Label text is dark
  on light fills and white on fills darker than the midpoint of the ramp.
- Suppression: a second ramp (white, then `#fbd3c0 #f4a07c #eb6834 #b8481d #7a2c0f`) over the ratio 0..1,
  fixed (not normalised to the run).

## Background field

The background is 0 (white) away from the devices, rises around each device and blends between devices that
are close. With x_i the centre of device node i and a_i its activity:

    f(x) = sum_i a_i K_i(x) / (sum_i K_i(x) + K0),   K_i(x) = exp(-|x - x_i|^2 / (2 sigma^2))

- sigma = `--field-spread` x the median node width (default 0.5, roughly 50 to 70 points for the usual node boxes),
  so the activity of a device spreads over a distance of the order of one node. Larger values blend more
  distant devices, smaller values give isolated spots.
- K0 = exp(-2): at 2 sigma from a lone device the field is half its activity; next to a device it is close to
  that device's activity; where all K_i are much smaller than K0 (devices further than about 3 sigma, i.e.
  about two node widths) it is 0. Two neighbouring nodes (about 1.5 node widths apart) blend smoothly;
  components that are not close are separated by white.
- Only devices count (not terminals or junctions). The field is evaluated on a grid of about 400 pixels on the
  longest side, drawn with the same colour map and 0..max normalisation as the nodes, so the colour bar is the
  scale of both the field and the nodes (linear). The view extends past the graph by 2 sigma so the fade to 0
  is visible.

## Limits

- The map is static: counters are totals over the whole run. Time windows would need per-window counters in
  the simulator and are a later feature.
- Activity is per device (sum over its output ports); the per-wavelength and per-port detail of the stats is
  not drawn.
- Layout is Graphviz `rankdir=LR`; long chains give wide figures (a 10-ring CROW is about 4:1). Node boxes are
  sized to their text. Folding or re-laying-out long graphs is not done here: the layout is to come from a
  schematic inferred from the netlist.
- The suppression disc is centred on the top-right corner of the node box, so it never covers the name or
  type line (the box width keeps a margin around the text that is larger than the disc radius).
