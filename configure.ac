AC_INIT(configure.ac)

AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE(yborm, 0.1)

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
        YB_BOOST([1.31.0],[],
            AC_MSG_ERROR([Package requires the Boost C++ libraries!]))
        YB_BOOST_THREAD([],
            AC_MSG_ERROR([Package requires the Boost threads library!]))
        YB_BOOST_DATETIME([],
            AC_MSG_ERROR([Package requires the Boost date_time library!]))
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
AM_CONDITIONAL([QT4_PRESENT], [test x$have_qt = xyes])
AM_CONDITIONAL([ODBC_PRESENT], [test x$have_odbc = xyes])
AM_CONDITIONAL([SQLITE3_PRESENT], [test x$have_sqlite3 = xyes])
AM_CONDITIONAL([SOCI_PRESENT], [test x$have_soci = xyes])
YB_CHECK_UNICODE([], [])
AM_CONDITIONAL([USING_WIDE_UNICODE], [test x$have_unicode = xyes])

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
    examples/Makefile
    examples/example1.sh
    examples/example2.sh
    examples/domain/Makefile
    examples/tutorial/Makefile
    examples/tutorial/domain/Makefile
    )

# vim:ts=4:sts=4:sw=4:et:
