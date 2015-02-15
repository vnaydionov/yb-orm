
AC_INIT([auth-example], [0.1])
AC_CONFIG_SRCDIR([src/auth.cpp])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE

AC_LANG_CPLUSPLUS
AC_PROG_CXX
AM_PROG_LIBTOOL

YB_QT([
    CPPFLAGS="$CPPFLAGS -DYB_USE_QT"
],[
    YB_WX([
        CPPFLAGS="$CPPFLAGS -DYB_USE_WX"
    ],[
        AM_PATH_XML2
        CPPFLAGS="$CPPFLAGS $XML_CPPFLAGS"
        BOOST_REQUIRE([1.31.0],
            AC_MSG_ERROR([Package requires the Boost C++ libraries!]))
        BOOST_THREADS([mt])
        BOOST_DATE_TIME([mt])
        CPPFLAGS="$CPPFLAGS -DYB_USE_TUPLE"
    ])
])
YB_ODBC([
    CPPFLAGS="$CPPFLAGS -DYB_USE_ODBC"
], [])
YB_SQLITE3([
    CPPFLAGS="$CPPFLAGS -DYB_USE_SQLITE3"
], [])
YB_SOCI([
    CPPFLAGS="$CPPFLAGS -DYB_USE_SOCI"
], [])
AM_CONDITIONAL([QT_PRESENT], [test x$have_qt = xyes])
AM_CONDITIONAL([ODBC_PRESENT], [test x$have_odbc = xyes])
AM_CONDITIONAL([SQLITE3_PRESENT], [test x$have_sqlite3 = xyes])
AM_CONDITIONAL([SOCI_PRESENT], [test x$have_soci = xyes])
YB_CHECK_UNICODE([], [])
YB_EXECINFO([], [])

YB_CHECK_YBORM([],
    AC_MSG_ERROR([YB.ORM library is required!]))

YB_TEST_DB

AC_OUTPUT(Makefile src/Makefile src/auth.sh src/domain/Makefile)

# vim:ts=4:sts=4:sw=4:et:
