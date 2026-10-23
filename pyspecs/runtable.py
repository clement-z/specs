"""Reader of the run table of a .step/.mc plan (F-02): the directory <output>.runs/ that `specs -f`
writes for a netlist with .step or .mc lines (doc/netlist.md, ".step and .mc").

The probe columns are NumPy arrays read with mmap_mode="r": selecting runs reads only their rows, so a
table larger than the memory can be filtered and processed run by run.

    from pyspecs.runtable import RunTable
    t = RunTable("traces/delete_me")          # or "traces/delete_me.runs"
    t.runs                                     # one row per run: status, seed, parameters, ...
    df = t.data(n2=2.36)                       # long table of the runs with n2 = 2.36 (status ok)
    for run, df in t.iter_runs(): ...          # one run at a time
    t.draws(runs=[0, 1])                       # every random draw of runs 0 and 1

Needs numpy and pandas.
"""

import json
import os

import numpy as np
import pandas as pd

DATA_COLUMNS = ["time_s", "signal", "wavelength_m", "power_W", "field_re", "field_im"]
INDEX_COLUMNS = ["run", "status", "exit_code", "seed", "mc_run"]
RANGE_COLUMNS = ["row_start", "row_count", "draw_start", "draw_count", "message"]


def _numeric_or_text(col):
    """A column of runs.csv as numbers (empty cells, e.g. no .mc or a failed run, as NaN), unless it
    holds text (a choice() of strings)"""
    if col.dtype != object:
        return col
    filled = col[col != ""]
    num = pd.to_numeric(filled, errors="coerce")
    if num.isna().any():
        return col
    return pd.to_numeric(col.replace("", np.nan))


class RunTable:
    """The run table in `path` (the .runs directory, or the trace name given to specs -o)."""

    def __init__(self, path):
        if not os.path.isdir(path) and os.path.isdir(path + ".runs"):
            path = path + ".runs"
        self.path = path
        with open(os.path.join(path, "table.manifest.json"), encoding="utf-8") as f:
            self.manifest = json.load(f)
        # stepped parameters, then the drawn top-level .param values (columns of runs.csv)
        self.params = [s["param"] for s in self.manifest["steps"]]
        self.drawn = list(self.manifest["drawn_parameters"])
        self._columns = {}
        self._runs = None

    # --- index
    @property
    def runs(self):
        """The run index (runs.csv): one row per run (the last line of a run recorded twice, e.g.
        failed then resumed), sorted by run, indexed by run."""
        if self._runs is None:
            df = pd.read_csv(os.path.join(self.path, "runs.csv"), keep_default_na=False,
                             dtype={"status": str, "message": str, "seed": "uint64"})
            for name in ["mc_run"] + self.params + self.drawn:
                df[name] = _numeric_or_text(df[name])
            df = df.drop_duplicates("run", keep="last").sort_values("run").set_index("run", drop=False)
            df.index.name = None
            self._runs = df
        return self._runs

    def select(self, runs=None, status="ok", **params):
        """The rows of `runs` (index) for the given runs (list of run numbers, None: all), status
        ("ok", "failed", "crashed", or None for any) and parameter values: name=value, or
        name=callable(series) -> bool series (e.g. L=lambda x: x > 1e-4)."""
        df = self.runs
        if runs is not None:
            df = df[df["run"].isin(list(np.atleast_1d(runs)))]
        if status is not None:
            df = df[df["status"] == status]
        for name, value in params.items():
            if name not in df.columns:
                raise KeyError("no parameter %r in the run table (parameters: %s)"
                               % (name, ", ".join(self.params + self.drawn)))
            df = df[value(df[name])] if callable(value) else df[df[name] == value]
        return df

    # --- columns
    def column(self, name):
        """A column of the probe table (time_s, signal, ...; run) or of the draws (draw_run,
        draw_name, draw_value), memory-mapped"""
        if name not in self._columns:
            arr = np.load(os.path.join(self.path, name + ".npy"), mmap_mode="r")
            self._columns[name] = arr
        return self._columns[name]

    def _names(self, file):
        with open(os.path.join(self.path, file), encoding="utf-8") as f:
            return f.read().splitlines()

    @property
    def signal_names(self):
        return self._names("signal_names.txt")

    @property
    def draw_names(self):
        return self._names("draw_names.txt")

    # --- data
    def run_data(self, run, signals=None):
        """The probe rows of one run (DataFrame: time_s, signal (name), wavelength_m, power_W,
        field_re, field_im)"""
        row = self.runs.loc[run]
        return self._rows(int(row["row_start"]), int(row["row_count"]), signals)

    def _rows(self, start, count, signals):
        sl = slice(start, start + count)
        names = np.asarray(self.signal_names, dtype=object)
        df = pd.DataFrame({c: np.asarray(self.column(c)[sl]) for c in DATA_COLUMNS})
        df["signal"] = names[df["signal"].to_numpy()] if count else pd.Series([], dtype=object)
        if signals is not None:
            df = df[df["signal"].isin(list(np.atleast_1d(signals)))]
        return df

    def iter_runs(self, runs=None, status="ok", signals=None, **params):
        """(run row of `runs`, its probe rows) for each selected run, one run in memory at a time"""
        for _, row in self.select(runs, status, **params).iterrows():
            yield row, self._rows(int(row["row_start"]), int(row["row_count"]), signals)

    def data(self, runs=None, status="ok", signals=None, **params):
        """One long DataFrame of the selected runs: run, the parameter columns (stepped, drawn,
        seed, mc_run), then the probe columns. Reads only the rows of those runs."""
        sel = self.select(runs, status, **params)
        keep = ["run", "seed", "mc_run"] + self.params + self.drawn
        parts = []
        for _, row in sel.iterrows():
            df = self._rows(int(row["row_start"]), int(row["row_count"]), signals)
            for name in reversed(keep):
                df.insert(0, name, row[name])
            parts.append(df)
        if not parts:
            return pd.DataFrame(columns=keep + DATA_COLUMNS)
        return pd.concat(parts, ignore_index=True)

    def draws(self, runs=None):
        """Every random draw (DataFrame: run, name (stream name), value), optionally of some runs
        only (their draw ranges are read)"""
        names = np.asarray(self.draw_names, dtype=object)
        sel = self.runs if runs is None else self.runs[self.runs["run"].isin(list(np.atleast_1d(runs)))]
        parts = []
        for _, row in sel.iterrows():
            sl = slice(int(row["draw_start"]), int(row["draw_start"]) + int(row["draw_count"]))
            parts.append(pd.DataFrame({
                "run": np.asarray(self.column("draw_run")[sl]),
                "name": names[np.asarray(self.column("draw_name")[sl])] if row["draw_count"] else [],
                "value": np.asarray(self.column("draw_value")[sl]),
            }))
        if not parts:
            return pd.DataFrame(columns=["run", "name", "value"])
        return pd.concat(parts, ignore_index=True)
