
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <termios.h>
#include <signal.h>
#include <errno.h>

#include <string.h>

#include "linklist.h"
#include "checkinput.h"
#include "board.c"
#include <assert.h>

#define SIGNAL_PLAY "SIGNAL_PLAY"
#define SIGNAL_LOGIN "SIGNAL_LOGIN"
#define SIGNAL_RANKING "SIGNAL_RANKING"
#define SIGNAL_ERROR "SIGNAL_ERROR"
#define SIGNAL_OK "SIGNAL_OK"
#define SIGNAL_CREATEUSER "SIGNAL_CREATEUSER"

#define RED "\x1B[31m"
#define RESET "\x1B[0m"
#define GREEN "\x1B[32m"
#define BUFF_SIZE 1024

struct termios org_opts, new_opts;
int res;

char token[] = "#";
char error[100];
char choice;
char user[100];
char send_msg[1024];
char recv_msg[1024];

int sockfd, portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;
pthread_t tid[1];

void play();
int menuGame();
void clearScreen()
{
  write(1, "\E[H\E[2J", 7);
}

void *on_signal(void *sockfd)
{ //xử lý dl server trả về
  char buffer[64];
  int n;
  int socket = *(int *)sockfd;
  int *player = (int *)malloc(sizeof(int *));

  while (1)
  {
    bzero(buffer, 64);
    n = read(socket, buffer, 64);

    if (n < 0)
    {
      perror("ERROR reading from socket");
      exit(1);
    }

    if (buffer[0] == 'i' || buffer[0] == 'e' || buffer[0] == '\0')
    {
      if (buffer[0] == 'i')
      {
        if (buffer[2] == 't')
        {
          printf("\nMake your move: \n");
        }
        if (buffer[2] == 'n')
        {
          printf("\nWaiting for opponent...\n");
        }
        if (buffer[2] == 'p')
        {
          *player = atoi(&buffer[3]);
          if (*player == 2)
          {
            printf("You're whites (%c)\n", buffer[3]);
          }
          else
          {
            printf("You're blacks (%c)\n", buffer[3]);
          }
        }
      }
      else if (buffer[0] == 'e')
      {
        // Syntax errors
        if (buffer[2] == '0')
        {
          switch (buffer[3])
          {
          case '0':
            printf(RED "  ↑ ('-' missing)\n" RESET);
            break;
          case '1':
            printf(RED "↑ (should be letter)\n" RESET);
            break;
          case '2':
            printf(RED "   ↑ (should be letter)\n" RESET);
            break;
          case '3':
            printf(RED " ↑ (should be number)\n" RESET);
            break;
          case '4':
            printf(RED " ↑ (out of range)\n" RESET);
            break;
          case '5':
            printf(RED "   ↑ (should be number)\n" RESET);
            break;
          case '6':
            printf(RED "   ↑ (out of range)\n" RESET);
            break;
          case '7':
            printf(RED "(out of range)\n" RESET);
            break;
          }
        }
        printf("\nerror %s\n", buffer);
      }
      // Check if it's an informative or error message
    }
    else if (buffer[0] == 'r')
    {
      printf("%s\n", buffer);
    }
    else if (buffer[0] == 'g')
    {
      switch (buffer[3])
      {
      case 'w':
        printf("WHITE WIN!\n");
        menuGame();
        break;
      case 'b':
        printf("BLACK WIN!\n");
        menuGame();
        break;
      }
    }
    else
    {
      // Print the board
      system("clear");
      if (*player == 1)
      {
        print_board_buff(buffer);
      }
      else
      {
        print_board_buff_inverted(buffer);
      }
    }

    bzero(buffer, 64);
  }
}
int responseRank = 0;
void *handleRank(void *sockfd)
{
  char buffer[1024];
  int n;
  int socket = *(int *)sockfd;
  int *player = (int *)malloc(sizeof(int *));

  while (1)
  {
    bzero(buffer, 1024);
    n = read(socket, buffer, 1024);

    if (n < 0)
    {
      perror("ERROR reading from socket");
      exit(1);
    }

    printf("Rank: \n%s\n", buffer);

    bzero(buffer, 1024);
    responseRank = 1;
    break;
  }
}

