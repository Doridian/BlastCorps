#!/bin/sh
# macos_check.sh BUILD_DIR: after port/tools/cross-macos-check.cmake's build
# (the targets n64_link, host and recomp), link its objects with ld64.lld
# as the macOS build would, leaving what libSystem, SDL2 and libepoxy give
# undefined (there are no libraries to link here), and list anything else
# still undefined: a name the port should have defined itself (a __wrap_,
# a section symbol, a prefix gone wrong).  docs/PORT.md, "macOS".
B=${1:?usage: macos_check.sh BUILD_DIR}
LD=${LD64:-ld64.lld}
objs=$(find "$B/CMakeFiles/host.dir" "$B/CMakeFiles/recomp.dir" -name '*.o' | sort)
[ -n "$objs" ] && [ -f "$B/gen/n64.o" ] || { echo "no objects in $B: build n64_link host recomp first"; exit 1; }
# shellcheck disable=SC2086
$LD -arch arm64 -platform_version macos 11.0 11.0 -execute -undefined dynamic_lookup \
    -o "$B/blastcorps.macho" "$B/gen/n64.o" $objs || exit 1
echo "linked $B/blastcorps.macho: $(echo "$objs" | wc -l) host objects and gen/n64.o"
# what the libraries would have to give (and what the stand-in libc's
# headers name instead of macOS's: musl's __errno_location, stderr)
llvm-nm -u "$B/blastcorps.macho" | sed 's/^ *U //' | sort -u > "$B/macos_undefined.txt"
grep -vE '^_(SDL_|epoxy_|gl[A-Z])' "$B/macos_undefined.txt" | grep -vxE \
    '_(abort|atexit|atof|atoi|bzero|calloc|clock_gettime|close|dladdr|errno|__error|exit|fclose|feof|ferror|fflush|fgets|fopen|fprintf|fputc|fputs|fread|free|fseek|ftell|fwrite|getenv|longjmp|malloc|memchr|memcmp|memcpy|memmove|memset|mmap|munmap|nanosleep|open|perror|printf|pthread_[a-z_]+|putchar|puts|qsort|read|realloc|setjmp|sigaction|sigaltstack|snprintf|sprintf|sqrt|sqrtf|sscanf|strchr|strcmp|strcpy|strdup|strerror|strlen|strncmp|strncpy|strrchr|strstr|strtod|strtol|strtoul|strtoull|sysconf|time|unlink|usleep|vfprintf|vsnprintf|write|_exit|__stack_chk_fail|__stack_chk_guard|__stderrp|__stdoutp|__stdinp|floor|floorf|ceil|ceilf|fabs|fabsf|round|roundf|lround|lroundf|pow|powf|sin|sinf|cos|cosf|tan|tanf|atan2|atan2f|fmod|fmodf|exp|log|log2|ldexp|frexp|trunc|truncf|fmin|fmax|fminf|fmaxf|mkdir|stat|fstat|access|rename|signal|raise|getpid|dlsym|dlopen|__assert_rtn|bsearch|ilogbf|vsprintf|_tlv_bootstrap|__errno_location|stderr|stdout)|dyld_stub_binder' \
    > "$B/macos_unexpected.txt"
echo "undefined: $(wc -l < "$B/macos_undefined.txt") (for libSystem, SDL2, libepoxy: $B/macos_undefined.txt)"
if [ -s "$B/macos_unexpected.txt" ]; then
    echo "not a library's (check these):"
    sed 's/^/  /' "$B/macos_unexpected.txt"
    exit 1
fi
echo "nothing else undefined"
