#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "linklist.h"

void getTime(char *str)
{
  struct tm *now = NULL;
  time_t time_value = 0;
  time_value = time(NULL);
  now = localtime(&time_value);
  sprintf(str, "%d-%d-%d#%d:%d:%d", now->tm_mday, now->tm_mon + 1, now->tm_year + 1900, now->tm_hour, now->tm_min, now->tm_sec);
}

void initList()
{
  root = NULL;
}

rank *initRankList(rank *pre, char username[], int point)
{
  rank *temp, *temp1, *temp2;
  temp = (rank *)malloc(sizeof(rank));
  strcpy(temp->username, username);
  temp->point = point;

  if (rankfirst == NULL)
  {
    rankfirst = temp;
    pre = rankfirst;
    temp->next = NULL;
  }
  else
  {
    temp1 = rankfirst;
    while (temp1)
    {
      if (temp->point > temp1->point)
      {
        if (temp1 == rankfirst)
        {
          temp->next = rankfirst;
          rankfirst = temp;
          break;
        }
        else
        {
          temp->next = temp2->next;
          temp2->next = temp;
          break;
        }
      }
      temp2 = temp1;
      if (temp1->next == NULL)
      {
        temp1->next = temp;
        temp->next = NULL;
        break;
      }
      temp1 = temp1->next;
    }
  }
}

void getID(char *t)
{
  struct timeval time;
  gettimeofday(&time, NULL);
  sprintf(t, "%ld%ld", time.tv_sec, time.tv_usec);
}

ClientInfo *newInfo(char *id, char user[], char addr[], int size, char logfile[])
{
  ClientInfo *info;
  int i;
  info = (ClientInfo *)malloc(sizeof(ClientInfo));
  info->id = (char *)malloc(strlen(id));
  strcpy(info->id, id);
  strcpy(info->user, user);
  strcpy(info->address, addr);
  info->size = size;
  info->table = (char *)malloc(size * size);
  for (i = 0; i < size * size; i++)
    (info->table)[i] = 0;
  strcpy(info->logfile, logfile);
  info->next = NULL;
}

ClientInfo *getInfo(char *id)
{
  ClientInfo *cur = root;
  while (cur != NULL)
  {
    if (strcmp(cur->id, id) == 0)
      return cur;
    cur = cur->next;
  }
  return NULL;
}

void freeInfo(ClientInfo *i)
{
  free(i->table);
  free(i->id);
}

char *addInfo(char addr[], int size, char user[])
{
  ClientInfo *i;
  ClientInfo *cur;
  char logfile[100];
  char id[30];
  char time[50];
  getID(id);
  getTime(time);
  sprintf(logfile, "%s#%s#%s#%s.log", user, id, addr, time);
  i = newInfo(id, user, addr, size, logfile);

  cur = root;
  while (cur != NULL && cur->next != NULL)
    cur = cur->next;

  if (cur == NULL)
  {
    i->next = NULL;
    root = i;
  }
  else
  {
    cur->next = i;
    i->next = NULL;
  }
  return i->id;
}

int removeInfo(char *id)
{
  ClientInfo *prev, *cur;
  cur = root;
  while (cur != NULL && strcmp(cur->id, id) != 0)
  {
    prev = cur;
    cur = cur->next;
  }

  if (cur == NULL)
    return -1;
  else if (cur == root)
  {
    root = root->next;
    freeInfo(cur);
    free(cur);
  }
  else
  {
    prev->next = cur->next;
    freeInfo(cur);
    free(cur);
  }

  return 0;
}

void printInfo(ClientInfo *i)
{
  if (i != NULL)
    printf("id= %s user = %s addr=%s size=%d log=%s\n", i->id, i->user, i->address, i->size, i->logfile);
}

void addToLinkList(char username[])
{
  rank *checkLast = rankfirst;
  rank *temp;
  temp = (rank *)malloc(sizeof(rank));

  strcpy(temp->username, username);
  temp->point = 0;
  temp->next = NULL;
  while (checkLast->next != NULL)
  {
    checkLast = checkLast->next;
  }

  checkLast->next = temp;

  writeFile();
}

rank *getAccount(char username[])
{
  rank *account;
  account = rankfirst;
  printf("%s\n", username);

  while (account)
  {
    if (strcmp(account->username, username) == 0)
    {
      break;
    }
    account = account->next;
  }
  return account;
}

void updatePoint(char username[], int point)
{
  rank *account;
  account = getAccount(username);
  account->point += point;

  writeFile();
}

void writeFile()
{
  FILE *f;
  rank *temp = rankfirst;
  remove("rank.txt");
  f = fopen("rank.txt", "w");

  while (temp)
  {
    fprintf(f, "%s %d\n", temp->username, temp->point);
    temp = temp->next;
  }
  fclose(f);
}

void readFile()
{
  FILE *f;
  char username[20];
  int point;
  rank *s1;
  f = fopen("rank.txt", "rt");
  if (f == NULL)
  {
    perror("Error while opening the file.\n");
    exit(EXIT_FAILURE);
  }
  else
  {
    while (1)
    {
      if (fscanf(f, "%s %d", username, &point) == EOF)
        break;
      s1 = initRankList(s1, username, point);
    }
    fclose(f);
  }
}

void printfRank()
{
  rank *temp;
  temp = rankfirst;
  while (temp)
  {
    printf("%s %d\n", temp->username, temp->point);
    temp = temp->next;
  }
}
