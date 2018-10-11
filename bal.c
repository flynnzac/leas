/* 
   bal, an extensible tool for keeping track of accounts
   Zach Flynn <zlflynn@gmail.com>
*/

/* 
   This program is free software: you can redistribute it and/or modify
   it under the terms of version 3 of the GNU General Public License as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <regex.h>
#include <libguile.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <readline/readline.h>
#include <csv.h>
#include <ctype.h>

/* transaction, account, and book types */

struct tsct
{
  double amount;
  unsigned year;
  unsigned month;
  unsigned day;
  char* desc;
};

typedef struct tsct tsct;

enum account_type
  {
    EXPENSE,
    INCOME,
    ASSET,
    LIABILITY
  };

typedef enum account_type account_type;

struct account
{
  account_type type;
  char* name;
  tsct* tscts;
  double ob;
  double curr;
  int n_tsct;
  int n_pos;
};

typedef struct account account;

int
delete_account (account* acct)
{
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

struct book
{
  account* accounts;
  int n_account;
  int n_pos; /* for use in reading data */
};


int
delete_book (struct book* book)
{
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

/* global vars */
struct book bal_book;
SCM bal_cur_acct;
int bal_prompton;
SCM bal_cur_file;
int bal_prompt_exit;

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
  return NULL;
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
  double total = 0.0;

  for (i=0; i < acct->n_tsct; i++)
    {
      total = total +
        acct->tscts[i].amount;
    }

  return scm_from_double(total);
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

/* selection functions */

char*
bal_select_account (const char* prompt)
{
  int j;
  char* c;
  if (bal_book.n_account==0)
    {
      printf("No account loaded.\n");
      return NULL;
    }
  else
    {
      for (j=0; j < bal_book.n_account; j++)
        {
          printf("%d: %s\n", j, bal_book.accounts[j].name);
        }
      c = readline(prompt);

      return c;
    }
}

int
bal_select_transaction (account* acct)
{
  int i,n;
  char* option;

  n = (acct->n_tsct - 19) >= 0 ? (acct->n_tsct - 19) : 0;

  for (i=n; i < acct->n_tsct; i++)
    {
      printf("%d: %u-%u-%u %s %f\n", i,
             acct->tscts[i].year,
             acct->tscts[i].month,
             acct->tscts[i].day,
             acct->tscts[i].desc,
             acct->tscts[i].amount);
    }

  option = readline ("Transaction #: ");
  if (anyalpha(option) > 0)
    {
      free(option);
      return -1;
    }
  else
    {
      i = atoi(option);
      free(option);
      return i;
    }
}

int
bal_select_day (struct tm* curtime_info,
                char** year,
                char** month,
                char** day)
{
  char *yprompt, *mprompt, *dprompt;
  char* tmp;

  yprompt = malloc(sizeof(char)*(strlen("Year []: ")+10));
  sprintf(yprompt, "Year [%d]: ", curtime_info->tm_year+1900);
  tmp = readline(yprompt);
  *year = tmp;
  free(yprompt);

  mprompt = malloc(sizeof(char)*(strlen("Month []: ")+10));
  sprintf(mprompt, "Month [%d]: ", curtime_info->tm_mon+1);
  tmp = readline(mprompt);
  *month = tmp;
  free(mprompt);
  
  dprompt = malloc(sizeof(char)*(strlen("Day []: ")+10));
  sprintf(dprompt, "Day [%d]: ", curtime_info->tm_mday);
  tmp = readline(dprompt);
  *day = tmp;
  free(dprompt);

  return 0;
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
    {
      buf[i] = ((char*)s)[i];
    }
  buf[len] = '\0';
  
  

  if (acct->n_pos > 0 || (strcmp(buf,acct->name)==0))
    {
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
      if (!strcmp(buf, "expense"))
        {
          book->accounts[book->n_account].type = EXPENSE;
        }
      else if (!strcmp(buf, "income"))
        {
          book->accounts[book->n_account].type = INCOME;
        }
      else if (!strcmp(buf, "asset"))
        {
          book->accounts[book->n_account].type = ASSET;
        }
      else if (!strcmp(buf, "liability"))
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
read_all_transactions_into_book (struct book* book,
                                 const char* base,
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
      name = malloc(sizeof(char)*(strlen(base) +
                                  strlen(tmp_dir) +
                                  strlen("/") +
                                  strlen(book->accounts[i].name)+
                                  strlen(".csv")+2));

      strcpy(name, tmp_dir);
      strcat(name, "/");
      strcat(name, base);
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
read_in (const char* base)
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
  strcpy(untar_cmd, "tar xaf ");
  strcat(untar_cmd, base);
  strcat(untar_cmd, ".btar");
  strcat(untar_cmd, " -C ");
  strcat(untar_cmd, tmp_dir);
	  
  system(untar_cmd);
  free(untar_cmd);

  /* read in accounts */
  account_file = malloc(sizeof(char)*(strlen(base) +
                                      strlen(tmp_dir) +
                                      strlen("/accounts.csv")+
                                      2));
  strcpy(account_file, tmp_dir);
  strcat(account_file, "/");
  strcat(account_file, base);
  strcat(account_file, "/accounts.csv");

  read_book_accounts_from_csv (&bal_book, account_file);
  free(account_file);

  /* read in transactions */

  read_all_transactions_into_book (&bal_book,
                                   base,
                                   tmp_dir);

  /* clean up */
  rm_cmd = malloc(sizeof(char)*(strlen("rm -R ") +
                                strlen(tmp_dir) + 
                                strlen(base)+
                                2));
  strcpy(rm_cmd, "rm -R ");
  strcat(rm_cmd, tmp_dir);
  strcat(rm_cmd, "/");
  strcat(rm_cmd, base);
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
      fprintf(fp, "%s,%f,%u-%u-%u,%s\n",
              acct->name, acct->tscts[i].amount,
              acct->tscts[i].year,
              acct->tscts[i].month,
              acct->tscts[i].day,
              acct->tscts[i].desc);
    }

  fclose(fp);
}

int
write_accounts (const char* file)
{
  int i;
  FILE* fp;

  fp = fopen (file, "w");
  if (fp==NULL)
    {
      printf("Could not open file to write accounts.\n");
      return 1;
    }
  for (i=0; i < bal_book.n_account; i++)
    {
      switch (bal_book.accounts[i].type)
        {
        case EXPENSE:
          fprintf(fp,"%s,", "expense");
          break;
        case INCOME:
          fprintf(fp,"%s,", "income");
          break;
        case ASSET:
          fprintf(fp,"%s,", "asset");
          break;
        case LIABILITY:
          fprintf(fp,"%s,", "liability");
          break;
        }
      fprintf(fp, "%s,%f,%f\n",
              bal_book.accounts[i].name,
              bal_book.accounts[i].ob,
              bal_book.accounts[i].curr);
    }
  fclose(fp);
  return 0;
}

int
write_out (const char* base)
{
  char* account_fn;
  char* tsct_fn;
  char* tar_cmd;
  char* rm_cmd;
  char* dir_cmd;
  char* tmp_dir;

  int i;

  tmp_dir = create_tmp_dir();
  if (tmp_dir==NULL)
    return 1;
  

  dir_cmd = malloc(sizeof(char)*(strlen(base)+strlen(tmp_dir)+strlen("mkdir ")+3));

  strcpy(dir_cmd, "mkdir ");
  strcat(dir_cmd, tmp_dir);
  strcat(dir_cmd, "/");
  strcat(dir_cmd, base);
  system(dir_cmd);

  
  account_fn = malloc(sizeof(char)*(strlen(base)+strlen(tmp_dir)+strlen("accounts.csv")+3));

  strcpy(account_fn, tmp_dir);
  strcat(account_fn, "/");
  strcat(account_fn, base);
  strcat(account_fn, "/");
  strcat(account_fn, "accounts.csv");
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
                       strlen(base) +
                       strlen(tmp_dir) + 
                       strlen("/.csv") +
                       2));

      strcpy(tsct_fn, tmp_dir);
      strcat(tsct_fn, "/");
      strcat(tsct_fn, base);
      strcat(tsct_fn, "/");
      strcat(tsct_fn, bal_book.accounts[i].name);
      strcat(tsct_fn, ".csv");
      
      write_transactions (tsct_fn, &bal_book.accounts[i]);
      free(tsct_fn);
    }

  tar_cmd = malloc(sizeof(char)*(strlen("tar caf ")+
                                 strlen(base)+
                                 strlen(".btar -C ")+
                                 strlen(tmp_dir)+
                                 strlen(base)+2));
  strcpy(tar_cmd, "tar caf ");
  strcat(tar_cmd, base);
  strcat(tar_cmd, ".btar -C ");
  strcat(tar_cmd, tmp_dir);
  strcat(tar_cmd, " ");
  strcat(tar_cmd, base);
  system(tar_cmd);
  free(tar_cmd);

  rm_cmd = malloc(sizeof(char)*(strlen("rm -R ")+
                                strlen(tmp_dir)+
                                strlen(base)+2));
  strcpy(rm_cmd, "rm -R ");
  strcat(rm_cmd, tmp_dir);
  strcat(rm_cmd, "/");
  strcat(rm_cmd, base);
  system(rm_cmd);
  free(rm_cmd);
  free(tmp_dir);

  return 0;
  
}

