#ifndef BAL_INTERFACE_H
#define BAL_INTERFACE_H

#include <readline/readline.h>
#include "bal_call_with_opts.h"

/* Add transactions and accounts */

SCM
bal_at (SCM account,
        SCM amount,
        SCM desc,
        SCM day)
{
  bal_add_tsct (&bal_book,
                account,
                amount,
                day,
                desc);


  bal_cur_acct = account;
  return SCM_UNDEFINED;
  
}

SCM
bal_aa (SCM name,
        SCM type,
        SCM ob,
        SCM currency)
{
  if (SCM_UNBNDP(currency))
    currency = scm_from_int (1);
  

  bal_add_account (&bal_book,
                   name,
                   type,
                   ob,
                   currency);

  bal_cur_acct = name;
  return SCM_UNDEFINED;
  
}

/* Edit existing transactions and accounts */

SCM
bal_et (SCM at_pair)
{

  SCM account = scm_car (at_pair);
  SCM tsct = scm_cdr (at_pair);
  
  int k = scm_to_int (account);
  int j = scm_to_int (tsct);

  bal_edit_transaction (&bal_book.accounts[k].tscts[j]);
  
  return SCM_UNDEFINED;
}

SCM
bal_ea (SCM cur_name,
        SCM name)
{
  char* cur_name_c = scm_to_locale_string (cur_name);
  char* name_c = scm_to_locale_string (name);
  account* acct = find_account_in_book(&bal_book, cur_name_c);

  free(acct->name);
  acct->name = malloc(sizeof(char)*(strlen(name_c)+1));
  strcpy(acct->name, name_c);

  char* current_account = scm_to_locale_string (bal_cur_acct);

  if (strcmp(current_account, cur_name_c)==0)
    {
      bal_cur_acct = name;
    }

  free(cur_name_c);
  free(name_c);
  free(current_account);

  return SCM_UNDEFINED;
}

/* Delete existing transactions and accounts */

SCM
bal_dt (SCM at_pair)
{
  SCM account = scm_car (at_pair);
  SCM tsct = scm_cdr (at_pair);
  
  int k = scm_to_int (account);
  int j = scm_to_int (tsct);
  int i;

  free(bal_book.accounts[k].tscts[j].desc);
  for (i=j; i < (bal_book.accounts[k].n_tsct-1); i++)
    {
      bal_book.accounts[k].tscts[i] = bal_book.accounts[k].tscts[i+1];
    }

  bal_book.accounts[k].n_tsct = bal_book.accounts[k].n_tsct - 1;
  return SCM_UNDEFINED;
}


SCM
bal_da (SCM account)
{
  char* account_c = scm_to_locale_string (account);
  int i;

  for (i=0; i < bal_book.n_account; i++)
    {
      if (strcmp(bal_book.accounts[i].name, account_c)==0)
        {
          break;
        }
    }

  delete_account (bal_book.accounts + i);
  bal_book.n_account = bal_book.n_account - 1;
  for (i=i; i < bal_book.n_account; i++)
    {
      bal_book.accounts[i] = bal_book.accounts[i+1];
    }

  char* current_account = scm_to_locale_string (bal_cur_acct);
  if (strcmp(current_account, account_c)==0)
    {
      bal_cur_acct = scm_from_locale_string (bal_book.accounts[0].name);
    }
  return SCM_UNDEFINED;
  
}


/* Get the current account name */

SCM
bal_get_current_acct ()
{
  return bal_cur_acct;
}

/* get "num" most recent transactions */

SCM
bal_get_transactions (SCM acct, SCM num)
{
  int num_c;
  SCM ret;
  char* acct_c;

  acct_c = scm_to_locale_string (acct);

  account* a = find_account_in_book (&bal_book, acct_c);
  int i,n;

  if (a->n_tsct == 0) return SCM_UNDEFINED;

  n = num_c < a->n_tsct ? num_c : a->n_tsct;
  ret = SCM_EOL;
  for (i=0; i < n; i++)
    {
      ret = scm_append (scm_list_2(ret,
                                   scm_list_1(tsct_to_scm(a->tscts[i]))));
    }

  free(acct_c);
  return ret;
}

