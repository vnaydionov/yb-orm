
AC_INIT([yb-orm], [0.4.6])
AC_CONFIG_SRCDIR([src/orm/data_object.cpp])
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
AM_CONDITIONAL([NO_STD_STRING], [test x$have_unicode = xyes ||
                test x$have_qt = xyes || test x$have_wx = xyes])
YB_EXECINFO([], [])

YB_PATH_CPPUNIT([1.10.0],[],
                [echo "WARNING!! You will not be able to run tests!!"])
AM_CONDITIONAL([CPPUNIT_PRESENT], [test x$no_cppunit != xyes])

YB_TEST_DB
AM_CONDITIONAL([DIALECT_SET], [test x$YBORM_DBTYPE != x])

AC_OUTPUT(Makefile
    include/Makefile
    include/yb/Makefile
    include/yb/util/Makefile
    include/yb/orm/Makefile
    src/Makefile
    src/util/Makefile
    src/orm/Makefile
    src/yborm_gen/Makefile
    tests/Makefile
    tests/test_main/Makefile
    tests/util/Makefile
    tests/orm/Makefile
    tests/orm/unit_tests.sh
    examples/Makefile
    examples/example1.sh
    examples/example2.sh
    examples/domain/Makefile
    examples/tutorial/Makefile
    examples/tutorial/domain/Makefile
    )

# vim:ts=4:sts=4:sw=4:et:
