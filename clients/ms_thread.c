/*
 * File:   ms_thread.c
 * Author: Mingqiang Zhuang
 *
 * Created on February 10, 2009
 *
 * (c) Copyright 2009, Schooner Information Technology, Inc.
 * http://www.schoonerinfotech.com/
 *
 */

#include "config.h"

#include "ms_thread.h"
#include "ms_setting.h"

/* global variable */
__thread ms_thread_t ms_thread;    /* each thread with a private ms_thread structure */

/* array of thread context structure, each thread has a thread context structure */
static ms_thread_ctx_t *ms_thread_ctx;

/* functions */
static void ms_set_current_time(void);
static void ms_check_sock_timeout(void);
static void ms_clock_handler(const int fd, const short which, void *arg);
static int ms_set_thread_cpu_affinity(int cpu);
static int ms_setup_thread(ms_thread_ctx_t *thread_ctx);
static void *ms_worker_libevent(void *arg);
static void ms_create_worker(void *(*func)(void *), void *arg);


/**
 *  time-sensitive callers can call it by hand with this,
 *  outside the normal ever-1-second timer
 */
static void ms_set_current_time()
{
  struct timeval timer;

  gettimeofday(&timer, NULL);
  ms_thread.curr_time= (rel_time_t)timer.tv_sec;
} /* ms_set_current_time */


/**
 *  used to check whether UDP of command are waiting timeout
 *  by the ever-1-second timer
 */
static void ms_check_sock_timeout(void)
{
  ms_conn_t *c= NULL;
  int time_diff= 0;

  for (int i= 0; i < ms_thread.thread_ctx->nconns; i++)
  {
    c= &ms_thread.conn[i];

    if (c->udp)
    {
      time_diff= (int)(ms_thread.curr_time - c->start_time.tv_sec);

      /* wait time out */
      if (time_diff > SOCK_WAIT_TIMEOUT)
      {
        /* calculate dropped packets count */
        if (c->recvpkt > 0)
        {
          __sync_fetch_and_add(&ms_stats.pkt_drop, c->packets - c->recvpkt);
        }

        __sync_fetch_and_add(&ms_stats.udp_timeout, 1);
        ms_reset_conn(c, true);
      }
    }
  }
} /* ms_check_sock_timeout */


/* if disconnect, the ever-1-second timer will call this function to reconnect */
static void ms_reconn_thread_socks(void)
{
  for (int i= 0; i < ms_thread.thread_ctx->nconns; i++)
  {
    ms_reconn_socks(&ms_thread.conn[i]);
  }
} /* ms_reconn_thread_socks */


/**
 * the handler of the ever-1-second timer
 *
 * @param fd, the descriptors of the socket
 * @param which, event flags
 * @param arg, argument
 */
static void ms_clock_handler(const int fd, const short which, void *arg)
{
  struct timeval t=
  {
    .tv_sec= 1, .tv_usec= 0
  };

  UNUSED_ARGUMENT(fd);
  UNUSED_ARGUMENT(which);
  UNUSED_ARGUMENT(arg);

  ms_set_current_time();

  if (ms_thread.initialized)
  {
    /* only delete the event if it's actually there. */
    evtimer_del(&ms_thread.clock_event);
    ms_check_sock_timeout();
  }
  else
  {
    ms_thread.initialized= true;
  }

  ms_reconn_thread_socks();

  evtimer_set(&ms_thread.clock_event, ms_clock_handler, 0);
  event_base_set(ms_thread.base, &ms_thread.clock_event);
  evtimer_add(&ms_thread.clock_event, &t);
} /* ms_clock_handler */


/**
 * used to bind thread to CPU if the system supports
 *
 * @param cpu, cpu index
 *
 * @return if success, return 0, else return -1
 */
static int ms_set_thread_cpu_affinity(int cpu)
{
  int ret= 0;

#ifdef HAVE_CPU_SET_T
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(cpu, &cpu_set);

  if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) == -1)
  {
    fprintf(stderr, "WARNING: Could not set CPU Affinity, continuing...\n");
    ret= 1;
  }
#else
  UNUSED_ARGUMENT(cpu);
#endif

  return ret;
} /* ms_set_thread_cpu_affinity */


/**
 * Set up a thread's information.
 *
 * @param thread_ctx, pointer of the thread context structure
 *
 * @return if success, return 0, else return -1
 */