int rank()
{
  int n = -1;
  pthread_create(&tid[0], NULL, &handleRank, &sockfd);
  n = write(sockfd, SIGNAL_RANKING, strlen(SIGNAL_RANKING)); //gửi dl lên server
  if (n < 0)
  { //kiem tra gui thanh cong hay k
    perror("ERROR writing to socket");
    exit(1);
  }
  responseRank = 0;
  while (responseRank == 0)
  {
  }
  fflush(stdin);
  printf("Press any key to continue...\n");
  while (getchar() != '\n')
  {
  }
  menuGame();
}
int menuGame()
{
  error[0] = '\0';
  while (1)
  {
    clearScreen();
    if (error[0] != '\0')
    {
      printf("Error: %s!\n", error);
      error[0] = '\0';
    }
    printf("----------FUNGAME----------\n");
    printf("\033[0;33m\tWelcome %s \033[0;37m\n", user);
    printf("\t1.New game\n");
    printf("\t2.Ranking\n");
    printf("\t3.Sign out\n");
    printf("---------------------------\n");
    printf("Your choice: ");
    scanf("%c", &choice);
    while (getchar() != '\n')
      ;
    if (choice == '1')
    {
      play();
      break;
    }
    else if (choice == '2')
    {
      rank();
      break;
    }
    else if (choice == '3')
    {
      return -1;
    }
    else
    {
      sprintf(error, "%c Not an optional", choice);
    }
  }
  return 0;
}
void play()
{
  char buffer[64];

  // Response thread
  pthread_create(&tid[0], NULL, &on_signal, &sockfd);  //tạo ra 1 luồng khi server gửi dl cho client thì nó sẽ chạy vào hàm on_signal
  n = write(sockfd, SIGNAL_PLAY, strlen(SIGNAL_PLAY)); //gửi dl lên server

  if (n < 0)
  { //kiem tra gui thanh cong hay k
    perror("ERROR writing to socket");
    exit(1);
  }
  while (1)
  {
    bzero(buffer, 64);
    fgets(buffer, 64, stdin);
    fflush(stdin);
    /* Send message to the server */
    n = write(sockfd, buffer, strlen(buffer)); //gửi dl lên server

    if (n < 0)
    { //kiem tra gui thanh cong hay k
      perror("ERROR writing to socket");
      exit(1);
    }
  }
}
void setCustomTerminal()
{
  res = 0;
  //store old setttings
  res = tcgetattr(STDIN_FILENO, &org_opts);
  assert(res == 0);
  //set new terminal parms
  memcpy(&new_opts, &org_opts, sizeof(new_opts));
  new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
  //new_opts.c_cc[VMIN] = 1;
  tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
}

void setDefaultTerminal()
{
  // lấy thông tin thiết bị đầu cuối hiện tại và lưu trữ trong org_opts
  //restore old settings
  res = tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);
  assert(res == 0);
}

