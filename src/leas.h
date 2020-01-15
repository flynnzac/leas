#ifndef LEAS_H
#define LEAS_H
#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <regex.h>
#include <libguile.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <csv.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>
#include <signal.h>
#include <sys/stat.h>
#include <getopt.h>

/* Convenience, Manipulation Functions */

int
pow_int (int base, int power)
{
  if (power < 0)
    return 0;
  
  if (power == 0)
    return 1;

  int result = base;

  for (int i = 1; i < power; i++)
    {
      result *= base;
    }

  return result;
}
  
char*
remove_ext (char* str)
{
  /* truncate a string before first dot */
  int i;
  for (i=strlen(str); i >= 1; i--)
    {
      if (str[i-1] == '.')
        {
          str[i-1] = '\0';
          break;
        }
    }
  return str;
}

int
digits (int num)
{
  /* count number of digits in a number */

  return floor(log10(num))+1;
}

int
anyalpha (const char* str)
{
  /* return 1 if any character in str is not a digit, 0 otherwise */
  int i;
  int any = 0;
  if (strlen(str)==0) return 1;

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

char*
copy_string (const char* str)
{
  /* copies a string, allocates mem for it, and returns a pointer to it */
  char* s = malloc(sizeof(char)*(strlen(str)+1));
  strcpy(s, str);
  return s;
}

char*
copy_string_insert_int (const char* str, int num)
{
  /* str has one %d in it which is replaces with num, a pointer to new
     string is returned */
  int d = digits(num);
  char* s = malloc(sizeof(char)*(strlen(str) - 2 + d + 1));
  sprintf(s, str, num);
  return s;
}

char*
create_tmp_dir ()
{
  /* creates a temporary directory and returns a string 
     giving file location */
  char* tmp_dir = copy_string("Leas_XXXXXX");
  mkdtemp(tmp_dir);

  return tmp_dir;
}

void
append_to_string (char** base, const char* add, const char* surround)
{
  /* replaces base with "$base$surround$add$surround" */
  int sz = strlen(*base)+strlen(add)+2*strlen(surround);
  *base = realloc(*base, sizeof(char)*(sz+1));
  strcat(*base, surround);
  strcat(*base, add);
  strcat(*base, surround);
}

/* transaction, account, and book types */
typedef struct 
{
  double amount;
  unsigned year;
  unsigned month;
  unsigned day;
  char* desc;
} tsct;

typedef enum 
  {
   EXPENSE = 1,
   INCOME = 2,
   ASSET = 4,
   LIABILITY = 8
  } account_type;

typedef enum
  {
   STRING,
   ACCOUNT,
   EXPENSE_ACCOUNT,
   LIABILITY_ACCOUNT,
   ASSET_ACCOUNT,
   INCOME_ACCOUNT,
   PAY_FROM_ACCOUNT,
   CURRENT_ACCOUNT,
   TYPE,
   TRANSACTION,
   DAY,
   OTHER
  } arg_type;

typedef struct
{
  account_type type;
  char* name;
  tsct* tscts;
  double ob;
  int n_tsct;
  int n_pos;
} account;

typedef struct
{
  account* accounts;
  int n_account;
  int n_pos; /* used in reading data */
} book;

/* prompt state */

typedef enum
  {
   PROMPT_OFF,
   PROMPT_COMMAND,
   PROMPT_SELECT
  } prompt_state;

/* destructors */

int
delete_account (account* acct)
{
  /* frees memory associated with account */
  int i;
  if (acct->n_tsct > 0)
    {
      for (i=0; i < acct->n_tsct; i++)
        {
          free(acct->tscts[i].desc);
        }
      free(acct->tscts);
    }

  free(acct->name);
  return 0;
}

int
delete_book (book* book)
{
  /* frees whole book */
  int i;
  if (book->n_account > 0)
    {
      for (i=0; i < book->n_account; i++)
        {
          delete_account (&book->accounts[i]);
        }
      free(book->accounts);
      book->n_account = 0;
    }
  book->n_pos = 0;

  return 0;
}

/* convert types to other types */

arg_type
arg_type_from_string (SCM type)
{
  /* takes a string describing argument type
     and returns an enum arg_type */
  char* type_c = scm_to_locale_string(type);
  arg_type t = OTHER;
  
  if (strcmp(type_c, "string")==0)
    t = STRING;
  else if (strcmp(type_c, "account")==0)
    t = ACCOUNT;
  else if (strcmp(type_c, "expense_account")==0)
    t = EXPENSE_ACCOUNT;
  else if (strcmp(type_c, "income_account")==0)
    t = INCOME_ACCOUNT;
  else if (strcmp(type_c, "asset_account")==0)
    t = ASSET_ACCOUNT;
  else if (strcmp(type_c, "liability_account")==0)
    t = LIABILITY_ACCOUNT;
  else if (strcmp(type_c, "pay_from_account")==0)
    t = PAY_FROM_ACCOUNT;
  else if (strcmp(type_c, "current_account")==0)
    t = CURRENT_ACCOUNT;
  else if (strcmp(type_c, "type")==0)
    t = TYPE;
  else if (strcmp(type_c, "transaction")==0)
    t = TRANSACTION;
  else if (strcmp(type_c, "day")==0)
    t = DAY;

  free(type_c);
  return t;
}

char*
account_type_to_string (account_type type)
{
  /* returns a string describing account type from enum account_type */
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
  /* given string, return enum account_type */
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

/* set attributes and arrange Leas objects */

/* set transaction time */
void
set_tsct_time (tsct* t, struct tm* time)
{
  t->year = time->tm_year + 1900;
  t->month = time->tm_mon + 1;
  t->day = time->tm_mday;
}

/* set transaction time from SCM list */
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

/* sort transactionis from earliest to latest,
   return 1 if a > b, -1 if a < b, and 0 if a=b.
 */
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

/* global vars */

static book leas_book;
static SCM leas_cur_acct;
static int leas_prompton;
static SCM leas_cur_file;
static int leas_prompt_exit;
static int leas_select_tsct_num;
static struct tm* leas_curtime;

/* check whether an (account,transaction) exists by number */
int
check_account_trans (int a, int t)
{
  if ((0 <= a) & (leas_book.n_account > a))
    {
      if ((0 <= t) & (leas_book.accounts[a].n_tsct > t))
        return 1;
      else
        return 0;
    }
  else
    return 0;
}

/* send code to guile interpreter */
SCM
handle_error (void* toeval_v,
              SCM key,
              SCM parameters)
{
  /* handle Guile error */
  SCM message = scm_from_locale_string("Uncaught throw to ~A: ~A\n");
  scm_simple_format(SCM_BOOL_T,
                    message,
                    scm_list_2(key, parameters));
  return SCM_UNSPECIFIED;
}

/* executes a piece of Guile code */
SCM
exec_string_safe (void* toeval_v)
{
  /* execute a string of Guile code */
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

/* executes a piece of Guile code and adds to history*/
SCM
exec_string_safe_history (void* toeval_v)
{
  /* execute a string and add to history */
  if (toeval_v != NULL && strcmp((char*) toeval_v, "") != 0)
    {
      add_history ((char*) toeval_v);
      return exec_string_safe (toeval_v);
    }
  else return SCM_UNDEFINED;
}

/* find accounts by name */
account*
find_account_in_book (book* book, const char* name)
{
  int i;
  for (i=0; i < book->n_account; i++)
    if (strcmp(book->accounts[i].name, name)==0)
      return (book->accounts + i);
  
  return NULL;
}

/* find account number in book */
int
find_account_location_in_book (book* book, const char* name)
{
  int i;
  for (i=0; i < book->n_account; i++)
    if (strcmp(book->accounts[i].name, name)==0)
      return i;
  
  return -1;
}

/* conversion functions to scheme objects */

/* convert transaction to Scheme list containing data */
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

/* convert account to Scheme list containing data */
SCM
acct_to_scm (const account a)
{
  SCM name = scm_from_locale_string(a.name);
  SCM type = scm_from_int(a.type);
  SCM ntsct = scm_from_int(a.n_tsct);
  SCM ob = scm_from_double(a.ob);

  return scm_list_4(name,type,ntsct,ob);
}

/* function to check whether transact after current time */
int
tsct_before_today (const tsct t)
{
  return (((leas_curtime->tm_year+1900) > (t.year)) ||
          (((leas_curtime->tm_mon+1) > (t.month)) &&
           ((leas_curtime->tm_year+1900) == (t.year))) ||
          ((leas_curtime->tm_mday >= t.day) &&
           (leas_curtime->tm_year+1900) == t.year &&
           (leas_curtime->tm_mon+1) == t.month));
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

/* selection functions for leas/call */

/* function to interactively select account */
char*
leas_select_account (const char* prompt, int kind)
{
  int j, ndigit;
  char* c;

  if (leas_book.n_account==0)
    {
      printf("No account.\n");
      return NULL;
    }
  else
    {
      ndigit = floor(log10(leas_book.n_account)) + 1;
      do
        {
          for (j=0; j < leas_book.n_account; j++)
            {
              if (kind < 0 || (leas_book.accounts[j].type & kind))
                printf("%*d: %s\n", ndigit+1, j, leas_book.accounts[j].name);
            }
          c = readline(prompt);
        } while (strcmp(c,"") && (anyalpha(c) || atoi(c) < 0 ||
                                  atoi(c) >= leas_book.n_account));

      return c;
    }
}

/* function to interactively select transaction */
int
leas_select_transaction (account* acct)
{
  int i,n,ndigit,maxlen,end;
  char* option;

  maxlen = 0;
  end = 0;
  while ((end < acct->n_tsct) && tsct_before_today(acct->tscts[end]))
    end++;

  n = (end - leas_select_tsct_num) >= 0 ?
    (end - leas_select_tsct_num) : 0;
  
  ndigit = floor(log10(acct->n_tsct)) + 1;

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
    } while (strcmp(option,"") && (anyalpha(option) || atoi(option) < 0 ||
                                   atoi(option) >= acct->n_tsct));
  if (strcmp(option,"")!=0)
    {
      i = atoi(option);
      free(option);
      return i;
    }

  return -1;
}

/* function to interactively select day */
char*
leas_select_day (struct tm* curtime_info)
{
  char* prompt;
  char* year; char* month; char* day;

  if (leas_prompton == PROMPT_SELECT)
    {
      prompt = copy_string_insert_int("Year [%d]: ",
                                      curtime_info->tm_year+1900);
      year = readline(prompt);
      free(prompt);
    }
  else
    return NULL;

  if (leas_prompton == PROMPT_SELECT)
    {
      prompt = copy_string_insert_int("Month [%d]: ",
                                      curtime_info->tm_mon+1);
      month = readline(prompt);
      free(prompt);
    }
  else
    return NULL;

  if (leas_prompton == PROMPT_SELECT)
    {
      prompt = copy_string_insert_int("Day [%d]: ",
                                      curtime_info->tm_mday);
      day = readline(prompt);
      free(prompt);
    }
  else
    return NULL;

  char* ret = malloc(sizeof(char)*(strlen("(list   )")+2+2+4+1));
  sprintf(ret, "(list %d %d %d)",
          strcmp(day,"")==0 ? curtime_info->tm_mday : atoi(day),
          strcmp(month,"")==0 ? (curtime_info->tm_mon+1) : atoi(month),
          strcmp(year,"")==0 ? (curtime_info->tm_year+1900) : atoi(year));
  return ret;
}

/* function to interactively select account type */
account_type
leas_select_account_type (char* prompt)
{
  int k;
  char* opt;
  
  printf("0: Expense\n");
  printf("1: Income\n");
  printf("2: Asset\n");
  printf("3: Liability\n");
  opt = readline (prompt);

  k = anyalpha(opt) > 0 ? -1 : pow_int(2,atoi(opt));

  return k;
}
/* Functions for reading .leas files */

/* Read transaction from CSV */
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

/* Read account info from CSV. */
void
account_cb1 (void* s, size_t len, void* data)
{
  char buf[len+1];
  int i;
  for (i=0; i < len; i++)
    buf[i] = ((char*)s)[i];

  buf[len] = '\0';
  
  book* book = data;
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

/* Read Accounts into a book from CSV file. */
void
read_book_accounts_from_csv (book* book,
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
      if (csv_parse (&p, buf, bytes_read, account_cb1, NULL, book)
          != bytes_read)
        {
          fprintf(stderr,
                  "Error parsing file: %s\n",
                  csv_strerror(csv_error(&p)));
          goto end_read_book;
        }
    }
  csv_fini(&p, account_cb1, NULL, book);
 end_read_book:
  fclose(fp);
  csv_free(&p);
}

/* Read transactions into book.*/
void
read_all_transactions_into_book (book* book,
                                 char* base,
                                 const char* tmp_dir)

{
  FILE* fp;
  int i, error;
  struct csv_parser p;
  char buf[1024];
  size_t bytes_read;
  char* name;

  char* fn = copy_string(basename(base));
  fn = remove_ext(fn);

  for (i=0; i < book->n_account; i++)
    {
      name = malloc(sizeof(char)*(strlen(fn) +
                                  strlen(tmp_dir) +
                                  strlen("//") +
                                  strlen(book->accounts[i].name)+
                                  strlen(".csv")+1));

      sprintf(name, "%s/%s/%s.csv", tmp_dir, fn, book->accounts[i].name);
      
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
          fprintf(stderr, "Data not found for account: %s\n",
                  book->accounts[i].name);
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
  free(fn);
}

/* Top-level function to read everything a Leas book in. */
int
read_in (char* base)
{
  char* untar_cmd;
  char* account_file;
  char* rm_cmd;
  char* tmp_dir;
  char* fn = copy_string(basename(base));
  fn = remove_ext(fn);

  

  /* create tmp dir */
  tmp_dir = create_tmp_dir();
  if (tmp_dir==NULL)
    return 1;

  /* untar archive */
  
  untar_cmd = malloc(sizeof(char)*(strlen("tar xaf ")+
                                   strlen(base)+
                                   strlen(" -C ")+
                                   strlen(tmp_dir)+1));

  sprintf(untar_cmd, "tar xaf %s -C %s", base, tmp_dir);
  system(untar_cmd);
  free(untar_cmd);

  /* read in accounts */

  account_file = malloc(sizeof(char)*(strlen(fn) +
                                      strlen(tmp_dir) +
                                      strlen("/accounts")+
                                      1));

  sprintf(account_file, "%s/%s/accounts", tmp_dir, fn);
  read_book_accounts_from_csv (&leas_book, account_file);
  free(account_file);

  /* read in transactions */
  read_all_transactions_into_book (&leas_book,
                                   base,
                                   tmp_dir);

  /* clean up */
  rm_cmd = malloc(sizeof(char)*(strlen("rm -R ") +
                                strlen(tmp_dir) + 1));
  sprintf(rm_cmd, "rm -R %s", tmp_dir);
  system(rm_cmd);
  free(rm_cmd); free(tmp_dir); free(fn);
  /* return success */
  return 0;
  
}

/* Functions for writing .leas files */

/* Write transactions from an account to a CSV file. */
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

/* Write accounts from leas_book to file. */
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
  for (i=0; i < leas_book.n_account; i++)
    {
      type = account_type_to_string(leas_book.accounts[i].type);
      fprintf(fp, "%s,", type);
      csv_fwrite(fp, leas_book.accounts[i].name,
                 strlen(leas_book.accounts[i].name));
      free(type);
      fprintf(fp, ",%f\n", leas_book.accounts[i].ob);
    }
  fclose(fp);
  return 0;
}

/* Write everything out to a file. */
int
write_out (char* base)
{
  char* cmd;
  char* account_fn;
  char* tsct_fn;
  char* tmp_dir;
  char* fn = copy_string(basename(base));

  fn = remove_ext(fn);
  int i;

  tmp_dir = create_tmp_dir();
  if (tmp_dir==NULL)
    return 1;

  cmd = malloc(sizeof(char)*(strlen(fn)+
                             strlen(tmp_dir)+
                             strlen("/")+1));

  sprintf(cmd, "%s/%s", tmp_dir, fn);
  mkdir(cmd, 0777);
  free(cmd);
  
  account_fn = malloc(sizeof(char)*(strlen(fn)+
                                    strlen(tmp_dir)+
                                    strlen("accounts")+
                                    strlen("//")+1));

  sprintf(account_fn, "%s/%s/accounts", tmp_dir, fn);
  i = write_accounts (account_fn);
  free(account_fn);
  
  if (i != 0)
    {
      free(tmp_dir);
      return i;
    }

  for (i=0; i < leas_book.n_account; i++)
    {
      tsct_fn = malloc
        (sizeof(char)*(strlen(leas_book.accounts[i].name) +
                       strlen(fn) +
                       strlen(tmp_dir) + 
                       strlen("//.csv") +
                       1));
      sprintf(tsct_fn, "%s/%s/%s.csv", tmp_dir, fn,
              leas_book.accounts[i].name);
      
      write_transactions (tsct_fn, &leas_book.accounts[i]);
      free(tsct_fn);
    }

  cmd = malloc(sizeof(char)*(strlen("tar caf ")+
                             strlen(base)+
                             strlen(" -C ")+
                             strlen(tmp_dir)+
                             strlen(fn)+2));
  sprintf(cmd, "tar caf %s -C %s %s", base, tmp_dir, fn);
  system(cmd);
  free(cmd);

  cmd = malloc(sizeof(char)*(strlen("rm -R ")+
                             strlen(tmp_dir)+
                             1));

  sprintf(cmd, "rm -R %s", tmp_dir);
  system(cmd);

  free(cmd); free(tmp_dir); free(fn);

  return 0;
}

/* Used to call a function with options specified interactively */
SCM
leas_call (SCM func, SCM options)
{
  int i,j,k;
  SCM pair;
  SCM name;

  arg_type type;

  char* tmp_str;
  char* name_c = NULL;
  char* opt;

  time_t curtime;
  struct tm* curtime_info;

  int len = scm_to_int (scm_length (options));

  char* func_c = scm_to_locale_string (func);
  char* command = copy_string(func_c);
  free(func_c);
  
  append_to_string(&command, " ", "");

  leas_prompton = PROMPT_SELECT;
  for (i=0; i < len; i++)
    {
      if (leas_prompton != PROMPT_SELECT)
        break;

      pair = scm_list_ref (options, scm_from_int(i));

      name = scm_car(pair);

      name_c = scm_to_locale_string (name);
      name_c = realloc (name_c, sizeof(char)*(strlen(name_c)+4));
      strcat(name_c, ": ");
      
      type = arg_type_from_string(scm_cdr(pair));

      switch (type)
        {
        case STRING:
          opt = readline(name_c);
          append_to_string(&command, opt, "\"");
          break;
        case ACCOUNT:
          opt = leas_select_account(name_c, -1);
          if (opt != NULL && strcmp(opt,"") && anyalpha(opt) == 0)
            {
              k = atoi(opt);
              append_to_string(&command,
                               leas_book.accounts[k].name,
                               "\"");
            } else leas_prompton = PROMPT_COMMAND;
          break;
        case EXPENSE_ACCOUNT:
          opt = leas_select_account(name_c, EXPENSE);
          if (opt != NULL && strcmp(opt,"") && anyalpha(opt) == 0)
            {
              k = atoi(opt);
              append_to_string(&command,
                               leas_book.accounts[k].name,
                               "\"");
            } else leas_prompton = PROMPT_COMMAND;
          break;
        case LIABILITY_ACCOUNT:
          opt = leas_select_account(name_c, LIABILITY);
          if (opt != NULL && strcmp(opt,"") && anyalpha(opt) == 0)
            {
              k = atoi(opt);
              append_to_string(&command,
                               leas_book.accounts[k].name,
                               "\"");
            } else leas_prompton = PROMPT_COMMAND;
          break;
        case ASSET_ACCOUNT:
          opt = leas_select_account(name_c, ASSET);
          if (opt != NULL && strcmp(opt,"") && anyalpha(opt) == 0)
            {
              k = atoi(opt);
              append_to_string(&command,
                               leas_book.accounts[k].name,
                               "\"");
            } else leas_prompton = PROMPT_COMMAND;
          break;
        case INCOME_ACCOUNT:
          opt = leas_select_account(name_c, INCOME);
          if (opt != NULL && strcmp(opt,"") && anyalpha(opt) == 0)
            {
              k = atoi(opt);
              append_to_string(&command,
                               leas_book.accounts[k].name,
                               "\"");
            } else leas_prompton = PROMPT_COMMAND;
          break;
        case PAY_FROM_ACCOUNT:
          opt = leas_select_account(name_c, ASSET | LIABILITY);
          if (opt != NULL && strcmp(opt,"") && anyalpha(opt) == 0)
            {
              k = atoi(opt);
              append_to_string(&command,
                               leas_book.accounts[k].name,
                               "\"");
            } else leas_prompton = PROMPT_COMMAND;
          break;
        case CURRENT_ACCOUNT:
          opt = scm_to_locale_string (leas_cur_acct);
          append_to_string(&command, opt, "\"");
          break;
        case TYPE:
          k = leas_select_account_type(name_c);
          if (k >= 0)
            {
              tmp_str = account_type_to_string(k);
              append_to_string(&command, tmp_str, "\"");
              free(tmp_str);
            } else leas_prompton = PROMPT_COMMAND;
          break;
        case TRANSACTION:
          printf("%s\n", name_c);
          opt = leas_select_account("Account: ", -1);
          if (opt!=NULL && strcmp(opt,"")!=0)
            {
              if (anyalpha(opt) == 0)
                {
                  k = atoi(opt);
                  j = leas_select_transaction (&leas_book.accounts[k]);

                  if (j >= 0)
                    {
                      tmp_str = malloc(sizeof(char)*(strlen("(cons  )") +
                                                     digits(k)+digits(j)+1));
                      sprintf(tmp_str, "(cons %d %d)", k, j);
                      append_to_string(&command, tmp_str, "");
                      free(tmp_str);
                    }
                }
            }
          else
            leas_prompton = PROMPT_COMMAND;
          break;
        case DAY:
          time(&curtime);
          curtime_info = localtime(&curtime);

          printf("%s\n", name_c);
          opt = leas_select_day (curtime_info);
          if (opt != NULL)
            append_to_string(&command, opt, "");
          break;
        default:
          opt = readline(name_c);
          append_to_string(&command, opt, "");
          break;
        }

      if (type != TYPE && opt != NULL) free(opt);
      free(name_c);
      
      if (i != (len-1))
        append_to_string(&command, " ", "");

    }

  if (leas_prompton == PROMPT_SELECT)
    {
      SCM ret = scm_c_catch (SCM_BOOL_T,
                             exec_string_safe,
                             command,
                             handle_error,
                             command,
                             NULL,
                             NULL);
      free(command);
      return ret;
    }

  free(command);
  leas_prompton = PROMPT_COMMAND;

  return SCM_UNDEFINED;
}

/* Main Scheme interface functions */

/* Add a transaction to an account. */
SCM
leas_at (SCM account_name,
         SCM amount,
         SCM desc,
         SCM day)
{
  char* account_c = scm_to_locale_string (account_name);
  account* acct = find_account_in_book (&leas_book, account_c);
  free(account_c);

  if (acct == NULL) return SCM_UNDEFINED;

  char* desc_c = scm_to_locale_string (desc);
  double amount_c = scm_to_double(amount);

  if (acct->n_tsct==0)
    acct->tscts = malloc(sizeof(tsct));
  else
    acct->tscts = realloc(acct->tscts, (acct->n_tsct+1)*sizeof(tsct));

  set_tsct_time_from_scm(&acct->tscts[acct->n_tsct], day);

  acct->tscts[acct->n_tsct].amount = amount_c;
  acct->tscts[acct->n_tsct].desc = copy_string(desc_c);

  acct->n_tsct = acct->n_tsct + 1;

  qsort(acct->tscts, acct->n_tsct, sizeof(tsct), sort_transactions);
  
  free(desc_c);

  leas_cur_acct = account_name;
  return SCM_UNDEFINED;
}

/* Add an account. */
SCM
leas_aa (SCM name,
         SCM type,
         SCM ob)
{

  char* type_str = scm_to_locale_string(type);
  account_type type_c = account_type_from_string(type_str);
  char* name_c = scm_to_locale_string (name);
  double ob_c = scm_to_double(ob);

  free(type_str);

  if (leas_book.n_account == 0)
    leas_book.accounts = malloc(sizeof(account));
  else
    leas_book.accounts = realloc(leas_book.accounts,
                                 sizeof(account)*(leas_book.n_account+1));

  leas_book.accounts[leas_book.n_account].type = type_c;
  leas_book.accounts[leas_book.n_account].n_tsct = 0;
  leas_book.accounts[leas_book.n_account].n_pos = 0;
  leas_book.accounts[leas_book.n_account].name = copy_string(name_c);

  leas_book.accounts[leas_book.n_account].ob = ob_c;
  leas_book.n_account++;

  free(name_c);

  leas_cur_acct = name;
  return SCM_UNDEFINED;
}

/* Rename existing accounts and adjust opening balances */
SCM
leas_ea (SCM cur_name,
         SCM name,
         SCM ob)
{
  char* cur_name_c = scm_to_locale_string (cur_name);
  account* acct = find_account_in_book(&leas_book, cur_name_c);
  if (acct == NULL)
    {
      free(cur_name_c);
      return SCM_UNDEFINED;
    }
  char* name_c = scm_to_locale_string (name);
  char* ob_c = scm_to_locale_string(ob);

  if (strcmp(name_c,"") != 0)
    {
      free(acct->name);
      acct->name = copy_string(name_c);
    }

  if (strcmp(ob_c,"") != 0)
    acct->ob = atof(ob_c);

  char* current_account = scm_to_locale_string (leas_cur_acct);

  if (strcmp(current_account, cur_name_c)==0)
    leas_cur_acct = name;

  free(ob_c); free(cur_name_c); free(name_c); free(current_account);

  return SCM_UNDEFINED;
}

/* Delete an existing transaction. */
SCM
leas_dt (SCM at_pair)
{
  SCM account = scm_car (at_pair);
  SCM tsct = scm_cdr (at_pair);
  
  int k = scm_to_int (account);
  int j = scm_to_int (tsct);
  int i;

  if (check_account_trans(k,j))
    {
      free(leas_book.accounts[k].tscts[j].desc);
      for (i=j; i < (leas_book.accounts[k].n_tsct-1); i++)
        leas_book.accounts[k].tscts[i] = leas_book.accounts[k].tscts[i+1];
      leas_book.accounts[k].n_tsct = leas_book.accounts[k].n_tsct - 1;
    }
  return SCM_UNDEFINED;

}

/* Delete an existing account. */
SCM
leas_da (SCM account)
{

  char* account_c = scm_to_locale_string (account);
  if (leas_book.n_account == 1)
    {
      printf("Cannot delete account: %s.", account_c);
      printf(" There must always be at least one account.\n");
      free(account_c);
      return SCM_UNDEFINED;
    }
  int i,flag;

  flag = 0;
  for (i=0; i < leas_book.n_account; i++)
    {
      if (strcmp(leas_book.accounts[i].name, account_c)==0)
        {
          flag = 1;
          break;
        }
    }
  
  if (flag != 0)
    {
      delete_account (leas_book.accounts + i);
      leas_book.n_account = leas_book.n_account - 1;
      for (i=i; i < leas_book.n_account; i++)
        leas_book.accounts[i] = leas_book.accounts[i+1];

      char* current_account = scm_to_locale_string (leas_cur_acct);
      if (strcmp(current_account, account_c)==0)
        leas_cur_acct = scm_from_locale_string (leas_book.accounts[0].name);

      free(current_account);
    }

  free(account_c);
  return SCM_UNDEFINED;  
}

/* Get the current account name */
SCM
leas_get_current_account ()
{
  if (leas_book.n_account==0) return scm_from_locale_string("");
      
  return leas_cur_acct;
}

/* Get number of accounts */
SCM
leas_get_number_of_accounts ()
{
  return scm_from_int(leas_book.n_account);
}

/* Get account location by name */
SCM
leas_get_account_location (SCM name_scm)
{
  char* name = scm_to_locale_string (name_scm);
  int loc = find_account_location_in_book (&leas_book, name);
  free(name);

  if (loc >= 0)
    return scm_from_int (loc);
  else
    return SCM_UNDEFINED;
}

/* Get "num" most recent transactions from "acct" */
SCM
leas_get_transactions (SCM acct, SCM num)
{
  int num_c = scm_to_int (num);
  SCM ret;
  char* acct_c;

  acct_c = scm_to_locale_string (acct);
  account* a = find_account_in_book (&leas_book, acct_c);
  free(acct_c);

  if (a==NULL) return SCM_EOL;
    
  int i,n;

  if (a->n_tsct == 0) return SCM_EOL;

  n = num_c < a->n_tsct ? num_c : a->n_tsct;
  ret = SCM_EOL;
  for (i=n-1; i >= 0; i--)
    ret = scm_append (scm_list_2
                      (ret,
                       scm_list_1
                       (tsct_to_scm(a->tscts[a->n_tsct-1-i]))));

  return ret;
}

/* Get all transactions from an account */
SCM
leas_get_all_transactions (SCM acct)
{
  SCM ret;
  char* acct_c;

  acct_c = scm_to_locale_string (acct);

  account* a = find_account_in_book (&leas_book, acct_c);
  free(acct_c);
  
  if (a==NULL) return SCM_EOL;
  
  int i;
  if (a->n_tsct == 0) return SCM_UNDEFINED;

  ret = SCM_EOL;
  for (i=0; i < a->n_tsct; i++)
    ret = scm_append (scm_list_2
                      (ret,
                       scm_list_1(tsct_to_scm(a->tscts[i]))));
  return ret;
}

/* Get all transactions from an account whose description matches a regular expression. */
SCM
leas_get_transactions_by_regex (SCM acct_s, SCM regex_s)
{

  char* acct_c = scm_to_locale_string (acct_s);
  account* acct = find_account_in_book (&leas_book, acct_c);
  free(acct_c);
  
  if (acct==NULL) return SCM_EOL;

  char* regex_txt = scm_to_locale_string (regex_s);
  regex_t regex;
  int i, error;
  SCM list, trans_list;
  tsct cur_day_t;

  set_tsct_time(&cur_day_t, leas_curtime);

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
  return list;
}

/* Get account information by name */
SCM
leas_get_account (SCM name)
{
  char* name_c = scm_to_locale_string(name);
  account* acct = find_account_in_book(&leas_book, name_c);

  free(name_c);
  if (acct == NULL)
    return SCM_EOL;
  else
    return acct_to_scm(*acct);
}

/* Get account information for all accounts */
SCM
leas_get_all_accounts ()
{
  int i;
  SCM ret;

  ret = SCM_EOL;
  for (i=0; i < leas_book.n_account; i++)
    ret = scm_append(scm_list_2
                     (ret,
                      scm_list_1
                      (acct_to_scm(leas_book.accounts[i]))));
  return ret;
}

/* Get transaction by number of account and number of transaction */
SCM
leas_get_transaction_by_location (SCM acct_num, SCM tsct_num)
{
  int acct_c = scm_to_int(acct_num);
  int tsct_c = scm_to_int(tsct_num);

  if (check_account_trans(acct_c, tsct_c))
    return tsct_to_scm(leas_book.accounts[acct_c].tscts[tsct_c]);
  else
    return SCM_EOL;
}

/* Get account by number */
SCM
leas_get_account_by_location (SCM acct_num)
{
  int acct_c = scm_to_int(acct_num);
  if (check_account_trans(acct_c,0))
    return acct_to_scm(leas_book.accounts[acct_c]);
  else
    return SCM_EOL;
}

/* Get transactions by day */
SCM
leas_get_transactions_by_day (SCM acct, SCM first_day, SCM last_day)
{
  char* acct_c = scm_to_locale_string(acct);
  account* acct_p = find_account_in_book(&leas_book, acct_c);
  free(acct_c);

  if (acct_p == NULL) return SCM_EOL;

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


  return ret;
}

/* Get total of an account by name */
SCM
leas_total_account (SCM acct)
{
  SCM ret;

  char* acct_c = scm_to_locale_string(acct);
  account* a = find_account_in_book(&leas_book, acct_c);
  free(acct_c);

  if (a == NULL) return SCM_EOL;
  
  ret = total_transactions(a);
  return scm_cons(acct, ret);
}

/* Get total for all accounts */
SCM
leas_total_all_accounts ()
{
  SCM ret, tmp;
  int i;

  ret = SCM_EOL;
  for (i = 0; i < leas_book.n_account; i++)
    {
      tmp = total_transactions(&leas_book.accounts[i]);
      ret = scm_append(scm_list_2
                       (ret,
                        scm_list_1
                        (scm_cons
                         (scm_from_locale_string
                          (leas_book.accounts[i].name),
                          tmp))));
    }
  return ret;
}

/* Get total for all accounts of a certain type*/
SCM
leas_total_all_accounts_of_type (SCM type_s)
{
  SCM ret, tmp;
  int i;
  account_type type = scm_to_int(type_s);

  ret = SCM_EOL;
  for (i = 0; i < leas_book.n_account; i++)
    {
      if (leas_book.accounts[i].type==type)
        {
          tmp = total_transactions(&leas_book.accounts[i]);
          ret = scm_append(scm_list_2
                           (ret,
                            scm_list_1
                            (scm_cons
                             (scm_from_locale_string
                              (leas_book.accounts[i].name),
                              tmp))));
        }
    }
  return ret;
}


/* Get total for account by type */
SCM
leas_total_by_account_type ()
{
  SCM tmp,tmpcur,tmptotal,tmpallcur,tmpalltotal;
  SCM ret = SCM_EOL;
  int i,j;
  char* type_str;

  tmpallcur = scm_from_double(0.0);
  tmpalltotal = scm_from_double(0.0);
  for (j=EXPENSE; j <= LIABILITY; j = 2*j)
    {
      tmpcur = scm_from_double(0.0);
      tmptotal = scm_from_double(0.0);
      for (i=0; i < leas_book.n_account; i++)
        {
          if (leas_book.accounts[i].type==j)
            {
              tmp = total_transactions(&leas_book.accounts[i]);
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

      free(type_str);
      
    }

  tmp = scm_cons(tmpallcur, tmpalltotal);
  ret = scm_append
    (scm_list_2
     (ret,
      scm_list_1
      (scm_cons(scm_from_locale_string("Worth"),
                tmp))));
  
  tmp = scm_from_double(0.0);
  for (i=0; i < leas_book.n_account; i++)
    {
      tmp = scm_sum(tmp,
                    scm_from_double(leas_book.accounts[i].ob));
    }
  
  ret = scm_append
    (scm_list_2
     (ret, scm_list_1(scm_cons(scm_from_locale_string("Balances"),
                               tmp))));
  
  return ret;
}

/* Quit */
SCM
leas_q ()
{
  leas_prompton = PROMPT_OFF;
  return SCM_UNDEFINED;
}

/* Print to screen */
SCM
leas_p (SCM x)
{
  scm_display(x, scm_current_output_port());
  printf("\n");

  return SCM_UNDEFINED;
}

/* Set current account */
SCM
leas_set_account (SCM acct)
{
  leas_cur_acct = acct;
  return SCM_UNDEFINED;
}

/* Write to file */
SCM
leas_write (SCM file)
{
  char* file_c = scm_to_locale_string(file);
  if (strcmp(file_c,"")==0)
    {
      free(file_c);
      file_c = scm_to_locale_string(leas_cur_file);
    }
  else
    {
      leas_cur_file = file;
    }
  
  write_out(file_c);
  free(file_c);

  return SCM_UNDEFINED;
}

/* Read file */
SCM
leas_read (SCM file)
{

  char* file_c = scm_to_locale_string(file);
  int i;
  delete_book(&leas_book);
  leas_book.n_account = 0;
  read_in(file_c);
  free(file_c);

  if (leas_book.n_account > 0)
    {
      leas_cur_acct = scm_from_locale_string(leas_book.accounts[0].name);
      for (i=0; i < leas_book.n_account; i++)
        qsort(leas_book.accounts[i].tscts,
              leas_book.accounts[i].n_tsct,
              sizeof(tsct),
              sort_transactions);
    }
  leas_cur_file = file;
  return SCM_UNDEFINED;
}

/* Get current file */
SCM
leas_get_current_file ()
{
  return leas_cur_file;
}

/* Set number of transactions to print when selecting transaction */
SCM
leas_set_select_transact_num (SCM num)
{
  leas_select_tsct_num = scm_to_int(num);
  return num;
}

/* Retrieve version number */
SCM
leas_v ()
{
  return scm_from_locale_string(PACKAGE_VERSION);
}

/* Set the day to be the dividing day between "past" and "future" */
SCM
leas_set_current_day (SCM dmy)
{
  leas_curtime->tm_mday = scm_to_int
    (scm_list_ref(dmy,scm_from_int(0)));
  leas_curtime->tm_mon = scm_to_int
    (scm_list_ref(dmy,scm_from_int(1))) - 1;
  leas_curtime->tm_year = scm_to_int
    (scm_list_ref(dmy,scm_from_int(2))) - 1900;
  return dmy;
}

/* Get the current day */
SCM
leas_get_current_day ()
{
  return scm_list_3
    (scm_from_int(leas_curtime->tm_mday),
     scm_from_int(leas_curtime->tm_mon + 1),
     scm_from_int(leas_curtime->tm_year + 1900));
}

/* Register all functions */
void*
register_guile_functions (void* data)
{
  /* Adding functions */
  scm_c_define_gsubr("leas/at", 4, 0, 0, &leas_at);
  scm_c_define_gsubr("leas/aa", 3, 0, 0, &leas_aa);

  /* Editing functions */
  scm_c_define_gsubr("leas/ea", 3, 0, 0, &leas_ea);

  /* Deleting functions */
  scm_c_define_gsubr("leas/da", 1, 0, 0, &leas_da);
  scm_c_define_gsubr("leas/dt", 1, 0, 0, &leas_dt);

  /* Get global variables */
  scm_c_define_gsubr("leas/get-current-account", 0, 0, 0,
                     &leas_get_current_account);
  scm_c_define_gsubr("leas/get-current-file", 0, 0, 0,
                     &leas_get_current_file);

  /* Get transactions */
  scm_c_define_gsubr("leas/get-transactions", 2, 0, 0,
                     &leas_get_transactions);
  scm_c_define_gsubr("leas/get-all-transactions", 1, 0, 0,
                     &leas_get_all_transactions);
  scm_c_define_gsubr("leas/get-transactions-by-regex", 2, 0, 0,
                     &leas_get_transactions_by_regex);
  scm_c_define_gsubr("leas/get-transaction-by-location", 2, 0, 0,
                     &leas_get_transaction_by_location);
  scm_c_define_gsubr("leas/get-transactions-by-day", 3, 0, 0,
                     &leas_get_transactions_by_day);
  
  /* Get accounts */
  scm_c_define_gsubr("leas/get-account", 1, 0, 0, &leas_get_account);
  scm_c_define_gsubr("leas/get-all-accounts", 0, 0, 0,
                     &leas_get_all_accounts);
  scm_c_define_gsubr("leas/get-number-of-accounts", 0, 0, 0,
                     &leas_get_number_of_accounts);
  scm_c_define_gsubr("leas/get-account-by-location", 1, 0, 0,
                     &leas_get_account_by_location);
  scm_c_define_gsubr("leas/get-account-location", 1, 0, 0,
                     &leas_get_account_location);
  
  /* Total accounts */
  scm_c_define_gsubr("leas/total-account", 1, 0, 0, &leas_total_account);
  scm_c_define_gsubr("leas/total-all-accounts", 0, 0, 0,
                     &leas_total_all_accounts);

  scm_c_define_gsubr("leas/total-all-accounts-of-type", 1, 0, 0,
                     &leas_total_all_accounts_of_type);

  scm_c_define_gsubr("leas/total-by-account-type", 0, 0, 0,
                     &leas_total_by_account_type);

  /* Setting commands */
  scm_c_define_gsubr("leas/set-select-transact-num", 1, 0, 0,
                     &leas_set_select_transact_num);

  /* Get and set the current day */
  scm_c_define_gsubr("leas/set-current-day", 1, 0, 0,
                     &leas_set_current_day);
  scm_c_define_gsubr("leas/get-current-day", 0, 0, 0,
                     &leas_get_current_day);

  /* interface commands */
  scm_c_define_gsubr("q", 0, 0, 0, &leas_q);
  scm_c_define_gsubr("p", 1, 0, 0, &leas_p);
  scm_c_define_gsubr("leas/set-account", 1, 0, 0, &leas_set_account);
  scm_c_define_gsubr("leas/write", 1, 0, 0, &leas_write);
  scm_c_define_gsubr("leas/read", 1, 0, 0, &leas_read);

  /* retrieve version */
  scm_c_define_gsubr("leas/v", 0, 0, 0, &leas_v);

  /* generic method to create interactive commands */
  scm_c_define_gsubr("leas/call", 2, 0, 0, &leas_call);

  return NULL;
}


#endif
