
//
// hash.c
//
// Copyright (c) 2012 TJ Holowaychuk <tj@vision-media.ca>
//

#include "hash.h"

/*
 * Set hash `key` to `val`.
 */

inline void
hash_set(hash_t *self, char *key, void *val) {
  int ret;
  khiter_t k = kh_put(ptr, self, key, &ret);
  kh_value(self, k) = val;
}

/*
 * Get hash `key`, or NULL.
 */

inline void *
hash_get(hash_t *self, char *key) {
  khiter_t k = kh_get(ptr, self, key);
  return k == kh_end(self) ? NULL : kh_value(self, k);
}

/*
 * Check if hash `key` exists.
 */

inline int
hash_has(hash_t *self, char *key) {
  khiter_t k = kh_get(ptr, self, key);
  return k != kh_end(self);
}

/*
 * Remove hash `key`.
 */

void
hash_del(hash_t *self, char *key) {
  khiter_t k = kh_get(ptr, self, key);
  kh_del(ptr, self, k);
}

