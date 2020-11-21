#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"
#include "kernel/fcntl.h"


#define MAX_CMD 8
#define MAX_PIPE (MAX_CMD - 1) * 2
struct command {
  char *args[MAXARG];
  char argc;
  char *input;
  char *output;
};

int parse_cmd(struct command *cmd, int cmd_sz, char *buf, int buf_sz);
int check_redirect(struct command *cmd, int cmd_sz);
void run_cmd(struct command *cmd, int cmd_sz);
void print_cmd(struct command *cmd, int cmd_sz);

int
main(void) {
  static char buf[128];
  static struct command cmd[MAX_CMD];
  int cmd_sz = 0;
  
  while((cmd_sz = parse_cmd(cmd, MAX_CMD, buf, sizeof buf)) >= 0){
    if(cmd_sz == 0) continue;
    if(strcmp("cd", cmd[0].args[0]) == 0){
      if(chdir(cmd[0].args[1]) < 0)
        fprintf(2, "cannot cd %s\n", cmd[0].args[1]);
      continue;
    }
    if(!check_redirect(cmd, cmd_sz)){
      fprintf(2, "bad redirect\n");
      continue;
    }
    // print_cmd(cmd, cmd_sz);
    run_cmd(cmd, cmd_sz);
  }
  exit(0);
}

int
parse_cmd(struct command *cmd, int cmd_sz, char *buf, int buf_sz) {
  int read_buf_sz = 0;
  int read_cmd_sz = 0;
  
  printf("@ ");
  gets(buf, buf_sz);
  read_buf_sz = strlen(buf);
  if(read_buf_sz < 1) return -1;
  if(read_buf_sz > 1) {
    buf[read_buf_sz - 1] = 0;   /* ignore \n */
    read_cmd_sz = 1;

    char *x = buf;
    while(*x) {
      int cmd_idx = read_cmd_sz - 1;
      while(*x && *x == ' ') *x++ = 0;
      
      cmd[cmd_idx].argc = 0;
      for(int i = 0; i < MAXARG; i++) cmd[cmd_idx].args[i] = 0;
      cmd[cmd_idx].input = 0;
      cmd[cmd_idx].output = 0;

      while(*x && *x != '<' && *x != '>' && *x != '|' && *x != ' ') {
        cmd[cmd_idx].argc++;
        cmd[cmd_idx].args[cmd[cmd_idx].argc - 1] = x;
        while(*x && *x != '<' && *x != '>' && *x != '|' && *x != ' ') x++;
        while(*x && *x == ' ') *x++ = 0; /* del ' ' after each param */
      }

      while(*x == '<' || *x == '>') {
        char redirect = *x++;
        while(*x && *x == ' ') x++;
        if(redirect == '<') cmd[cmd_idx].input = x; else cmd[cmd_idx].output = x;
        while(*x && *x != '<' && *x != '>' && *x != '|' && *x != ' ') x++;
        while(*x && *x == ' ') *x++ = 0;
      }

      if(*x == '|') {
        x++;
        if(cmd[cmd_idx].argc > 0) {
          read_cmd_sz++;
        }else{
          fprintf(2, "bad pipe\n");
          return 0;
        }
      }
    }
  }
  if((read_cmd_sz > 1) && cmd[read_cmd_sz - 1].argc < 1)
    return read_cmd_sz - 1;
  return read_cmd_sz;
}


int
check_redirect(struct command *cmd, int cmd_sz) {
  if(cmd_sz <= 1) return 1;
  for(int i = 0; i < cmd_sz; i++) {
    if(i == 0 && cmd[i].output != 0) return 0;
    if(i == cmd_sz - 1 && cmd[i].input != 0) return 0;
    if((i != 0 && i != cmd_sz - 1) && (cmd[i].input != 0 || cmd[i].output != 0)) return 0;
  }
  return 1;
}

void
run_cmd(struct command *cmd, int cmd_sz) {
  int p[MAX_PIPE];
  for(int i = 0; i < MAX_PIPE; i++) p[i] = 0;

  for(int i = 0; i < cmd_sz; i++) {
    int *current_pipe = p + i * 2;
    int *prev_pipe = p + (i - 1) * 2;

    if(i != cmd_sz - 1) {
      pipe(current_pipe);
    }

    if(fork() > 0){
      if(i == 0 && i != cmd_sz - 1) {
        close(current_pipe[1]);
      }
      if(i != 0 && i != cmd_sz - 1) {
        close(current_pipe[1]);
        close(prev_pipe[0]);
      }
      if(i == cmd_sz - 1 && i != 0) {
        close(prev_pipe[0]);
      }
      if (i % 2 == 1){
        wait(0);
        wait(0);
      }
    }else{
      if(i == 0 && i != cmd_sz - 1) {
        close(1);
        dup(current_pipe[1]);
        close(current_pipe[0]);
        close(current_pipe[1]);
      }
      if(i != 0 && i != cmd_sz - 1) {
        close(current_pipe[0]);
        close(0);
        dup(prev_pipe[0]);
        close(prev_pipe[0]);
        close(1);
        dup(current_pipe[1]);
        close(current_pipe[1]);
      }
      if(i == cmd_sz - 1 && i != 0) {
        close(0);
        dup(prev_pipe[0]);
        close(prev_pipe[0]);
      }
      
      // redirect
      if(cmd[i].input) {
        close(0);
        open(cmd[i].input, O_RDONLY);
      }
      if(cmd[i].output) {
        close(1);
        open(cmd[i].output, O_CREATE|O_WRONLY);
      }

      exec(cmd[i].args[0], cmd[i].args);
      exit(0);
    }
  }
  wait(0);
}


void 
print_cmd(struct command *cmd, int cmd_sz) {
  for(int i = 0; i < cmd_sz; i++) {
    printf("cmd[%d].args", i);
    for(int j = 0; j < cmd[i].argc; j++)
      printf(" %s(%d)", cmd[i].args[j], strlen(cmd[i].args[j]));
    printf("\n");
    printf("cmd[%d].argc %d\n", i, cmd[i].argc);
    printf("cmd[%d].input %s\n", i, cmd[i].input);
    printf("cmd[%d].output %s\n", i, cmd[i].output);
  }
}