/* Get all transactions from an account */

SCM
bal_get_all_transactions (SCM acct)
{
  int num_c;
  SCM ret;
  char* acct_c;

  acct_c = scm_to_locale_string (acct);

  account* a = find_account_in_book (&bal_book, acct_c);
  int i,n;

  if (a->n_tsct == 0) return SCM_UNDEFINED;

  ret = SCM_EOL;
  for (i=0; i < a->n_tsct; i++)
    {
      ret = scm_append (scm_list_2(ret,
                                   scm_list_1(tsct_to_scm(a->tscts[i]))));
    }

  free(acct_c);
  return ret;
}

SCM
bal_get_transactions_by_regex (SCM acct_s, SCM regex_s)
{
  char* regex_txt = scm_to_locale_string (regex_s);
  char* acct_c = scm_to_locale_string (acct_s);
  account* acct = find_account_in_book (&bal_book, acct_c);

  regex_t regex;
  int i, error;
  SCM list, trans_list;

  list = SCM_EOL;

  error = regcomp(&regex, regex_txt, 0);
  if (error)
    {
      fprintf(stderr, "Bad regex.\n");
      return SCM_UNDEFINED;
    }
  for (i=0; i < acct->n_tsct; i++)
    {
      error = regexec(&regex, acct->tscts[i].desc,
                      0, NULL, 0);
      if (!error)
        {
          trans_list = scm_list_1(tsct_to_scm(acct->tscts[i]));
          list = scm_append (scm_list_2(list, trans_list));
        }
    }
  
  free(regex_txt);
  free(acct_c);
  return list;
}

SCM
bal_get_transactions_by_day (SCM acct_s, SCM first_day, SCM last_day)
{
  
}



/* Get account information by name */

SCM
bal_get_account (SCM name)
{
  char* name_c = scm_to_locale_string (name);
  account* acct = find_account_in_book (&bal_book, name_c);
  SCM type = scm_from_int (acct->type);
  SCM curr = scm_from_double (acct->curr);
  SCM ntsct = scm_from_int (acct->n_tsct);

  free(name_c);
  return scm_list_4(name, type, curr, ntsct);
}

/* Get account information for all accounts */

SCM
bal_get_all_accounts ()
{
  int i;
  SCM ret;

  ret = SCM_EOL;
  for (i=0; i < bal_book.n_account; i++)
    {
      ret = scm_append (scm_list_2(ret,
                                   scm_list_1(scm_list_4(scm_from_locale_string(bal_book.accounts[i].name),
                                                         scm_from_int (bal_book.accounts[i].type),
                                                         scm_from_double (bal_book.accounts[i].curr),
                                                         scm_from_int (bal_book.accounts[i].n_tsct)))));
    }
  
  return ret;
}


/* Get total of an account by name */

SCM
bal_total_account (SCM acct)
{
  SCM ret;

  char* acct_c = scm_to_locale_string (acct);
  account* a = find_account_in_book (&bal_book, acct_c);

  ret = total_transactions (a);
  free(acct_c);
  return scm_cons(acct, ret);
}

/* Get total for all accounts */

SCM
bal_total_all_accounts ()
{
  SCM ret, tmp;
  int i;

  ret = SCM_EOL;
  for (i = 0; i < bal_book.n_account; i++)
    {
      tmp = total_transactions (&bal_book.accounts[i]);
      ret = scm_append (scm_list_2 (ret,
                                    scm_list_1(scm_cons (scm_from_locale_string(bal_book.accounts[i].name),
                                                         tmp))));
    }
  return ret;
}

