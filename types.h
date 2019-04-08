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

