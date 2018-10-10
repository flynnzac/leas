#ifndef BAL_WRITE_H
#define BAL_WRITE_H

void
write_transactions (const char* file, account* acct)
{
  int i;
  FILE* fp;

  fp = fopen (file, "w");
  
  for (i=0; i < acct->n_tsct; i++)
    {
      fprintf(fp, "%s,%f,%u-%u-%u,%s\n",
              acct->name, acct->tscts[i].amount,
              acct->tscts[i].year,
              acct->tscts[i].month,
              acct->tscts[i].day,
              acct->tscts[i].desc);
    }

  fclose(fp);
}

void
write_accounts (const char* file)
{
  int i;
  FILE* fp;

  fp = fopen (file, "w");
  for (i=0; i < bal_book.n_account; i++)
    {
      switch (bal_book.accounts[i].type)
        {
        case EXPENSE:
          fprintf(fp,"%s,", "expense");
          break;
        case INCOME:
          fprintf(fp,"%s,", "income");
          break;
        case ASSET:
          fprintf(fp,"%s,", "asset");
          break;
        case LIABILITY:
          fprintf(fp,"%s,", "liability");
          break;
        }
      fprintf(fp, "%s,%f,%f\n",
              bal_book.accounts[i].name,
              bal_book.accounts[i].ob,
              bal_book.accounts[i].curr);
    }
  fclose(fp);
}

void
write_out (const char* base)
{
  char* account_fn;
  char* tsct_fn;
  char* dir_cmd;
  char* base_dir;
  char* tar_cmd;
  char* rm_cmd;
  int i;

  dir_cmd = malloc(sizeof(char)*(strlen(base)+strlen("mkdir ")+1));
  strcpy(dir_cmd, "mkdir ");
  strcat(dir_cmd, base);
  system(dir_cmd);
  free(dir_cmd);

  account_fn = malloc(sizeof(char)*(strlen(base)+strlen("accounts.csv")+2));
  strcpy(account_fn, base);
  strcat(account_fn, "/");
  strcat(account_fn, "accounts.csv");

  write_accounts (account_fn);
  free(account_fn);

  for (i=0; i < bal_book.n_account; i++)
    {
      tsct_fn = malloc
        (sizeof(char)*(strlen(bal_book.accounts[i].name) +
                       strlen(base) +
                       strlen("/.csv") +
                       1));

      strcpy(tsct_fn, base);
      strcat(tsct_fn, "/");
      strcat(tsct_fn, bal_book.accounts[i].name);
      strcat(tsct_fn, ".csv");
      
      write_transactions (tsct_fn, &bal_book.accounts[i]);
      free(tsct_fn);
    }

  tar_cmd = malloc(sizeof(char)*(strlen("tar caf ")+strlen(base)+strlen(".baltar ")+strlen(base)+1));
  strcpy(tar_cmd, "tar caf ");
  strcat(tar_cmd, base);
  strcat(tar_cmd, ".baltar ");
  strcat(tar_cmd, base);
  system(tar_cmd);
  free(tar_cmd);

  rm_cmd = malloc(sizeof(char)*(strlen("rm -R ")+strlen(base)+1));
  strcpy(rm_cmd, "rm -R ");
  strcat(rm_cmd, base);
  system(rm_cmd);
  free(rm_cmd);
  
}


#endif
