#ifndef BAL_UTILITY_H
#define BAL_UTILITY_H

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

SCM exec_string_safe_history (void* toeval_v)
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
    {
      if (strcmp(book->accounts[i].name, name)==0)
        {
          return (book->accounts + i);
        }
    }
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
total_transactions (const account* acct)
{
  int i;
  double total = acct->ob;

  for (i=0; i < acct->n_tsct; i++)
    {
      total = total +
        acct->tscts[i].amount;
    }

  return scm_from_double(total);
}


#endif
