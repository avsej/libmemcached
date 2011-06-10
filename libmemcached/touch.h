/* LibMemcached
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Adjust expiration on an existing key.
 *
 */

#ifndef __LIBMEMCACHED_TOUCH_H__
#define __LIBMEMCACHED_TOUCH_H__

#ifdef __cplusplus
extern "C" {
#endif


LIBMEMCACHED_API
memcached_return_t memcached_touch(memcached_st *ptr,
                                   const char *key, size_t key_length,
                                   time_t expiration);

LIBMEMCACHED_API
memcached_return_t memcached_touch_by_key(memcached_st *ptr,
                                          const char *master_key, size_t master_key_length,
                                          const char *key, size_t key_length,
                                          time_t expiration);




#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_TOUCH_H__ */
