// a single header file is required
#include <ev.h>

#include <stdio.h> // for puts
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <dlfcn.h>

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
  char buffer[256];
  size_t len = sizeof(buffer)-1;
  memset(buffer,0, len+1);

  puts ("stdin ready");
  read(w->fd, buffer, len);
  fdprintf(w->fd, len, "%s", buffer);
  // for one-shot events, one must manually stop the watcher
  // with its corresponding stop function.
  // ev_io_stop (EV_A_ w);

  // this causes all nested ev_run's to stop iterating
  ev_break (EV_A_ EVBREAK_ONE);
}

// another callback, this time for a time-out
static void
timeout_cb (EV_P_ ev_timer *w, int revents)
{
  puts ("timeout");
  // this causes the innermost ev_run to stop iterating
  //ev_break (EV_A_ EVBREAK_ONE);
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
  // this causes the innermost ev_run to stop iterating

  ev_timer_stop(loop, &repetitive_timeout_watcher);
  ev_timer_set(&repetitive_timeout_watcher, 1.0, 0);
  ev_timer_start(loop, &repetitive_timeout_watcher);
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
  // this causes the innermost ev_run to stop iterating

  ev_timer_start(loop, &repetitive_timeout_watcher);
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
  // simple non-repeating 5.5 second timeout
  ev_timer_init (&again_timeout_watcher, again_timeout_cb, 1.0, 0.);
  again_timeout_watcher.repeat = 60.;
  ev_timer_again(loop, &again_timeout_watcher);
}

static void mon_thread(void *arg)
{
  struct ev_loop* loop=*((struct ev_loop*)arg);
  ev_run (loop, 0);
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

int
main (void)
{
 //dlopen ("./libhello.so", RTLD_LAZY);
  // use the default event loop unless you have special needs
  struct ev_loop *loop = EV_DEFAULT;

  test_io(loop);
  test_onetime_timer(loop);
  test_repetitive_timer(loop);
  test_again_timer(loop);

  // now wait for events to arrive
  LoopThread(loop);
  while(true);


  // break was called, so exit
  return 0;
}