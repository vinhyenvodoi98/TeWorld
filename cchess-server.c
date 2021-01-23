#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>
#include <locale.h>
#include <signal.h>

#include "linklist.h"
#include "checkinput.h"
#include "board.c"

#define SIGNAL_PLAY "SIGNAL_PLAY"
#define SIGNAL_LOGIN "SIGNAL_LOGIN"
#define SIGNAL_RANKING "SIGNAL_RANKING"
#define SIGNAL_ERROR "SIGNAL_ERROR"
#define SIGNAL_OK "SIGNAL_OK"
#define SIGNAL_CREATEUSER "SIGNAL_CREATEUSER"

// Waiting player conditional variable
pthread_cond_t player_to_join;
pthread_mutex_t general_mutex;
pthread_t tid[2];

char token[] = "#";
char send_msg[1024];
char recv_msg[1024];

int is_p1_playing = 0;
int is_p2_playing = 0;

struct clt
{
  int sockfd;
  char *username;
};

struct clt clients[20];
int ind = 0;

bool is_diagonal(int, int);

// Match player
int challenging_player = 0;
int player_is_waiting = 0;

void move_piece(wchar_t **board, int *move)
{ //thuc hien di chuyen nuoc di
  // Move piece in board from origin to dest
  board[move[2]][move[3]] = board[*move][move[1]];
  board[*move][move[1]] = 0;
}

bool emit(int client, char *message, int message_size)
{
  return true;
}

void translate_to_move(int *move, char *buffer)
{ //bien doi tu cu phap thanh 1 mang chua dl cua diem dau và diem dich

  printf("buffer: %s\n", buffer);

  *(move) = 8 - (*(buffer + 1) - '0');
  move[1] = (*(buffer) - '0') - 49;
  move[2] = 8 - (*(buffer + 4) - '0');
  move[3] = (*(buffer + 3) - '0') - 49;

  // printf("[%d, %d] to [%d, %d]\n", *(move), move[1], move[2], move[3]);
}

bool is_diagonal_clear(wchar_t **board, int *move)
{ //kiem tra xem duong chéo có trông hay không

  int *x_moves = (int *)malloc(sizeof(int));
  int *y_moves = (int *)malloc(sizeof(int));

  *(x_moves) = *(move)-move[2];
  *(y_moves) = move[1] - move[3];

  int *index = (int *)malloc(sizeof(int));
  *(index) = abs(*x_moves) - 1; //abs trả về giá trị tuyệt đối của x_move
  wchar_t *item_at_position = (wchar_t *)malloc(sizeof(wchar_t));

  // Iterate thru all items excepting initial posi
  while (*index > 0)
  {

    if (*x_moves > 0 && *y_moves > 0)
    {
      printf("%lc [%d,%d]\n", board[*move - *index][move[1] - *index], *move - *index, move[1] - *index);
      *item_at_position = board[*move - *index][move[1] - *index];
    }
    if (*x_moves > 0 && *y_moves < 0)
    {
      printf("%lc [%d,%d]\n", board[*move - *index][move[1] + *index], *move - *index, move[1] + *index);
      *item_at_position = board[*move - *index][move[1] + *index];
    }
    if (*x_moves < 0 && *y_moves < 0)
    {
      printf("%lc [%d,%d]\n", board[*move + *index][move[1] + *index], *move + *index, move[1] + *index);
      *item_at_position = board[*move + *index][move[1] + *index];
    }
    if (*x_moves < 0 && *y_moves > 0)
    {
      printf("%lc [%d,%d]\n", board[*move + *index][move[1] - *index], *move + *index, move[1] - *index);
      *item_at_position = board[*move + *index][move[1] - *index];
    }

    if (*item_at_position != 0)
    {
      free(index);
      free(x_moves);
      free(y_moves);
      free(item_at_position);
      return false;
    }

    (*index)--;
  }

  free(index);
  free(x_moves);
  free(y_moves);
  free(item_at_position);

  return true;
}

