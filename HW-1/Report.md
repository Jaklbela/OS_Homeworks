###Жмурина Ксения Игоревна БПИ218, задание сделано на оценку 4, вариант 7
##Разработать программу, заменяющую все гласные буквы в заданной ASCII-строке их ASCII кодами в шестнадцатиричной системе счисления. Код каждого символа задавать в формате «0xDD»,
где D — шестнадцатиричная цифра от 0 до F.
## В программе присутствует три процесса, создающихся отдельно и две pipe. Изначально первый процесс после запуска считывает текст из файла в pipe и передает второму процессу. Второй процесс извлекает из pipe данные в отдельную переменную и начинает оперировать ими. После этого передает новую переменную во второй ppe,  с которым работает третий процесс, выводящий содержимое в файл.
##Код на си:
```c
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <string.h>

const int buf_size = 5000;

int main(int argc, char** argv) {
  if (argc != 3) {      // Проверяю количество входных данных.
    printf("Wrong file names");
    exit(-1);
  }
  
    ssize_t size;
  char str_buf[buf_size];      // Инициализирую буфер.
  ssize_t read_size;
  int count = 0;

  int fd[2], fd2[2];
    
  if (pipe(fd) < 0) {
    printf("Can't create pipe\n");
    exit(-1);
  }
  
  if (pipe(fd2) < 0) {
    printf("Can't create second pipe\n");
    exit(-1);
  }

  pid_t fst = fork();

  if (fst == -1) {
     printf("Couldn't create first child.\n");
     exit(-1);
  } else if (fst == 0) {
    printf("First processe online\n");
    int fdfst;
    if ((fdfst = open(argv[1], O_RDONLY)) < 0) {     // Открываю первый файл.
      printf("Can't open file\n");
      exit(-1);
    }
    read_size = read(fdfst, str_buf, buf_size); // Читаю в буфер.
    
    if(read_size == -1) {
      printf("Can't write this file\n");
      exit(-1);
    } else {
      printf("File message recorded to buffer.\n");
    }

    size = write(fd[1], str_buf, read_size);  // Пишу в трубу данные
    if(size != read_size) {
      printf("Can't write all string to pipe\n");
      exit(-1);
    } else {
      printf("File message recorded to pipe.\n");
    }

    if(close(fdfst) < 0) {      // Закрываю файл.
      printf("Can't close file\n");
    }
    exit(EXIT_SUCCESS);
  } 
      pid_t snd = fork();
      if (snd == -1) {
      printf("Couldn't create second child.\n");
      exit(-1);
    } else if (snd == 0) {
    sleep(2);
          printf("Second processe online\n");
          read_size = read(fd[0], str_buf, buf_size);
          if(read_size == -1) {
            printf("Can't read message from pipe.\n");
            exit(-1);
          } else {
            printf("Pipe message has been read.\n");
          }
          
          char text[read_size * 4];
        int i = 0;
        while (i < read_size * 4) {
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
          text[i + 1] = '0';
          text[i + 2] = '0';
          text[i + 3] = '0';
          i += 4;
        }
      }

      size = write(fd2[1], text, read_size * 4);  // Пишу в трубу данные.
      if(size != read_size * 4) {
        printf("Can't write all message to pipe\n");
        exit(-1);
      } else {
        printf("Modifies message recorded to pipe.\n");
      }
      if(close(fd[0]) < 0){
            printf("Can't close writing side of pipe\n");
            exit(-1);
      }
      if(close(fd[1]) < 0){
            printf("Can't close writing side of pipe\n");
            exit(-1);
      }
      exit(EXIT_SUCCESS);
    } 
    
    pid_t trd = fork();
    if (trd == -1) {
      printf("Couldn't create third child.\n");
      exit(-1);
    } else if (trd == 0) {
    sleep(3);
          printf("Third processe online\n");
          char text[read_size * 4];
          size = read(fd2[0], text, read_size * 4);    // Читаю данные из трубы.
          if(size < 0) {
            printf("Can't read string from pipe\n");
            exit(-1);
          }
          
          int fdsnd;
          if((fdsnd = open(argv[2], O_WRONLY | O_CREAT, 0666)) < 0) {  // Открываю второй файл.
            printf("Can't open file\n");
            exit(-1);
          }
          size = write(fdsnd, text, count);  // Пишу из трубы во второй файл.
          if(size != count) {
            printf("Can't write all string\n");
            exit(-1);
          } else {
            printf("Result recorded to file.\n");
          }
          if(close(fdsnd) < 0) {    // Закрываю обе части трубы
            printf("Can't close reading side of pipe\n"); 
            exit(-1);
          }
          if(close(fd2[0]) < 0) {    // Закрываю обе части трубы
            printf("Can't close reading side of pipe\n"); 
            exit(-1);
          }
          if(close(fd2[1]) < 0){
            printf("Can't close writing side of pipe\n");
            exit(-1);
          }
          exit(EXIT_SUCCESS);
      }
      
    waitpid(fst, NULL, 0);
    waitpid(snd, NULL, 0);
    waitpid(trd, NULL, 0);
  return 0;
}
```
