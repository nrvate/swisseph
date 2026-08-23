# de200.eph

A JPL planetary ephemeris, tracked here so `tests/jplreal.c` (G15) can open a
real one. Everything else in this directory is Swiss Ephemeris `.se1` data;
this is the only JPL binary.

    source   https://ssd.jpl.nasa.gov/ftp/eph/planets/Linux/de200/lnxm1600p2170.200
             (the file https://www.astro.com/swisseph-download/jplfiles/ links to)
    size     41 MB
    md5      20eab53d5537c63612513678805a2676
    span     1600 .. 2170 CE
    header   "JPL Planetary Ephemeris DE200/DE200"

⚠️ That md5 is **not** the one in upstream's `readme.md`, which lists
`1ef6191b614b2b854adae8675b1b981f`. JPL has republished the file since that
readme was written. Two independent downloads of the current file agree byte
for byte, and it reads correctly: DE200 against the DE441-derived `.se1` files
gives ~0.01" on the inner planets and 0.1-0.37" on the outer ones, which is the
difference between a 1981 ephemeris and a 2020 one rather than a fault.

de200 is the smallest of the four JPL files (de406 is 190 MB, de431 and de441
are 2.6 GB each). It is here to prove the reader works on a genuine file, not
to compute with -- for that, fetch a modern one and point `SE_JPL_FILE` at it.
