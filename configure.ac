#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([Leas], [0.1.0], [github.com/flynnzac/leas])
AC_CONFIG_SRCDIR([src/leas.c])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([csv], [csv_init], [], [AC_MSG_ERROR([Can't find libcsv.])])
AC_CHECK_LIB([m], [floor], [], [AC_MSG_ERROR([Can't find -lm.])])
AC_CHECK_LIB([readline], [readline], [], [AC_MSG_ERROR([Can't find Readline library.])])
AC_CHECK_LIB([guile-2.2], [scm_with_guile], [], [AC_MSG_ERROR([Can't find Guile.])])

# Load Guile and GTK Flags
PKG_CHECK_MODULES([DEPENDENCIES], [guile-2.2])
AC_SUBST(DEPENDENCIES_CFLAGS)
AC_SUBST(DEPENDENCIES_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor regcomp strcspn])

AM_INIT_AUTOMAKE([subdir-objects])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
