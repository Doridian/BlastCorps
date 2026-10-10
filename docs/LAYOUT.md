# What depends on the N64's layout

Every build of the port keeps each N64-named variable at its N64 address.
The scattered layout (docs/PORT.md, "The scattered layout",
`-DPORT_SCATTER`) moves every variable and every fixed buffer (the
regions) somewhere else, padded, and the
check build (`-DPORT_SCATTER_CHECK=ON`) reports each access that only
works because of the N64's layout: a name or an address next to the data
on the N64 instead of the variable the data is in.  This file is that
list for us.v10, the input for making the port an ordinary C program,
whose variables are wherever the compiler puts them.  It is empty now:
the game's C, the engine's C (`port/engine`) and the host read their data
through the variable it is in.

## How it was made

Three check builds (32-bit native-endian movable, seeds 1, 2 and 3:
`S1`..`S3`) at 8ce6fce; each ran the quick tier's 19 scenarios with
`--gameplay` and the TAS.  Then

```
port/tools/scatter_list.py --static build/sc1-mn32 \
    --run S1:attract build/sc1-mn32 build/sc1-mn32/test/quick/attract/log.txt ... \
    --run S1:TAS build/sc1-mn32 build/sc1-mn32/test/tas-free/log.txt ...
```

(one `--run` per scenario and seed).  No run logged a `scatter:` line,
every quick scenario's gameplay digest is the reference's, and every TAS
played through with 57 platinum, all 125,297 of the log's reads matched
and the reference's gameplay digest (`7ec6ef73a5265afa`): the scattered
builds play the same game as the default ones.  The static report
(port-arena's) has no name the N64 side declares inside another variable.

## How the rows went

The first list (at 4803fbe) had 17 alias, 7 past-end and 37 label rows,
and 256 names inside other variables (241 of them in the asm data).  They
went in these ways:

- **A name inside a variable became its field or element** (`D_802F499A`
  is `D_802F49E0[i]`, the player's bytes `D_80364AF0[n].field`, the Pak's
  `D_8039B628`'s, the scheduler's `D_80315440`'s).  Where the field access
  doesn't compile the same under IDO, the header keeps the `extern` for
  IDO and defines the name as the field under `TARGET_PC` (sched.h's
  pattern).
- **A `T x[1]` or a too-small array got its real size**, and a list's end
  pointer is `&x[n]`.
- **The asm data's names inside its variables are variables of their
  own** (`port/tools/asm2c.py`, `n64_labels`; `LABEL_TYPES` types the
  tables), and the engine's walks across labels say which table they
  go to next.
- **The host reads the game through its variables** (`port/host/sched_vars.h`
  for the scheduler's frame count and level time; the player's and the
  vehicles' coordinates by their own names).
- **Reads past an end that the N64 makes too** stay what they read there,
  through the variable that holds it: `func_8026BCE0` draws an entry
  whose colour indices were never set (stale bytes, up to 0xFF), and
  `port_color_pair` (26570.c) reads the N64's bytes past `D_802F47B0`'s
  tables from `D_802F49F4` and its neighbours.

## The kinds

A row in the list below would be one of these.

- **alias**: a name the C declares as its own that is inside another
  variable (a field, the second byte of a word): in the scattered layout
  it has room of its own, and the container doesn't see what is written
  through it.
- **past end**: an access past the end of a C variable, into what follows
  it on the N64: a `T x[1]` or a `T x` that is really an array or a
  struct, a loop past an array's end.
- **label**: the same past an asm data variable (a label), into the next
  label of the same file.
- **before**: an access before a variable's start (a negative index, or
  a walk that started elsewhere); **address**: an N64 place a variable
  left, reached by an address made otherwise than from the variable.

"N64 layout has" is where the N64 has the byte the access reached (the
name there and the offset); "a walk: then N more" is a site that went on
through N more places (the first is the cause).  A site of `?` has no
line (code the compiler made for several).

## Not covered

- The host's own reads of game memory by address (the renderer's display
  lists and textures, the audio, the loaders) the check doesn't see.  The
  guard (docs/PORT.md, "The guard", `-DPORT_SCATTER_GUARD=ON`) does, for
  the N64's places: with the fixed buffers and the front end's variables
  moved, nothing is left in the first 4 MB, which faults, and the guard
  build plays the quick tier and the TAS as the references.  An access by
  the host from one moved variable into its new neighbour neither sees.
- The front end's variables with `-DPORT_SCATTER_FE=OFF` (it is on by
  default: they move, and the level pool with them, `port/src/overlay.c`
  restoring each from port-arena's list).
- The other versions: jp's `func_8026BCE0` is its own C
  (`port/engine/jp_26570.c`) and still indexes past `D_802F47B0`.

## Summary

| kind | rows |
| --- | --- |

## Names inside other variables (static)

0 names the N64 side declares that are inside another variable (port-arena's report), whether a run reached them or not; `seen` marks those a run did.

| name | inside | C or asm | users | seen |
| --- | --- | --- | --- | --- |

