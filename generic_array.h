#ifndef GENERIC_ARRAY_H
#define GENERIC_ARRAY_H


#include <stdlib.h>
#include <stdint.h>



// What about alignment???


// XXX: Macro abuse incoming


#define DECL_ARRAY_OF(type,capacity) \
#  typedef struct {  \
#    uint8_t raw[sizeof((type)) * (capacity)]; \
#    size_t cap; \
#    size_t filled; \
#  } type ## _array


#define ARR_TYPENAME(type) type ## _array
      

//
// List operations
//


// FIXME: runtime checks or no??
#define DEFINE_GET(type) \
# (type) get_ ## type ## _array(const ARR_TYPENAME(type) arr, size_t index) { \
#   return ((type*)arr)[index]; \
# }


//
// Stack operations
//

#endif // GENERIC_ARRAY_H
