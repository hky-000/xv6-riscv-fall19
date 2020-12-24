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

// 命令可以少些一个参数，之后再给
int main(int argc, char *argv[]){

  if(argc < 2) {
    fprintf(2, "usage: xargs <cmd> ...\n");
    exit();
  }

  char *lineSplit[32]; // 存放加工后的命令和参数的位置,将之后的参数放在后面
  int j = 0;
  for(int i = 1; i < argc; i++){
    lineSplit[j++] = argv[i];
  }

  char block[32]; // 读入的参数的存放区
  char buf[32]; // 加工block，使每个参数用0隔开，并以0结尾
  char *p = buf; // 参数的起始地址
  int c = 0; // 标记处理的字符位置
  int m = 0; // 每次读取的字符总长度
  //ctrl + d作为整个命令的结束（read返回的长度<0）
  while((c = read(0, block, sizeof(block))) > 0){
    for(int l = 0; l < c; l++){
      if(block[l] == '\n'){ //新的一行
        buf[m] = 0;
        m = 0; // 初始化，为下一次做准备
        lineSplit[j++] = p;
        p = buf; // 初始化，为下一次做准备
        lineSplit[j] = 0;
        j = argc - 1; // 初始化，为下一次做准备
        if(Fork() == 0){
            exec(argv[1], lineSplit); // lineSplit有命令，初始参数，后来输入的参数
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
