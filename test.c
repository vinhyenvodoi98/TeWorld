#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>

#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>
#include <locale.h>
#include <signal.h>

#include "linklist.h"
#include "checkinput.h"
#include "board.c"

void copyFile() {
	FILE* sFile = fopen("tmp.txt", "r+");
	FILE* dFile = fopen("rank.txt", "w+");
	char ch;
	while( ( ch = fgetc(sFile) ) != EOF )
      fputc(ch, dFile);

  fclose(sFile);
  fclose(dFile);
}

struct user_rank
{
	char username[100];
	int point;
};

struct user_rank ranks[100],abc;
int ur_i = 0;
char temp1[100],temp2[100],temp3[100];
void increasePoint(char* username) {
  FILE* f = fopen("rank.txt", "r+");
  FILE* tmpFile = fopen("tmp.txt", "w+");
  if(f == NULL ){
    printf("Error open file!!!\n");  
  }
  char line[50];
  char rank[1024];
  rank[0] = '\0';
  while(fgets(line, 100, f) !=NULL) {
    char* org;
    strcpy(org, line);
    char* user = strtok(line, "#");
    char* p = strtok(NULL, "#");
      //printf("point: %s\n", p);
    int point = atoi(p);
    
    strcpy(ranks[ur_i].username , user);
    ranks[ur_i].point = point;


    printf("%s....%s\n", user,ranks[ur_i].username);

    if (strcmp(user, username) == 0) {
      ranks[ur_i].point += 50;
    }
    ur_i += 1;
 //   printf("%d\n", ur_i);

    //
  }

  for (int i = 0; i < ur_i; ++i)
  {
  	for (int j = 0; j < ur_i; ++j)
  	{
		if (ranks[i].point > ranks[j].point) 
		{
			char* tempName;
			int tempPoint;

			strcpy(tempName , ranks[i].username);
			tempPoint = ranks[i].point;

			strcpy(ranks[i].username, ranks[j].username);
			ranks[i].point = ranks[j].point;

			strcpy(ranks[j].username, tempName);
			ranks[j].point = tempPoint;
		}
		  		/* code */
  	}
  }
  // for (int i = 0; i < ur_i; ++i)
  // {
  // 	/* code */
  // 	for (int j = 0; j < ur_i; ++j)
  // 	{
  // 		/* code */
  // 		if (ranks[i].point > ranks[j].point)
  // 		{
  // 			abc = ranks[i];
  // 			ranks[i] = ranks[j];
  // 			ranks[j] = abc;
  // 		}
  // 	}
  // }


    for (int i = 0; i < ur_i; ++i)
    {
      printf("%s#%d\n", ranks[i].username, ranks[i].point);
      fputs(ranks[i].username, tmpFile);
      fputs("#", tmpFile);
      fprintf(tmpFile, "%d", ranks[i].point);
      fputs("\n", tmpFile);	
    }

  fclose(f);
  fclose(tmpFile);
  copyFile();
}

int main() {
	increasePoint("hanh");
	return 0;
}