bool is_syntax_valid(int player, char *move)
{ //kiem tra cu phap cô hop le hay k
  // Look for -
  if (move[2] != '-')
  {
    send(player, "e-00", 4, 0);
    return false;
  }
  //First and 3th should be characters
  if (move[0] - '0' < 10)
  {
    send(player, "e-01", 4, 0);
    return false;
  }
  if (move[3] - '0' < 10)
  {
    send(player, "e-02", 4, 0);
    return false;
  }

  //Second and 5th character should be numbers
  if (move[1] - '0' > 10)
  {
    send(player, "e-03", 4, 0);
    return false;
  }
  if (move[1] - '0' > 8)
  {
    send(player, "e-04", 4, 0);
    return false;
  }
  if (move[4] - '0' > 10)
  {
    send(player, "e-05", 4, 0);
    return false;
  }
  if (move[4] - '0' > 8)
  {
    send(player, "e-06", 4, 0);
    return false;
  }
  // Move out of range
  if (move[0] - '0' > 56 || move[3] - '0' > 56)
  {
    send(player, "e-07", 4, 0);
    return false;
  }

  return true;
}

void broadcast(wchar_t **board, char *one_dimension_board, int player_one, int player_two)
{ //gui dl cua ban co,chieu ban co,2 ng choi

  to_one_dimension_char(board, one_dimension_board);

  printf("\tSending board to %d and %d size(%lu)\n", player_one, player_two, sizeof(one_dimension_board));
  send(player_one, one_dimension_board, 64, 0);
  send(player_two, one_dimension_board, 64, 0);
  printf("\tSent board...\n");
}

int get_piece_team(wchar_t **board, int x, int y)
{ //kiem tra quan den hay trang

  switch (board[x][y])
  {
  case white_king:
    return -1;
  case white_queen:
    return -1;
  case white_rook:
    return -1;
  case white_bishop:
    return -1;
  case white_knight:
    return -1;
  case white_pawn:
    return -1;
  case black_king:
    return 1;
  case black_queen:
    return 1;
  case black_rook:
    return 1;
  case black_bishop:
    return 1;
  case black_knight:
    return 1;
  case black_pawn:
    return 1;
  }

  return 0;
}

void promote_piece(wchar_t **board, int destX, int destY, int team)
{ //nhập hậu
  if (team == 1)
  {
    board[destX][destY] = black_queen;
  }
  else if (team == -1)
  {
    board[destX][destY] = white_queen;
  }
}

int get_piece_type(wchar_t piece)
{ //kiem tra la quân gi

  switch (piece)
  {
  case white_king:
    return 0;
  case white_queen:
    return 1;
  case white_rook:
    return 2;
  case white_bishop:
    return 3;
  case white_knight:
    return 4;
  case white_pawn:
    return 5;
  case black_king:
    return 0;
  case black_queen:
    return 1;
  case black_rook:
    return 2;
  case black_bishop:
    return 3;
  case black_knight:
    return 4;
  case black_pawn:
    return 5;
  }
  return -1;
}

bool is_rect(int x_moves, int y_moves)
{ //kiểm tra nó có đi đường thẳng hay k

  if ((x_moves != 0 && y_moves == 0) || (y_moves != 0 && x_moves == 0))
  {
    return true;
  }
  return false;
}

int is_rect_clear(wchar_t **board, int *move, int x_moves, int y_moves)
{ //kiểm tra đường thẳng có clear hay k

  // Is a side rect
  int *index = (int *)malloc(sizeof(int));

  if (abs(x_moves) > abs(y_moves))
  {
    *index = abs(x_moves) - 1;
  }
  else
  {
    *index = abs(y_moves) - 1;
  }

  // Iterate thru all items excepting initial position
  while (*index > 0)
  {

    if (*(move)-move[2] > 0)
    {
      if (board[*move - (*index)][move[1]] != 0)
      {
        free(index);
        return false;
      }
    }
    if (*(move)-move[2] < 0)
    {
      if (board[*move + (*index)][move[1]] != 0)
      {
        free(index);
        return false;
      }
    }
    if (move[1] - move[3] > 0)
    {
      if (board[*move][move[1] - (*index)] != 0)
      {
        free(index);
        return false;
      }
    }
    if (move[1] - move[3] < 0)
    {
      if (board[*move][move[1] + (*index)] != 0)
      {
        free(index);
        return false;
      }
    }

    (*index)--;
  }

  free(index);
  return true;
}

