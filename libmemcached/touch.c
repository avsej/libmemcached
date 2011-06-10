#include "common.h"
#include "touch.h"

memcached_return_t memcached_touch(memcached_st *ptr,
                                   const char *key, size_t key_length,
                                   time_t expiration)
{
  if ((ptr->flags.binary_protocol) == 0)
    return MEMCACHED_NOT_SUPPORTED;

  memcached_return_t rc;

  LIBMEMCACHED_MEMCACHED_TOUCH_START();
  unlikely (memcached_server_count(ptr) == 0)
    return MEMCACHED_NO_SERVERS;

  rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol);
  unlikely (rc != MEMCACHED_SUCCESS)
    return rc;

  protocol_binary_request_touch request= {.bytes= {0}};
  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= PROTOCOL_BINARY_CMD_TOUCH;
  request.message.header.request.extlen= 4;
  request.message.header.request.keylen= htons((uint16_t)(key_length + ptr->prefix_key_length));
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  request.message.header.request.bodylen= htonl((uint32_t)(key_length + ptr->prefix_key_length + request.message.header.request.extlen));
  request.message.body.expiration = htonl((uint32_t) expiration);

  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, x);
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
  }

  LIBMEMCACHED_MEMCACHED_TOUCH_END();

  return rc;
}
