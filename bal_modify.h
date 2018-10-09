#ifndef BAL_ENTRY_H
#define BAL_ENTRY_H

int
bal_add_tsct (struct book* book,
              SCM account_name,
              SCM amount,
              SCM day,
              SCM desc)
{
  char* account_c = scm_to_locale_string (account_name);
  char* desc_c = scm_to_locale_string (desc);
  double amount_c = scm_to_double(amount);
  account* acct = find_account_in_book (book, account_c);
  char* day_str_c = scm_to_locale_string (day);
  tsct t;
  struct tm tm_c;

  strptime(day_str_c, "%Y-%m-%d", &tm_c);

  t.year = tm_c.tm_year + 1900;
  t.month = tm_c.tm_mon + 1;
  t.day = tm_c.tm_mday;
  t.amount = amount_c;
  t.desc = malloc(sizeof(char)*(strlen(desc_c)+1));
  strcpy(t.desc, desc_c);

  if (acct->n_tsct==0)
    {
      acct->tscts = malloc(sizeof(tsct));
    }
  else
    {
      acct->tscts = realloc(acct->tscts,
                            (acct->n_tsct+1)*
                            sizeof(tsct));
    }

  acct->tscts[acct->n_tsct] = t;
  acct->n_tsct = acct->n_tsct + 1;

  qsort(acct->tscts, acct->n_tsct, sizeof(tsct), sort_transactions);
  
  free(account_c);
  free(desc_c);
  free(day_str_c);

  return 0;
}

int
bal_add_account (struct book* book,
                 SCM name,
                 SCM type,
                 SCM opening_balance,
                 SCM currency)
{

  char* type_c = scm_to_locale_string (type);
  char* name_c = scm_to_locale_string (name);
  double ob_c = scm_to_double(opening_balance);
  double curr_c = scm_to_double(currency);
  account_type type_ac;

  if (book->n_account == 0)
    {
      book->accounts = malloc(sizeof(account));
    }
  else
    {
      book->accounts = realloc(book->accounts,
                               sizeof(account)*(book->n_account+1));
    }

  if (strcmp(type_c, "expense")==0)
    {
      type_ac = EXPENSE;
    }
  else if (strcmp(type_c, "income")==0)
    {
      type_ac = INCOME;
    }
  else if (strcmp(type_c, "asset")==0)
    {
      type_ac = ASSET;
    }
  else if (strcmp(type_c, "liability")==0)
    {
      type_ac = LIABILITY;
    }

  book->accounts[book->n_account].type = type_ac;
  book->accounts[book->n_account].n_tsct = 0;
  book->accounts[book->n_account].n_pos = 0;
  book->accounts[book->n_account].name =
    malloc(sizeof(char)*(strlen(name_c)+1));
  strcpy(book->accounts[book->n_account].name,
         name_c);

  book->accounts[book->n_account].ob = ob_c;
  book->accounts[book->n_account].curr = curr_c;

  book->n_account = book->n_account + 1;

  free(type_c);
  free(name_c);
  
  return 0;
}

int
bal_edit_transaction (tsct* t)
{
  char* option1;
  char* option2;
  char** year = malloc(sizeof(char*));
  char** month = malloc(sizeof(char*));
  char** day = malloc(sizeof(char*));

  struct tm tm;

  tm.tm_year = t->year-1900;
  tm.tm_mon = t->month - 1;
  tm.tm_mday = t->day;

  option1 = readline ("Amount: ");
  if (strcmp(option1,"") != 0)
    {
      t->amount = atof(option1);
    }

  bal_select_day (&tm, year, month, day);

  if (strcmp(*year,"") != 0)
    {
      t->year = atoi(*year);
    }

  if (strcmp(*month,"") != 0)
    {
      t->month = atoi(*month);
    }

  if (strcmp(*day,"") != 0)
    {
      t->day = atoi(*day);
    }

  option2 = readline("Description: ");

  if (strcmp(option2, "") != 0)
    {
      free(t->desc);
      t->desc = malloc(sizeof(char)*(strlen(option2)+1));
      strcpy(t->desc, option2);
    }

  free(option2);


  free(year);
  free(month);
  free(day);

  return 0;
  
}

  
  
                     
                     
    
#endif