bool is_diagonal(int x_moves, int y_moves)
{ //kiểm tra có phải đường chéo hay không

  if ((abs(x_moves) - abs(y_moves)) != 0)
  {
    return false;
  }

  return true;
}

int getManitud(int origin, int dest)
{
  return (abs(origin - dest));
}

bool eat_piece(wchar_t **board, int x, int y)
{ //kiểm tra xem có ăn được k
  return (get_piece_team(board, x, y) != 0);
}

void freeAll(int *piece_team, int *x_moves, int *y_moves)
{ //giải phóng bàn cờ
  free(piece_team);
  free(x_moves);
  free(y_moves);
}

bool is_move_valid(wchar_t **board, int player, int team, int *move)
{ //kiểm tra nước đi có hợp lệ hay không

  int *piece_team = (int *)malloc(sizeof(int *));
  int *x_moves = (int *)malloc(sizeof(int *));
  int *y_moves = (int *)malloc(sizeof(int *));

  *piece_team = get_piece_team(board, *(move), move[1]);
  *x_moves = getManitud(*move, move[2]);
  *y_moves = getManitud(move[1], move[3]);

  // General errors
  if (board[*(move)][move[1]] == 0)
  {
    send(player, "e-08", 4, 0);
    return false;
  } // If selected piece == 0 there's nothing selected
  if (*piece_team == get_piece_team(board, move[2], move[3]))
  {
    send(player, "e-09", 4, 0);
    return false;
  } // If the origin piece's team == dest piece's team is an invalid move

  // Check if user is moving his piece
  if (team != *piece_team)
  {
    send(player, "e-07", 4, 0);
    return false;
  }

  printf("Player %d(%d) [%d,%d] to [%d,%d]\n", player, *piece_team, move[0], move[1], move[2], move[3]);

  // XMOVES = getManitud(*move, move[2])
  // YMOVES = getManitud(move[1], move[3])
  printf("Moved piece -> %d \n", get_piece_type(board[*(move)][move[1]]));
  switch (get_piece_type(board[*(move)][move[1]]))
  {
  case 0: /* --- ♚ --- */
    if (*x_moves > 1 || getManitud(move[1], move[3]) > 1)
    {
      send(player, "e-10", 5, 0);
      freeAll(piece_team, x_moves, y_moves);
      return false;
    }
    break;
  case 2: /* --- ♜ --- */
    if (!is_rect(*x_moves, *y_moves))
    {
      send(player, "e-30", 5, 0);
      freeAll(piece_team, x_moves, y_moves);
      return false;
    }
    if (!is_rect_clear(board, move, *x_moves, *y_moves))
    {
      send(player, "e-31", 4, 0);
      freeAll(piece_team, x_moves, y_moves);
      return false;
    }
    if (eat_piece(board, move[2], move[3]))
    {
      send(player, "i-99", 4, 0);
      freeAll(piece_team, x_moves, y_moves);
      return true;
    }
    break;
  case 3: /* ––– ♝ ––– */
    if (!is_diagonal(*x_moves, getManitud(move[1], move[3])))
    {
      send(player, "e-40", 4, 0);
      freeAll(piece_team, x_moves, y_moves);
      return false; // Check if it's a valid diagonal move
    }
    if (!is_diagonal_clear(board, move))
    {
      send(player, "e-41", 4, 0);
      freeAll(piece_team, x_moves, y_moves);
      return false;
    }
    if (eat_piece(board, move[2], move[3]))
    {
      send(player, "i-99", 4, 0);
      freeAll(piece_team, x_moves, y_moves);
      return true;
    }
    break;
  case 4: /* --- ♞ --- */
    if ((abs(*x_moves) != 1 || abs(*y_moves) != 2) && (abs(*x_moves) != 2 || abs(*y_moves) != 1))
    {
      send(player, "e-50", 4, 0);
      freeAll(piece_team, x_moves, y_moves);
      return false;
    }
    if (eat_piece(board, move[2], move[3]))
    {
      send(player, "i-99", 4, 0);
      freeAll(piece_team, x_moves, y_moves);
      return true;
    }
    break;
  case 5: /* --- ♟ --- */
    if (*piece_team == 1 && move[2] == 0)
    {
      printf("Promoting piece\n");
      promote_piece(board, move[2], move[3], *piece_team);
      send(player, "i-98", 4, 0);
      freeAll(piece_team, x_moves, y_moves);
      return true;
    }
    if (*piece_team == -1 && move[2] == 7)
    {
      printf("Promoting piece\n");
      promote_piece(board, move[2], move[3], *piece_team);
      send(player, "i-98", 4, 0);
      freeAll(piece_team, x_moves, y_moves);
      return true;
    }
    // Moving in Y axis
    if (getManitud(move[1], move[3]) != 0)
    {
      if (!is_diagonal(*x_moves, *y_moves) || (get_piece_team(board, move[2], move[3]) == 0))
      {
        send(player, "e-60", 4, 0);
        freeAll(piece_team, x_moves, y_moves);
        return false; // Check if it's a diagonal move and it's not an empty location
      }
      if (eat_piece(board, move[2], move[3]))
      {
        send(player, "i-99", 4, 0);
        freeAll(piece_team, x_moves, y_moves);
        return true; // Check if there's something to eat
      }
    }
    else
    {
      // Check if it's the first move
      if (move[0] == 6 && *piece_team == 1 && *x_moves == 2)
      {
        printf("First move\n");
        return true;
      }
      if (move[0] == 1 && *piece_team == -1 && *x_moves == 2)
      {
        printf("First move\n");
        return true;
      }
      if (*x_moves > 1)
      {
        send(player, "e-62", 5, 0);
        freeAll(piece_team, x_moves, y_moves);
        return false;
      }
    }
    break;
  default:
    break;
  }

  freeAll(piece_team, x_moves, y_moves);
  return true;
}

