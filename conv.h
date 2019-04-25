/* convert types to other types */

arg_type
type_from_string (SCM type)
{
  char* type_c = scm_to_locale_string(type);
  arg_type t = OTHER;
  
  if (strcmp(type_c, "string")==0)
    t = STRING;

  if (strcmp(type_c, "account")==0)
    t = ACCOUNT;
  
  if (strcmp(type_c, "current_account")==0)
    t = CURRENT_ACCOUNT;

  if (strcmp(type_c, "type")==0)
    t = TYPE;

  if (strcmp(type_c, "transaction")==0)
    t = TRANSACTION;

  if (strcmp(type_c, "day")==0)
    t = DAY;

  free(type_c);
  return t;
}

char*
account_type_to_string (account_type type)
{
  char* t;
  switch (type)
    {
    case EXPENSE:
      t = copy_string("expense");
      break;
    case INCOME:
      t = copy_string("income");
      break;
    case ASSET:
      t = copy_string("asset");
      break;
    case LIABILITY:
      t = copy_string("liability");
      break;
    default:
      t = copy_string("none");
      break;
    }

  return t;

}

account_type
account_type_from_string (char* type)
{
  account_type t;
  if (strcmp(type, "expense")==0)
    t = EXPENSE;
  else if (strcmp(type, "income")==0)
    t = INCOME;
  else if (strcmp(type, "asset")==0)
    t = ASSET;
  else if (strcmp(type, "liability")==0)
    t = LIABILITY;
  else
    t = -1;

  return t;
}

void
set_tsct_time (tsct* t, struct tm* time)
{
  t->year = time->tm_year + 1900;
  t->month = time->tm_mon + 1;
  t->day = time->tm_mday;

}

void
set_tsct_time_from_scm (tsct* t, SCM time)
{
  t->year = scm_to_int
    (scm_list_ref(time,scm_from_int(2)));
  t->month = scm_to_int
    (scm_list_ref(time,scm_from_int(1)));
  t->day = scm_to_int
    (scm_list_ref(time,scm_from_int(0)));

}
