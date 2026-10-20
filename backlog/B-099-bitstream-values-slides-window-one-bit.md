# B-099: `bitstream_as_values` slides a window one bit at a time instead of cutting words, and pads a whole extra word
severity: low | category: python | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: utils/analyze_detector_trace.py:373-380
## Problem
`bitstream[i:i+n]` with `i` the word index slides by one bit (should be `i*n:(i+1)*n`) and `n - len % n` appends `n` padding bits when the length is already a multiple of `n`. `create_random_bitstream_file` (used to write `bitstream.txt` for sources) therefore emits overlapping words and an extra trailing zero word.
## Evidence
```
bitstream_as_values([1,0,1,1,0,0,1,0], 2) -> [2, 1, 3, 2, 0]     expected [2, 3, 0, 2]
bitstream_as_values([1,0,1,1], 4)         -> [11, 6]             expected [11]
```
## Expected
Consecutive non-overlapping words; padding only when needed.
## Suggested fix
```
-    padding_length = nbits_per_value - (len(bitstream) % nbits_per_value)
+    padding_length = (-len(bitstream)) % nbits_per_value
...
-        values.append(int(''.join([str(b) for b in bitstream[i:i+nbits_per_value]]), 2))
+        values.append(int(''.join(str(b) for b in bitstream[i*nbits_per_value:(i+1)*nbits_per_value]), 2))
```
If B-098 deletes the file, drop this finding.
## Golden impact
none.
## Related
B-098.

- Review ids: F-A6-24.
## Verification
Ran the function copy on the inputs above.