static int ms_setup_thread(ms_thread_ctx_t *thread_ctx)
{
  ms_thread.thread_ctx= thread_ctx;
  ms_thread.nactive_conn= thread_ctx->nconns;
  ms_thread.initialized= false;
  static int cnt= 0;

  gettimeofday(&ms_thread.startup_time, NULL);

  ms_thread.base= event_init();
  if (ms_thread.base == NULL)
  {
    if (__sync_fetch_and_add(&cnt, 1) == 0)
    {
      fprintf(stderr, "Can't allocate event base.\n");
    }

    return -1;
  }

  ms_thread.conn=
    (ms_conn_t *)malloc((size_t)thread_ctx->nconns * sizeof(ms_conn_t));
  if (ms_thread.conn == NULL)
  {
    if (__sync_fetch_and_add(&cnt, 1) == 0)
    {
      fprintf(
        stderr,
        "Can't allocate concurrency structure for thread descriptors.");
    }

    return -1;
  }
  memset(ms_thread.conn, 0, (size_t)thread_ctx->nconns * sizeof(ms_conn_t));

  for (int i= 0; i < thread_ctx->nconns; i++)
  {
    ms_thread.conn[i].conn_idx= i;
    if (ms_setup_conn(&ms_thread.conn[i]) != 0)
    {
      /* only output this error once */
      if (__sync_fetch_and_add(&cnt, 1) == 0)
      {
        fprintf(stderr, "Initializing connection failed.\n");
      }

      return -1;
    }
  }

  return 0;
} /* ms_setup_thread */


/**
 * Worker thread: main event loop
 *
 * @param arg, the pointer of argument
 *
 * @return void*
 */
static void *ms_worker_libevent(void *arg)
{
  ms_thread_ctx_t *thread_ctx= (ms_thread_ctx_t *)arg;

  /**
   * If system has more than one cpu and supports set cpu
   * affinity, try to bind each thread to a cpu core;
   */
  if (ms_setting.ncpu > 1)
  {
    ms_set_thread_cpu_affinity(thread_ctx->thd_idx % ms_setting.ncpu);
  }

  if (ms_setup_thread(thread_ctx) != 0)
  {
    exit(1);
  }

  /* each thread with a timer */
  ms_clock_handler(0, 0, 0);

  event_base_loop(ms_thread.base, 0);

  return NULL;
} /* ms_worker_libevent */


/**
 * Creates a worker thread.
 *
 * @param func, the callback function
 * @param arg, the argument to pass to the callback function
 */
static void ms_create_worker(void *(*func)(void *), void *arg)
{
  pthread_t thread;
  pthread_attr_t attr;
  int ret;

  pthread_attr_init(&attr);

  if ((ret= pthread_create(&thread, &attr, func, arg)) != 0)
  {
    fprintf(stderr, "Can't create thread: %s.\n", strerror(ret));
    exit(1);
  }
} /* ms_create_worker */


/* initialize threads */
void ms_thread_init()
{
  ms_thread_ctx=
    (ms_thread_ctx_t *)malloc(
      sizeof(ms_thread_ctx_t) * (size_t)ms_setting.nthreads);
  if (ms_thread_ctx == NULL)
  {
    fprintf(stderr, "Can't allocate thread descriptors.");
    exit(1);
  }

  for (int i= 0; i < ms_setting.nthreads; i++)
  {
    ms_thread_ctx[i].thd_idx= i;
    ms_thread_ctx[i].nconns= ms_setting.nconns / ms_setting.nthreads;

    /**
     *  If only one server, all the connections in all threads
     *  connects the same server. For support multi-servers, simple
     *  distribute thread to server.
     */
    ms_thread_ctx[i].srv_idx= i % ms_setting.srv_cnt;
    ms_thread_ctx[i].tps_perconn= ms_setting.expected_tps
                                  / ms_setting.nconns;
    ms_thread_ctx[i].exec_num_perconn= ms_setting.exec_num
                                       / ms_setting.nconns;
  }

  /* Create threads after we've done all the epoll setup. */
  for (int i= 0; i < ms_setting.nthreads; i++)
  {
    ms_create_worker(ms_worker_libevent, (void *)&ms_thread_ctx[i]);
  }
} /* ms_thread_init */


/* cleanup some resource of threads when all the threads exit */
void ms_thread_cleanup()
{
  if (ms_thread_ctx != NULL)
  {
    free(ms_thread_ctx);
  }
} /* ms_thread_cleanup */