//ham kiem tra het co: 0 chua het, -1 trang thang, 1 la den thang
int end_game(wchar_t **board)
{
  int wk = 0;
  int bk = 0;

  for (int i = 0; i < 8; ++i)
  {
    for (int j = 0; j < 8; ++j)
    {
      // printf("%6d ", board[i][j]);
      if (board[i][j] == white_king)
      { //vua trang
        printf("white_king is alive");
        wk = 1;
      }
      else if (board[i][j] == black_king)
      { //vua den
        printf("black_king is alive");
        bk = 1;
      }
    }
  }
  printf("\n");

  if (wk != 0 && bk != 0)
    return 0;
  else if (wk != 0)
    return -1;
  else
    return 1;
}

void copyFile()
{
  FILE *sFile = fopen("tmp.txt", "r+");
  FILE *dFile = fopen("rank.txt", "w+");
  char ch;
  while ((ch = fgetc(sFile)) != EOF)
    fputc(ch, dFile);

  fclose(sFile);
  fclose(dFile);
}

int ur_i = 0;
char temp1[100], temp2[100], temp3[100];

void increasePoint(char *username)
{
  updatePoint(username, 100);
}

void *game_room(void *client_socket)
{
  /* If connection is established then start communicating */
  int player_one = *(int *)client_socket;
  int n, player_two;
  char buffer[64];
  int *move = (int *)malloc(sizeof(int) * 4);

  // Create a new board
  wchar_t **board = create_board();
  char *one_dimension_board = create_od_board();
  initialize_board(board);

  player_is_waiting = 1; // Set user waiting

  pthread_mutex_lock(&general_mutex);                 // Wait for player two
  pthread_cond_wait(&player_to_join, &general_mutex); // Wait for player wants to join signal

  // TODO lock assigning player mutex
  player_two = challenging_player; // Asign the player_two to challenging_player
  player_is_waiting = 0;           // Now none is waiting

  pthread_mutex_unlock(&general_mutex); // Unecesary?
  //kiem tra ket noi tu server den client 1,2 co oke k
  if (send(player_one, "i-p1", 4, 0) < 0)
  {
    perror("ERROR writing to socket");
    exit(1);
  }
  if (send(player_two, "i-p2", 4, 0) < 0)
  {
    perror("ERROR writing to socket");
    exit(1);
  }

  sleep(1); //dung hoat dong cua luong trong 1s

  // Broadcast the board to all the room players
  broadcast(board, one_dimension_board, player_one, player_two); //dui du lieu ban co one_dimension_board

  sleep(1);

  bool syntax_valid = false;
  bool move_valid = false;

  while (1)
  { //xu li nuoc di

    send(player_one, "i-tm", 4, 0);
    send(player_two, "i-nm", 4, 0);

    // Wait until syntax and move are valid
    printf("Waiting for move from player one (%d)... sending i\n", player_one);

    while (!syntax_valid || !move_valid)
    {                    //xu li nuoc di cho thang 1
      bzero(buffer, 64); //khoi tao mang ten la buffe co kich thuoc 64

      printf("Checking syntax and move validation (%d,%d)\n", syntax_valid, move_valid);
      if (read(player_one, buffer, 6) < 0)
      { //doc dl tư client tư client gui cho server khi di ,nuoc di nhap vao mang buffe
        perror("ERROR reading from socket");
        exit(1);
      }
      printf("Player one (%d) move: %s\n", player_one, buffer);

      syntax_valid = is_syntax_valid(player_one, buffer); //kiem tra cu phap nuoc di co dung k
      if (syntax_valid)
      {
        translate_to_move(move, buffer); // Convert to move
      }

      // TODO
      move_valid = is_move_valid(board, player_one, 1, move); //kiem tra nuoc di co hop le k
    }

    printf("Player one (%d) made move\n", player_one);

    syntax_valid = false;
    move_valid = false;

    // Apply move to board
    move_piece(board, move);

    // Send applied move board
    broadcast(board, one_dimension_board, player_one, player_two); //gui dl den tat ca nguoi choi trong 1 ban co
    sleep(1);

    //kiem tra het co

    if (end_game(board) == -1)
    { //quan trắng thang (nguoi choi 1 thang)
      send(player_one, "go_w", 4, 0);
      send(player_two, "go_w", 4, 0);
      is_p1_playing = 0;
      is_p2_playing = 0;

      //goi ham cong diem
      for (int i = 0; i < ind + 1; ++i)
      {
        if (clients[i].sockfd == player_two)
        {
          printf("winner: %s", clients[i].username);
          increasePoint(clients[i].username);
        }
        else if (clients[i].sockfd == player_one)
        {
          printf("loser: %s\n", clients[i].username);
          // decreasePoint(clients[i].username);
        }
      }
      break;
    }
    else if (end_game(board) == 1)
    {
      send(player_one, "go_b", 4, 0);
      send(player_two, "go_b", 4, 0);
      is_p1_playing = 0;
      is_p2_playing = 0;

      for (int i = 0; i < ind + 1; ++i)
      {
        if (clients[i].sockfd == player_one)
        {
          printf("winner: %s", clients[i].username);
          increasePoint(clients[i].username);
        }
        else if (clients[i].sockfd == player_two)
        {
          printf("loser: %s\n", clients[i].username);
          // decreasePoint(clients[i].username);
        }
      }
      break;
    }

    send(player_one, "i-nm", 4, 0);
    send(player_two, "i-tm", 4, 0);

    printf("Waiting for move from player two (%d)\n", player_two);

    while (!syntax_valid || !move_valid)
    {
      bzero(buffer, 64);

      if (read(player_two, buffer, 6) < 0)
      {
        perror("ERROR reading from socket");
        exit(1);
      }

      syntax_valid = is_syntax_valid(player_two, buffer);

      translate_to_move(move, buffer); // Convert to move

      move_valid = is_move_valid(board, player_two, -1, move);
    }
    printf("Player two (%d) made move\n", player_two);

    syntax_valid = false;
    move_valid = false;

    // Apply move to board
    move_piece(board, move);

    // Send applied move board
    broadcast(board, one_dimension_board, player_one, player_two);
    sleep(1);

    //kiem tra het co
    if (end_game(board) == -1)
    { //quan trắng thang (nguoi choi 1 thang)
      send(player_one, "go_w", 4, 0);
      send(player_two, "go_w", 4, 0);
      is_p1_playing = 0;
      is_p2_playing = 0;
      for (int i = 0; i < ind + 1; ++i)
      {
        if (clients[i].sockfd == player_two)
        {
          printf("winner: %s", clients[i].username);
          increasePoint(clients[i].username);
        }
        else if (clients[i].sockfd == player_one)
        {
          printf("loser: %s\n", clients[i].username);
          // decreasePoint(clients[i].username);
        }
      }
      break;
    }
    else if (end_game(board) == 1)
    {
      send(player_one, "go_b", 4, 0);
      send(player_two, "go_b", 4, 0);
      is_p1_playing = 0;
      is_p2_playing = 0;
      for (int i = 0; i < ind + 1; ++i)
      {
        if (clients[i].sockfd == player_one)
        {
          printf("winner: %s", clients[i].username);
          increasePoint(clients[i].username);
        }
        else if (clients[i].sockfd == player_two)
        {
          printf("loser: %s\n", clients[i].username);
          // decreasePoint(clients[i].username);
        }
      }
      break;
    }
  }

  /* delete board */
  free(move);
  free_board(board);
}

