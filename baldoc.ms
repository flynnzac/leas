.RP no
\R'PD 0.6v'
\R'PS 10000'
.ND
.TL
The
.CW bal
manual
.AU
.AB no
.CW bal
is an extensible tool for keeping accounts in order and studying past spending habits. This manual gives a basic overview of the package and provides a full reference manual to the various Guile Scheme functions
.CW bal
exposes.
.LP
Copyright (c) 2018 Zach Flynn.
Permission is granted to copy, distribute and/or modify this document
under the terms of the FreeBSD Documentation License. A copy of the license is included in the section entitled "FreeBSD
Documentation License".
.AE
.NH
Introduction
.LP
.B bal
is an extensible personal account manager, recording all the good, bad, terrible, and mundane financial decisions you make and helping you plan for the future. It is extensible because its prompt is simply a Guile Scheme interpreter (where you don't enclose the outer S-expression in parenthesis) so it can execute arbitrary code on your transactions. It has a built-in function for making Scheme functions interactive that allows you to specify that the function takes "accounts" or "transactions" (which are simply Scheme lists with certain values) as arguments and have the user select the account or transaction from a menu.
.LP
The main interface is a command prompt interface using the Scheme interpreter with some built-in C functions that can be called from Scheme, but of course, other interfaces could be developed by running Scheme code at the interpreter.
.LP
This document describes basic usage of
.B bal
using the built-in functions and gives a full reference manual to the functions that are available from Scheme to aid in writing your own code to automate whatever tasks your financial life needs automated (for example, a script to update the value of your stock or mutual fund holdings by fetching the latest prices from the internet).
.NH
Data types
.LP
There are two data types in
.CW bal
of interest to programming it in Scheme: the account
and the transaction.  This brief section discusses the key concepts.
.LP
.I "Transaction and Account Numbers."
Each transaction has a "coordinate" given by the pair (cons account_number transaction_number).  Transactions and accounts are numbered starting at 0, accounts are numbered in the order in which they were created, and transactions are in chronological order by day.  Transaction number is the element number in the list returned by
.CW "(bal/get-all-transactions account_name)."
Account number is the element number of the account in the list returned by
.CW "(bal/get-all-accounts)."
.LP
.I "Accounts in Scheme."
When Scheme functions return accounts they do so as a four-element list,
.RS
.CW "(account_name account_type number_of_transactions opening_balance)"
.RE
.LP
.I "Transactions in Scheme."
When Scheme functions return transactions they do so as a five-element list,
.RS
.CW "(description amount year month day)"
.RE
.NH
Interactive functions
.LP
The command prompt is a Scheme interpreter except that the outer-most S expression should not be enclosed in parenthesis to save on typing.  This section describes the "interactive functions" \(em functions that can be entered without arguments at the prompt and then prompt the user for options \(em available in
.CW bal.
.NH 2
Functions to add accounts and transactions
.IP  
.CW "(aa)"
.QP
Add account. Prompts for the name of the account, the kind of account it is, and its opening balance.
.IP 
.CW "(at)"
.QP
Add transaction.  Prompts for the account to add the transaction to, the amount of the transaction, a description of the transaction, and when the transaction was made.
.IP 
.CW "(t)"
.QP
Add a transfer between accounts.  This is the most commonly-used command because most financial events involve a transaction taking money from one account and another transaction putting money in the other.  Prompts for the "from" and "to" accounts, the amount of the transfer, a description of the transaction, and when the transaction took place.
.NH 2
Functions to edit accounts transactions
.IP  
.CW "(ea)"
.QP
Rename account.
.IP 
.CW "(et [n])"
.QP
Edit transaction.  Prompts for the transaction to edit and all the various properties of a transaction as if you were adding the transaction anew.  Entering nothing for a cateogry leaves it unchanged.  Optionally, include a different number of transactions than the default to show when prompting for the transaction (any transaction number can be entered even if it is not displayed in the menu; the menu is just a convenience).
.IP 
.CW "(da)"
.QP
Delete account.
.IP 
.CW "(dt)"
.QP
Delete transaction.
.NH 2
Functions to print account and transaction information
.IP  
.CW "(la)"
.QP
Lists all accounts and their present and future balances.
.IP 
.CW "(lt)"
.QP
Lists the most recent transaction in the current account.  The number of transactions to list is set by the Scheme variable
.CW bal/number-to-quick-list
so entering the following at the prompt will change the number to list:
.CW "set! bal/number-to-quick-list NUM"
.IP 
.CW "(ltn)"
.QP
The same as
.CW "(lt)"
except that it prompts for the number to list and for the account to
list the transactions from.
.IP 
.CW "(ltbd)"
.QP
list transactions in current account by day, prompting for the first
and last day to show.
.IP 
.CW "(bt)"
.QP
List total balances in Expense, Income, Asset, and Liability accounts and total Worth (Asset+Liability) versus Opening Balances for all accounts.
.IP 
.CW "(re)"
.QP
Search for transactions in the current account with a description that matches a regular expression (prompted for).
.NH 2
Utility functions
.IP  
.CW "(sa)"
.QP
Set current account.
.IP 
.CW "(ca)"
.QP
Print current account.
.IP 
.CW "(w)"
.QP
Write account to file (prompted for name).
.IP 
.CW "(r)"
.QP
Read saved accounts from file.
.IP 
.CW "(p X)"
.QP
Print the Scheme object
.CW "X."
.IP 
.CW "(v)"
.QP
Print out the
.CW bal
version.
.IP 
.CW "(sd)"
.QP
Set current day.
.CW "(bt)"
and
.CW "(la)"
will now total transactions before and after this day.
.IP 
.CW "(cd)"
.QP
Print current day in YYYY-MM-DD format.
.IP 
.CW "(q)"
.QP
quit
.CW bal
.NH
Creating interactive functions
.LP
To create interactive functions, use the Scheme function
.LP
.DS C
.ft CW
(bal/call func options)
.DE
.LP
.ad l
.CW func
is a string giving the function name to call, and
.CW options
is a list of pairs containing (in its car) the prompt to give the user and the type of the argument to require (in its cdr).  The following types are recognized in
.CW bal.
.IP  
string \(em the option will be treated as a string in the function call
.IP 
account \(em the option will be to select an
.I existing
account (the name of which will be passed as a string to the function
call).
.IP 
current_account \(em the name of the current account will be passed as a string (the user will not see a prompt for this option).
.IP 
type \(em prompt to select an account type (Asset, Liability, Income, Expense).
.IP 
transaction \(em prompt to select an existing transaction, passed as a pair giving the account number and the transaction number.
.IP 
daystr \(em prompt to select a year, a month, and a day, passed as a string in YYYY-MM-DD format.
.IP 
day \(em prompt to select a year, a month, and a day, passed as a list with three integers in the following order: day, month, year.
.IP 
other \(em passed exactly as entered (the user can enter any Scheme expression and it will just be copied as an argument to the function).
.LP
.B Example.
The interactive function
.CW "(t)"
creates a transfer from one account to another account.  It is written in the following way,
.DS I
.ft CW
(define t
  (lambda ()
    (bal/call "bal/t"
              (list
               (cons "To Account" "account")
               (cons "From Account" "account")
               (cons "Amount" "real")
               (cons "Description" "string")
               (cons "Day" "daystr")))))
.DE
.CW "bal/t"
is also a Scheme function. It adds a negative transactions to the "from account" and a positive transaction to the "to account". Its source is,
.DS I
.ft CW
(define bal/t
  (lambda (to-account from-account amount desc day)
    (let ((to-type (list-ref (bal/get-account to-account) 1))
          (from-type (list-ref (bal/get-account from-account) 1)))
      (bal/at to-account amount desc day)
      (bal/at from-account (* -1 amount) desc day))))
.DE
.NH
Non-interactive functions
.LP
.IP  
.CW "(bal/at account amount desc day)"
.QP
adds a transaction to an account where
.CW account
is the name of the account,
.CW amount
is the amount of the transaction,
.CW desc
is a string describing the transaction, and
.CW day
gives the day of the transaction in YYYY-MM-DD format.
.IP 
.CW "(bal/aa name type ob)"
.QP
adds a new account with name
.I name
and
.I type
is one of ("expense", "income", "asset", "liability") and gives the type of the account and
.I ob
gives the opening balance for the account.
.IP 
.CW "(bal/et (cons account_number transaction_number))"
.QP
edits transaction located at
.CW account_number
and
.CW transaction_number
with interactive prompts for what to modify.  To programmatically modify transactions, delete and add the transaction with
.CW bal/dt
and
.CW bal/at.
.IP 
.CW "(bal/ea current_account_name new_name)"
.QP
rename account from
.CW current_account_name
to
.CW new_name.
.IP 
.CW "(bal/da account_name)"
.QP
delete account with name
.CW account_name.
.IP 
.CW "(bal/dt (cons account_number transaction_number))"
.QP
delete transaction.
.IP 
.CW "(bal/get-current-account)"
.QP
returns a string with the name of the current account.
.IP 
.CW "(bal/get-number-of-accounts)"
.QP
return the number of accounts.
.IP 
.CW "(bal/get-transactions account_name how_many)"
.QP
Return
.QP how_many
of the latest transactions from account with
.QP account_name.
.IP 
.CW "(bal/get-all-transactions account_name)"
.QP
Return all transactions from
.CW account_name.
Each transaction is a five element list with elements (description, amount, year, month, day).
.IP 
.CW "(bal/get-transactions-by-regex account_name regex)"
.QP
Return all transactions from
.CW account_name
with descriptions that match
.CW regex.
.IP 
.CW "(bal/get-account account_name)"
.QP
Returns the account with name
.CW account_name,
a four element list, (name,type_of_account,number_transactions,opening_balance).
.IP 
.CW "(bal/get-all-accounts)"
.QP
Returns a list of all the accounts where each account is a four element list, (name,type_of_account,number_transactions,opening_balance).
.IP 
.CW "(bal/get-transaction-by-location account_num transact_num)"
.QP
Returns the transaction at account number and transaction number, a five-element list (description, amount, year, month, day).
.IP 
.CW "(bal/get-account-by-location account_num)"
.QP
Return account corresponding to
.CW acocunt_num.
.IP 
.CW "(bal/get-transactions-by-day account_name first_day last_day)"
.QP
Return a list of transactions between the
.CW first_day
and
.CW last_day
within the account with name
.CW account_name.  Both days are in YYYY-MM-DD format.
.IP 
.CW "(bal/total-account account_name)"
.QP
Returns the sum of all transactions within the account with name,
.CW account_name.
.IP 
.CW "(bal/total-all-accounts)"
.br
 Returns a list of pairs where each pair has in its
.CW car
the name of the account and in its
.CW cdr
the sum of all transactions within that account.
.IP 
.CW "(bal/total-by-account-type)"
.QP
Returns a list of pairs which have in their
.CW car
the name of the account type (Income, Expense, Asset, Liability), "Worth" (Assets + Liabilities), and "Balances" (for total opening balances) and in its
.CW cdr
the total sum of transactions within each account type.
.IP 
.CW "(bal/set-account account_name)"
.QP
Sets the current account to
.CW account_name.
.IP 
.CW "(bal/write file)"
.QP
Writes all accounts to
.CW file.
.IP 
.CW "(bal/read file)"
.QP
Read in accounts from
.CW file.
.IP 
.CW "(bal/get-current-file)"
.QP
Returns the name of the current default save file.
.IP 
.CW "(bal/set-select-transact-number num)"
.QP
Sets number of transactions to show when selecting a transaction to
.CW num.
On any transaction selection screen you can enter any transaction number whether it is displayed.
.IP 
.CW "(bal/v)"
.QP
Returns a string giving the version of
.CW bal.
.IP 
.CW "(bal/t to_account from_account amount desc day)"
.QP
Transfers from
.CW from_account
to
.CW to_account
a transaction in
.CW amount
with description
.CW desc
on day (in YYYY-MM-DD format)
.CW day.
.IP
.CW "(bal/set-current-day (list day month year))"
.QP
Sets the current day to the year, month, and day provided as arguments.  The function returns the list passed to it, unaltered.
.IP
.CW "(bal/get-current-day)"
.QP
Returns a three element list,
.CW "(list day month year),"
giving the current day, month, and year.
.NH
FreeBSD Documentation License
.LP
Copyright 1994-2018 The FreeBSD Project. All rights reserved.

Redistribution and use in source (Groff) and 'compiled' forms (SGML, HTML, PDF, PostScript, RTF and so forth) with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code (Groff) must retain the above copyright notice, this list of conditions and the following disclaimer as the first lines of this file unmodified.

Redistributions in compiled form (transformed to other DTDs, converted to PDF, PostScript, RTF and other formats) must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS DOCUMENTATION IS PROVIDED BY ZACH FLYNN "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ZACH FLYNN BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS DOCUMENTATION, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

