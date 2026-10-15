# Notices

This project's own code is licensed under the GNU Affero General Public
License, version 3 or later: see [LICENSE](LICENSE) and the README's
License section for what that covers.

A built port (the native binary, or the WebAssembly page) contains or links
the third-party material below.  Each entry gives its license and where the
full text is.  The decompiled game and the SDK's C (`blastcorps/src`,
`blastcorps/include/2.0I`, `port/snapshot`) are part of the repository and
are kept in the published port (docs/DISTRIBUTION.md, item 7).

| Component | Where it is | License |
| --- | --- | --- |
| SDL2 | the native build links it; the WebAssembly build uses Emscripten's SDL2 port | zlib |
| libepoxy | the native build's OpenGL loader | MIT |
| musl libc | the WebAssembly build's C library (Emscripten's copy) | MIT |
| Emscripten runtime | the WebAssembly build's toolchain and runtime | MIT or UIUC/NCSA |
| stb_image 2.30 | `port/third_party/stb/stb_image.h` | public domain or MIT, at your choice |
| stb_truetype 1.26 | `port/third_party/stb/stb_truetype.h` | public domain |
| gzip 1.2.4 deflate | `port/third_party/gzip-1.2.4/` (`deflate.c`, `trees.c`, `bits.c`) | GNU GPL, version 2 or later |
| Stardos Stencil | `port/fonts/StardosStencil-Bold.ttf` | SIL Open Font License 1.1 |

---

## SDL2

Copyright (C) Sam Lantinga <slouken@libsdl.org>

```
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
```

## libepoxy

```
Copyright © 2013-2014 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
```

libepoxy's generated code is derived from Khronos's XML registry, which is
under the following license:

```
Copyright (c) 2013 The Khronos Group Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and/or associated documentation files (the
"Materials"), to deal in the Materials without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Materials, and to
permit persons to whom the Materials are furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Materials.

THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
```

## musl libc

Copyright © 2005-2020 Rich Felker, et al. (the authors listed in musl's
`COPYRIGHT`, which also covers the parts licensed under the BSD and other
MIT-compatible terms that it names).

```
Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

## Emscripten

Emscripten (the compiler, the runtime and its ports' build glue) is
available under two licenses: the MIT license and the University of
Illinois/NCSA Open Source License.  The MIT text below is from
`/usr/lib/emscripten/LICENSE` (the emsdk install); the NCSA text is in the
same file, and a user may take either.

```
Copyright (c) 2010-2014 Emscripten authors, see AUTHORS file.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

The musl libc that Emscripten ships (`system/lib/libc/musl/COPYRIGHT`) is the
same MIT text as the musl section above.

## stb_image and stb_truetype

`port/third_party/stb/` is Sean Barrett's stb libraries.  `stb_truetype.h` is
public domain (its header says so).  `stb_image.h` is public domain or MIT, at
your choice; its header carries both, and the MIT text is in the file under
"ALTERNATIVE A - MIT License".

## gzip 1.2.4 (deflate)

`port/third_party/gzip-1.2.4/` carries `deflate.c`, `trees.c` and `bits.c`
from GNU gzip 1.2.4, Copyright (C) 1992-1993 Jean-loup Gailly and the Free
Software Foundation.  They are under the GNU General Public License, version 2
or (at your option) any later version; the full text is `COPYING` in that
directory.  The project's code, which is AGPL-3.0-or-later, may use them under
those terms.

## Stardos Stencil

`port/fonts/StardosStencil-Bold.ttf`, with the Reserved Font Names "Stardos"
and "Stardos Stencil":

```
Copyright (c) 2011 by vernon adams (vern@newtypography.co.uk),
with Reserved Font Names "Stardos" and "Stardos Stencil".
```

It is licensed under the SIL Open Font License, version 1.1; the full text is
`port/fonts/OFL.txt`.
