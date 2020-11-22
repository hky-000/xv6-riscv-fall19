#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAX_CHAR 128
#define MAX_ARG 32

int getcmd(char* buf, int nbuf){
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  buf[strlen(buf) - 1] = 0; // chop \n
  return 0;
}

int runcmd(char *argv1, char** argv){
  char** pipe_argv = 0;
  char *stdin = 0;
  char *stdout = 0;

  for(char** v = argv; *v != 0; ++v){
    if(strcmp(*v, "<") == 0){
      *v = 0;
      stdin = *(v + 1);
      ++v;
    }
    if(strcmp(*v, ">") == 0){
      *v = 0;
      stdout = *(v + 1);
      ++v;
    }
    if(strcmp(*v, "|") == 0){
      *v = 0;
      pipe_argv = v + 1;
      break;
    }
  }

  if(fork() == 0){
    int fd[2];
    if(pipe_argv != 0){
      pipe(fd);
      if(fork() == 0){
        close(fd[1]);
        //重定向读
        close(0);
        dup(fd[0]);//
        runcmd(pipe_argv[0], pipe_argv);
        close(fd[0]);
        //close(0);
        exit(0);
      }
      close(fd[0]);
      //重定向写
      close(1);
      dup(fd[1]);//
    }

    // <
    if(stdin != 0){
      close(0);
      if(open(stdin, O_RDONLY) != 0){
        fprintf(2, "open stdin %s failed!\n", stdin);
        exit(1);
      }
    }
    // >
    if(stdout != 0){
      close(1);
      if(open(stdout, O_CREATE | O_WRONLY) != 1){
        fprintf(2, "open stdout %s failed!\n", stdout);
        exit(1);
      }
    }
    
    exec(argv1, argv);

    // 读写口全部关闭
    if(stdin != 0) close(0);
    if(stdout != 0) close(1);
    if(pipe_argv != 0){
      close(fd[1]);
      //close(1);
      wait(0);
    }
    exit(0);
  }else{
    wait(0);
  }
  return 0;
}

int main(void)
{
  char buf[MAX_CHAR];
  char *argv[MAX_ARG];
  int argc = 0;

  //引用于sh.c:150
  int fd;
  // Ensure that three file descriptors are open.
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }

  printf("@ ");
  while(getcmd(buf, MAX_CHAR) >= 0) {
    //将每一个参数抽离出来并以'\0'为结尾
    int nbuf = strlen(buf);
    argc = 0; // 每一次都要初始为0
    argv[argc++] = buf;
    for(int i = 0; i < nbuf; i++) {
      if(buf[i] == ' ') {
        buf[i] = '\0';
        argv[argc++] = &buf[i + 1];
      }
    }
    argv[argc] = 0;

    runcmd(argv[0], argv);

    printf("@ ");
  }
  exit(0);
}
