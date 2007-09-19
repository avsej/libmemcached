#include <memcached.h>

memcached_return memcached_delete(memcached_st *ptr, char *key, size_t key_length,
                                  time_t expiration)
{
  size_t send_length;
  memcached_return rc;
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  unsigned int server_key;

  rc= memcached_connect(ptr);

  if (rc != MEMCACHED_SUCCESS)
    return rc;

  server_key= memcached_generate_hash(key, key_length) % ptr->number_of_hosts;

  if (expiration)
    send_length= snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, 
                          "delete %.*s %u\r\n", key_length, key, expiration);
  else
    send_length= snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, 
                          "delete %.*s\r\n", key_length, key);
  if ((write(ptr->hosts[server_key].fd, buffer, send_length) == -1))
  {
    fprintf(stderr, "failed set on %.*s TCP\n", key_length+1, key);

    return MEMCACHED_WRITE_FAILURE;
  }

  return memcached_response(ptr, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, server_key);
}