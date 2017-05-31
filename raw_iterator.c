#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "raw_iterator.h"


static void check_bounds(raw_iterator* rit, size_t n);


void rit_init(raw_iterator* rit, uint8_t* data, size_t size) {
  rit->data = rit->curr = data; 
  rit->size = size;
}


void rit_reset(raw_iterator* rit) {
  rit->curr = rit->data;
}


void rit_read(raw_iterator* rit, size_t n, void* saved) {
  check_bounds(rit, n);
  memcpy(saved, rit->curr, n);
  rit->curr += n;  
}


void rit_write(raw_iterator* rit, size_t n, void const* src) {
  check_bounds(rit, n);
  memcpy(rit->curr, src, n);
  rit->curr += n;
}


static void check_bounds(raw_iterator* rit, size_t n) {
  // Bounds check
  ptrdiff_t bounds_diff = (rit->curr + n) - rit->data;

  assert(bounds_diff <= rit->size);
}
