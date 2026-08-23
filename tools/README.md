# Analysis tools

Throwaway-quality static analysis used to produce
[../notes/CONFIG-MAP.md](../notes/CONFIG-MAP.md). Kept so the analysis can be
re-run and checked, not because they are good.

| Tool | What it does |
|---|---|
| `fnmap.py FILE [LINE...]` | maps a line number to its enclosing top-level function |
| `cfgmap.py FIELD...` | for each `swed.<FIELD>`, lists the functions that write and read it |

## Read this before trusting the output

`fnmap.py` does **not** parse C. It relies on this codebase's layout: every
top-level definition starts at column 0 and ends with a `}` at column 0. An
earlier version counted braces instead and was **wrong** — `#if`/`#else` blocks
corrupt the count, and it placed `sweph.c:7452` in `swe_get_planet_name` when
that line is in `open_jpl_file`.

Validate before use:

```sh
python3 tools/fnmap.py sweph.c 7452 402 1344 7651
#   7452 -> open_jpl_file
#   402  -> swe_calc
#   1344 -> swe_set_ephe_path
#   7651 -> swi_fixstar_calc_from_record
```

`cfgmap.py` does **not follow pointer aliases**. `struct sid_data *sip =
&swed.sidd;` then `sip->sid_mode = ...` is invisible to it. Enumerate the
aliases separately and chase them by hand:

```sh
grep -rhoE '&swed\.[a-z_]+' *.c | sort | uniq -c | sort -rn
```

That is how the `sidd`, `astro_models` and `topd` writers in CONFIG-MAP.md were
found. Do the same for any field before relying on a "no writers" result.
