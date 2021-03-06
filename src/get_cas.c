/* Copyright (C) 2008 Patrick Galbraith, Brian Aker

  See COPYING file found with distribution for license.

*/

#include <mysql.h>
#include <string.h>

#include <stdio.h>

#include <libmemcached/memcached.h>
#include "common.h"

my_bool memc_get_cas_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
uint64_t memc_get_cas(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void memc_get_cas_deinit(UDF_INIT *initid);

my_bool memc_get_cas_by_key_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
uint64_t memc_get_cas_by_key(UDF_INIT *initid, UDF_ARGS *args,
                      __attribute__ ((unused)) char *is_null,
                      __attribute__ ((unused)) char *error);
void memc_get_cas_by_key_deinit(UDF_INIT *initid);


my_bool memc_get_cas_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
  unsigned int x;
  memcached_return rc;
  memc_function_st *container;


  /* this is how to fail */
  if (args->arg_count != 1)
  {
    strncpy(message,
            "one argument must be supplied: memc_get_cas('<key>').",
            MYSQL_ERRMSG_SIZE);
    return 1;
  }

  args->arg_type[0]= STRING_RESULT;

  initid->max_length= MEMC_UDF_MAX_SIZE;
  container= calloc(1, sizeof(memc_function_st));

  /* Init the memcached_st we will use for this pass */
  rc= memc_get_servers(&container->memc);
  memcached_result_create(&container->memc, &container->results);

  initid->ptr= (char *)container;

  return 0;
}

/*
  memc_get_cas
  get cached object, takes hash-key arg
*/
uint64_t memc_get_cas(UDF_INIT *initid, UDF_ARGS *args,
                __attribute__ ((unused)) char *is_null,
                __attribute__ ((unused)) char *error)
{
  uint64_t cas;
  char *value;
  unsigned long length= 0;
  memcached_return rc;

  memc_function_st *container= (memc_function_st *)initid->ptr;

  rc= memcached_mget(&container->memc, args->args, (size_t *)args->lengths, 1);

  memcached_fetch_result(&container->memc, &container->results, &rc);

  length= memcached_result_length(&container->results);

  if (! length)
  {
    cas= 0;
  }
  value= memcached_result_value(&container->results);

  cas= memcached_result_cas(&container->results);
  return cas;
}

/* de-init UDF */
void memc_get_cas_deinit(UDF_INIT *initid)
{
  /* if we allocated initid->ptr, free it here */
  memc_function_st *container= (memc_function_st *)initid->ptr;

  memcached_result_free(&container->results);
  memcached_free(&container->memc);
  free(container);

  return;
}

my_bool memc_get_cas_by_key_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
  memcached_return rc;
  memc_function_st *container;

  /* this is how to fail */
  if (args->arg_count != 2)
  {
    strncpy(message,
            "2 arguments must be supplied: memc_get_cas_by_key('<master key>', '<key>')",
            MYSQL_ERRMSG_SIZE);
    return 1;
  }

  args->arg_type[0]= args->arg_type[1]= STRING_RESULT;

  initid->max_length= MEMC_UDF_MAX_SIZE;
  container= calloc(1,sizeof(memc_function_st));

  /* Init the memcached_st we will use for this pass */
  rc= memc_get_servers(&container->memc);
  memcached_result_create(&container->memc, &container->results);

  initid->ptr= (char *)container;

  return 0;
}

/*
  memc_get_cas_by_key
  get item cas value, takes key arg
*/
uint64_t memc_get_cas_by_key(UDF_INIT *initid, UDF_ARGS *args,
                      __attribute__ ((unused)) char *is_null,
                      __attribute__ ((unused)) char *error)
{
  /* how do I utilise this? Print out in case of error? */
  memcached_return rc;
  unsigned long length= 0;
  char *value;
  char **keys;
  size_t *lengths;
  uint64_t cas;
  memcached_result_st *results;

  keys= args->args;
  keys++;
  lengths= (size_t*)args->lengths;
  lengths++;

  memc_function_st *container= (memc_function_st *)initid->ptr;
  memcached_result_create(&container->memc, &container->results);

  rc= memcached_mget_by_key(&container->memc,
                              args->args[0],
                              (size_t )args->lengths[0],
                              keys,
                              lengths,
                              args->arg_count - 1);

  memcached_fetch_result(&container->memc, &container->results, &rc);
  length= memcached_result_length(&container->results);
  if (! length)
  {
    cas= 0;
  }
  value= memcached_result_value(&container->results);
  cas= memcached_result_cas(&container->results);

  return cas;
}

/* de-init UDF */
void memc_get_cas_by_key_deinit(UDF_INIT *initid)
{
  /* if we allocated initid->ptr, free it here */
  memc_function_st *container= (memc_function_st *)initid->ptr;

  memcached_result_free(&container->results);
  memcached_free(&container->memc);
  free(container);

  return;
}