/* Get total for account by type */
SCM
bal_total_by_account_type ()
{
  SCM tmp;
  SCM ret = SCM_EOL;
  int i,j;

  for (j=EXPENSE; j <= LIABILITY; j++)
    {
      tmp = scm_from_double(0.0);
      for (i=0; i < bal_book.n_account; i++)
        {
          if (bal_book.accounts[i].type==j)
            {
              tmp = scm_sum(tmp, total_transactions
                            (&bal_book.accounts[i]));
            }
          
        }
      switch (j)
        {
        case EXPENSE:
          ret = scm_append(scm_list_2(ret,
                                      scm_list_1
                                      (scm_cons(scm_from_locale_string("expense"),
                                                tmp))));
          break;
        case INCOME:
          ret = scm_append(scm_list_2(ret,
                                      scm_list_1
                                      (scm_cons(scm_from_locale_string("income"),
                                                tmp))));
          break;
        case ASSET:
          ret = scm_append(scm_list_2(ret,
                                      scm_list_1
                                      (scm_cons(scm_from_locale_string("asset"),
                                                tmp))));
          break;
        case LIABILITY:
          ret = scm_append(scm_list_2(ret,
                                      scm_list_1
                                      (scm_cons(scm_from_locale_string("liability"),
                                                tmp))));
          break;
        }
          
    }
  
  return ret;
}


/* Quit prompt */

SCM
bal_quit ()
{
  bal_prompton = 0;
  return SCM_UNDEFINED;
}

/* Print to screen */

SCM
bal_print (SCM x)
{
  scm_display (x, scm_current_output_port());
  printf("\n");

  return SCM_UNDEFINED;
}

/* Set current account */

SCM
bal_set_account (SCM acct)
{
  bal_cur_acct = acct;
  return SCM_UNDEFINED;
}

/* Write to file */

SCM
bal_write (SCM file)
{
  char* file_c = scm_to_locale_string (file);
  write_out(file_c);
  free(file_c);
  return SCM_UNDEFINED;
}

/* Register all functions */

void*
register_guile_functions (void* data)
{
  /* Adding functions */
  scm_c_define_gsubr("bal/at", 4, 0, 0, &bal_at);
  scm_c_define_gsubr("bal/aa", 4, 0, 0, &bal_aa);

  /* Editing functions */
  scm_c_define_gsubr("bal/et", 1, 0, 0, &bal_et);
  scm_c_define_gsubr("bal/ea", 2, 0, 0, &bal_ea);

  /* Deleting functions */
  scm_c_define_gsubr("bal/da", 1, 0, 0, &bal_da);
  scm_c_define_gsubr("bal/dt", 1, 0, 0, &bal_dt);

  /* Get current account name */
  scm_c_define_gsubr("bal/get-current-account", 0, 0, 0,
                     &bal_get_current_acct);

  /* Get transactions */
  scm_c_define_gsubr("bal/get-transactions", 2, 0, 0,
                     &bal_get_transactions);
  scm_c_define_gsubr("bal/get-all-transactions", 1, 0, 0,
                     &bal_get_all_transactions);
  scm_c_define_gsubr("bal/get-transactions-by-regex", 2, 0, 0,
                     &bal_get_transactions_by_regex);

  /* Get accounts */
  scm_c_define_gsubr("bal/get-account", 1, 0, 0, &bal_get_account);
  scm_c_define_gsubr("bal/get-all-accounts", 0, 0, 0,
                     &bal_get_all_accounts);
  
  /* Total accounts */
  scm_c_define_gsubr("bal/total-account", 1, 0, 0, &bal_total_account);
  scm_c_define_gsubr("bal/total-all-accounts", 0, 0, 0,
                     &bal_total_all_accounts);
  scm_c_define_gsubr("bal/total-by-account-type", 0, 0, 0,
                     &bal_total_by_account_type);
  

  /* interface commands */
  scm_c_define_gsubr("q", 0, 0, 0, &bal_quit);
  scm_c_define_gsubr("p", 1, 0, 0, &bal_print);
  scm_c_define_gsubr("bal/set-account", 1, 0, 0, &bal_set_account);
  scm_c_define_gsubr("bal/write", 1, 0, 0, &bal_write);
  

  /* generic method to create interactive commands */
  scm_c_define_gsubr("bal/call-with-opts", 2, 0, 0, &bal_call_with_opts);
  
  return NULL;
}

#endif
