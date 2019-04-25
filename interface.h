/* main interface functions */
/* Add transactions and accounts */

SCM
bal_at (SCM account_name,
        SCM amount,
        SCM desc,
        SCM day)
{
  char* account_c = scm_to_locale_string (account_name);
  char* desc_c = scm_to_locale_string (desc);
  double amount_c = scm_to_double(amount);
  account* acct = find_account_in_book (&bal_book, account_c);

  if (acct->n_tsct==0)
    acct->tscts = malloc(sizeof(tsct));
  else
    acct->tscts = realloc(acct->tscts, (acct->n_tsct+1)*sizeof(tsct));
  
  acct->tscts[acct->n_tsct].year = scm_to_int
    (scm_list_ref
     (day,scm_from_int(2)));
  acct->tscts[acct->n_tsct].month = scm_to_int
    (scm_list_ref
     (day,scm_from_int(1)));
  acct->tscts[acct->n_tsct].day = scm_to_int
    (scm_list_ref
     (day,scm_from_int(0)));
  acct->tscts[acct->n_tsct].amount = amount_c;
  acct->tscts[acct->n_tsct].desc = copy_string(desc_c);

  acct->n_tsct = acct->n_tsct + 1;

  qsort(acct->tscts, acct->n_tsct, sizeof(tsct), sort_transactions);
  
  free(account_c);
  free(desc_c);

  bal_cur_acct = account_name;
  return SCM_UNDEFINED;
}

SCM
bal_aa (SCM name,
        SCM type,
        SCM ob)
{

  char* type_str = scm_to_locale_string(type);
  account_type type_c = account_type_from_string(type_str);
  free(type_str);
  char* name_c = scm_to_locale_string (name);
  double ob_c = scm_to_double(ob);

  if (bal_book.n_account == 0)
    bal_book.accounts = malloc(sizeof(account));
  else
    bal_book.accounts = realloc(bal_book.accounts,
                                sizeof(account)*(bal_book.n_account+1));

  bal_book.accounts[bal_book.n_account].type = type_c;
  bal_book.accounts[bal_book.n_account].n_tsct = 0;
  bal_book.accounts[bal_book.n_account].n_pos = 0;
  bal_book.accounts[bal_book.n_account].name =
    copy_string(name_c);

  bal_book.accounts[bal_book.n_account].ob = ob_c;
  bal_book.n_account++;

  free(name_c);

  bal_cur_acct = name;
  return SCM_UNDEFINED;
}

/* Rename existing accounts and adjust opening balances */

SCM
bal_ea (SCM cur_name,
        SCM name,
        SCM ob)
{
  char* cur_name_c = scm_to_locale_string (cur_name);
  char* name_c = scm_to_locale_string (name);
  char* ob_c = scm_to_locale_string(ob);
  account* acct = find_account_in_book(&bal_book, cur_name_c);

  if (strcmp(name_c,"") != 0)
    {
      free(acct->name);
      acct->name = copy_string(name_c);
    }

  if (strcmp(ob_c,"") != 0)
    acct->ob = atof(ob_c);

  char* current_account = scm_to_locale_string (bal_cur_acct);

  if (strcmp(current_account, cur_name_c)==0)
    bal_cur_acct = name;

  free(ob_c);
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
    bal_book.accounts[k].tscts[i] = bal_book.accounts[k].tscts[i+1];
  
  bal_book.accounts[k].n_tsct = bal_book.accounts[k].n_tsct - 1;
  return SCM_UNDEFINED;
}


SCM
bal_da (SCM account)
{
  char* account_c = scm_to_locale_string (account);
  int i,flag;

  flag = 0;
  for (i=0; i < bal_book.n_account; i++)
    if (strcmp(bal_book.accounts[i].name, account_c)==0)
      {
        flag = 1;
        break;
      }
  
  if (flag != 0)
    {
      delete_account (bal_book.accounts + i);
      bal_book.n_account = bal_book.n_account - 1;
      for (i=i; i < bal_book.n_account; i++)
        bal_book.accounts[i] = bal_book.accounts[i+1];

      char* current_account = scm_to_locale_string (bal_cur_acct);
      if (strcmp(current_account, account_c)==0)
        bal_cur_acct = scm_from_locale_string (bal_book.accounts[0].name);

      free(current_account);
    }

  free(account_c);
  return SCM_UNDEFINED;  
}

/* Get the current account name */

SCM
bal_get_current_account ()
{
  return bal_cur_acct;
}

