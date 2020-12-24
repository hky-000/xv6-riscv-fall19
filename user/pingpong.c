#include "kernel/types.h"
#include "user/user.h"

typedef short pid_t;

pid_t Fork(void){
  pid_t pid;

  if((pid = fork()) < 0){
    printf("Fork error");
  }
  return pid;
}

int main(void) 
{
  int parent_fd[2], child_fd[2];
  int pid;
  char buf[6];
  
  pipe(parent_fd); //0读 1写
  pipe(child_fd);

  // 先关闭不用的口，通过写完之后的close告知已经写完
  // 初始，没有数据写入read一直阻塞
  if((pid = Fork()) == 0){
    close(parent_fd[1]);
    close(child_fd[0]);
    read(parent_fd[0], buf, 6); //直到父进程写完才解除阻塞
    close(parent_fd[0]);
    printf("%d: received ping\n", getpid()); // 进程号
    write(child_fd[1], buf, 6);
    close(child_fd[1]);
  }else{
    close(parent_fd[0]);
    close(child_fd[1]);
    write(parent_fd[1], "hello\n", 6);
    close(parent_fd[1]);
    read(child_fd[0], buf, 6);  //直到子进程写完才解除阻塞
    close(child_fd[0]);
    printf("%d: received pong\n", getpid());
  }

  exit();
}

