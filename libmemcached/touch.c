#include "common.h"
#include "touch.h"

memcached_return_t memcached_touch(memcached_st *ptr,
                                   const char *key, size_t key_length,
                                   time_t expiration)
{
  return memcached_touch_by_key(ptr, key, key_length, key, key_length, expiration);
}

memcached_return_t memcached_touch_by_key(memcached_st *ptr,
                                          const char *master_key, size_t master_key_length,
                                          const char *key, size_t key_length,
                                          time_t expiration)
{
  if ((ptr->flags.binary_protocol) == 0)
    return MEMCACHED_NOT_SUPPORTED;

  memcached_return_t rc;
  uint32_t master_server_key;

  LIBMEMCACHED_MEMCACHED_TOUCH_START();
  unlikely (memcached_server_count(ptr) == 0)
    return MEMCACHED_NO_SERVERS;

  unlikely (ptr->flags.verify_key && (memcached_key_test(&key, &key_length, 1) == MEMCACHED_BAD_KEY_PROVIDED))
    return MEMCACHED_BAD_KEY_PROVIDED;

  if (!(master_key && master_key_length))
  {
    master_key = key;
    master_key_length = key_length;
  }
  unlikely (ptr->flags.verify_key && (memcached_key_test((const char * const *)&master_key, &master_key_length, 1) == MEMCACHED_BAD_KEY_PROVIDED))
    return MEMCACHED_BAD_KEY_PROVIDED;

  master_server_key= memcached_generate_hash_with_redistribution(ptr, master_key, master_key_length);

  protocol_binary_request_touch request= {.bytes= {0}};
  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= PROTOCOL_BINARY_CMD_TOUCH;
  request.message.header.request.extlen= 4;
  request.message.header.request.keylen= htons((uint16_t)(key_length + ptr->prefix_key_length));
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  request.message.header.request.bodylen= htonl((uint32_t)(key_length + ptr->prefix_key_length + request.message.header.request.extlen));
  request.message.body.expiration = htonl((uint32_t) expiration);

  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, master_server_key);
  struct libmemcached_io_vector_st vector[]=
  {
    { .length= sizeof(request.bytes), .buffer= request.bytes },
    { .length= ptr->prefix_key_length, .buffer= ptr->prefix_key },
    { .length= key_length, .buffer= key }
  };

  if ((rc= memcached_vdo(instance, vector, 3, true)) != MEMCACHED_SUCCESS)
  {
    memcached_io_reset(instance);
    return (rc == MEMCACHED_SUCCESS) ? MEMCACHED_WRITE_FAILURE : rc;
  }

  LIBMEMCACHED_MEMCACHED_TOUCH_END();

  return rc;
}
