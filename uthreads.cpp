#include "uthreads.h"
#include <signal.h>
#include <deque>
#include <stdlib.h>

typedef struct TCB
{
    unsigned int tid;
    sigjmp_buf env;
    unsigned int quantums;
    unsigned int to_sleep;
    bool paused;
} TCB;

std::deque<TCB *> ready;
std::set<unsigned int> freeIds;
std::map<unsigned int, TCB *> blocked;
TCB *running;
int quantum_time;

void timer_handler(int sig)
{
  for (auto &it = blocked.begin(); it != blocked.end(); it++)
    {
      if (it -> second -> to_sleep != 0)
        {
          if (it -> second -> to_sleep-- == 0 && !paused)
            {
              blocked.erase(it -> second -> to_sleep);
              ready.push_back(it -> second -> to_sleep);
            }

        }
    }
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
  main_thread -> to_sleep = 0;
  main_thread -> paused = false;

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

    quantum_time = quantum_usecs;
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

int uthread_block(int tid)
{
  // TODO: Check what happens if ready is empty
  if (tid == running -> tid)
    {
      TCB *current = running;
      int ret_val = sigsetjmp(current -> env, 1);
      if (ret_val == 0)
        {
          blocked[tid] = current;
          current -> paused = true;
          current = ready.pop_front ();
          current->quantums++;
          running = current;
          siglongjmp (current->env, 1);
        }
    }
    else
    {
      for (const auto& elem : ready)
        {
          if (elem -> tid == tid)
            {
              blocked[tid] = elem;
              ready.erase(elem);
              return 0;
            }
        }
        std::cerr << "thread library error: No thread with ID " << tid << std::endl;
      return -1;
    }
}

int uthread_resume(int tid)
{
  TCB *to_resume = blocked.find(tid);
  if (to_resume != blocked.end())
    {
      to_resume -> paused = false;
      if (to_resume -> to_sleep == 0)
        {
          blocked.erase (to_resume);
          ready.push_back (to_resume);
        }
    }
    else
    {
      if (running -> tid == tid)
        return 0;
      for (const auto& elem : ready)
          if (elem -> tid == tid)
            return 0;
      return -1;
    }
}

int uthread_sleep(int num_quantums)
{
  blocked[tid] = running;
  running -> to_sleep = num_quantums;
  running = ready.pop_front ();
  running->quantums++;
  siglongjmp (running->env, 1);

}

int uthread_get_tid()
{
  return running -> tid;
}