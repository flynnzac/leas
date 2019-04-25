/* selection functions */

char*
bal_select_account (const char* prompt)
{
  int j, ndigit;
  char* c;

  if (bal_book.n_account==0)
    {
      printf("No account loaded.\n");
      return NULL;
    }
  else
    {
      ndigit = (bal_book.n_account / 10) + 1;
      do
        {
          for (j=0; j < bal_book.n_account; j++)
            {
              printf("%*d: %s\n", ndigit+1, j, bal_book.accounts[j].name);
            }
          c = readline(prompt);
        } while (strcmp(c,"")  && (anyalpha(c) || atoi(c) < 0 ||
                                   atoi(c) >= bal_book.n_account));

      return c;
    }
}

int
bal_select_transaction (account* acct)
{
  int i,n,ndigit,maxlen,end;
  char* option;

  maxlen = 0;
  end = 0;
  while ((end < acct->n_tsct) && tsct_before_today(acct->tscts[end]))
    end++;

  n = (end - bal_select_tsct_num) >= 0 ?
    (end - bal_select_tsct_num) : 0;
  
  ndigit = (acct->n_tsct / 10) + 1;

  for (i=n; i < end; i++)
    if (maxlen < strlen(acct->tscts[i].desc))
      maxlen = strlen(acct->tscts[i].desc);
  do
    {
      for (i=n; i < end; i++)
	printf("%*d: %4u-%02u-%02u %-*s % 12.2f\n",
	       ndigit,
	       i,
	       acct->tscts[i].year,
	       acct->tscts[i].month,
	       acct->tscts[i].day,
	       maxlen+1,
	       acct->tscts[i].desc,
	       acct->tscts[i].amount);

      option = readline ("Transaction #: ");
    }
  while (strcmp(option,"") && (anyalpha(option) || atoi(option) < 0 ||
                               atoi(option) >= acct->n_tsct));
  if (strcmp(option,"")!=0)
    {
      i = atoi(option);
      free(option);
      return i;
    }

  return -1;
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

account_type
bal_select_account_type (char* prompt)
{
  int k;
  char* opt;
  
  printf("%d: Expense\n", EXPENSE);
  printf("%d: Income\n", INCOME);
  printf("%d: Asset\n", ASSET);
  printf("%d: Liability\n", LIABILITY);
  opt = readline (prompt);
  if (anyalpha(opt) > 0)
    {
      k = -1;
    }
  else
    {
      k = atoi(opt);
    }

  return k;
}
