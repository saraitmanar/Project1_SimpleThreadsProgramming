#include "syscall.h"

int main() {
    // Exit with code 42; kernel should print "Process [pid] exits with 42"
    Exit(42);
    return 0; // never reached
}
