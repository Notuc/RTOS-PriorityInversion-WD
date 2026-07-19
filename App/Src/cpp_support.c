// Prevents linker errors when mixing C and C++
#include <sys/types.h>

// Heap end symbol from linker script
extern char _end;

caddr_t _sbrk(int incr) {
  static char *heap_end = &_end;
  char *prev = heap_end;
  heap_end += incr;
  return (caddr_t)prev;
}

// Called if a pure virtual function is invoked —
// should never happen but linker requires the symbol
void __cxa_pure_virtual(void) {
  while (1)
    ; // trap — add UART log here in debug builds
}
