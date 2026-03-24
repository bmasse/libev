// a single header file is required
#include <ev++.h>

#include <stdio.h> // for puts
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>
#include <iostream>


static ev_async papi_ev_async;
ev_async* GetPapiEvAsync(){return &papi_ev_async;}

pthread_mutex_t gLoopMutexCall;
void LoopMutexCallRelease(void)
{
  pthread_mutex_unlock(&gLoopMutexCall);

}
void LoopMutexCallAcquire(void)
{
  pthread_mutex_unlock(&gLoopMutexCall);

}


// every watcher type has its own typedef'd struct
// with the name ev_TYPE
ev_io stdin_watcher;
ev_timer timeout_watcher;
ev_timer repetitive_timeout_watcher;
ev_timer again_timeout_watcher;

int
fdprintf ( int fd, size_t bufmax, const char * fmt, ... )
{
  char * buffer;
  int n;
  va_list ap;

  buffer = ( char * ) malloc ( bufmax );
  if ( !buffer )
    return 0;

  va_start ( ap, fmt );
  n = vsnprintf ( buffer, bufmax, fmt, ap );
  va_end ( ap );

  write ( fd, buffer, n );
  free ( buffer );
  return n;
}

// all watcher callbacks have a similar signature
// this callback is called when data is readable on stdin
static void
stdin_cb (EV_P_ ev_io *w, int revents)
{
  const bool blockingcode = false;
  char buffer[256];
  size_t len = sizeof(buffer)-1;
  memset(buffer,0, len+1);

  puts ("stdin ready\n");
  if ( blockingcode )
    for (int i=0;i<2;i++)
    {
      read(w->fd, buffer, len);
      fdprintf(w->fd, len, "--- %s\n", buffer);
    }
  else
  {
    read(w->fd, buffer, len);
    fdprintf(w->fd, len, "%s\n", buffer);
  }
  // for one-shot events, one must manually stop the watcher
  // with its corresponding stop function.
  // ev_io_stop (EV_A_ w);

  // this causes all nested ev_run's to stop iterating
  //ev_break (EV_A_ EVBREAK_ONE);
}

// another callback, this time for a time-out
static void
timeout_cb (EV_P_ ev_timer *w, int revents)
{
  puts ("timeout\n");
  // this causes the innermost ev_run to stop iterating
  // ev_break (EV_A_ EVBREAK_ONE);
}

// another callback, this time for a time-out
static void
repetitive_timeout_cb (EV_P_ ev_timer *w, int revents)
{
  static unsigned long long counter = 0;
  char buffer[256];
  size_t len = sizeof(buffer)-1;
  memset(buffer,0, len+1);
  snprintf(buffer, len, "Repetitive timeout: # %lld\n", counter++);
  puts (buffer);
  printf("number of loop iterations %d\n", ev_iteration(loop));

  if ( (w == &repetitive_timeout_watcher) && false)
  {
    printf("w = &repetitive_timeout_watcher -- %p = %p\n", w, &repetitive_timeout_watcher);
  }
  ev_timer_stop(loop, w);
  ev_timer_set(w, 1.0, 0);
  ev_timer_start(loop, w);
}

// another callback, this time for a time-out
static void
again_timeout_cb (EV_P_ ev_timer *w, int revents)
{
  static unsigned long long counter = 0;
  char buffer[256];
  size_t len = sizeof(buffer)-1;
  memset(buffer,0, len+1);
  snprintf(buffer, len, "Again timeout: # %lld\n", counter++);
  puts (buffer);

  ev_timer_again(loop, &repetitive_timeout_watcher);
}

void test_io(struct ev_loop *loop)
{
  // initialise an io watcher, then start it
  // this one will watch for stdin to become readable
  ev_io_init (&stdin_watcher, stdin_cb, /*STDIN_FILENO*/ 0, EV_READ);
  ev_io_start (loop, &stdin_watcher);
}

void test_onetime_timer(struct ev_loop *loop)
{
  // initialise a timer watcher, then start it
  // simple non-repeating 5.5 second timeout
  ev_timer_init (&timeout_watcher, timeout_cb,5.5, 0.);
  ev_timer_start (loop, &timeout_watcher);
}

void test_repetitive_timer(struct ev_loop *loop)
{
  // initialise a timer watcher, then start it
  // simple non-repeating 5.5 second timeout
  ev_timer_init (&repetitive_timeout_watcher, repetitive_timeout_cb, 1.0, 0.);
  ev_timer_start (loop, &repetitive_timeout_watcher);
}

void test_again_timer(struct ev_loop *loop)
{
  // initialise a timer watcher, then start it
  // repeating every 60 seconds timeout
  ev_timer_init (&again_timeout_watcher, again_timeout_cb, 1.0, 0.);
  again_timeout_watcher.repeat = 1.;
  ev_timer_again(loop, &again_timeout_watcher);
}
/******************************************************************************/
static void *mon_thread(void *arg)
{
  struct ev_loop* loop=((struct ev_loop*)arg);
  ev_run (loop, 0);
  return NULL;
}

