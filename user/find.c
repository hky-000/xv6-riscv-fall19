#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

char* fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p = path + strlen(path); p >= path && *p != '/'; p--);
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;

  memmove(buf, p, strlen(p)+1); // 包含终止标志
  return buf;
}

// 找到路径path下，所有文件名为findName的文件
void find(char *path, char *findName){ 
	int fd;
	struct stat st;	
	if((fd = open(path, 0)) < 0){
		fprintf(2, "find: cannot open %s\n", path);
		return;
	}
	if(fstat(fd, &st) < 0){ // 返回path的文件信息
		fprintf(2, "find: cannot stat %s\n", path);
		close(fd);
		return;
	}

	char buf[512], *p;	
	struct dirent de;
	switch(st.type){	
		case T_FILE:
			//系统文件名与要查找的文件名，若一致，打印系统文件完整路径
			if(strcmp(fmtname(path), findName) == 0){ 
				printf("%s\n", path);
			}	
			break;
		case T_DIR:
			if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
				printf("find: path too long\n");
				break;
			}
			strcpy(buf, path);
			p = buf + strlen(buf);
			*p++ = '/';
			while(read(fd, &de, sizeof(de)) == sizeof(de)){
				if(de.inum == 0 || strcmp(de.name, ".")==0 || strcmp(de.name, "..")==0)
					continue;				
				memmove(p, de.name, strlen(de.name));
				p[strlen(de.name)] = 0;
				find(buf, findName);
			}
			break;
	}
	close(fd);
}

int main(int argc, char *argv[]){
	if(argc < 3){
		printf("find: find <path> <fileName>\n");
		exit();
	}
	find(argv[1], argv[2]); // 在某路径中查找某文件
	exit();
}
