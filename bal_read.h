#ifndef BAL_READ_H
#define BAL_READ_H

#include <csv.h>

/* functions to read in balance data from file */

void
transaction_cb1 (void* s, size_t len, void* data)
{
  account* acct = (account*) data;
  char buf[len+1];
  int i;
  struct tm t;

  for (i=0; i < len; i++)
    {
      buf[i] = ((char*)s)[i];
    }
  buf[len] = '\0';
  
  

  if (acct->n_pos > 0 || (strcmp(buf,acct->name)==0))
    {
      printf("%s\n", buf);
      printf("%d\n", len);


      if (acct->n_pos==0)
        {
          if (acct->n_tsct==0)
            {
              acct->tscts = malloc(sizeof(tsct));
            }
          else
            {
              acct->tscts = realloc(acct->tscts,
                                    (acct->n_tsct+1)*sizeof(tsct));
            }
        }

      switch (acct->n_pos)
        {
        case 1:
          acct->tscts[acct->n_tsct].amount = atof(buf);
          printf("Transaction value: %f\n", acct->tscts[acct->n_tsct].amount);
          break;
        case 2:
          strptime(buf, "%Y-%m-%d", &t);
          acct->tscts[acct->n_tsct].year = t.tm_year+1900;
          acct->tscts[acct->n_tsct].month = t.tm_mon+1;
          acct->tscts[acct->n_tsct].day = t.tm_mday;
          break;
        case 3:
          acct->tscts[acct->n_tsct].desc =
            malloc(sizeof(char)*(strlen(buf)+1));
          strcpy(acct->tscts[acct->n_tsct].desc,
                 buf);
          break;
        default:
          break;
        }
      if (acct->n_pos == 3)
        {
          acct->n_pos = 0;
          acct->n_tsct = acct->n_tsct + 1;
        }
      else if (acct->n_pos < 3)
        {
          acct->n_pos = acct->n_pos + 1;
        }
    }
}

void
account_cb1 (void* s, size_t len, void* data)
{
  int error;
  char buf[len+1];
  int i;
  for (i=0; i < len; i++)
    {
      buf[i] = ((char*)s)[i];
    }
  buf[len] = '\0';


  struct book* book = (struct book*) data;
  if (book->n_pos==0)
    {
      if (book->n_account==0)
        {
          book->accounts = malloc(sizeof(account));
        }
      else
        {
          book->accounts = realloc(book->accounts,
                                   (book->n_account+1)*sizeof(account));
        }
    }

  switch (book->n_pos)
    {
    case 0:
      if (!strcmp(buf, "EXPENSE"))
        {
          book->accounts[book->n_account].type = EXPENSE;
        }
      else if (!strcmp(buf, "INCOME"))
        {
          book->accounts[book->n_account].type = INCOME;
        }
      else if (!strcmp(buf, "ASSET"))
        {
          book->accounts[book->n_account].type = ASSET;
        }
      else if (!strcmp(buf, "LIABILITY"))
        {
          book->accounts[book->n_account].type = LIABILITY;
        }
      else
        {
          fprintf(stderr, "Invalid account type.\n");
          exit(1);
        }
      break;
    case 1:
      book->accounts[book->n_account].name = malloc(sizeof(char)*(strlen(buf)+1));
      strcpy(book->accounts[book->n_account].name, buf);
      break;
    case 2:
      book->accounts[book->n_account].ob = atof(buf);
      break;
    case 3:
      book->accounts[book->n_account].curr = atof(buf);
      break;
    default:
      break;
    }
  if (book->n_pos==3)
    {
      book->n_pos = 0;
      book->accounts[book->n_account].n_tsct = 0;
      book->n_account = book->n_account + 1;
    }
  else if (book->n_pos < 3)
    {
      book->n_pos = book->n_pos + 1;
    }
}

void
read_book_accounts_from_csv (struct book* book,
                             const char* account_file)
{
  FILE* fp;
  struct csv_parser p;
  char buf[1024];
  size_t bytes_read;
  int error;

  book->n_account = 0;
  book->n_pos = 0;
  /* Make accounts from account_file */

  error = csv_init (&p, 0);
  if (error != 0)
    {
      fprintf(stderr, "CSV Parser initialization failed.\n");
      exit(1);
    }

  fp = fopen(account_file, "rb");
  if (!fp) exit(1);

  while ((bytes_read=fread(buf,1,1024,fp)) > 0)
    {
      if (csv_parse (&p, buf, bytes_read, account_cb1, NULL, book) != bytes_read)
        {
          fprintf(stderr,
                  "Error parsing file: %s\n",
                  csv_strerror(csv_error(&p)));
          exit(1);
        }
    }
  csv_fini(&p, account_cb1, NULL, book);
  fclose(fp);
  csv_free(&p);


}

void
read_all_transactions_into_book (struct book* book)

{
  FILE* fp;
  int i, error;
  struct csv_parser p;
  char buf[1024];
  size_t bytes_read;
  char* title;


  for (i=0; i < book->n_account; i++)
    {
      title = malloc(sizeof(char)*(strlen(book->accounts[i].name)+
                                   strlen(".csv")+1));
      strcpy(title, book->accounts[i].name);
      strcat(title, ".csv");
      
      error = csv_init (&p,0);
      if (error != 0)
        {
          fprintf(stderr, "CSV Parser initialization failed.\n");
          exit(1);
        }

      fp = fopen(title, "rb");
      if (!fp) exit(1);

      book->accounts[i].n_pos = 0;
      book->accounts[i].n_tsct = 0;

      while ((bytes_read=fread(buf,1,1024,fp)) > 0)
        {
          if (csv_parse (&p, buf, bytes_read,
                         transaction_cb1, NULL, &book->accounts[i]) !=
              bytes_read)
            {
              fprintf(stderr,
                      "Error parsing file: %s\n",
                      csv_strerror(csv_error(&p)));
              exit(1);
            }
        }
      csv_fini(&p, transaction_cb1, NULL, &book->accounts[i]);
      fclose(fp);
      csv_free(&p);
      free(title); 
    }
}

int
read_in (const char* base)
{
  /* untar archive */

  /* read in accounts */

  /* read in transactions */
  
}



#endif
