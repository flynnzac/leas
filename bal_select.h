#ifndef BAL_SELECT_H
#define BAL_SELECT_H

#include <readline/readline.h>

char*
bal_select_account ()
{
  int j;
  char* c;
  if (bal_book.n_account==0)
    {
      printf("No account loaded.\n");
      return NULL;
    }
  else
    {
      for (j=0; j < bal_book.n_account; j++)
        {
          printf("%d: %s\n", j, bal_book.accounts[j].name);
        }
      c = readline("Account: ");

      return c;
    }
}

int
bal_select_transaction (account* acct)
{
  int i,n;
  char* option;

  n = (acct->n_tsct - 19) >= 0 ? (acct->n_tsct - 19) : 0;

  for (i=n; i < acct->n_tsct; i++)
    {
      printf("%d: %u-%u-%u %s %f\n", i,
             acct->tscts[i].year,
             acct->tscts[i].month,
             acct->tscts[i].day,
             acct->tscts[i].desc,
             acct->tscts[i].amount);
    }

  option = readline ("Transaction #: ");
  i = atoi(option);

  free(option);
  return i;
}

int
bal_select_day (struct tm* curtime_info,
                char** year,
                char** month,
                char** day)
{
  char *yprompt, *mprompt, *dprompt;
  char* tmp;

  yprompt = malloc(sizeof(char)*(strlen("Year []: ")+10));
  sprintf(yprompt, "Year [%d]: ", curtime_info->tm_year+1900);
  tmp = readline(yprompt);
  *year = tmp;
  free(yprompt);

  mprompt = malloc(sizeof(char)*(strlen("Month []: ")+10));
  sprintf(mprompt, "Month [%d]: ", curtime_info->tm_mon+1);
  tmp = readline(mprompt);
  *month = tmp;
  free(mprompt);
  
  dprompt = malloc(sizeof(char)*(strlen("Day []: ")+10));
  sprintf(dprompt, "Day [%d]: ", curtime_info->tm_mday);
  tmp = readline(dprompt);
  *day = tmp;
  free(dprompt);

  return 0;
}

                      

#endif
