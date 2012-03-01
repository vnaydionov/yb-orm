AC_INIT(src/auth.cpp)

AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE(auth, 0.1)

AC_LANG_CPLUSPLUS
AC_PROG_CXX
AM_PROG_LIBTOOL

AM_PATH_XML2
CPPFLAGS="$CPPFLAGS $XML_CPPFLAGS"

# required dependencies
YB_BOOST([1.31.0],[],
    AC_MSG_ERROR([Package requires the Boost C++ libraries!]))
YB_BOOST_THREAD([],
    AC_MSG_ERROR([Package requires the Boost threads library!]))
YB_BOOST_DATETIME([],
    AC_MSG_ERROR([Package requires the Boost date_time library!]))
YB_ODBC([],
    AC_MSG_ERROR([ODBC library is required!]))

# optional dependencies
YB_CHECK_DBPOOL3()
AM_CONDITIONAL([DBPOOL3_PRESENT], [test x$ac_dbpool_present = xyes])

YB_CHECK_YBORM([],
    AC_MSG_ERROR([YB.ORM library is required!]))

YB_TEST_DB

AC_OUTPUT(Makefile src/Makefile src/auth.sh src/domain/Makefile)

# vim:ts=4:sts=4:sw=4:et:
