AC_INIT([systems-utils], [0.1], [jh1@fastmail.com])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_PROG_CC_C99

AX_PTHREAD(, [AC_MSG_ERROR([libpthread required])])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 bench/Makefile
])

AC_OUTPUT
