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

void prime(int *data, int num){
	if(num == 1){  //终止判断
		printf("prime %d\n", *data);
		return;
	}
	
  int base = *data;
  int p[2];
	int temp;
  int i;
	printf("prime %d\n", base);
  
	pipe(p);
  if(Fork() == 0){
    close(p[0]);
    //从data数组中读出字符，向管道写入所有字符
    for(i = 0; i < num; i++){
      temp = *(data + i);
      write(p[1], &temp, 4); //int 4个字节
    }
    close(p[1]);
    exit();
  }else{
    close(p[1]);
    int count = 0;
    int buf;
    int flag;
    // 从管道读出字符，并且只有当前轮挑选出来的字符可以写入data数组
    while((flag = read(p[0], &buf, 4)) != 0){
      if(buf % base != 0){
        *data = buf;
        data += 1;
        count++;
      }
    }
    close(p[0]);
    prime(data - count, count);
    
    wait();
    exit();
  }
}

int main(){
  int allData[34];  //2~35
	int i;
	for(i = 0 ; i < 34; i++){
		allData[i] = i+2;
	}
	prime(allData, 34);
  exit();
}
