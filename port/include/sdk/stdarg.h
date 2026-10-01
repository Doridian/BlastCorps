/* The port's own SDK headers (PR/ultratypes.h has the why): variadic
   arguments, the compiler's. */
#ifndef PORT_SDK_STDARG_H
#define PORT_SDK_STDARG_H

#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_arg __builtin_va_arg
#define va_end __builtin_va_end

#endif
