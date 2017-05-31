#ifndef BUSYWAIT_H
#define BUSYWAIT_H


#include <stdio.h>
#include <stdbool.h>
#include <time.h>


// The type of 'trying functions', which may fail until a (time-dependent)
// condition is met. A wrapper function will probably be required.
// TODO: docs on param types, retval, etc., postcondition, etc.
typedef bool (try_fn)(void* args, void* retval);


// A parametrizable busy-wait loop with timeout
bool busy_wait_until(struct timeval timeout, try_fn fn, void* args, void* retval);


#endif // BUSYWAIT_H
