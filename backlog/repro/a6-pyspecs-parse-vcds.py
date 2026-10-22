"""A6 repro: run pyspecs.specsparse.parse_vcd_dataframe over VCD files.

usage: python3 -I a6-pyspecs-parse-vcds.py <repo-root-containing-pyspecs/> file1.vcd [file2.vcd ...]
needs: numpy pandas matplotlib pyDigitalWaveTools (tested: pandas 2.3.3, numpy 2.0.2, pyDigitalWaveTools 1.2, py3.9)
Prints, per file, the detected simulation type, the dataframe shape, or the exception raised.
"""
import contextlib
import io
import sys
import traceback
import warnings

sys.path.insert(0, sys.argv[1])
from pyspecs.specsparse import parse_vcd_dataframe  # noqa: E402

for f in sys.argv[2:]:
    buf = io.StringIO()
    try:
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            with contextlib.redirect_stdout(buf):
                probes, pdet = parse_vcd_dataframe(f)
        sim = [l for l in buf.getvalue().splitlines() if l.startswith("Type of simulation")]
        print(f"{f}: probes={probes.shape} pdet={pdet.shape} {sim} warnings={sorted({x.category.__name__ for x in w})}")
    except Exception as e:  # noqa: BLE001
        tb = traceback.extract_tb(sys.exc_info()[2])[-1]
        print(f"{f}: EXC {type(e).__name__}: {e} ({tb.filename.split('/')[-1]}:{tb.lineno})")
