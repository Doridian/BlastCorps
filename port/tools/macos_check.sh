#!/bin/sh
# macos_check.sh BUILD_DIR: after port/tools/cross-macos-check.cmake's build
# (the object libraries: game_c libaudio_c port_n64 engine_c game_data fe_c
# host), link its objects with ld64.lld
# as the macOS build would, leaving what libSystem, SDL2 and libepoxy give
# undefined (there are no libraries to link here), and list anything else
# still undefined: a name the port should have defined itself (a __wrap_,
# a section symbol, a prefix gone wrong).  docs/PORT.md, "macOS".
B=${1:?usage: macos_check.sh BUILD_DIR}
LD=${LD64:-ld64.lld}
dirs=
for t in game_c libaudio_c port_n64 engine_c game_data fe_c host; do
    [ -d "$B/CMakeFiles/$t.dir" ] || { echo "no objects of $t in $B: build its object libraries first"; exit 1; }
    dirs="$dirs $B/CMakeFiles/$t.dir"
done
# shellcheck disable=SC2086
objs=$(find $dirs -name '*.o' | sort)
# shellcheck disable=SC2086
$LD -arch arm64 -platform_version macos 11.0 11.0 -execute -undefined dynamic_lookup \
    -o "$B/blastcorps.macho" $objs || exit 1
echo "linked $B/blastcorps.macho: $(echo "$objs" | wc -l) objects"
# what the libraries would have to give (and what the stand-in libc's
# headers name instead of macOS's: musl's __errno_location, stderr)
llvm-nm -u "$B/blastcorps.macho" | sed 's/^ *U //' | sort -u > "$B/macos_undefined.txt"
grep -vE '^_(SDL_|epoxy_|gl[A-Z])' "$B/macos_undefined.txt" | grep -vxE \
    '_(abort|atexit|atof|atoi|bzero|calloc|clock_gettime|close|dladdr|errno|__error|exit|fclose|feof|ferror|fflush|fgets|fopen|fprintf|fputc|fputs|fread|free|fseek|ftell|fwrite|getenv|longjmp|malloc|memchr|memcmp|memcpy|memmove|memset|mmap|munmap|nanosleep|open|perror|printf|pthread_[a-z_]+|putchar|puts|qsort|read|realloc|setjmp|sigaction|sigaltstack|snprintf|sprintf|sqrt|sqrtf|sscanf|strchr|strcmp|strcpy|strdup|strerror|strlen|strncmp|strncpy|strrchr|strstr|strtod|strtol|strtoul|strtoull|sysconf|time|unlink|usleep|vfprintf|vsnprintf|write|_exit|__stack_chk_fail|__stack_chk_guard|__stderrp|__stdoutp|__stdinp|floor|floorf|ceil|ceilf|fabs|fabsf|round|roundf|lround|lroundf|pow|powf|sin|sinf|cos|cosf|tan|tanf|atan2|atan2f|fmod|fmodf|exp|log|log2|ldexp|frexp|trunc|truncf|fmin|fmax|fminf|fmaxf|mkdir|stat|fstat|access|rename|signal|raise|getpid|dlsym|dlopen|__assert_rtn|bsearch|ilogbf|vsprintf|_tlv_bootstrap|__errno_location|stderr|stdout|acos|asinf|expf|aligned_alloc|isxdigit|tolower|toupper|strcat|memset_pattern16|__sincosf_stret|__assert_fail)|dyld_stub_binder' \
    > "$B/macos_unexpected.txt"
echo "undefined: $(wc -l < "$B/macos_undefined.txt") (for libSystem, SDL2, libepoxy: $B/macos_undefined.txt)"
if [ -s "$B/macos_unexpected.txt" ]; then
    echo "not a library's (check these):"
    sed 's/^/  /' "$B/macos_unexpected.txt"
    exit 1
fi
echo "nothing else undefined"
