AC_INIT([libpdsns], [0.0.1], [david@kubasek.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC

LT_INIT
AC_ENABLE_SHARED
AC_DISABLE_STATIC

AC_CONFIG_HEADERS([config.h])

#
# libraries
#
AC_CHECK_LIB([pth], [pth_init], [], [
	echo "ERROR: libpth (GNU portable threads) not found"
	exit 1
])

AC_CHECK_LIB([xml2], [xmlReadFile], [], [
	echo "ERROR: libconfig not found"
	exit 1
])

AC_CHECK_LIB([glib-2.0], [g_hash_table_new], [], [
	echo "ERROR: glib not found"
	exit 1
])
#
# compile time substitutions
#
AC_SUBST(LIB_IDENTIFIER, pdsns)

#
# environment specifics
# may be useless
#
AC_CHECK_SIZEOF(time_t)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(char)

AC_CONFIG_FILES([
	Makefile
	doc/Makefile
	src/Makefile
	prez/Makefile
])
AC_OUTPUT
