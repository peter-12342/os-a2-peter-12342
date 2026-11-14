
#include "kernel/types.h"
#include "kernel/pinfo.h"
#include "user/user.h"

#define NPROC_TEST 64

// Find a process by PID using our getpinfo syscall.
int
getstats(int pid, struct pinfo *result)
{
  struct pinfo info[NPROC_TEST];
  int count = getpinfo(info, NPROC_TEST);

  if(count < 0)
    return -1;

  for(int i = 0; i < count; i++){
    if(info[i].pid == pid){
      *result = info[i];
      return 0;
    }
  }

  return -1;
}

// Keep a process busy for about 40 global ticks.
void
cpu_worker(void)
{
  int start = uptime();
  volatile uint64 counter = 0;

  while(uptime() - start < 40){
    counter++;
  }

  exit(0);
}

int
main(void)
{
  struct pinfo a1, a2, b1, b2;
  int p1, p2;
  int failures = 0;

  printf("\n=== MLFQ Scheduler Test ===\n");

  // Test 1: Basic getpinfo argument validation.
  printf("\nTest 1: getpinfo arguments\n");

  struct pinfo info[1];

  if(getpinfo(info, 0) == 0 &&
     getpinfo(info, -1) == -1 &&
     getpinfo(info, 1) == 1 &&
     getpinfo((struct pinfo*)0xffffffffffffffffULL, 1) == -1)
    printf("PASS: argument handling\n");
  else {
    printf("FAIL: argument handling\n");
    failures++;
  }

  // Test 2: Create two CPU-intensive processes.
  printf("\nTest 2: CPU-intensive processes\n");

  p1 = fork();
  if(p1 < 0){
    printf("FAIL: first fork\n");
    exit(1);
  }
  if(p1 == 0)
    cpu_worker();

  p2 = fork();
  if(p2 < 0){
    printf("FAIL: second fork\n");
    wait(0);
    exit(1);
  }
  if(p2 == 0)
    cpu_worker();

  // Allow both children to execute.
  pause(12);

  if(getstats(p1, &a1) < 0 ||
     getstats(p2, &b1) < 0){
    printf("FAIL: cannot find child processes\n");
    failures++;
  } else {
    printf("First snapshot:\n");
    printf("P1: level=%d ticks=%d nsched=%d\n",
           a1.level, a1.runticks, a1.nsched);
    printf("P2: level=%d ticks=%d nsched=%d\n",
           b1.level, b1.runticks, b1.nsched);

    // Wait and measure again.
    pause(12);

    if(getstats(p1, &a2) < 0 ||
       getstats(p2, &b2) < 0){
      printf("FAIL: second snapshot\n");
      failures++;
    } else {
      printf("\nSecond snapshot:\n");
      printf("P1: level=%d ticks=%d nsched=%d\n",
             a2.level, a2.runticks, a2.nsched);
      printf("P2: level=%d ticks=%d nsched=%d\n",
             b2.level, b2.runticks, b2.nsched);

      // Runtime counters must never decrease.
      if(a2.runticks >= a1.runticks &&
         b2.runticks >= b1.runticks &&
         a2.nsched >= a1.nsched &&
         b2.nsched >= b1.nsched)
        printf("PASS: counters are monotonic\n");
      else {
        printf("FAIL: counters decreased\n");
        failures++;
      }

      // Both CPU-intensive processes should get CPU time.
      if(a2.runticks > 0 && b2.runticks > 0)
        printf("PASS: both processes received CPU time\n");
      else {
        printf("FAIL: a process received no CPU ticks\n");
        failures++;
      }

      if(a2.level >= 0 && a2.level <= 3 &&
         b2.level >= 0 && b2.level <= 3)
        printf("PASS: priority levels in valid range\n");
      else {
        printf("FAIL: invalid priority level\n");
        failures++;
      }
    }
  }

  wait(0);
  wait(0);

  printf("\n=== Result: %d failures ===\n", failures);
  exit(failures ? 1 : 0);
}

