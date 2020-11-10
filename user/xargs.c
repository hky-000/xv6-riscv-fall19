#include "kernel/types.h"
#include "user/user.h"

typedef int pid_t;

pid_t Fork(void){
  pid_t pid;

  if((pid = fork()) < 0){
    printf("Fork error");
  }
  return pid;
}

int main(int argc, char *argv[]){

  if(argc < 2) {
    fprintf(2, "usage: xargs <cmd> ...\n");
    exit();
  }

  char *lineSplit[32];
  int j = 0;
  for(int i = 1; i < argc; i++){
    lineSplit[j++] = argv[i];
  }

  char block[32];
  char buf[32];
  char *p = buf;
  int c,m = 0;
  //ctrl + d作为整个命令的结束（read返回的长度<0）
  while((c = read(0, block, sizeof(block))) > 0){
    for(int l = 0; l < c; l++){
      if(block[l] == '\n'){ //新的一行
        buf[m] = 0;
        m = 0;
        lineSplit[j++] = p;
        p = buf;
        lineSplit[j] = 0;
        j = argc - 1;
        if(Fork() == 0){
            exec(argv[1], lineSplit);
        }                
        wait();
      }else if(block[l] == ' ') { //下一个
        buf[m++] = 0;
        lineSplit[j++] = p;
        p = &buf[m];
      }else {
        buf[m++] = block[l];
      }
    }
  }
  exit();
}
