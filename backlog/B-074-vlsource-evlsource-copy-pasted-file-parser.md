# B-074: VLSource and EVLSource are copy-pasted (file parser, runner)
severity: low | category: style/maintainability | effort: M | fix-class: needs-design | verdict: CONFIRMED
location: src/devices/value_list_source.{h,cpp}, src/devices/electrical_value_list_source.{h,cpp}, src/parser/parse_element.cpp:697-834,850-973
## Problem
`diff` shows that EVLSource is a sed-clone of VLSource. They differ only in the value type, the number of columns, and the use of `OpticalOutputPort` versus a direct write. The `create()` functions are also near-duplicates. The clones already diverge:
- VL logs "invalid time for signal emission" while EVL skips silently.
- Neither validates values.
- Both have the first-line bug (B-053).
## Evidence
`diff src/devices/value_list_source.h src/devices/electrical_value_list_source.h`: about 20 differing lines out of 140.
## Expected
One CSV reader and one runner template.
## Suggested fix
Factor a `ValueListFile` reader returning `vector<vector<double>>` (with header and comment handling), plus a `ValueListSourceBase<T>` template, and share the list-parsing helper in parse_element.cpp. Several layouts are possible, which is why this is needs-design.
## Golden impact
None if behaviour is preserved.
## Related
B-053, B-063, B-064.

- Review ids: F-A3-28.
## Verification
diff output.
