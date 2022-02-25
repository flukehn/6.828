#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
/*
char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}
*/
int is_match(char *a,char *b){
  int n=strlen(a);
  int m=strlen(b);
  int i,j;
  for(i=0;i+m<=n;++i){
    for(j=0;j<m;++j)
      if(a[i+j]!=b[j])break;
    if(j>=m) return 1;
  }
  return 0;
}
void
find(char *path, char *str)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  //fprintf(2, "ok %s %s\n",path,str);
  if(is_match(path,str)) {
    printf("%s\n",path);
  }
  //fprintf(2, "ok2 %s %s\n",path,str);
  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }
  
  switch(st.type){
  case T_FILE:
    //printf("%s %d %d %l\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      if(!strcmp(de.name, ".") || 
          !strcmp(de.name, "..")) continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf, str);
      /*
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      */
      //printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if(argc!=3) {
    fprintf(2,"Usage: find dir str\n");
    exit(1);
  }
  find(argv[1],argv[2]);
  exit(0);
}
