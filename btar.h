/* functions for reading .btar files */

void
transaction_cb1 (void* s, size_t len, void* data)
{
  account* acct = (account*) data;
  char buf[len+1];
  int i; 
  struct tm t;

  for (i=0; i < len; i++)
    buf[i] = ((char*)s)[i];

  buf[len] = '\0';
  
  if (acct->n_pos > 0 || (strcmp(buf,acct->name)==0))
    {
      if (acct->n_pos==0)
        {
          if (acct->n_tsct==0)
	    acct->tscts = malloc(sizeof(tsct));
          else
	    acct->tscts = realloc(acct->tscts,
				  (acct->n_tsct+1)*sizeof(tsct));
        }

      switch (acct->n_pos)
        {
        case 1:
          acct->tscts[acct->n_tsct].amount = atof(buf);
          break;
        case 2:
          strptime(buf, "%Y-%m-%d", &t);
	  set_tsct_time(&acct->tscts[acct->n_tsct], &t);
          break;
        case 3:
	  acct->tscts[acct->n_tsct].desc = copy_string(buf);
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
  char buf[len+1];
  int i;
  for (i=0; i < len; i++)
    buf[i] = ((char*)s)[i];

  buf[len] = '\0';
  
  struct book* book = (struct book*) data;
  if (book->n_pos==0)
    {
      if (book->n_account==0)
	book->accounts = malloc(sizeof(account));
      else
	book->accounts = realloc(book->accounts,
				 (book->n_account+1)*sizeof(account));
    }

  switch (book->n_pos)
    {
    case 0:
      book->accounts[book->n_account].type = account_type_from_string(buf);
      if (book->accounts[book->n_account].type < 0)
        {
          fprintf(stderr, "Invalid account type: %s.\n", buf);
          exit(1);
        }
      break;
    case 1:
      book->accounts[book->n_account].name = copy_string(buf);
      break;
    case 2:
      book->accounts[book->n_account].ob = atof(buf);
      break;
    default:
      break;
    }
  if (book->n_pos==2)
    {
      book->n_pos = 0;
      book->accounts[book->n_account].n_tsct = 0;
      book->n_account = book->n_account + 1;
    }
  else if (book->n_pos < 2)
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
read_all_transactions_into_book (struct book* book,
                                 char* base,
                                 const char* tmp_dir)

{
  FILE* fp;
  int i, error;
  struct csv_parser p;
  char buf[1024];
  size_t bytes_read;
  char* name;

  for (i=0; i < book->n_account; i++)
    {
      name = malloc(sizeof(char)*(strlen(basename(base)) +
                                  strlen(tmp_dir) +
                                  strlen("//") +
                                  strlen(book->accounts[i].name)+
                                  strlen(".csv")+1));

      strcpy(name, tmp_dir);
      strcat(name, "/");
      strcat(name, basename(base));
      strcat(name, "/");
      strcat(name, book->accounts[i].name);
      strcat(name, ".csv");
      
      error = csv_init (&p,0);
      if (error != 0)
        {
          fprintf(stderr, "CSV Parser initialization failed.\n");
          exit(1);
        }

      fp = fopen(name, "rb");
      free(name);
      if (fp==NULL)
        {
          fprintf(stderr, "Data not found for account: %s\n", book->accounts[i].name);
          continue;
        }

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
    }
}

int
read_in (char* base)
{
  char* untar_cmd;
  char* account_file;
  char* rm_cmd;
  char* tmp_dir;

  /* create tmp dir */
  tmp_dir = create_tmp_dir();
  if (tmp_dir==NULL)
    return 1;

  /* untar archive */
  untar_cmd = malloc(sizeof(char)*(strlen("tar xaf ")+
                                   strlen(base)+
                                   strlen(".btar -C ")+
                                   strlen(tmp_dir)+1));

  sprintf(untar_cmd, "tar xaf %s.btar -C %s", base, tmp_dir);
  system(untar_cmd);
  free(untar_cmd);

  /* read in accounts */
  account_file = malloc(sizeof(char)*(strlen(basename(base)) +
                                      strlen(tmp_dir) +
                                      strlen("/accounts")+
                                      12));

  sprintf(account_file, "%s/%s/accounts", tmp_dir, basename(base));
  read_book_accounts_from_csv (&bal_book, account_file);
  free(account_file);

  /* read in transactions */
  read_all_transactions_into_book (&bal_book,
                                   base,
                                   tmp_dir);

  /* clean up */
  rm_cmd = malloc(sizeof(char)*(strlen("rm -R ") +
                                strlen(tmp_dir) +
				strlen("/") +
                                strlen(basename(base))+
                                1));

  sprintf(rm_cmd, "rm -R %s/%s", tmp_dir, basename(base));
  system(rm_cmd);
  free(rm_cmd);
  free(tmp_dir);
  
  /* return success */
  return 0;
  
}

/* functions for writing .btar files */

void
write_transactions (const char* file, account* acct)
{
  int i;
  FILE* fp;

  fp = fopen (file, "w");
  
  for (i=0; i < acct->n_tsct; i++)
    {
      csv_fwrite(fp, acct->name, strlen(acct->name));
      fprintf(fp, ",%f,%u-%u-%u,",
              acct->tscts[i].amount,
              acct->tscts[i].year,
              acct->tscts[i].month,
              acct->tscts[i].day);
      csv_fwrite(fp, acct->tscts[i].desc,
                 strlen(acct->tscts[i].desc));
      fprintf(fp, "\n");
    }

  fclose(fp);
}

int
write_accounts (const char* file)
{
  int i;
  FILE* fp;
  char* type;

  fp = fopen (file, "w");
  if (fp==NULL)
    {
      fprintf(stderr, "Could not open file to write accounts.\n");
      return 1;
    }
  for (i=0; i < bal_book.n_account; i++)
    {
      type = account_type_to_string(bal_book.accounts[i].type);
      fprintf(fp, "%s,", type);
      csv_fwrite(fp, bal_book.accounts[i].name,
                 strlen(bal_book.accounts[i].name));
      free(type);
      fprintf(fp, ",%f\n", bal_book.accounts[i].ob);
    }
  fclose(fp);
  return 0;
}

int
write_out (char* base)
{
  char* cmd;
  char* account_fn;
  char* tsct_fn;
  char* tmp_dir;

  int i;

  tmp_dir = create_tmp_dir();
  if (tmp_dir==NULL)
    return 1;

  cmd = malloc(sizeof(char)*(strlen(basename(base))+
			     strlen(tmp_dir)+
			     strlen("mkdir /")+1));

  sprintf(cmd, "mkdir %s/%s", tmp_dir, basename(base));
  system(cmd);
  free(cmd);
  
  account_fn = malloc(sizeof(char)*(strlen(basename(base))+
                                    strlen(tmp_dir)+
                                    strlen("accounts")+
				    strlen("//")+1));

  sprintf(account_fn, "%s/%s/accounts", tmp_dir, basename(base));
  i = write_accounts (account_fn);
  free(account_fn);
  
  if (i != 0)
    {
      free(tmp_dir);
      return i;
    }

  for (i=0; i < bal_book.n_account; i++)
    {
      tsct_fn = malloc
        (sizeof(char)*(strlen(bal_book.accounts[i].name) +
                       strlen(basename(base)) +
                       strlen(tmp_dir) + 
                       strlen("//.csv") +
                       1));
      sprintf(tsct_fn, "%s/%s/%s.csv", tmp_dir, basename(base),
	      bal_book.accounts[i].name);
      
      write_transactions (tsct_fn, &bal_book.accounts[i]);
      free(tsct_fn);
    }

  cmd = malloc(sizeof(char)*(strlen("tar caf ")+
			     strlen(base)+
			     strlen(".btar -C ")+
			     strlen(tmp_dir)+
			     strlen(basename(base))+2));
  sprintf(cmd, "tar caf %s.btar -C %s %s", base, tmp_dir, basename(base));
  system(cmd);
  free(cmd);

  cmd = malloc(sizeof(char)*(strlen("rm -R ")+
			     strlen(tmp_dir)+
			     1));

  sprintf(cmd, "rm -R %s", tmp_dir);
  system(cmd);
  free(cmd);
  free(tmp_dir);

  return 0;
}