/* Get number of accounts */
SCM
bal_get_number_of_accounts ()
{
  return scm_from_int(bal_book.n_account);
}

/* Get account location by name */
SCM
bal_get_account_location (SCM name_scm)
{
  char* name = scm_to_locale_string (name_scm);
  int loc = find_account_location_in_book (&bal_book, name);
  free(name);

  if (loc >= 0)
    return scm_from_int (loc);
  else
    return SCM_UNDEFINED;
}

/* get "num" most recent transactions */

SCM
bal_get_transactions (SCM acct, SCM num)
{
  int num_c = scm_to_int (num);
  SCM ret;
  char* acct_c;

  acct_c = scm_to_locale_string (acct);

  account* a = find_account_in_book (&bal_book, acct_c);
  int i,n;

  if (a->n_tsct == 0) return SCM_UNDEFINED;

  n = num_c < a->n_tsct ? num_c : a->n_tsct;
  ret = SCM_EOL;
  for (i=n-1; i >= 0; i--)
    ret = scm_append (scm_list_2
		      (ret,
		       scm_list_1
		       (tsct_to_scm(a->tscts[a->n_tsct-1-i]))));
  free(acct_c);
  return ret;
}

/* Get all transactions from an account */
SCM
bal_get_all_transactions (SCM acct)
{
  SCM ret;
  char* acct_c;

  acct_c = scm_to_locale_string (acct);

  account* a = find_account_in_book (&bal_book, acct_c);
  int i;
  if (a->n_tsct == 0) return SCM_UNDEFINED;

  ret = SCM_EOL;
  for (i=0; i < a->n_tsct; i++)
    ret = scm_append (scm_list_2
		      (ret,
		       scm_list_1(tsct_to_scm(a->tscts[i]))));
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
  tsct cur_day_t;

  set_tsct_time(&cur_day_t, bal_curtime);

  list = SCM_EOL;

  error = regcomp(&regex, regex_txt, 0);
  if (error)
    {
      fprintf(stderr, "Bad regex.\n");
    }
  else
    {
      for (i=0; i < acct->n_tsct; i++)
        {
          if (sort_transactions(&cur_day_t, &acct->tscts[i]) < 0)
            continue;
      
          error = regexec(&regex, acct->tscts[i].desc,
                          0, NULL, 0);
          if (!error)
            {
              trans_list = scm_list_1(tsct_to_scm(acct->tscts[i]));
              list = scm_append (scm_list_2(list, trans_list));
            }
        }
    }
  
  free(regex_txt);
  free(acct_c);
  return list;
}

/* Get account information by name */

SCM
bal_get_account (SCM name)
{
  char* name_c = scm_to_locale_string (name);
  account* acct = find_account_in_book (&bal_book, name_c);

  free(name_c);
  return acct_to_scm(*acct);
}

/* Get account information for all accounts */

SCM
bal_get_all_accounts ()
{
  int i;
  SCM ret;

  ret = SCM_EOL;
  for (i=0; i < bal_book.n_account; i++)
    ret = scm_append (scm_list_2
		      (ret,
		       scm_list_1
		       (acct_to_scm(bal_book.accounts[i]))));
  return ret;
}

/* Get transaction by number of account and number of transaction */
SCM
bal_get_transaction_by_location (SCM acct_num, SCM tsct_num)
{
  int acct_c = scm_to_int(acct_num);
  int tsct_c = scm_to_int(tsct_num);

  return tsct_to_scm(bal_book.accounts[acct_c].tscts[tsct_c]);
}

/* Get account by number */
SCM
bal_get_account_by_location (SCM acct_num)
{
  int acct_c = scm_to_int(acct_num);
  return acct_to_scm(bal_book.accounts[acct_c]);
}

/* Get transactions by day */
SCM
bal_get_transactions_by_day (SCM acct, SCM first_day, SCM last_day)
{
  char* acct_c = scm_to_locale_string(acct);
  account* acct_p = find_account_in_book(&bal_book, acct_c);

  tsct first_day_t;
  tsct last_day_t;

  set_tsct_time_from_scm(&first_day_t, first_day);
  set_tsct_time_from_scm(&last_day_t, last_day);
  
  SCM ret = SCM_EOL;
  int i;

  for (i=0; i < acct_p->n_tsct; i++)
    {
      if (sort_transactions(&last_day_t, &acct_p->tscts[i]) < 0)
        break;

      if (sort_transactions(&first_day_t, &acct_p->tscts[i]) <= 0)
        {
          ret = scm_append
            (scm_list_2
             (ret,
              scm_list_1(tsct_to_scm(acct_p->tscts[i]))));
        }

    }
  free(acct_c);

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
      ret = scm_append (scm_list_2
			(ret,
			 scm_list_1
			 (scm_cons
			  (scm_from_locale_string
			   (bal_book.accounts[i].name),
			   tmp))));
    }
  return ret;
}

