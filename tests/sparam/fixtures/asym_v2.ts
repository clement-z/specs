! SPECS test fixture (tests/sparam/gen_fixtures.py): constant 2-port with 4 different entries.
! SPECS-convention values: S11 = 0.1, S21 = 0.8j, S12 = 0.3, S22 = -0.2
! (written conjugated, engineering convention: S21 is stored as 0 -0.8).
! [Two-Port Data Order] 21_12: S11 S21 S12 S22
! Port[1] = left
! Port[2] = right
[Version] 2.0
# Hz S RI R 50
[Number of Ports] 2
[Two-Port Data Order] 21_12
[Number of Frequencies] 5
[Network Data]
187370286250000.0 0.1 0.0 0.0 -0.8 0.3 0.0 -0.2 0.0
190344417777777.78 0.1 0.0 0.0 -0.8 0.3 0.0 -0.2 0.0
193414489032258.06 0.1 0.0 0.0 -0.8 0.3 0.0 -0.2 0.0
196585218360655.75 0.1 0.0 0.0 -0.8 0.3 0.0 -0.2 0.0
199861638666666.66 0.1 0.0 0.0 -0.8 0.3 0.0 -0.2 0.0
[End]
