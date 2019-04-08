/* utility functions */

int
anyalpha (const char* str)
{
  int i;
  int any = strlen(str) > 0 ? 0 : 1;
  if (any==1) return any;
  for (i=0; i < strlen(str); i++)
    {
      if (isdigit(str[i]) == 0)
        {
          any = 1;
          break;
        }
    }
  return any;
}
  
int
sort_transactions (const void* a, const void* b)
{
  const tsct* a_t = a;
  const tsct* b_t = b;

  if (a_t->year > b_t->year) return 1;
  else if (a_t->year < b_t->year) return -1;

  if (a_t->month > b_t->month) return 1;
  else if (a_t->month < b_t->month) return -1;

  if (a_t->day > b_t->day) return 1;
  else if (a_t->day < b_t->day) return -1;
  else return 0;
}

SCM
handle_error (void* toeval_v,
              SCM key,
              SCM parameters)
{
  SCM message = scm_from_locale_string("Uncaught throw to ~A: ~A\n");
  scm_simple_format(SCM_BOOL_T,
                    message,
                    scm_list_2(key, parameters));
  return SCM_UNSPECIFIED;
}


SCM
exec_string_safe (void* toeval_v)
{
  char* toeval = (char*) toeval_v;
  SCM ret;
  char* command;
  int i;
  
  command = malloc(sizeof(char)*(strlen(toeval)+3));
  command[0] = '(';
  for (i=1; i <= strlen(toeval); i++)
    {
      command[i] = toeval[i-1];
    }
  command[strlen(toeval)+1] = ')';
  command[strlen(toeval)+2] = '\0';

  ret = scm_c_eval_string (command);

  free(command);

  return ret;
}

SCM
exec_string_safe_history (void* toeval_v)
{
  if (strcmp((char*) toeval_v, "") != 0)
    {
      add_history ((char*) toeval_v);
      return exec_string_safe (toeval_v);
    }
  else
    {
      return SCM_UNDEFINED;
    }
}

account*
find_account_in_book (struct book* book, const char* name)
{
  int i;
  for (i=0; i < book->n_account; i++)
    if (strcmp(book->accounts[i].name, name)==0)
      return (book->accounts + i);
  
  return NULL;
}

int
find_account_location_in_book (struct book* book, const char* name)
{
  int i;
  for (i=0; i < book->n_account; i++)
    if (strcmp(book->accounts[i].name, name)==0)
      return i;
  
  return -1;
}

SCM
tsct_to_scm (const tsct t)
{
  SCM amount = scm_from_double(t.amount);
  SCM year = scm_from_uint(t.year);
  SCM month = scm_from_uint(t.month);
  SCM day = scm_from_uint(t.day);
  SCM desc = scm_from_locale_string (t.desc);

  return scm_list_5(desc,amount,year,month,day);
}

SCM
acct_to_scm (const account a)
{
  SCM name = scm_from_locale_string(a.name);
  SCM type = scm_from_int(a.type);
  SCM ntsct = scm_from_int(a.n_tsct);
  SCM ob = scm_from_double(a.ob);

  return scm_list_4(name,type,ntsct,ob);
}

char*
create_tmp_dir ()
{
  int i;
  char* tmp_dir;
  char buffer[100];
  FILE* fp;

  fp = popen("mktemp -d", "r");
  if (fp == NULL)
    {
      fprintf(stderr, "Could not create temporary directory.\n");
      return NULL;
    }

  i = 0;
  while (fgets(buffer, sizeof(buffer)-1, fp) != NULL)
    {
      if (i==0)
        {
          tmp_dir = malloc(sizeof(char)*101);
          strcpy(tmp_dir, buffer);
        }
      else
        {
          tmp_dir = realloc(tmp_dir, sizeof(char)*(i+1)*101);
          strcat(tmp_dir, buffer);
        }
      i++;
    }
  tmp_dir[strcspn(tmp_dir, "\n")] = 0;
  pclose(fp);

  return tmp_dir;
}

/* function to check whether transact after current time */
int
tsct_before_today (const tsct t)
{
  return (((bal_curtime->tm_year+1900) > (t.year)) ||
          (((bal_curtime->tm_mon+1) > (t.month)) &&
           ((bal_curtime->tm_year+1900) == (t.year))) ||
          ((bal_curtime->tm_mday >= t.day) &&
           (bal_curtime->tm_year+1900) == t.year &&
           (bal_curtime->tm_mon+1) == t.month));
}

/* function to add up transactions */
SCM
total_transactions (const account* acct)
{
  int i;
  double total = acct->ob;
  double total_cur = acct->ob;

  for (i=0; i < acct->n_tsct; i++)
    {
      total = total + acct->tscts[i].amount;
      if (tsct_before_today(acct->tscts[i]))
	total_cur = total_cur + acct->tscts[i].amount;
    }

  return scm_cons(scm_from_double(total_cur),
                  scm_from_double(total));
}

