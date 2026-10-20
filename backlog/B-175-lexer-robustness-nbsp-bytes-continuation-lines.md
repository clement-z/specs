# B-175: Lexer robustness: NBSP bytes, continuation lines, last line without EOL, `.end`
severity: low | category: parser | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parser.l:15, :516-521, :526-531, :534
## Problem
- `WS [ \xC2\xA0\t]` matches the UTF-8 bytes 0xC2 and 0xA0 individually, so a stray 0xC2 is treated as whitespace.
- Continuation requires `+` in column 0, and a comment line between continued lines breaks it. The error message
  then quotes the comment.
- On a last line without EOL, errors quote the *previous* line.
- `.end` does not stop parsing.
## Evidence
- `cwsrc1\xC2 a ...` gives rc 0 (element `CWSRC1`).
- `wg1 a b` / `* the length` / `+ length=1e-3` gives `syntax error -- offending token: "length" -- while parsing
  line: "* the length"`.
- No-EOL file: `Parsing error (noeol.cir:3)` quoting line 2.
## Expected
Match NBSP as the 2-byte sequence. Allow comments inside continuations (ngspice does). Quote the right line.
## Suggested fix
`WS ([ \t]|\xC2\xA0)`. Make the continuation rule tolerate interleaved `*` lines and leading blanks. Set
`current_line` without requiring trailing context.
## Golden impact
None expected.
## Related
B-113, B-155.

- Discovery seeds: S3.27 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-39.
## Verification
Ran each case on Release.
