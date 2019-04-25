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


typedef enum
  {
   STRING,
   ACCOUNT,
   CURRENT_ACCOUNT,
   TYPE,
   TRANSACTION,
   DAY,
   OTHER
  }
  arg_type;

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



struct account
{
  account_type type;
  char* name;
  tsct* tscts;
  double ob;
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
  int n_pos; /* used in reading data */
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

