#Жмурина Ксения Игоревна БПИ218,  вариант 34
##Узнав о планах преступников по
шифрованию текстов, Шерлок Холмс предложил лондонской полиции специальную машину для дешифровки сообщений злоумышленников. Реализовать приложение, дешифрующее кодированный текст. В качестве ключа используется известная кодовая таблица, устанавливающая однозначное соответствие между каждой буквой и каким-нибудь числом. Процессом узнавания кода в решении задачи пренебречь. Каждый процесс–дешифровщик декодирует свои кусочки текста, многократно получаемые от менеджера. Распределение фрагментов
текста между процессами–дешифровщиками осуществляется процессом–
менеджером, который передает каждому процессу фрагмент зашифрованного текста, получает от него результат, передает следующий зашифрованный фрагмент. Он же собирает из отдельных
фрагментов зашифрованный текст. Количество процессов задает12
ся опционально. Каждый процесс может выполнять свою работу за
случайное время.
## Программа принимает на вход имена файлов входных и выходных данных а также количество создаваемых процессов. Отдельно есть методы, регулирующие работу семафор (используются семафоры System V), а также декодеры. Сама программа создает в цикле несколько процессов, которые дешифруют кусочки текста, основываясь на номере процесса и размеру обрабатываемого отрывка. Затем программа ждет, пока все процессы отрабатывают и записывает получившийся буфер в файл.
## Оценка 4. Семафоры System V.
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
    
  if (pipe(fd) < 0) {             // Создаю трубы.
    printf("Can't create pipe\n");
    exit(-1);
  }
  
  if (pipe(fd2) < 0) {
    printf("Can't create second pipe\n");
    exit(-1);
  }

  pid_t fst = fork(); // Создаю первый процесс.

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
      pid_t snd = fork();   // Создаю второй процесс.
      if (snd == -1) {
      printf("Couldn't create second child.\n");
      exit(-1);
    } else if (snd == 0) {
    sleep(2);
          printf("Second processe online\n");
          read_size = read(fd[0], str_buf, buf_size);  // Считываю текст из трубы.
          if(read_size == -1) {
            printf("Can't read message from pipe.\n");
            exit(-1);
          } else {
            printf("Pipe message has been read.\n");
          }
          
          char text[read_size * 4];               // Алгоритм замены.
        int i = 0;
        for (int j = 0; j < read_size; j++) {
        if (str_buf[j] == 'A') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '4';
          text[i + 3] = '1';
          i += 4;
        } else if (str_buf[j] == 'a') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '6';
          text[i + 3] = '1';
          i += 4;
        } else if (str_buf[j] == 'E') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '4';
          text[i + 3] = '5';
          i += 4;
        } else if (str_buf[j] == 'e') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '6';
          text[i + 3] = '5';
          i += 4;
        } else if (str_buf[j] == 'I') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '4';
          text[i + 3] = '9';
          i += 4;
        } else if (str_buf[j] == 'i') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '6';
          text[i + 3] = '9';
          i += 4;
        } else if (str_buf[j] == 'O') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '4';
          text[i + 3] = 'F';
          i += 4;
        } else if (str_buf[j] == 'o') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '6';
          text[i + 3] = 'f';
          i += 4;
        } else if (str_buf[j] == 'U') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '5';
          text[i + 3] = '5';
          i += 4;
        } else if (str_buf[j] == 'u') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '7';
          text[i + 3] = '5';
          i += 4;
        } else if (str_buf[j] == 'Y') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '5';
          text[i + 3] = '9';
          i += 4;
        } else if (str_buf[j] == 'y') {
          text[i] = '0';
          text[i + 1] = 'x';
          text[i + 2] = '7';
          text[i + 3] = '9';
          i += 4;
        } else {
          text[i] = str_buf[j];
          text[i + 1] = '0';
          text[i + 2] = '0';
          text[i + 3] = '0';
          i += 4;
        }
      }

      size = write(fd2[1], text, read_size * 4);  // Пишу во вторую трубу данные.
      if(size != read_size * 4) {
        printf("Can't write all message to pipe\n");
        exit(-1);
      } else {
        printf("Modifies message recorded to pipe.\n");
      }
      if(close(fd[0]) < 0){                 // Закрываю первую трубу.
            printf("Can't close writing side of pipe\n");
            exit(-1);
      }
      if(close(fd[1]) < 0){
            printf("Can't close writing side of pipe\n");
            exit(-1);
      }
      exit(EXIT_SUCCESS);
    } 
    
    pid_t trd = fork();             // Создаю третий процесс.
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
