AC_INIT(configure.ac)

AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE(yborm, 0.1)

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
YB_PATH_CPPUNIT([1.10.0],[],
                [echo "WARNING!! You will not be able to run tests!!"])
AM_CONDITIONAL([CPPUNIT_PRESENT], [test x$no_cppunit != xyes])
YB_CHECK_DBPOOL3()
AM_CONDITIONAL([DBPOOL3_PRESENT], [test x$ac_dbpool_present = xyes])

YB_TEST_DB

AC_OUTPUT(Makefile
    lib/Makefile
    lib/test/Makefile
    lib/util/Makefile
    lib/util/test/Makefile
    lib/orm/Makefile
    lib/orm/test/Makefile
    src/Makefile
    examples/Makefile
    examples/example1.sh
    examples/example2.sh
    examples/domain/Makefile)

# vim:ts=4:sts=4:sw=4:et:
