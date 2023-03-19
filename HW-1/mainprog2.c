#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define readpipe "/tmp/readpipe"
#define writepipe "/tmp/writepipe"

void toup(char *str) {
    int l = strlen(str);
    for (int i = 0; i < l; i++) {
        if (strchr("bcdfghjklmnpqrstvwxz", str[i]) != NULL) {
            str[i] -= 32;
        }
    }
}

int main(int argc, char** argv) {
  if (argc != 3) {      // Проверяю количество входных данных.
    printf("Wrong file names");
    exit(-1);
  }
  
  int buf_size = 5000;
    ssize_t size;
  char string[buf_size];
  char str_buf[buf_size];      // Инициализирую буфер.
  ssize_t read_size;
  int count = 0;

    mknod(readpipe, S_IFIFO | 0666, 0);
    mknod(writepipe, S_IFIFO | 0666, 0);

  pid_t fst = fork();

  if (fst == -1) {
     printf("Couldn't create first child.\n");
     exit(-1);
  } else if (fst == 0) {
    printf("First processe online\n");
    int readfd = open(argv[1], O_RDONLY);
    read(readfd, string, sizeof(string));

    int readpipe = open(readpipe, O_WRONLY);
    write(readpipe, string, sizeof(string));

    close(readfd);
    close(readpipe);
        
    exit(EXIT_SUCCESS);
  } 
      pid_t snd = fork();
      if (snd == -1) {
      printf("Couldn't create second child.\n");
      exit(-1);
    } else if (snd == 0) {
          printf("Second processe online\n");
          int write_pipe = open(writepipe, O_RDONLY);
            read(write_pipe, str_buf, sizeof(buf_size));
            int writefd = open(argv[2], O_WRONLY | O_CREAT, 0666);
            write(writefd, str_buf, strlen(buf_size));
            close(writefd);
            close(writepipe);
            unlink(readpipe);
            unlink(writepipe);
      exit(EXIT_SUCCESS);
    } 
    
    pid_t trd = fork();
    if (trd == -1) {
      printf("Couldn't create third child.\n");
      exit(-1);
    } else if (trd == 0) {
          printf("Third processe online\n");
          char text[read_size * 4];
        int i = 0;
        for (int j = 0; j < read_size; j++) {
        if (str_buf[i] == 'A') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '4';
          text[i + 3] = '1';
          i += 4;
        } else if (str_buf[i] == 'a') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '6';
          text[i + 3] = '1';
          i += 4;
        } else if (str_buf[i] == 'E') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '4';
          text[i + 3] = '5';
          i += 4;
        } else if (str_buf[i] == 'e') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '6';
          text[i + 3] = '5';
          i += 4;
        } else if (str_buf[i] == 'I') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '4';
          text[i + 3] = '9';
          i += 4;
        } else if (str_buf[i] == 'i') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '6';
          text[i + 3] = '9';
          i += 4;
        } else if (str_buf[i] == 'O') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '4';
          text[i + 3] = 'F';
          i += 4;
        } else if (str_buf[i] == 'o') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '6';
          text[i + 3] = 'f';
          i += 4;
        } else if (str_buf[i] == 'U') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '5';
          text[i + 3] = '5';
          i += 4;
        } else if (str_buf[i] == 'u') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '7';
          text[i + 3] = '5';
          i += 4;
        } else if (str_buf[i] == 'Y') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '5';
          text[i + 3] = '9';
          i += 4;
        } else if (str_buf[i] == 'y') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '7';
          text[i + 3] = '9';
          i += 4;
        } else {
          text[i] = str_buf[i];
          i++;
        }
      }
    int read_pipe = open(readpipe, O_RDONLY);
    int write_pipe = open(writepipe, O_WRONLY);
    read(read_pipe, text, sizeof(text));
    
    write(write_pipe, text, sizeof(text));

            close(read_pipe);
            close(write_pipe);
        exit(EXIT_SUCCESS);
    }
      
    waitpid(fst, NULL, 0);
    waitpid(snd, NULL, 0);
    waitpid(trd, NULL, 0);
  return 0;
}
