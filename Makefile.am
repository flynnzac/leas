bin_PROGRAMS = leas
leas_SOURCES = src/leas.c
leasdir = $(datadir)
leas_DATA = src/leas.scm
leas_CFLAGS = $(DEPENDENCIES_CFLAGS) -DLEAS_SCM_INSTALL='"$(datadir)/leas.scm"'
leas_LDFLAGS = $(DEPENDENCIES_LIBS) 
man1_MANS = docs-src/leas.1

check_PROGRAMS=test_util test_convert test_transaction_times
check_PROGRAMS += test_check_account_trans
check_PROGRAMS += test_find_account
check_PROGRAMS += test_total_transactions

test_util_SOURCES=test/test_util.c
test_util_CFLAGS = $(DEPENDENCIES_CFLAGS) -DLEAS_SCM_INSTALL='"$(datadir)/leas.scm"'
test_util_LDFLAGS = $(DEPENDENCIES_LIBS) 

test_convert_SOURCES=test/test_convert.c
test_convert_CFLAGS = $(DEPENDENCIES_CFLAGS) -DLEAS_SCM_INSTALL='"$(datadir)/leas.scm"'
test_convert_LDFLAGS = $(DEPENDENCIES_LIBS) 

test_transaction_times_SOURCES=test/test_transaction_times.c
test_transaction_times_CFLAGS = $(DEPENDENCIES_CFLAGS) -DLEAS_SCM_INSTALL='"$(datadir)/leas.scm"'
test_transaction_times_LDFLAGS = $(DEPENDENCIES_LIBS) 

test_check_account_trans_SOURCES=test/test_check_account_trans.c
test_check_account_trans_CFLAGS = $(DEPENDENCIES_CFLAGS) -DLEAS_SCM_INSTALL='"$(datadir)/leas.scm"'
test_check_account_trans_LDFLAGS = $(DEPENDENCIES_LIBS) 

test_find_account_SOURCES=test/test_find_account.c
test_find_account_CFLAGS = $(DEPENDENCIES_CFLAGS) -DLEAS_SCM_INSTALL='"$(datadir)/leas.scm"'
test_find_account_LDFLAGS = $(DEPENDENCIES_LIBS) 

test_total_transactions_SOURCES=test/test_total_transactions.c
test_total_transactions_CFLAGS = $(DEPENDENCIES_CFLAGS) -DLEAS_SCM_INSTALL='"$(datadir)/leas.scm"'
test_total_transactions_LDFLAGS = $(DEPENDENCIES_LIBS) 


TESTS=test_util test_convert test_transaction_times
TESTS += test_check_account_trans
TESTS += test_find_account
TESTS += test_total_transactions
