/* 
   bal - for keeping accounts in order and studying past spending habits
   Copyright Zach Flynn <zlflynn@gmail.com>

   This program is free software: you can redistribute it and/or
   modify it under the terms of version 3 of the GNU General Public
   License as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <https://www.gnu.org/licenses/>.
*/

#define BAL_VERSION "0.2.0-dev"
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

/* convenience, manipulation functions */

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
          str = realloc(str, (i+1)*sizeof(char));
          break;
        }
    }
  return str;
}

int
digits (int num)
{
  /* count number of digits in a number */
  int n = 1;
  while ((num / 10) > 0)
    {
      n++;
      num = num / 10;
    }
  return n;
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
  tmp_dir = malloc(sizeof(char));
  tmp_dir[0] = '\0';

  while (fgets(buffer, sizeof(buffer)-1, fp) != NULL)
    {
      tmp_dir = realloc(tmp_dir, sizeof(char)*((i+1)*100+1));
      strcat(tmp_dir, buffer);
      i++;
    }
  tmp_dir[strcspn(tmp_dir, "\n")] = 0;
  pclose(fp);

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
   EXPENSE,
   INCOME,
   ASSET,
   LIABILITY
  } account_type;

typedef enum
  {
   STRING,
   ACCOUNT,
   EXPENSE_ACCOUNT,
   LIABILITY_ACCOUNT,
   ASSET_ACCOUNT,
   INCOME_ACCOUNT,
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
type_from_string (SCM type)
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

/* set attributes and arrange bal objects */

void
set_tsct_time (tsct* t, struct tm* time)
{
  /* set transaction time */
  t->year = time->tm_year + 1900;
  t->month = time->tm_mon + 1;
  t->day = time->tm_mday;
}

void
set_tsct_time_from_scm (tsct* t, SCM time)
{
  /* set transaction time from SCM list */
  t->year = scm_to_int
    (scm_list_ref(time,scm_from_int(2)));
  t->month = scm_to_int
    (scm_list_ref(time,scm_from_int(1)));
  t->day = scm_to_int
    (scm_list_ref(time,scm_from_int(0)));
}

int
sort_transactions (const void* a, const void* b)
{
  /* sort transactionis from earliest to latest */
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

static book bal_book;
static SCM bal_cur_acct;
static int bal_prompton;
static SCM bal_cur_file;
static int bal_prompt_exit;
static int bal_select_tsct_num;
static struct tm* bal_curtime;


int
check_account_trans (int a, int t)
{
  /* check whether an (account,transaction) exists by number */
  
  if ((0 <= a) & (bal_book.n_account > a))
    {
      if ((0 <= t) & (bal_book.accounts[a].n_tsct > t))
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

SCM
exec_string_safe_history (void* toeval_v)
{
  /* execute a string and add to history */
  if (strcmp((char*) toeval_v, "") != 0)
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

/* selection functions for bal/call */

char*
bal_select_account (const char* prompt, int kind)
{
  int j, ndigit;
  char* c;

  if (bal_book.n_account==0)
    {
      printf("No account.\n");
      return NULL;
    }
  else
    {
      ndigit = (bal_book.n_account / 10) + 1;
      do
        {
          for (j=0; j < bal_book.n_account; j++)
            {
	      if (bal_book.accounts[j].type == kind || kind < 0)
		printf("%*d: %s\n", ndigit+1, j, bal_book.accounts[j].name);
            }
          c = readline(prompt);
        } while (strcmp(c,"") && (anyalpha(c) || atoi(c) < 0 ||
                                  atoi(c) >= bal_book.n_account));

      return c;
    }
}

int
bal_select_transaction (account* acct)
{
  int i,n,ndigit,maxlen,end;
  char* option;

  maxlen = 0;
  end = 0;
  while ((end < acct->n_tsct) && tsct_before_today(acct->tscts[end]))
    end++;

  n = (end - bal_select_tsct_num) >= 0 ?
    (end - bal_select_tsct_num) : 0;
  
  ndigit = (acct->n_tsct / 10) + 1;

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

char*
bal_select_day (struct tm* curtime_info)
{
  char* prompt;
  char* year; char* month; char* day;

  prompt = copy_string_insert_int("Year [%d]: ", curtime_info->tm_year+1900);
  year = readline(prompt);
  free(prompt);

  prompt = copy_string_insert_int("Month [%d]: ", curtime_info->tm_mon+1);
  month = readline(prompt);
  free(prompt);

  prompt = copy_string_insert_int("Day [%d]: ", curtime_info->tm_mday);
  day = readline(prompt);
  free(prompt);

  char* ret = malloc(sizeof(char)*(strlen("(list   )")+2+2+4+1));
  sprintf(ret, "(list %d %d %d)",
          strcmp(day,"")==0 ? curtime_info->tm_mday : atoi(day),
          strcmp(month,"")==0 ? (curtime_info->tm_mon+1) : atoi(month),
          strcmp(year,"")==0 ? (curtime_info->tm_year+1900) : atoi(year));
  return ret;
}

account_type
bal_select_account_type (char* prompt)
{
  int k;
  char* opt;
  
  printf("%d: Expense\n", EXPENSE);
  printf("%d: Income\n", INCOME);
  printf("%d: Asset\n", ASSET);
  printf("%d: Liability\n", LIABILITY);
  opt = readline (prompt);

  k = anyalpha(opt) > 0 ? -1 : atoi(opt);

  return k;
}
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
                                strlen(fn)+
                                1));

  sprintf(rm_cmd, "rm -R %s/%s", tmp_dir, fn);
  system(rm_cmd);

  free(rm_cmd); free(tmp_dir); free(fn);
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
  char* fn = copy_string(basename(base));

  fn = remove_ext(fn);
  int i;

  tmp_dir = create_tmp_dir();
  if (tmp_dir==NULL)
    return 1;

  cmd = malloc(sizeof(char)*(strlen(fn)+
                             strlen(tmp_dir)+
                             strlen("mkdir /")+1));

  sprintf(cmd, "mkdir %s/%s", tmp_dir, fn);
  system(cmd);
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

  for (i=0; i < bal_book.n_account; i++)
    {
      tsct_fn = malloc
        (sizeof(char)*(strlen(bal_book.accounts[i].name) +
                       strlen(fn) +
                       strlen(tmp_dir) + 
                       strlen("//.csv") +
                       1));
      sprintf(tsct_fn, "%s/%s/%s.csv", tmp_dir, fn,
              bal_book.accounts[i].name);
      
      write_transactions (tsct_fn, &bal_book.accounts[i]);
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

/* function to call a function with options specified interactively */

SCM
bal_call (SCM func, SCM options)
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

  bal_prompton = 2;
  for (i=0; i < len; i++)
    {
      if (bal_prompton != 2)
        break;

      pair = scm_list_ref (options, scm_from_int(i));

      name = scm_car(pair);

      name_c = scm_to_locale_string (name);
      name_c = realloc (name_c, sizeof(char)*(strlen(name_c)+4));
      strcat(name_c, ": ");
      
      type = type_from_string(scm_cdr(pair));

      switch (type)
        {
        case STRING:
          opt = readline(name_c);
          append_to_string(&command, opt, "\"");
          break;
        case ACCOUNT:
          opt = bal_select_account(name_c, -1);
          if (opt != NULL && strcmp(opt,"") && anyalpha(opt) == 0)
            {
              k = atoi(opt);
              append_to_string(&command,
                               bal_book.accounts[k].name,
                               "\"");
            } else bal_prompton = 1;
          break;
        case EXPENSE_ACCOUNT:
          opt = bal_select_account(name_c, EXPENSE);
          if (opt != NULL && strcmp(opt,"") && anyalpha(opt) == 0)
            {
              k = atoi(opt);
              append_to_string(&command,
                               bal_book.accounts[k].name,
                               "\"");
            } else bal_prompton = 1;
          break;
        case LIABILITY_ACCOUNT:
          opt = bal_select_account(name_c, LIABILITY);
          if (opt != NULL && strcmp(opt,"") && anyalpha(opt) == 0)
            {
              k = atoi(opt);
              append_to_string(&command,
                               bal_book.accounts[k].name,
                               "\"");
            } else bal_prompton = 1;
          break;
        case ASSET_ACCOUNT:
          opt = bal_select_account(name_c, ASSET);
          if (opt != NULL && strcmp(opt,"") && anyalpha(opt) == 0)
            {
              k = atoi(opt);
              append_to_string(&command,
                               bal_book.accounts[k].name,
                               "\"");
            } else bal_prompton = 1;
          break;
        case INCOME_ACCOUNT:
          opt = bal_select_account(name_c, INCOME);
          if (opt != NULL && strcmp(opt,"") && anyalpha(opt) == 0)
            {
              k = atoi(opt);
              append_to_string(&command,
                               bal_book.accounts[k].name,
                               "\"");
            } else bal_prompton = 1;
          break;	  	  
        case CURRENT_ACCOUNT:
          opt = scm_to_locale_string (bal_cur_acct);
          append_to_string(&command, opt, "\"");
          break;
        case TYPE:
          k = bal_select_account_type(name_c);
          if (k >= 0)
            {
              tmp_str = account_type_to_string(k);
              append_to_string(&command, tmp_str, "\"");
              free(tmp_str);
            } else bal_prompton = 1;
          break;
        case TRANSACTION:
          printf("%s\n", name_c);
          opt = bal_select_account("Account: ", -1);
          if (opt!=NULL && strcmp(opt,"")!=0)
            {
              if (anyalpha(opt) == 0)
                {
                  k = atoi(opt);
                  j = bal_select_transaction (&bal_book.accounts[k]);

                  if (j >= 0)
                    {
                      tmp_str = malloc(sizeof(char)*(strlen("(cons  )") +
                                                     digits(k)+digits(j)+1));
                      sprintf(tmp_str, "(cons %d %d)", k, j);
                      append_to_string(&command, tmp_str, "");
                      free(tmp_str);
                    }
                }
            } else bal_prompton = 1;
          break;
        case DAY:
          time(&curtime);
          curtime_info = localtime(&curtime);

          printf("%s\n", name_c);
          opt = bal_select_day (curtime_info);
          append_to_string(&command, opt, "");
          break;
        default:
          opt = readline(name_c);
          append_to_string(&command, opt, "");
          break;
        }

      if (type != TYPE) free(opt);
      free(name_c);
      
      if (i != (len-1))
        append_to_string(&command, " ", "");

    }

  if (bal_prompton == 2)
    {
      SCM ret = scm_c_catch (SCM_BOOL_T,
                             exec_string_safe,
                             command,
                             handle_error,
                             command,
                             NULL,
                             NULL);
      return ret;
    }

  free(command);
  bal_prompton = 1;

  return SCM_UNDEFINED;
}

/* Main Scheme interface functions */

/* Add transactions and accounts */
SCM
bal_at (SCM account_name,
        SCM amount,
        SCM desc,
        SCM day)
{
  char* account_c = scm_to_locale_string (account_name);
  account* acct = find_account_in_book (&bal_book, account_c);
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
  char* name_c = scm_to_locale_string (name);
  double ob_c = scm_to_double(ob);

  free(type_str);

  if (bal_book.n_account == 0)
    bal_book.accounts = malloc(sizeof(account));
  else
    bal_book.accounts = realloc(bal_book.accounts,
                                sizeof(account)*(bal_book.n_account+1));

  bal_book.accounts[bal_book.n_account].type = type_c;
  bal_book.accounts[bal_book.n_account].n_tsct = 0;
  bal_book.accounts[bal_book.n_account].n_pos = 0;
  bal_book.accounts[bal_book.n_account].name = copy_string(name_c);

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
  account* acct = find_account_in_book(&bal_book, cur_name_c);
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

  char* current_account = scm_to_locale_string (bal_cur_acct);

  if (strcmp(current_account, cur_name_c)==0)
    bal_cur_acct = name;

  free(ob_c); free(cur_name_c); free(name_c); free(current_account);

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

  if (check_account_trans(k,j))
    {
      free(bal_book.accounts[k].tscts[j].desc);
      for (i=j; i < (bal_book.accounts[k].n_tsct-1); i++)
        bal_book.accounts[k].tscts[i] = bal_book.accounts[k].tscts[i+1];
      bal_book.accounts[k].n_tsct = bal_book.accounts[k].n_tsct - 1;
    }
  return SCM_UNDEFINED;

}


SCM
bal_da (SCM account)
{

  char* account_c = scm_to_locale_string (account);
  if (bal_book.n_account == 1)
    {
      printf("Cannot delete account: %s.", account_c);
      printf(" There must always be at least one account.\n");
      free(account_c);
      return SCM_UNDEFINED;
    }
  int i,flag;

  flag = 0;
  for (i=0; i < bal_book.n_account; i++)
    {
      if (strcmp(bal_book.accounts[i].name, account_c)==0)
        {
          flag = 1;
          break;
        }
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
  if (bal_book.n_account==0) return scm_from_locale_string("");
      
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
bal_get_all_transactions (SCM acct)
{
  SCM ret;
  char* acct_c;

  acct_c = scm_to_locale_string (acct);

  account* a = find_account_in_book (&bal_book, acct_c);
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

SCM
bal_get_transactions_by_regex (SCM acct_s, SCM regex_s)
{

  char* acct_c = scm_to_locale_string (acct_s);
  account* acct = find_account_in_book (&bal_book, acct_c);
  free(acct_c);
  
  if (acct==NULL) return SCM_EOL;

  char* regex_txt = scm_to_locale_string (regex_s);
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
  return list;
}

/* Get account information by name */

SCM
bal_get_account (SCM name)
{
  char* name_c = scm_to_locale_string (name);
  account* acct = find_account_in_book (&bal_book, name_c);

  free(name_c);
  if (acct == NULL)
    return SCM_EOL;
  else
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

  if (check_account_trans(acct_c, tsct_c))
    return tsct_to_scm(bal_book.accounts[acct_c].tscts[tsct_c]);
  else
    return SCM_EOL;
}

/* Get account by number */
SCM
bal_get_account_by_location (SCM acct_num)
{
  int acct_c = scm_to_int(acct_num);
  if (check_account_trans(acct_c,0))
    return acct_to_scm(bal_book.accounts[acct_c]);
  else
    return SCM_EOL;
}

/* Get transactions by day */
SCM
bal_get_transactions_by_day (SCM acct, SCM first_day, SCM last_day)
{
  char* acct_c = scm_to_locale_string(acct);
  account* acct_p = find_account_in_book(&bal_book, acct_c);
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
bal_total_account (SCM acct)
{
  SCM ret;

  char* acct_c = scm_to_locale_string (acct);
  account* a = find_account_in_book (&bal_book, acct_c);
  free(acct_c);

  if (a == NULL) return SCM_EOL;
  
  ret = total_transactions (a);
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

void
bal_exit ()
{
  int c;

  if (bal_prompt_exit)
    {
      printf("Save file? (y/n) ");
      c = getchar();

      if (c=='y')
        {
          (void) bal_write (bal_cur_file);
        }
    }
  delete_book (&bal_book);
}

int
dummy_event ()
{
  return 0;
}

void
interrupt_handler (int status)
{
  rl_replace_line("",0);
  if (bal_prompton==2) bal_prompton = 1;
  rl_done = 1;
}

void
bal_standard_func (char* file)
{
  char* command = copy_string("load ");
  append_to_string(&command, file, "\"");
  (void) exec_string_safe(command);
  free(command);
}

int
main (int argc, char** argv)
{
  SCM ret;
  char* prompt;
  char* command;
  char* fname;
  int i;
  int k;
  time_t curtime_time;
  int load = 1;

  time(&curtime_time);
  bal_curtime = localtime(&curtime_time);

  bal_prompt_exit = 1;
  bal_select_tsct_num = 19;

  scm_with_guile (&register_guile_functions, NULL);
  bal_standard_func(BAL_SCM_INSTALL);
  bal_cur_file = scm_from_locale_string ("_.btar");

  rl_event_hook = dummy_event;
  signal(SIGINT,interrupt_handler);
  
  while ((k = getopt(argc, argv, "f:l:sn")) != -1)
    {
      switch (k)
        {
        case 'n':
          load = 0;
          break;
        case 'l':
          scm_c_primitive_load(optarg);
          break;
        case 'f':
          bal_cur_file = scm_from_locale_string (optarg);
          fname = scm_to_locale_string (bal_cur_file);

          if (access(fname, R_OK) != -1)
            {
              free(fname);
              fname = scm_to_locale_string(bal_cur_file);
              read_in (fname);
              if (bal_book.n_account > 0)
                bal_cur_acct = scm_from_locale_string
                  (bal_book.accounts[0].name);
            }
          free(fname);
          break;
        case 's':
          bal_prompt_exit = 0;
          break;
        case '?':
          if (optopt=='l')
            fprintf (stderr, "Option -l requires an argument.\n");
          else if (optopt=='f')
            fprintf (stderr, "Option -f requires an argument.\n");
          else
            fprintf (stderr, "Unknown option, -%c.\n", optopt);
	  
          exit(1);
          break;
        default:
          abort();
          break;
        }
    }

  if (load)
    {
      char* home = getenv("HOME");
      char* balrc;

      balrc = copy_string(home);
      append_to_string(&balrc, "/.balrc.scm", "");
      
      if (access(balrc, R_OK) != -1)
        scm_c_primitive_load(balrc);

      free(balrc);
    }

  if (optind < argc)
    {
      for (i=optind; i < argc; i++)
        ret = scm_c_catch (SCM_BOOL_T,
                           exec_string_safe_history,
                           argv[i],
                           handle_error,
                           argv[i],
                           NULL,
                           NULL);
      bal_exit();
      return 0;
    }


  if (bal_book.n_account == 0)
    {
      /* create cash account if no other account */
      bal_aa (scm_from_locale_string("Cash"),
              scm_from_locale_string("asset"),
              scm_from_double(0.0));
    }
  
  bal_prompton = 1;

  while (bal_prompton)
    {
      ret = scm_c_catch (SCM_BOOL_T,
                         exec_string_safe,
                         "bal/prompt",
                         handle_error,
                         "bal/prompt",
                         NULL,
                         NULL);

      prompt = scm_to_locale_string (ret);
      command = readline(prompt);
      
      ret = scm_c_catch (SCM_BOOL_T,
                         exec_string_safe_history,
                         command,
                         handle_error,
                         command,
                         NULL,
                         NULL);
      
      free(prompt); free(command);
    }
  scm_gc();
  bal_exit();
  return 0;
}