/* Get total for all accounts of a certain type*/

SCM
bal_total_all_accounts_of_type (SCM type_s)
{
  SCM ret, tmp;
  int i;
  account_type type = scm_to_int(type_s);

  ret = SCM_EOL;
  for (i = 0; i < bal_book.n_account; i++)
    {
      if (bal_book.accounts[i].type==type)
        {
          tmp = total_transactions (&bal_book.accounts[i]);
          ret = scm_append (scm_list_2
			    (ret,
			     scm_list_1
			     (scm_cons
			      (scm_from_locale_string
			       (bal_book.accounts[i].name),
			       tmp))));
        }
    }
  return ret;
}


/* Get total for account by type */
SCM
bal_total_by_account_type ()
{
  SCM tmp,tmpcur,tmptotal,tmpallcur,tmpalltotal;
  SCM ret = SCM_EOL;
  int i,j;
  char* type_str;

  tmpallcur = scm_from_double(0.0);
  tmpalltotal = scm_from_double(0.0);
  for (j=EXPENSE; j <= LIABILITY; j++)
    {
      tmpcur = scm_from_double(0.0);
      tmptotal = scm_from_double(0.0);
      for (i=0; i < bal_book.n_account; i++)
        {
          if (bal_book.accounts[i].type==j)
            {
              tmp = total_transactions(&bal_book.accounts[i]);
              tmpcur = scm_sum(tmpcur, scm_car(tmp));
              tmptotal = scm_sum(tmptotal, scm_cdr(tmp));
              if (j==ASSET || j == LIABILITY)
                {
                  tmpallcur = scm_sum(tmpallcur, scm_car(tmp));
                  tmpalltotal = scm_sum(tmpalltotal, scm_cdr(tmp));
                }
            }
        }
      tmp = scm_cons(tmpcur, tmptotal);

      type_str = account_type_to_string(j);
      type_str[0] = toupper(type_str[0]);

      ret = scm_append
        (scm_list_2
         (ret,
          scm_list_1
          (scm_cons(scm_from_locale_string(type_str), tmp))));
    }

  tmp = scm_cons(tmpallcur, tmpalltotal);
  ret = scm_append
    (scm_list_2
     (ret,
      scm_list_1
      (scm_cons(scm_from_locale_string("Worth"),
                tmp))));
  
  tmp = scm_from_double(0.0);
  for (i=0; i < bal_book.n_account; i++)
    {
      tmp = scm_sum(tmp,
                    scm_from_double(bal_book.accounts[i].ob));
    }
  
  ret = scm_append
    (scm_list_2
     (ret, scm_list_1(scm_cons(scm_from_locale_string("Balances"),
                               tmp))));
  
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
  if (strcmp(file_c,"")==0)
    {
      free(file_c);
      file_c = scm_to_locale_string (bal_cur_file);
    }
  else
    {
      bal_cur_file = file;
    }
  
  write_out(file_c);
  free(file_c);

  return SCM_UNDEFINED;
}

/* Read file */

SCM
bal_read (SCM file)
{

  char* file_c = scm_to_locale_string(file);
  int i;
  delete_book (&bal_book);
  bal_book.n_account = 0;
  read_in (file_c);
  free(file_c);

  if (bal_book.n_account > 0)
    {
      bal_cur_acct = scm_from_locale_string (bal_book.accounts[0].name);
      for (i=0; i < bal_book.n_account; i++)
	qsort(bal_book.accounts[i].tscts,
	      bal_book.accounts[i].n_tsct,
	      sizeof(tsct),
	      sort_transactions);
    }
  bal_cur_file = file;
  return SCM_UNDEFINED;
}

/* Get current file */
SCM
bal_get_current_file ()
{
  return bal_cur_file;
}

/* set number of transactions to print when selecting transact */
SCM
bal_set_select_transact_num (SCM num)
{
  bal_select_tsct_num = scm_to_int (num);
  return num;
}

/* retrieve version number */
SCM
bal_v ()
{
  return scm_from_locale_string(BAL_VERSION);
}