/* main interface functions */

/* function to call a function with options specified interactively */

SCM
bal_call (SCM func, SCM options)
{

  int i,j,k;
  SCM pair;
  SCM name, type;

  char* name_c = NULL;
  char* type_c = NULL;
  char* opt;

  time_t curtime;
  struct tm* curtime_info;

  int len = scm_to_int (scm_length (options));
  char** year = malloc(sizeof(char*));
  char** month = malloc(sizeof(char*));
  char** day = malloc(sizeof(char*));

  char* func_c = scm_to_locale_string (func);
  char* command = malloc((strlen(func_c)+3)*sizeof(char));

  strcpy(command, func_c);
  strcat(command, " ");

  free(func_c);

  for (i=0; i < len; i++)
    {
      pair = scm_list_ref (options, scm_from_int(i));

      name = scm_car(pair);
      type = scm_cdr(pair);

      name_c = scm_to_locale_string (name);
      name_c = realloc (name_c, sizeof(char)*(strlen(name_c)+4));
      strcat(name_c, ": ");
      
      type_c = scm_to_locale_string (type);

      if (strcmp(type_c, "string")==0)
        {
          opt = readline(name_c);
          command = realloc (command,
                             (strlen(command)+strlen(opt)+5)*sizeof(char));
          strcat(command, "\"");
          strcat(command, opt);
          strcat(command, "\"");
        }
      else if (strcmp(type_c, "account")==0)
        {
          opt = bal_select_account(name_c);
          if (anyalpha(opt) == 0)
            {
              k = atoi(opt);
              command = realloc (command,
                                 (strlen(command)+strlen(bal_book.accounts[k].name)+5)*sizeof(char));
              strcat(command, "\"");
              strcat(command, bal_book.accounts[k].name);
              strcat(command, "\"");
            }
        }
      else if (strcmp(type_c, "default_account")==0)
        {
          opt = scm_to_locale_string (bal_cur_acct);
          command = realloc(command,
                            (strlen(command) + strlen(opt)+5)*sizeof(char));
          strcat(command, "\"");
          strcat(command, opt);
          strcat(command, "\"");

        }
      else if (strcmp(type_c, "type")==0)
        {
          printf("%d: Expense\n", EXPENSE);
          printf("%d: Income\n", INCOME);
          printf("%d: Asset\n", ASSET);
          printf("%d: Liability\n", LIABILITY);
          opt = readline (name_c);
          if (anyalpha(opt) > 0)
            {
              k = -1;
            }
          else
            {
              k = atoi(opt);
            }

          switch (k)
            {
            case EXPENSE:
              command = realloc (command,
                                 (strlen(command)+strlen("expense")+5)*sizeof(char));
              strcat(command, "\"expense\"");
              break;
            case INCOME:
              command = realloc (command,
                                 (strlen(command)+strlen("income")+5)*sizeof(char));
              strcat(command, "\"income\"");
              break;
            case ASSET:
              command = realloc (command,
                                 (strlen(command)+strlen("asset")+5)*sizeof(char));
              strcat(command, "\"asset\"");
              break;
            case LIABILITY:
              command = realloc (command,
                                 (strlen(command)+strlen("liability")+5)*sizeof(char));
              strcat(command, "\"liability\"");
              break;
            default:
              command = realloc (command,
                                 (strlen(command)+strlen("liability")+5)*sizeof(char));
              strcat(command, "\"none\"");
              break;
            }

        }
      else if (strcmp(type_c, "currency")==0)
        {
          opt = readline(name_c);
          if (strcmp(opt,"")==0)
            {
              command = realloc(command,
                                (strlen(command)+3)*sizeof(char));
              strcat(command, "1");
            }
          else
            {
              command = realloc(command,
                                (strlen(command)+strlen(opt)+3)*sizeof(char));
              strcat(command, opt);
            }
        }
      else if (strcmp(type_c, "transaction")==0)
        {
          printf("%s\n", name_c);
          opt = bal_select_account("Account: ");

          if (opt==NULL) return SCM_UNDEFINED;

          if (anyalpha(opt) == 0)
            {
              k = atoi(opt);
          
              free(opt);

              j = bal_select_transaction (&bal_book.accounts[k]);

              if (j < 0)
                opt = malloc(sizeof(char));
              else
                opt = malloc(sizeof(char)*(((k % 10) + 1) + 1 +
                                           ((j % 10) + 1) + 1));
              if (j >= 0)
                {

                  sprintf(opt, "(cons %d %d)", k, j);

                  command = realloc(command,
                                    (strlen(command)+
                                     strlen(opt)+3)*sizeof(char));

                  strcat(command, opt);
                }
            }
          
        }
      else if (strcmp(type_c, "day")==0)
        {

          time(&curtime);
          curtime_info = localtime(&curtime);

          printf("%s\n", name_c);

          bal_select_day (curtime_info, year, month, day);

          command = realloc(command,
                            sizeof(char)*(strlen(command)+4+2+2+10));

          
          curtime_info->tm_year = strcmp(*year,"")==0 ? (curtime_info->tm_year+1900) : atoi(*year);
          curtime_info->tm_mon = strcmp(*month,"")==0 ? (curtime_info->tm_mon+1) : atoi(*month);
          curtime_info->tm_mday = strcmp(*day,"")==0 ? curtime_info->tm_mday : atoi(*day);
          
          sprintf(command, "%s\"%d-%d-%d\"", command,
                  curtime_info->tm_year,
                  curtime_info->tm_mon,
                  curtime_info->tm_mday);

          
          
        }
      else
        {
          opt = readline(name_c);
          command = realloc (command,
                             (strlen(command)+strlen(opt)+5)*sizeof(char));
          strcat(command, opt);
        }


      if (strcmp(type_c,"day") != 0)
        {
          free(opt);
        }

      free(name_c);
      free(type_c);
      
      if (i != (len-1))
        {
          command = realloc(command,
                            (strlen(command)+2)*sizeof(char));
          strcat(command, " ");
        }

    }

  SCM ret = scm_c_catch (SCM_BOOL_T,
                         exec_string_safe,
                         command,
                         handle_error,
                         command,
                         NULL,
                         NULL);
  free(year);
  free(month);
  free(day);

  free(command);
  return ret;
}


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


  bal_cur_acct = account_name;
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
  
  char* type_c = scm_to_locale_string (type);
  char* name_c = scm_to_locale_string (name);
  double ob_c = scm_to_double(ob);
  double curr_c = scm_to_double(currency);
  account_type type_ac;

  if (bal_book.n_account == 0)
    {
      bal_book.accounts = malloc(sizeof(account));
    }
  else
    {
      bal_book.accounts = realloc(bal_book.accounts,
                                  sizeof(account)*(bal_book.n_account+1));
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

  bal_book.accounts[bal_book.n_account].type = type_ac;
  bal_book.accounts[bal_book.n_account].n_tsct = 0;
  bal_book.accounts[bal_book.n_account].n_pos = 0;
  bal_book.accounts[bal_book.n_account].name =
    malloc(sizeof(char)*(strlen(name_c)+1));
  strcpy(bal_book.accounts[bal_book.n_account].name,
         name_c);

  bal_book.accounts[bal_book.n_account].ob = ob_c;
  bal_book.accounts[bal_book.n_account].curr = curr_c;

  bal_book.n_account = bal_book.n_account + 1;

  free(type_c);
  free(name_c);

  bal_cur_acct = name;
  return SCM_UNDEFINED;
  
}

