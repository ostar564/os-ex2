#include "uthreads.h"
#include <signal.h>
#include <deque>
#include <stdlib.h>

typedef struct TCB
{
    unsigned int tid;
    sigjmp_buf env;
    unsigned int quantums;
} TCB;

std::deque<TCB *> ready;
std::set<unsigned int> freeIds;
std::map<unsigned int, TCB> blocked;
TCB *running;

void timer_handler(int sig)
{
  if (!ready.empty())
  {
    TCB *current = running;
    int ret_val = sigsetjmp(current -> env, 1);
    if (ret_val == 0)
      {
        ready.push_back (current);
        current = ready.pop_front ();
        current->quantums++;
        running = current;
        siglongjmp (current->env, 1);
      }
  }
  if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
  {
    std::cerr << ("system error: Timer didn't start") << std::endl;
    exit(1);
  }
}

int uthread_init(int quantum_usecs)
{
  for (int i = 1; i < MAX_THREAD_NUM; i++)
    freeIds.insert(i);
  TCB *main_thread;
  main_thread -> tid = 0;
  main_thread -> quantums = 1; // TODO: Reassure this starts at 1

  sigsetjmp(main_thread -> env, 1);
  sigemptyset(main_thread -> env -> __saved_mask);

  running = main_thread;
  struct sigaction sa = {0};
  struct itimerval timer;

  // Install timer_handler as the signal handler for SIGVTALRM.
  sa.sa_handler = &timer_handler;
  if (sigaction(SIGVTALRM, &sa, NULL) < 0)
    {
      printf("sigaction error.");
    }

  // Configure the timer to expire after 1 sec... */
  timer.it_value.tv_sec = quantum_usecs / (1e6);        // first time interval, seconds part
  timer.it_value.tv_usec = quantum_usecs % (1e6);        // first time interval, microseconds part

//  // configure the timer to expire every 3 sec after that.
//  timer.it_interval.tv_sec = quantum_usecs / (1e6);    // following time intervals, seconds part
//  timer.it_interval.tv_usec = quantum_usecs % (1e6);    // following time intervals, microseconds part

  // Start a virtual timer. It counts down whenever this process is executing.
  if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
    {
      printf("setitimer error.");
    }

}