void LoopThread( struct ev_loop *loop )
{
  static struct ev_loop* ev_loop_ptr = NULL;
  ev_loop_ptr = loop;
  int retVal;
  static pthread_t tid;
  retVal = pthread_create( &(tid), NULL, &mon_thread, (void *)ev_loop_ptr);
  if(retVal > 0){
    printf("Erreur de phtread\n");
  }
}
/******************************************************************************/


/******************************************************************************/
/**
 * Test sur la mémoire et la libération des timers.
 * Hypothèse 1: si on arrête le timer avec ev_timer_stop, on peut libérer la mémoire.
 * 
 * Créer un thread qui génère plein de ev_timer (allocation dynamique avec calloc())
 * On peut voir les données de l'app en faisant Ctrl-Z; cat /proc/PID/stat ou cat /proc
 * La colonne 23 donne 

/******************************************************************************/
static int flux_timer_cb_sousfct( int count)
{
  //printf("%s, count = %d\n", __FUNCTION__, count);
  return count;
}

typedef int (*sousfct)(int);

typedef struct monTimer_s {
  ev_timer timer;
  sousfct fct;
  int val;
}monTimer_S;

static void
flux_timer_cb (EV_P_ ev_timer *w, int revents)
{
  ev_timer_stop(loop, w);
  monTimer_S *monTimer = (monTimer_S *)w;
  monTimer->fct(monTimer->val);
  free((void*)monTimer);
}

static void *memory_thread(void *arg)
{
  int counteur = 0;
  struct ev_loop *loop = (struct ev_loop *)arg;
  printf("sizeof(ev_timer) = %ld\n", sizeof(ev_timer));

  while(true)
  {
    //printf("loop %s\n", __FUNCTION__);
    monTimer_S *ptr;
    int retVal;
    retVal = usleep(100*1000);
    ptr = (monTimer_S *)calloc(1,sizeof(monTimer_S));
    ptr->fct = flux_timer_cb_sousfct;
    ptr->val = counteur++;
    ev_timer_init(&ptr->timer, flux_timer_cb, 1.0,0);
    ev_timer_start(loop, &ptr->timer);
    ev_async_send (EV_DEFAULT_ GetPapiEvAsync());
    //printf("%s\n", __FUNCTION__);
  }
}

void test_memory_allocation( struct ev_loop *loop)
{
  int retVal;
  static pthread_t tid;
  retVal = pthread_create( &(tid), NULL, &memory_thread, (void *)loop);
  if(retVal > 0){
    printf("Erreur de phtread\n");
  }
}
/******************************************************************************/

/******************************************************************************/
// Test presence of epool
/******************************************************************************/
void test()
{
   struct ev_loop *epoller = ev_loop_new (EVBACKEND_EPOLL | EVFLAG_NOENV);

   if (!epoller)
   {
     printf ("no epoll found here, maybe it hides under your chair\n");
     exit (1);
   }
   if( ev_is_default_loop(epoller))
   {
     printf("epoller is a default loop\n");
   }
   else {
         printf("epoller is not a default loop\n");

   }
}
/******************************************************************************/
// Async Event
/******************************************************************************/
static void papi_handle_async_event(EV_P_ ev_async *w, int revents)
{
    std::cout << "papi_handle_async_event" << std::endl;
}

void testAsyncEvent(EV_P)
{
  static ev_async papi_ev_async;
  // Initialize the Papi async watcher.
  ev_async_init(&papi_ev_async, papi_handle_async_event);
  ev_async_start(loop, &papi_ev_async);
}

/******************************************************************************/
// Main
/******************************************************************************/
pthread_mutex_t gLoopMutex = PTHREAD_MUTEX_INITIALIZER;

void LoopMutexRelease(EV_P)
{
  //printf("%s\n", __FUNCTION__);
  pthread_mutex_unlock(&gLoopMutex);
}
void LoopMutexAcquire(EV_P)
{
  //printf("%s\n", __FUNCTION__);
  pthread_mutex_unlock(&gLoopMutex);
}

int
main (void)
{
  // use the default event loop unless you have special needs
  struct ev_loop *loop = EV_DEFAULT;
  printf("\npid is %d\n", getpid());
  ev_set_loop_release_cb(loop, LoopMutexRelease,LoopMutexAcquire);
  testAsyncEvent(loop);
  test_io(loop);
  //test_onetime_timer(loop);
  //test_repetitive_timer(loop);
  test_again_timer(loop);
  test_memory_allocation(loop);

#if 0
  LoopThread(loop);

  while(true)
  {
    sleep(5);
    printf("\npid is %d\n", getpid());
  };
#else
  // now wait for events to arrive
  while(true)
  {
    int retVal;
    retVal = ev_run (loop, 0);
    //printf("No more watcher, ev_run() return with %d\n", retVal);
  }
#endif  
  // break was called, so exit
  return 0;
}