.RP no
\R'PD 0.6v'
\R'PS 11000'
.ND
.TL
bal - an extensible tool for keeping accounts in order and studying past spending habits
.AU
Zach Flynn
.AB
Gives basic overview of the package and provides a full reference manual to the various Guile Scheme functions bal exposes.
This documentation can be freely modified and redistributed under the terms of the GNU Free Documentation License.
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
Basic usage
.LP
The command prompt is a Scheme interpreter except that the outer-most S expression should not be enclosed in parenthesis to save on typing.  The following
.NH
Creating interactive functions
.LP
To create interactive functions, use the Scheme function
.LP
.c(
.I (bal/call func options\).
.c)
.I func
is a string giving the function name to call, and
.I options
is a list of pairs containing (in its car) the prompt to give the user and the type of the argument to require (in its cdr).  The following types are recognized in
.B bal.
.IP \(bu
string - the option will be treated as a string in the function call
.IP \(bu
account - the option will be to select an
.I existing
account (the name of which will be passed as a string to the function call).
.IP \(bu
current_account - the name of the current account will be passed as a string (the user will not see a prompt for this option).
.IP \(bu
type - prompt to select an account type (Asset, Liability, Income, Expense).
.IP \(bu
currency - prompt to select a value of the currency (by default: 1) in an account, passed as a number to the function.  
.IP \(bu
transaction - prompt to select an existing transaction, passed as a pair giving the account number and the transaction number.
.IP \(bu
day - prompt to select a year, a month, and a day, passed as a string in YYYY-MM-DD format.
.IP \(bu
other - passed exactly as entered (the user can enter any Scheme expression and it will just be copied as an argument to the function).
.NH
All bal functions


