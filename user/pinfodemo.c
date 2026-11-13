#include "kernel/types.h"
#include "kernel/pinfo.h"
#include "user/user.h"

int
main(void)
{
  struct pinfo info[64];

  int count = getpinfo(info, 64);

  if(count < 0){
    printf("getpinfo failed\n");
    exit(1);
  }

  printf("Found %d processes\n", count);
  printf("PID STATE LEVEL RUNTICKS NSCHED CTIME NAME\n");

  for(int i = 0; i < count; i++){
    printf("%d %d %d %d %d %d %s\n",
           info[i].pid,
           info[i].state,
           info[i].level,
           info[i].runticks,
           info[i].nsched,
           info[i].ctime,
           info[i].name);
  }

  printf("max=0: %d\n", getpinfo(info, 0));
  printf("max=-1: %d\n", getpinfo(info, -1));

  exit(0);
}
