#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int
main(int argc, char *argv[])
{
  int p[2];
  pipe(p);
  int pid=fork();
  if(pid){
    close(p[0]);
    for(int i=2;i<=35;++i) {
      write(p[1],&i,4);
    }
    close(p[1]);
    wait(0);
  }else{
    int son=0,n=0,m=0;
    int q[2];
HD:
    memcpy(q,p,sizeof q);
    close(q[1]);
    while(read(q[0],&n,4)){
      if(!son){
        printf("prime %d\n",n);
        pipe(p);
        m=n;
        son=fork();
        if(!son) goto HD;
        close(p[0]);
      }else if(n%m){
        write(p[1],&n,4); 
      }
    }
    close(q[0]);
    if(son){
      close(p[1]);
      wait(0);
    }
  }
  exit(0);
}