/* Edit existing transactions and accounts */

SCM
bal_et (SCM at_pair)
{

  SCM account = scm_car (at_pair);
  SCM ts = scm_cdr (at_pair);
  
  int k = scm_to_int (account);
  int j = scm_to_int (ts);
  tsct* t;

  if (j < bal_book.accounts[k].n_tsct)
    {
      t = &bal_book.accounts[k].tscts[j];
    }
  else
    {
      return SCM_UNDEFINED;
    }
    
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

/* Get number of accounts */
SCM
bal_get_number_of_accounts ()
{
  return scm_from_int(bal_book.n_account);
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
  SCM ret;
  char* acct_c;

  acct_c = scm_to_locale_string (acct);

  account* a = find_account_in_book (&bal_book, acct_c);
  int i;

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


/* Get opening balances */
SCM
bal_opening_balances ()
{
  SCM ret = SCM_EOL;
  int i;

  for (i=0; i < bal_book.n_account; i++)
    {
      ret = scm_append
        (scm_list_2
         (ret,
          scm_list_1
          (scm_cons
           (scm_from_locale_string(bal_book.accounts[i].name),
            scm_from_double(bal_book.accounts[i].ob)))));
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
          ret = scm_append
            (scm_list_2
             (ret,
              scm_list_1
              (scm_cons(scm_from_locale_string("expense"),
                        tmp))));
          break;
        case INCOME:
          ret = scm_append
            (scm_list_2
             (ret,
              scm_list_1
              (scm_cons(scm_from_locale_string("income"),
                        tmp))));
          break;
        case ASSET:
          ret = scm_append
            (scm_list_2
             (ret,
              scm_list_1
              (scm_cons(scm_from_locale_string("asset"),
                        tmp))));
          break;
        case LIABILITY:
          ret = scm_append
            (scm_list_2
             (ret,
              scm_list_1
              (scm_cons(scm_from_locale_string("liability"),
                        tmp))));
          break;
        }
          
    }

  tmp = scm_from_double(0.0);
  for (i=0; i < bal_book.n_account; i++)
    {
      tmp = scm_sum(tmp,
                    scm_from_double(bal_book.accounts[i].ob));
    }
  
  ret = scm_append
    (scm_list_2
     (ret, scm_list_1(scm_cons(scm_from_locale_string("balances"),
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
  write_out(file_c);
  free(file_c);
  bal_cur_file = file;
  return SCM_UNDEFINED;
}

/* Read file */

SCM
bal_read (SCM file)
{
  char* file_c = scm_to_locale_string (file);
  delete_book (&bal_book);
  bal_book.n_account = 0;
  read_in (file_c);
  free(file_c);
  if (bal_book.n_account > 0)
    {
      bal_cur_acct = scm_from_locale_string (bal_book.accounts[0].name);
    }
  return SCM_UNDEFINED;
}

/* Get current file */
SCM
bal_get_current_file ()
{
  return bal_cur_file;
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

  /* Get global variables */
  scm_c_define_gsubr("bal/get-current-account", 0, 0, 0,
                     &bal_get_current_acct);
  scm_c_define_gsubr("bal/get-current-file", 0, 0, 0,
                     &bal_get_current_file);

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
  scm_c_define_gsubr("bal/get-number-of-accounts", 0, 0, 0,
                     &bal_get_number_of_accounts);
  
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
  scm_c_define_gsubr("bal/read", 1, 0, 0, &bal_read);

  /* get opening balances for accounts */
  scm_c_define_gsubr("bal/opening-balances", 0, 0, 0,
                     &bal_opening_balances);

  /* generic method to create interactive commands */
  scm_c_define_gsubr("bal/call", 2, 0, 0, &bal_call);
  
  return NULL;
}

/* main scheme functions to build the command-line interface */

#define QUOTE(...) #__VA_ARGS__

void
bal_standard_func ()
{
  scm_c_eval_string
    (QUOTE(
           (define bal/number-to-quick-list 10)
           (define aa
            (lambda ()
             (bal/call "bal/aa"
              (list
               (cons "Account" "string")
               (cons "Type" "type")
               (cons "Opening Balance" "real")
               (cons "Currency [1]" "currency")))))
           (define at
            (lambda ()
             (bal/call "bal/at"
              (list
               (cons "Account" "default_account")
               (cons "Amount" "real")
               (cons "Description" "string")
               (cons "Day" "day")))))

           (define ltn
            (lambda ()
             (let ((tscts (bal/call "bal/get-transactions"
                           (list
                            (cons "Account" "default_account")
                            (cons "How many?" "integer")))))
              (map-in-order
               (lambda (x)
                (display
                 (string-append
                  (number->string (list-ref x 2))
                  "-"
                  (number->string (list-ref x 3))
                  "-"
                  (number->string (list-ref x 4))
                  " "
                  (list-ref x 0)
                  " "
                  (number->string (list-ref x 1))
                  "\n")))
               tscts))
             (display "\n")))


           (define print-tscts
            (lambda (k)
             (if (list? k)
               (map-in-order
                (lambda (x)
                 (display
                  (string-append
                   (number->string (list-ref x 2))
                   "-"
                   (number->string (list-ref x 3))
                   "-"
                   (number->string (list-ref x 4))
                   " "
                   (list-ref x 0)
                   " "
                   (number->string (list-ref x 1))
                   "\n")))
                k))))




           (define et
            (lambda ()
             (bal/call "bal/et"
              (list
               (cons "Transaction" "transaction")))))

           (define lt
            (lambda ()
             (print-tscts (bal/get-transactions (bal/get-current-account) bal/number-to-quick-list))))


      
           (define ea
            (lambda ()
             (bal/call "bal/ea"
              (list
               (cons "Account" "account")
               (cons "New account name" "string")))))

           (define da
            (lambda ()
             (bal/call "bal/da"
              (list
               (cons "Account" "account")))))

           (define dt
            (lambda ()
             (bal/call "bal/dt"
              (list
               (cons "Transaction" "transaction")))))

           (define la
            (lambda ()
             (let ((accts (bal/total-all-accounts)))
              (map-in-order
               (lambda (x)
                (display
                 (string-append
                  (car x)
                  " "
                  (number->string (cdr x))
                  "\n")))
               accts))))

           (define bt
            (lambda ()
             (let ((accts (bal/total-by-account-type)))
              (map-in-order
               (lambda (x)
                (display
                 (string-append
                  (car x)
                  " "
                  (number->string (cdr x))
                  "\n")))
               accts)
              (display
               (string-append
                "assets + liabilities + income + expenses: "
                (number->string (apply + (map cdr (cdr (reverse accts)))))
                "\n")))))


           (define re
            (lambda ()
             (print-tscts
              (bal/call "bal/get-transactions-by-regex"
               (list
                (cons "Account" "default_account")
                (cons "Regular Expression" "string"))))))


           (define sa
            (lambda ()
             (bal/call "bal/set-account"
              (list
               (cons "Account" "account")))))

           (define ca
            (lambda ()
             (display
              (string-append (bal/get-current-account) "\n"))))
           
           (define w
            (lambda ()
             (bal/call "bal/write"
              (list
               (cons "File" "string")))))

           (define r
            (lambda ()
             (bal/call "bal/read"
              (list
               (cons "File" "string")))))

           (define bal/prompt
            (lambda ()
             (if (= (bal/get-number-of-accounts) 0)
               ":> "
                 (string-append
                  "("
                  (bal/get-current-account)
                  ") :> "))))
           
           (define bal/t
            (lambda (to-account from-account amount desc day)
             (let ((to-type (list-ref (bal/get-account to-account) 1))
                   (from-type (list-ref (bal/get-account from-account) 1)))
              (bal/at to-account amount desc day)
              (bal/at from-account (* -1 amount) desc day))))

           (define t
            (lambda ()
             (bal/call "bal/t"
              (list
               (cons "To Account" "account")
               (cons "From Account" "account")
               (cons "Amount" "real")
               (cons "Description" "string")
               (cons "Day" "day")))))
             
           ));
      
}

void
bal_exit (int exit_code)
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
  exit (exit_code);
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

  bal_prompt_exit = 1;
  
  scm_with_guile (&register_guile_functions, NULL);
  bal_standard_func();
  bal_cur_acct = SCM_UNDEFINED;

  bal_cur_file = scm_from_locale_string ("_");
  
  while ((k = getopt(argc, argv, "f:l:s")) != -1)
    {
      switch (k)
        {
        case 'l':
          scm_c_primitive_load(optarg);
          break;
        case 'f':
          bal_cur_file = scm_from_locale_string (optarg);
          break;
        case 's':
          bal_prompt_exit = 0;
          break;
        case '?':
          if (optopt=='l')
            {
              fprintf (stderr, "Option -l requires an argument.\n");
              exit(1);
            }
          else if (optopt=='f')
            {
              fprintf (stderr, "Option -f requires an argument.\n");
              exit(1);
            }
          else
            {
              fprintf (stderr, "Unknown option, -%c.\n", optopt);
              exit(1);
            }
          break;
        default:
          abort();
          break;
        }
    }

  fname = scm_to_locale_string (bal_cur_file);
  fname = realloc(fname, sizeof(char)*(strlen(fname)+
                                       strlen(".btar")
                                       +1));
  strcat(fname, ".btar");
  
  if (access(fname, R_OK) != -1)
    {
      free(fname);
      fname = scm_to_locale_string(bal_cur_file);
      read_in (fname);
      if (bal_book.n_account > 0)
        {
          bal_cur_acct = scm_from_locale_string
            (bal_book.accounts[0].name);
        }
      free(fname);
    }

  if (optind < argc)
    {
      for (i=optind; i < argc; i++)
        {
          ret = scm_c_catch (SCM_BOOL_T,
                             exec_string_safe_history,
                             argv[i],
                             handle_error,
                             argv[i],
                             NULL,
                             NULL);
        }

      bal_exit(0);
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
      free(prompt);
      free(command);
    }

  bal_exit(0);
  return 0;
}


                        
  
