#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int p[2];
int
main(int argc, char *argv[])
{
  pipe(p);
  int pid=getpid();
  int son=fork();
  if(son==0){
    int _=read(p[0],&son,4);
    if(_)fprintf(1,"%d: received ping\n",son);
    write(p[1],"!",2);
  }else{
    write(p[1],&son,4); 
    char t[11];
    int _=read(p[0],t,2);
    if(_)fprintf(1,"%d: received pong\n",pid);
  }
  exit(0);
}
