#include "kernel/types.h"
#include "user/user.h"
int gc(){
  char c;
  if(read(0,&c,1)==1){
    //fprintf(2,"%d",c);
    return c;
  }else{
    return -1;
  }
}
int gets_(char *x){
  int n=0,c;
  while((c=gc())!=-1 && (c=='\n' ||c==' ')); 
  if(c==-1) return -1;
  do{
  *x++=c;
  n+=1;
  }while((c=gc())!=-1 && c!='\n');
  *x=0;
  return n;
}
char buf[513];
char *ptr[50];
int
main(int argc, char *argv[])
{
#define GG {fprintf(2,"Usage: xargs command\n");exit(1);}
  if(argc<2) GG;
  for(int i=1;i<=argc;++i)
    ptr[i-1]=argv[i];
  while(gets_(buf)>0){
    //fprintf(2,buf);
    ptr[argc-1]=buf;
    ptr[argc]=0;
    int pid=fork();
    if(pid==0){
      exec(argv[1],ptr);
      exit(0);
    }
  }
  exit(0);
}