/* set the day to be the dividing day between "past" and "future" */
SCM
bal_set_current_day (SCM dmy)
{
  bal_curtime->tm_mday = scm_to_int
    (scm_list_ref(dmy,scm_from_int(0)));
  bal_curtime->tm_mon = scm_to_int
    (scm_list_ref(dmy,scm_from_int(1))) - 1;
  bal_curtime->tm_year = scm_to_int
    (scm_list_ref(dmy,scm_from_int(2))) - 1900;
  return dmy;
}

/* get the current day */
SCM
bal_get_current_day ()
{
  return scm_list_3
    (scm_from_int(bal_curtime->tm_mday),
     scm_from_int(bal_curtime->tm_mon + 1),
     scm_from_int(bal_curtime->tm_year + 1900));
}

/* Register all functions */
void*
register_guile_functions (void* data)
{
  /* Adding functions */
  scm_c_define_gsubr("bal/at", 4, 0, 0, &bal_at);
  scm_c_define_gsubr("bal/aa", 3, 0, 0, &bal_aa);

  /* Editing functions */
  scm_c_define_gsubr("bal/ea", 3, 0, 0, &bal_ea);

  /* Deleting functions */
  scm_c_define_gsubr("bal/da", 1, 0, 0, &bal_da);
  scm_c_define_gsubr("bal/dt", 1, 0, 0, &bal_dt);

  /* Get global variables */
  scm_c_define_gsubr("bal/get-current-account", 0, 0, 0,
                     &bal_get_current_account);
  scm_c_define_gsubr("bal/get-current-file", 0, 0, 0,
                     &bal_get_current_file);

  /* Get transactions */
  scm_c_define_gsubr("bal/get-transactions", 2, 0, 0,
                     &bal_get_transactions);
  scm_c_define_gsubr("bal/get-all-transactions", 1, 0, 0,
                     &bal_get_all_transactions);
  scm_c_define_gsubr("bal/get-transactions-by-regex", 2, 0, 0,
                     &bal_get_transactions_by_regex);
  scm_c_define_gsubr("bal/get-transaction-by-location", 2, 0, 0,
                     &bal_get_transaction_by_location);
  scm_c_define_gsubr("bal/get-transactions-by-day", 3, 0, 0,
                     &bal_get_transactions_by_day);
  
  /* Get accounts */
  scm_c_define_gsubr("bal/get-account", 1, 0, 0, &bal_get_account);
  scm_c_define_gsubr("bal/get-all-accounts", 0, 0, 0,
                     &bal_get_all_accounts);
  scm_c_define_gsubr("bal/get-number-of-accounts", 0, 0, 0,
                     &bal_get_number_of_accounts);
  scm_c_define_gsubr("bal/get-account-by-location", 1, 0, 0,
                     &bal_get_account_by_location);
  scm_c_define_gsubr("bal/get-account-location", 1, 0, 0,
                     &bal_get_account_location);
  
  /* Total accounts */
  scm_c_define_gsubr("bal/total-account", 1, 0, 0, &bal_total_account);
  scm_c_define_gsubr("bal/total-all-accounts", 0, 0, 0,
                     &bal_total_all_accounts);

  scm_c_define_gsubr("bal/total-all-accounts-of-type", 1, 0, 0,
                     &bal_total_all_accounts_of_type);

  scm_c_define_gsubr("bal/total-by-account-type", 0, 0, 0,
                     &bal_total_by_account_type);

  /* Setting commands */
  scm_c_define_gsubr("bal/set-select-transact-num", 1, 0, 0,
                     &bal_set_select_transact_num);

  /* Get and set the current day */
  scm_c_define_gsubr("bal/set-current-day", 1, 0, 0,
                     &bal_set_current_day);
  scm_c_define_gsubr("bal/get-current-day", 0, 0, 0,
                     &bal_get_current_day);

  /* interface commands */
  scm_c_define_gsubr("q", 0, 0, 0, &bal_quit);
  scm_c_define_gsubr("p", 1, 0, 0, &bal_print);
  scm_c_define_gsubr("bal/set-account", 1, 0, 0, &bal_set_account);
  scm_c_define_gsubr("bal/write", 1, 0, 0, &bal_write);
  scm_c_define_gsubr("bal/read", 1, 0, 0, &bal_read);

  /* retrieve version */
  scm_c_define_gsubr("bal/v", 0, 0, 0, &bal_v);

  /* generic method to create interactive commands */
  scm_c_define_gsubr("bal/call", 2, 0, 0, &bal_call);

  return NULL;
}

