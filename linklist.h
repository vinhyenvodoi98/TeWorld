#include <stdlib.h>
#include <string.h>
typedef struct _ClientInfo
{
  char *id;
  char user[100];
  char address[20];
  int size;
  char *table;
  char logfile[100];
  struct _ClientInfo *next;
} ClientInfo;

typedef struct rank
{
  char username[20];
  int point;
  struct rank *next;
} rank;

rank *rankfirst;

rank *initRankList();

ClientInfo *root;

void initList();

ClientInfo *newInfo(char *id, char user[], char addr[], int size, char logfile[]);

ClientInfo *getInfo(char *id);

void freeInfo(ClientInfo *i);

char *addInfo(char addr[], int size, char user[]);

int removeInfo(char *id);

void printInfo(ClientInfo *i);

void updatePoint();

void addToLinkList(char username[]);

void writeFile();

void readFile();

void printfRank();