#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"
char* fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;
  for(p=path+strlen(path); p >= path && *p != '/'; p--);
  p++;
  if(strlen(p) >= DIRSIZ)
    return p;
  int n = strlen(p);
  memmove(buf, p, n);
  buf[n] = 0;
  return buf;
}
void find(char *path, char* target)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  if((fd = open(path, 0)) < 0) return;
  if(fstat(fd, &st) < 0)
  {
    close(fd);
    return ;
  }
  if(strcmp(fmtname(path), target) == 0)
    printf("%s\n", path);
  if(st.type == T_DIR)
  {
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
      printf("find: path too long\n");
      return ;
    }
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0) continue;
      if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;
      if(stat(buf, &st) < 0) continue;
      strcpy(buf, path);
      p = buf+strlen(buf);
      *p++ = '/';
      memmove(p, de.name, sizeof(de.name));
      p[strlen(de.name)] = 0;
      find(buf, target);
    }
  }
  close(fd);
}
int main(int argc, char **argv)
{
    if(argc < 3)
    {
        printf("Usage: find [direction] [file name]\n");
        exit(0);
    }
    else
    {
        find(argv[1], argv[2]);
        exit(0);
    }
}