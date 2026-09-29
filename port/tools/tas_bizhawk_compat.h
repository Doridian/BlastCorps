/* Force-included into BizHawk's core for a Linux build (port/tools/tas.sh):
   it is Windows-only in a few places.  The semaphore it pauses on at every VI
   for frame advance isn't needed by a front end that runs freely, and
   neither is the SEH block around main_run or main_toggle_pause's
   MessageBox. */
typedef void *HANDLE;
#define INFINITE 0
#define CreateSemaphore(a, b, c, d) ((HANDLE)0)
#define WaitForSingleObject(a, b) ((void)0)
#define ReleaseSemaphore(a, b, c) ((void)0)
#define __try if (1)
#define __except(x) else
#define MessageBox(a, b, c, d) ((void)0)
