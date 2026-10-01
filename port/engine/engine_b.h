/* Shared by engine-B's hand-written replacements of Rare's handwritten
   engine (hd_code 5BF40, 5CB60, 5FD50, 60D50, 60F60, 77E20, 7F8B0, 89250,
   8A080, 8A2E0; hd_front_end 1B100).

   ENG_COST(n) charges n MIPS instructions: the original's basic blocks on
   the path taken, so the game's pace (and the TAS) stays as it was.  It is
   a placeholder until the shared mechanism (docs/PORT.md, "Replacing the
   engine") is in. */
#ifndef ENGINE_B_H
#define ENGINE_B_H

#include "common.h"

#ifndef ENG_COST
#define ENG_COST(n) ((void)0)
#endif

#endif
