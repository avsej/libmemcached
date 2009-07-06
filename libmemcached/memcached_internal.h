/*
 * Summary: Internal functions used by the library. Not for public use!
 * Copy: See Copyright for the status of this software.
 *
 * Author: Trond Norbye
 */

#ifndef LIBMEMCACHED_MEMCACHED_INTERNAL_H
#define LIBMEMCACHED_MEMCACHED_INTERNAL_H

#if defined(BUILDING_LIBMEMCACHED)

#ifdef __cplusplus
extern "C" {
#endif

void libmemcached_free(memcached_st *ptr, void *mem);
void *libmemcached_malloc(memcached_st *ptr, const size_t size);
void *libmemcached_realloc(memcached_st *ptr, void *mem, const size_t size);
void *libmemcached_calloc(memcached_st *ptr, size_t nelem, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* BUILDING_LIBMEMCACHED */
#endif /* LIBMEMCACHED_MEMCACHED_INTERNAL_H */