/*
Lấy password
*/
int getPassword(char pass[])
{
  int i = 0;
  setCustomTerminal();
  while ((pass[i] = getchar()) != '\n')
    i++;
  pass[i] = '\0';
  setDefaultTerminal();
  return i;
}
int menuSignin()
{
  error[0] = '\0';
  while (1)
  {
    int n = write(sockfd, SIGNAL_LOGIN, strlen(SIGNAL_LOGIN)); //gửi dl lên server

    if (n < 0)
    { //kiem tra gui thanh cong hay k
      perror("ERROR writing to socket");
      exit(1);
    }
    char pass[100];
    char *str;
    clearScreen();
    printf("----------CHESS----------\n");
    printf("\tSign in\n");
    printf("---------------------------\n");
    printf("Username: ");
    fgets(user, BUFF_SIZE, stdin);
    user[strlen(user) - 1] = '\0';

    printf("Password: ");
    getPassword(pass);
    //      printf("%s|\n",pass);
    sprintf(send_msg, "%s#%s", user, pass);
    send(sockfd, send_msg, strlen(send_msg), 0);
    n = read(sockfd, recv_msg, 64);
    recv_msg[n] = '\0';
    str = strtok(recv_msg, token);
    if (strcmp(str, SIGNAL_OK) == 0)
    {
      menuGame();
      break;
    }
    else if (strcmp(str, SIGNAL_ERROR) == 0)
    {
      str = strtok(NULL, token);
      strcpy(error, str);
      printf("%s\n", error);
      return -1;
    }
  }
  return 0;
}
int menuRegister()
{
  char pass[100];
  char repass[100];
  char *str;
  error[0] = '\0';
  while (1)
  {
    int n = write(sockfd, SIGNAL_CREATEUSER, strlen(SIGNAL_CREATEUSER));
    clearScreen();
    printf("----------CHESS----------\n");
    printf("\tRegister\n");
    printf("---------------------------\n");
    printf("Username: ");
    fgets(user, BUFF_SIZE, stdin);
    user[strlen(user) - 1] = '\0';

    printf("Password: ");
    getPassword(pass);
    printf("\nConfirm password: ");
    getPassword(repass);
    //create  user and pass
    sprintf(send_msg, "%s#%s#%s", user, pass, repass);
    send(sockfd, send_msg, strlen(send_msg), 0);
    n = read(sockfd, recv_msg, 64);
    recv_msg[n] = '\0';
    str = strtok(recv_msg, token);
    if (strcmp(str, SIGNAL_OK) == 0)
    {
      menuGame();
      break;
    }
    else if (strcmp(str, SIGNAL_ERROR) == 0)
    {
      str = strtok(NULL, token);
      strcpy(error, str);
      printf("%s\n", error);
      return -1;
    }
  }
  return 0;
}
int menuStart()
{
  error[0] = '\0';
  while (1)
  {
    clearScreen();
    if (error[0] != '\0')
    {
      printf("Error: %s!\n", error);
      error[0] = '\0';
    }
    printf("----------CHESS----------\n");
    printf("\t1.Sign in\n");
    printf("\t2.Register\n");
    printf("\t3.Exit\n");
    printf("---------------------------\n");
    printf("Your choice: ");
    scanf("%c", &choice);
    while (getchar() != '\n')
      ;
    if (choice == '1')
    {
      if (menuSignin() == 0)
        break;
    }
    else if (choice == '2')
    {
      if (menuRegister() == 0)
        break;
    }
    else if (choice == '3')
      return -1;
    else
    {
      sprintf(error, "%c not an optional", choice);
    }
  }

  return 0;
}
int main(int argc, char *argv[])
{
  int err;
  char *serverAddress;
  if (argc != 3)
  {
    printf("Syntax Error.\n");
    printf("Syntax: ./client IPAddress PortNumber\n");
    return 0;
  }
  if (check_port(argv[2]) == 0)
  {
    printf("Port invalid\n");
    return 0;
  }
  serverAddress = argv[1];
  int PORT = atoi(argv[2]);
  setlocale(LC_ALL, "en_US.UTF-8");

  printf("Connecting to %s:%d\n", "localhost", PORT);

  /* Create a socket point */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0)
  {
    perror("ERROR opening socket");
    exit(1);
  }

  server = gethostbyname("localhost");

  if (server == NULL)
  {
    fprintf(stderr, "ERROR, no such host\n");
    exit(0);
  }

  //  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = inet_addr(serverAddress);
  //  serv_addr.sin_family = AF_INET;
  //  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  //  serv_addr.sin_port = htons(PORT);

  // Now connect to the server
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    perror("ERROR connecting");
    exit(1);
  }
  menuStart();
  fflush(stdin);
  return 0;
}