int isValid(char *username, char *password)
{
  FILE *f = fopen("user.txt", "r+");
  if (f == NULL)
  {
    printf("Error open file!!!\n");
    return 0;
  }
  char line[100];
  char *temp;
  if (password != NULL)
  {
    while (fgets(line, 100, f) != NULL)
    {
      temp = line;
      while (temp[0] != '#')
        temp++; // get user pass, gap # thi dung
      temp[0] = '\0';
      temp++;
      if (temp[strlen(temp) - 1] == '\n')
        temp[strlen(temp) - 1] = '\0';
      if (strcmp(line, username) == 0 && strcmp(temp, password) == 0)
      {
        fclose(f);
        return 1;
      }
    }
    fclose(f);
    return 0;
  }
  else
  {
    while (fgets(line, 100, f) != NULL)
    {
      temp = line;
      while (temp[0] != '#')
        temp++;
      temp[0] = '\0';
      if (strcmp(line, username) == 0)
      {
        fclose(f);
        return 1;
      }
    }
    fclose(f);
    return 0;
  }
}
void registerUser(char *username, char *password)
{
  addToLinkList(username);
  FILE *f = fopen("user.txt", "a");
  fprintf(f, "%s#%s\n", username, password);
  fclose(f);
}

void *on_request(void *client_socket)
{
  int sockfd = *(int *)client_socket;
  while (1)
  {
    char buffer[64];
    memset(buffer, 0, 64); //set buffer về 0
    int n = read(sockfd, buffer, 64);
    if (n < 0)
    {
      printf("ERROR on receive\n");
    }
    printf("%s\n", buffer);
    if (strcmp(buffer, "rank") == 0)
    {
    }
    else if (strcmp(buffer, SIGNAL_LOGIN) == 0)
    {
      char *user, *pass;
      int n = read(sockfd, recv_msg, 64);
      user = strtok(recv_msg, token);
      pass = strtok(NULL, token);
      if (isValid(user, pass))
        strcpy(send_msg, SIGNAL_OK);
      else
        sprintf(send_msg, "%s#%s", SIGNAL_ERROR, "Username or Password is incorrect");
      send(sockfd, send_msg, strlen(send_msg), 0);
      clients[ind].sockfd = sockfd;

      clients[ind].username = malloc(sizeof(char) * (strlen(user) + 1));
      strcpy(clients[ind].username, user);

      ind += 1;
      memset(user, 0, sizeof(user));
      memset(pass, 0, sizeof(pass));
    }
    else if (strcmp(buffer, SIGNAL_CREATEUSER) == 0)
    {
      char *user, *pass, *repass;
      int n = read(sockfd, recv_msg, 64);
      user = strtok(recv_msg, token);
      pass = strtok(NULL, token);
      repass = strtok(NULL, token);
      if (isValid(user, NULL))
      {
        sprintf(send_msg, "%s#%s", SIGNAL_ERROR, "Account existed");
      }
      else
      {
        if (strcmp(pass, repass) != 0)
        {
          sprintf(send_msg, "%s#%s", SIGNAL_ERROR, "Password does not match");
        }
        else
        {
          registerUser(user, pass);
          sprintf(send_msg, SIGNAL_OK);
        }
      }
      send(sockfd, send_msg, strlen(send_msg), 0);
    }
    else if (strcmp(buffer, SIGNAL_RANKING) == 0)
    {
      FILE *f = fopen("rank.txt", "r");
      if (f == NULL)
      {
        printf("Error open file!!!\n");
      }
      char line[50];
      char rank[1024];
      rank[0] = '\0';
      while (fgets(line, 100, f) != NULL)
      {
        strcat(rank, line);
        //printf("%s\n", line);
      }
      fclose(f);
      send(sockfd, rank, strlen(rank), 0);
    }
    else if (strcmp(buffer, SIGNAL_PLAY) == 0)
    {
      pthread_mutex_lock(&general_mutex); // Unecesary?
      // Create thread if we have no user waiting
      if (player_is_waiting == 0)
      { //bien dem xem có bn ng dang cho
        printf("Connected player, creating new game room...\n");
        pthread_create(&tid[0], NULL, &game_room, &sockfd); //ham tao ra 1 luong chay ham game_room
        pthread_mutex_unlock(&general_mutex);               // Unecesary?
        is_p1_playing = 1;
        while (is_p1_playing)
        {
        }
      }
      // If we've a user waiting join that room
      else
      {
        // Send user two signal
        printf("Connected player, joining game room... %d\n", sockfd);
        challenging_player = sockfd;          //socket id cua thang thu 2
        pthread_mutex_unlock(&general_mutex); // Unecesary?
        pthread_cond_signal(&player_to_join); //gui ra 1 tin hieu den o nhơ cua  bien
        is_p2_playing = 1;
        while (is_p2_playing)
        {
        }
      }
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("Syntax Error.\n");
    printf("Syntax: ./server PortNumber\n");
    return 0;
  }
  if (check_port(argv[1]) == 0)
  {
    printf("Port invalid\n");
    return 0;
  }
  int PORT = atoi(argv[1]);
  setlocale(LC_ALL, "en_US.UTF-8");

  int sockfd, client_socket, port_number, client_length;
  char buffer[64];
  struct sockaddr_in server_address, client;
  int n;

  // Conditional variable
  pthread_cond_init(&player_to_join, NULL);
  pthread_mutex_init(&general_mutex, NULL);

  /* First call to socket() function */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0)
  {
    perror("ERROR opening socket");
    exit(1);
  }

  /* Initialize socket structure */
  bzero((char *)&server_address, sizeof(server_address));

  server_address.sin_family = AF_INET;         //mac dinh
  server_address.sin_addr.s_addr = INADDR_ANY; //ip server
  server_address.sin_port = htons(PORT);       //port number

  /* Now bind the host address using bind() call.*/
  if (bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
  {
    perror("ERROR on binding");
    exit(1);
  }

  /* MAX_QUEUE */
  listen(sockfd, 20);
  printf("Server listening on port %d\n", PORT);

  readFile();

  while (1)
  {
    client_length = sizeof(client);
    // CHECK IF WE'VE A WAITING USER

    /* Accept actual connection from the client */
    client_socket = accept(sockfd, (struct sockaddr *)&client, (unsigned int *)&client_length);
    printf("New connection , socket fd is %d , ip is : %s , port : %d \n ", client_socket, inet_ntoa(client.sin_addr), ntohs(client.sin_port));

    // ktra ket noi co thanh cong hay k
    if (client_socket < 0)
    {
      pthread_exit(&tid[1]);
      perror("ERROR on accept");
      exit(1);
    }

    pthread_create(&tid[1], NULL, &on_request, &client_socket);
  }

  close(sockfd);

  return 0;
}
