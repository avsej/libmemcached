#include <brian.h>
/*
  Sample test application.
*/
#include <assert.h>
#include <memcached.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_test(void)
{
  memcached_st memc;

  (void)memcached_init(&memc);
  memcached_deinit(&memc);
}

void allocation_test(void)
{
  memcached_st *memc;
  memc= memcached_init(NULL);
  assert(memc);
  memcached_deinit(memc);
}

void set_test(void)
{
  memcached_st *memc;
  memcached_return rc;
  char *key= "foo";
  char *value= "when we sanitize";

  memc= memcached_init(NULL);
  assert(memc);
  rc= memcached_set(memc, key, strlen(key), 
                    value, strlen(value),
                    (time_t)0, (uint16_t)0);
  assert(rc == MEMCACHED_SUCCESS);
  
  memcached_deinit(memc);
}

void add_test(void)
{
  memcached_st *memc;
  memcached_return rc;
  char *key= "foo";
  char *value= "when we sanitize";

  memc= memcached_init(NULL);
  assert(memc);
  rc= memcached_add(memc, key, strlen(key), 
                    value, strlen(value),
                    (time_t)0, (uint16_t)0);
  assert(rc == MEMCACHED_NOTSTORED);
  
  memcached_deinit(memc);
}

void replace_test(void)
{
  memcached_st *memc;
  memcached_return rc;
  char *key= "foo";
  char *value= "when we sanitize";

  memc= memcached_init(NULL);
  assert(memc);
  rc= memcached_replace(memc, key, strlen(key), 
                    value, strlen(value),
                    (time_t)0, (uint16_t)0);
  assert(rc == MEMCACHED_SUCCESS);
  
  memcached_deinit(memc);
}

void delete_test(void)
{
  memcached_st *memc;
  memcached_return rc;
  char *key= "foo";
  char *value= "when we sanitize";

  memc= memcached_init(NULL);
  assert(memc);
  rc= memcached_set(memc, key, strlen(key), 
                    value, strlen(value),
                    (time_t)0, (uint16_t)0);
  assert(rc == MEMCACHED_SUCCESS);

  rc= memcached_delete(memc, key, strlen(key), (time_t)0);
  assert(rc == MEMCACHED_SUCCESS);
  
  memcached_deinit(memc);
}

void flush_test(void)
{
  memcached_st *memc;
  memcached_return rc;

  memc= memcached_init(NULL);
  assert(memc);
  rc= memcached_flush(memc, 0);
  assert(rc == MEMCACHED_SUCCESS);

  memcached_deinit(memc);
}

void get_test(void)
{
  memcached_st *memc;
  memcached_return rc;
  char *key= "foo";
  char *string;
  size_t string_length;
  uint16_t flags;

  memc= memcached_init(NULL);
  assert(memc);
  
  string= memcached_get(memc, key, strlen(key),
                        &string_length, &flags, &rc);

  assert(string_length ==  0);
  assert(!string);

  memcached_deinit(memc);
}

void get_test2(void)
{
  memcached_st *memc;
  memcached_return rc;
  char *key= "foo";
  char *value= "when we sanitize";
  char *string;
  size_t string_length;
  uint16_t flags;

  memc= memcached_init(NULL);
  assert(memc);
  rc= memcached_set(memc, key, strlen(key), 
                    value, strlen(value),
                    (time_t)0, (uint16_t)0);
  assert(rc == MEMCACHED_SUCCESS);

  string= memcached_get(memc, key, strlen(key),
                        &string_length, &flags, &rc);

  assert(string_length == strlen(value));
  assert(!memcmp(string, value, string_length));

  free(string);

  memcached_deinit(memc);
}

void stats_hostname_test(void)
{
  memcached_return rc;
  memcached_stat_st stat;
  rc= memcached_stat_hostname(&stat, NULL,
                              "localhost", 
                              MEMCACHED_DEFAULT_PORT);
}

void increment_test(void)
{
  memcached_st *memc;
  unsigned int new_number;
  memcached_return rc;
  char *key= "number";
  char *value= "0";

  memc= memcached_init(NULL);
  assert(memc);
  rc= memcached_set(memc, key, strlen(key), 
                    value, strlen(value),
                    (time_t)0, (uint16_t)0);
  assert(rc == MEMCACHED_SUCCESS);

  rc= memcached_increment(memc, key, strlen(key),
                          1, &new_number);
  assert(rc == MEMCACHED_SUCCESS);
  assert(new_number == 1);

  rc= memcached_increment(memc, key, strlen(key),
                          1, &new_number);
  assert(rc == MEMCACHED_SUCCESS);
  assert(new_number == 2);

  memcached_deinit(memc);
}

void decrement_test(void)
{
  memcached_st *memc;
  unsigned int new_number;
  memcached_return rc;
  char *key= "number";
  char *value= "3";

  memc= memcached_init(NULL);
  assert(memc);
  rc= memcached_set(memc, key, strlen(key), 
                    value, strlen(value),
                    (time_t)0, (uint16_t)0);
  assert(rc == MEMCACHED_SUCCESS);

  rc= memcached_decrement(memc, key, strlen(key),
                          1, &new_number);
  assert(rc == MEMCACHED_SUCCESS);
  assert(new_number == 2);

  rc= memcached_decrement(memc, key, strlen(key),
                          1, &new_number);
  assert(rc == MEMCACHED_SUCCESS);
  assert(new_number == 1);

  memcached_deinit(memc);
}

int main(void)
{
  /* Clean the server before beginning testing */
  flush_test();

  init_test();
  allocation_test();
  set_test();
  add_test();
  replace_test();
  flush_test();
  delete_test();
  flush_test();
  get_test();
  get_test2();
  stats_hostname_test();

  increment_test();
  decrement_test();

  /* Clean up whatever we might have left */
  flush_test();
  return 0;
}