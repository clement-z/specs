# B-064: "Error: Cannot open the file." does not name the file
severity: low | category: cli | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/devices/value_list_source.h:58-61, src/devices/electrical_value_list_source.h:55-58
## Problem
A missing values file aborts with a generic message, which makes failures like `cir/paul_3ring_puf` opaque.
## Evidence
Golden `cir/paul_3ring_puf/stderr.txt`: `Error: Cannot open the file.`
## Expected
The message names the path.
## Suggested fix
In both files:
```
-            cerr << "Error: Cannot open the file." << endl;
+            cerr << "Error: Cannot open the file \"" << filename << "\"." << endl;
```
## Golden impact
`cir/paul_3ring_puf` stderr becomes `Error: Cannot open the file "paul_puf_bitstream_158b_3wl.csv".` Re-record that case. Nothing else changes.
## Related
B-053.

- Discovery seeds: S3.30 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-18.
## Verification
Code reading. This is safe because it only touches the text of an error path.
