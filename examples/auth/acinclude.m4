dnl YB_BOOST([MINIMUM-VERSION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Test for the Boost C++ libraries of a particular version (or newer)
dnl Defines:
dnl   BOOST_CPPFLAGS to the set of flags required to compiled Boost
AC_DEFUN([YB_BOOST], 
[
    AC_SUBST(BOOST_CPPFLAGS)
    AC_SUBST(BOOST_LDFLAGS)
    AC_SUBST(BOOST_LIBS)
    AC_SUBST(BOOST_LIBS_R)
    BOOST_CPPFLAGS=""
    BOOST_LDFLAGS=""
    BOOST_LIBS=""
    BOOST_LIBS_R=""
    # Use MT version of boost for tests
    use_boost_mt="no"

    AC_ARG_WITH([boost-includes],
        AC_HELP_STRING([--with-boost-includes=DIR],
            [Directory where the Boost C++ includes reside]),
        [ac_boost_includes="$withval"])
    AC_ARG_WITH([boost-libs],
        AC_HELP_STRING([--with-boost-libs=DIR],
            [Directory where the Boost C++ libraries reside]),
        [ac_boost_libs="$withval"])
    AC_ARG_ENABLE([boost-debug],
        AC_HELP_STRING([--enable-boost-debug],
            [Link against debug version of Boost libraries]),
        [ac_boost_debug="$enableval"])

    boost_min_version=ifelse([$1], ,1.20.0,[$1])
    WANT_BOOST_MAJOR=`expr $boost_min_version : '\([[0-9]]\+\)'`
    WANT_BOOST_MINOR=`expr $boost_min_version : '[[0-9]]\+\.\([[0-9]]\+\)'`
    WANT_BOOST_SUB_MINOR=`expr $boost_min_version : '[[0-9]]\+\.[[0-9]]\+\.\([[0-9]]\+\)'`

    AC_MSG_CHECKING([for the Boost C++ libraries, version $boost_min_version or newer])

    # Save and modify CXXFLAGS
    BOOST_CPPFLAGS="-ftemplate-depth-100"
    if test "x$ac_boost_includes" != "x" ; then
        BOOST_CPPFLAGS="$BOOST_CPPFLAGS -I$ac_boost_includes"
    fi
    if test "x$ac_boost_libs" != "x" ; then
        BOOST_LDFLAGS="-L$ac_boost_libs"
    fi
    ac_save_cxxflags="$CXXFLAGS"
    CXXFLAGS="$ac_save_cxxflags $BOOST_CPPFLAGS"

    AC_LANG_PUSH(C++)
    AC_TRY_LINK([
#include <string>
#include <boost/lexical_cast.hpp>
],
        [ std::string s = boost::lexical_cast<std::string>(123); int x = boost::lexical_cast<int>(s); ],
        [have_boost="yes"],
        [
            AC_MSG_RESULT(no)
            have_boost="no"
            ifelse([$3], , :, [$3])
        ])

    if test "$have_boost" = "yes"; then
        WANT_BOOST_VERSION=`expr $WANT_BOOST_MAJOR \* 100000 \+ $WANT_BOOST_MINOR \* 100 \+ $WANT_BOOST_SUB_MINOR`
        AC_TRY_COMPILE([
#include <boost/version.hpp>
],
            [
#if BOOST_VERSION >= $WANT_BOOST_VERSION
// Everything is okay
#else
#  error Boost version is too old
#endif
],
            [
                AC_MSG_RESULT(yes)
                if test "$build_os" = "mingw32"; then
                    boost_libsuff=mgw`gcc -dumpversion | sed 's/\(@<:@0-9@:>@\+\)\.\(@<:@0-9@:>@\+\)\..*/\1\2/'` 
                else
                    boost_libsuff=`if gcc -v 2>&1 | grep -q 'Ubuntu 4\.2\.' ; then echo gcc42; else echo gcc; fi`
                fi
                boost_libsuff_r=$boost_libsuff-mt;
                if test "x$enable_debug" = xyes || \
                        test "x$ac_boost_debug" = xyes ; then
                    boost_libsuff=$boost_libsuff-d;
                    boost_libsuff_r=$boost_libsuff_r-d;
                fi

                AC_RUN_IFELSE(
                        [AC_LANG_PROGRAM([dnl
#include <boost/version.hpp>
#include <fstream>
], [dnl
    std::ofstream o("conftest.out");
    o << BOOST_VERSION / 100000 << "_" << (BOOST_VERSION / 100) % 1000;
    if (BOOST_VERSION % 100)
        o << "_" << BOOST_VERSION % 100;
    o.close();
    return 0;
])],
                        [
                        boost_libsuff=$boost_libsuff-`cat conftest.out`
                        boost_libsuff_r=$boost_libsuff_r-`cat conftest.out`
                        ],
                        [AC_MSG_FAILURE([test run of Boost program failed])]
                        )
                ifelse([$2], , :, [$2])
            ],
            [
                AC_MSG_RESULT([no, version of installed Boost libraries is too old])
                ifelse([$3], , :, [$3])
            ])
    fi

    AC_LANG_POP(C++)

    # Restore CXXFLAGS
    CXXFLAGS="$ac_save_cxxflags"
])

AC_DEFUN([YB_TRY_LINK_BOOST_THREAD], [
    AC_LANG_PUSH(C++)
    AC_TRY_LINK([ 
/*
        #include <boost/thread.hpp> 
        bool bRet = 0;
        void thdfunc() { bRet = 1; }
*/
        #include <boost/thread/mutex.hpp>
    ], [
/*
        boost::thread thrd(&thdfunc);
        thrd.join();
        return bRet == 1;
*/
        boost::mutex m;
        {
            boost::mutex::scoped_lock lock(m);
        }
        return 0;
    ], [
        ifelse([$1], , :, [$1])
        use_boost_mt="yes"
    ], [
        ifelse([$2], , :, [$2])
    ])
    AC_LANG_POP(C++)
])

AC_DEFUN([YB_TRY_LINK_BOOST_DATETIME], [
    AC_LANG_PUSH(C++)
    AC_TRY_LINK([ 
        #include <boost/date_time/gregorian/gregorian.hpp> 
    ], [
        using namespace boost::gregorian;
        date d = from_string("1978-01-27");
        return d == boost::gregorian::date(1978, Jan, 27);
    ], [
        ifelse([$1], , :, [$1])
    ], [ 
        ifelse([$2], , :, [$2])
    ])
    AC_LANG_POP(C++)
])

dnl YB_BOOST_THREAD([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Test for the Boost thread library
dnl Call this BEFORE calling YB_BOOST_DATETIME or YB_BOOST_REGEX,
dnl if you're going to link with thread library and with regex, date_time.
dnl Defines
dnl   BOOST_LDFLAGS to the set of flags required to compile boost_thread
AC_DEFUN([YB_BOOST_THREAD], 
[
    AC_REQUIRE([YB_BOOST])

    AC_MSG_CHECKING([whether we can use boost_thread library])

    if test "$build_os" != "mingw32" ; then
        BOOST_CPPFLAGS="$BOOST_CPPFLAGS -D_REENTRANT -pthread"
    fi
    ac_save_cxxflags="$CXXFLAGS"
    ac_save_ldflags="$LDFLAGS"
    ac_save_libs="$LIBS"
    CXXFLAGS="$ac_save_cxxflags $BOOST_CPPFLAGS"
    LDFLAGS="$ac_save_ldflags $BOOST_LDFLAGS"

    LIBS="$ac_save_libs -lboost_thread-$boost_libsuff_r"
    YB_TRY_LINK_BOOST_THREAD([
        BOOST_LIBS_R="$BOOST_LIBS_R -lboost_thread-$boost_libsuff_r"
        AC_MSG_RESULT([yes])
        ifelse([$1], , :, [$1])
    ], [
        LIBS="$ac_save_libs -lboost_thread-mt"
        YB_TRY_LINK_BOOST_THREAD([
            BOOST_LIBS_R="$BOOST_LIBS_R -lboost_thread-mt"
            AC_MSG_RESULT([yes])
            ifelse([$1], , :, [$1])
        ], [
            LIBS="$ac_save_libs -lboost_thread"
            YB_TRY_LINK_BOOST_THREAD([
                BOOST_LIBS_R="$BOOST_LIBS_R -lboost_thread"
                AC_MSG_RESULT([yes])
                ifelse([$1], , :, [$1])
            ], [
                AC_MSG_RESULT([no])
                ifelse([$2], , :, [$2])
            ])
        ])
    ])

    CXXFLAGS="$ac_save_cxxflags"
    LDFLAGS="$ac_save_ldflags"
    LIBS="$ac_save_libs"
])

dnl YB_BOOST_DATETIME([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Test for the Boost datetime library
dnl Defines
dnl   BOOST_LIBS or BOOST_LIBS_R to the set of flags required to link against boost_datetime
AC_DEFUN([YB_BOOST_DATETIME], 
[
    AC_REQUIRE([YB_BOOST])

    AC_MSG_CHECKING([whether we can use boost_datetime library])

    ac_save_cxxflags="$CXXFLAGS"
    ac_save_ldflags="$LDFLAGS"
    ac_save_libs="$LIBS"
    CXXFLAGS="$ac_save_cxxflags $BOOST_CPPFLAGS"
    LDFLAGS="$ac_save_ldflags $BOOST_LDFLAGS"

    if test "x$use_boost_mt" = "xno" ; then
        LIBS="$ac_save_libs -lboost_date_time-$boost_libsuff"
    else
        LIBS="$ac_save_libs -lboost_date_time-$boost_libsuff_r"
    fi
    YB_TRY_LINK_BOOST_DATETIME([
        BOOST_LIBS="$BOOST_LIBS -lboost_date_time-$boost_libsuff"
        BOOST_LIBS_R="$BOOST_LIBS_R -lboost_date_time-$boost_libsuff_r"
        AC_MSG_RESULT([yes])
        ifelse([$1], , :, [$1])
    ], [
        if test "x$use_boost_mt" = "xno" ; then
            LIBS="$ac_save_libs -lboost_date_time"
        else
            LIBS="$ac_save_libs -lboost_date_time-mt"
        fi
        YB_TRY_LINK_BOOST_DATETIME([
            BOOST_LIBS="$BOOST_LIBS -lboost_date_time"
            BOOST_LIBS_R="$BOOST_LIBS_R -lboost_date_time-mt"
            AC_MSG_RESULT([yes])
            ifelse([$1], , :, [$1])
        ], [
            AC_MSG_RESULT([no])
            ifelse([$2], , :, [$2])
        ])
    ])

    CXXFLAGS="$ac_save_cxxflags"
    LDFLAGS="$ac_save_ldflags"
    LIBS="$ac_save_libs"
])

dnl YB_ODBC([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Test for the ODBC library
dnl Defines
dnl   ODBC_LIBS to the set of flags required to link against ODBC
AC_DEFUN([YB_ODBC],
[
    AC_SUBST(ODBC_LIBS)
    AC_MSG_CHECKING([whether we can use ODBC library])

    if test "$build_os" = "mingw32"; then
        ODBC_LIBS="-lodbc32"
    else
        ODBC_LIBS="-lodbc"
    fi
    ac_save_libs="$LIBS"
    LIBS="$ac_save_libs $ODBC_LIBS"

    AC_LANG_PUSH(C)
    AC_TRY_LINK(
        [
#if defined(__WIN32__) || defined(_WIN32)
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
        ],
        [
        HENV env;
        SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
        return 1;
        ], 
        [
        AC_MSG_RESULT([yes])
        ifelse([$1], , :, [$1])
        ],
        [ 
        AC_MSG_RESULT([no])
        ifelse([$2], , :, [$2])
        ])

    AC_LANG_POP(C)
    LIBS="$ac_save_libs"
])

dnl
dnl YB_PATH_CPPUNIT(MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
AC_DEFUN([YB_PATH_CPPUNIT],
[
    AC_ARG_WITH([cppunit-prefix],
        AC_HELP_STRING([--with-cppunit-prefix=PFX],
            [Prefix where CppUnit is installed (optional)]),
        [cppunit_config_prefix="$withval"], [cppunit_config_prefix=""])
    AC_ARG_WITH([cppunit-exec-prefix],
        AC_HELP_STRING([--with-cppunit-exec-prefix=PFX],
            [Exec prefix where CppUnit is installed (optional)]),
        [cppunit_config_exec_prefix="$withval"], [cppunit_config_exec_prefix=""])

    if test x$cppunit_config_exec_prefix != x ; then
        cppunit_config_args="$cppunit_config_args --exec-prefix=$cppunit_config_exec_prefix"
        if test x${CPPUNIT_CONFIG+set} != xset ; then
            CPPUNIT_CONFIG=$cppunit_config_exec_prefix/bin/cppunit-config
        fi
    fi
    if test x$cppunit_config_prefix != x ; then
        cppunit_config_args="$cppunit_config_args --prefix=$cppunit_config_prefix"
        if test x${CPPUNIT_CONFIG+set} != xset ; then
            CPPUNIT_CONFIG=$cppunit_config_prefix/bin/cppunit-config
        fi
    fi

    AC_PATH_PROG(CPPUNIT_CONFIG, cppunit-config, no)
    cppunit_version_min=$1

    AC_MSG_CHECKING(for Cppunit - version >= $cppunit_version_min)
    no_cppunit=""
    if test "$CPPUNIT_CONFIG" = "no" ; then
        no_cppunit=yes
    else
        CPPUNIT_CFLAGS=`$CPPUNIT_CONFIG --cflags`
        CPPUNIT_LIBS=`$CPPUNIT_CONFIG --libs`
        cppunit_version=`$CPPUNIT_CONFIG --version`

        cppunit_major_version=`echo $cppunit_version | \
            sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
        cppunit_minor_version=`echo $cppunit_version | \
            sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
        cppunit_micro_version=`echo $cppunit_version | \
            sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

        cppunit_major_min=`echo $cppunit_version_min | \
            sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
        cppunit_minor_min=`echo $cppunit_version_min | \
            sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
        cppunit_micro_min=`echo $cppunit_version_min | \
            sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

        cppunit_version_proper=`expr \
            $cppunit_major_version \> $cppunit_major_min \| \
            $cppunit_major_version \= $cppunit_major_min \& \
            $cppunit_minor_version \> $cppunit_minor_min \| \
            $cppunit_major_version \= $cppunit_major_min \& \
            $cppunit_minor_version \= $cppunit_minor_min \& \
            $cppunit_micro_version \>= $cppunit_micro_min `

        if test "$cppunit_version_proper" = "1" ; then
            # Save CXXFLAGS, LIBS
            ac_save_cxxflags="$CXXFLAGS"
            ac_save_libs="$LIBS"

            # Modify CXXFLAGS, LIBS
            CXXFLAGS="$ac_save_cxxflags $CPPUNIT_CFLAGS"
            LIBS="$ac_save_libs $CPPUNIT_LIBS"

            AC_LANG_PUSH(C++)
            AC_TRY_LINK([
#include <cppunit/ui/text/TestRunner.h>
],
                [CppUnit::TextUi::TestRunner runner;],
                [link_ok=yes],[link_ok=no])
            AC_LANG_POP(C++)

            # Restore CXXFLAGS, LIBS
            CXXFLAGS="$ac_save_cxxflags"
            LIBS="$ac_save_libs"

            if test "x$link_ok" != "xyes" ; then
                AC_MSG_RESULT(no)
                no_cppunit=yes
            else
                AC_MSG_RESULT([$cppunit_major_version.$cppunit_minor_version.$cppunit_micro_version])
            fi
        else
            AC_MSG_RESULT(no)
            no_cppunit=yes
        fi
    fi

    if test "x$no_cppunit" = x ; then
        ifelse([$2], , :, [$2])     
    else
        CPPUNIT_CFLAGS=""
        CPPUNIT_LIBS=""
        ifelse([$3], , :, [$3])
    fi

    AC_SUBST(CPPUNIT_CFLAGS)
    AC_SUBST(CPPUNIT_LIBS)
])

dnl
dnl YB_CHECK_DBPOOL3([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Check for legacy Yandex.DBpool3
AC_DEFUN([YB_CHECK_DBPOOL3],
[
    AC_SUBST(DBPOOL3_CXXFLAGS)
    AC_SUBST(DBPOOL3_LDFLAGS)
    AC_SUBST(DBPOOL3_LIBS)
    DBPOOL3_CXXFLAGS=""
    DBPOOL3_LDFLAGS=""
    DBPOOL3_LIBS=""

    AC_ARG_WITH([dbpool-includes],
        AC_HELP_STRING([--with-dbpool-includes=DIR],
            [Place where Yandex.DBpool3 includes are]),
        [ac_dbpool_includes="$withval"],[ac_dbpool_includes=""])
    AC_ARG_WITH([dbpool-libs],
        AC_HELP_STRING([--with-dbpool-libs=DIR],
            [Place where Yandex.DBpool3 library is]),
        [ac_dbpool_libs="$withval"],[ac_dbpool_libs=""])
    AC_ARG_ENABLE([static-dbpool],
        AC_HELP_STRING([--enable-static-dbpool],
            [Link against Yandex.DBpool3 statically]),
        [ac_use_static_dbpool="$enableval"])

    AC_MSG_CHECKING([for Yandex.DBpool3])

    # Save CXXFLAGS, LDFLAGS, LIBS
    ac_save_cxxflags="$CXXFLAGS"
    ac_save_ldflags="$LDFLAGS"
    ac_save_libs="$LIBS"

    # Modify CXXFLAGS, LDFLAGS, LIBS
    if test "x$ac_dbpool_includes" != "x" ; then
        DBPOOL3_CXXFLAGS="-I$ac_dbpool_includes"
    fi
    if test "x$ac_dbpool_libs" != "x" ; then
        DBPOOL3_LDFLAGS="-L$ac_dbpool_libs"
    fi
    if test "x$ac_use_static_dbpool" = "x" ; then
        DBPOOL3_LIBS="-lmypp3"
    else
        DBPOOL3_LDFLAGS=""
        MYPP_DEPS="-lz -ldl -lyandex-common-logger"
        MYPP_LIBA="libmypp3.a"
        if test "x$ac_dbpool_libs" = "x" ; then
            DBPOOL3_LIBS="$MYPP_LIBA $MYPP_DEPS"
        else
            DBPOOL3_LIBS="$ac_dbpool_libs/$MYPP_LIBA $MYPP_DEPS"
        fi
    fi
    CXXFLAGS="$ac_save_cxxflags $DBPOOL3_CXXFLAGS"
    LDFLAGS="$ac_save_ldflags $DBPOOL3_LDFLAGS"
    LIBS="$ac_save_libs $DBPOOL3_LIBS"

    AC_LANG_PUSH(C++)
    AC_TRY_LINK([
#include <dbpool/row.h>
#include <dbpool/statement.h>
#include <string>
],
        [mypp::Row r; r["field1"].asLong(); mypp::Statement st(NULL, "xyz");],
        [ac_dbpool_present=yes],[ac_dbpool_present=no])
    AC_LANG_POP(C++)

    if test "$ac_dbpool_present" = "yes" ; then
        AC_MSG_RESULT([yes])
        ifelse([$1], , :, [$1])
    else
        DBPOOL3_CXXFLAGS=""
        DBPOOL3_LDFLAGS=""
        DBPOOL3_LIBS=""
        AC_MSG_RESULT([no])
        ifelse([$2], , :, [$2])
    fi
    # Restore CXXFLAGS, LDFLAGS, LIBS
    CXXFLAGS="$ac_save_cxxflags"
    LDFLAGS="$ac_save_ldflags"
    LIBS="$ac_save_libs"
])

AC_DEFUN([YB_TEST_DB], 
[
    AC_SUBST(YBORM_DBTYPE)
    AC_SUBST(YBORM_DB)
    AC_SUBST(YBORM_USER)
    AC_SUBST(YBORM_PASSWD)
    YBORM_DBTYPE=""
    YBORM_DB=""
    YBORM_USER=""
    YBORM_PASSWD=""
    AC_ARG_WITH([test-dbtype],
        AC_HELP_STRING([--with-test-dbtype=MYSQL|INTERBASE|ORACLE],
            [Specify SQL dialect of the test database]),
        [YBORM_DBTYPE="$withval"],[YBORM_DBTYPE=""])
    AC_ARG_WITH([test-db],
        AC_HELP_STRING([--with-test-db=DSN],
            [Specify ODBC DSN entry for the test database]),
        [YBORM_DB="$withval"],[YBORM_DB=""])
    AC_ARG_WITH([test-user],
        AC_HELP_STRING([--with-test-user=USER],
            [Specify database user name to connect to the test database]),
        [YBORM_USER="$withval"],[YBORM_USER=""])
    AC_ARG_WITH([test-passwd],
        AC_HELP_STRING([--with-test-passwd=PASSWD],
            [Specify database password to connect to the test database]),
        [YBORM_PASSWD="$withval"],[YBORM_PASSWD=""])
])

dnl
dnl YB_CHECK_YBORM([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Check for YB.ORM library
AC_DEFUN([YB_CHECK_YBORM],
[
    AC_SUBST(YBORM_CXXFLAGS)
    AC_SUBST(YBORM_LDFLAGS)
    AC_SUBST(YBORM_LIBS)
    AC_SUBST(YBORM_GEN_DOMAIN)
    AC_SUBST(YBORM_GEN_SQL)
    YBORM_CXXFLAGS=""
    YBORM_LDFLAGS=""
    YBORM_LIBS=""
    YBORM_GEN_DOMAIN=""
    YBORM_GEN_SQL=""

    AC_ARG_WITH([yborm-includes],
        AC_HELP_STRING([--with-yborm-includes=DIR],
            [Place where YB.ORM includes are]),
        [ac_yborm_includes="$withval"],[ac_yborm_includes=""])
    AC_ARG_WITH([yborm-libs],
        AC_HELP_STRING([--with-yborm-libs=DIR],
            [Place where YB.ORM libraries are]),
        [ac_yborm_libs="$withval"],[ac_yborm_libs=""])
    AC_ARG_WITH([yborm-bin],
        AC_HELP_STRING([--with-yborm-bin=DIR],
            [Place where YB.ORM executables are]),
        [ac_yborm_bin="$withval"],[ac_yborm_bin=""])

    AC_MSG_CHECKING([for YB.ORM])

    # Save CXXFLAGS, LDFLAGS, LIBS
    ac_save_cxxflags="$CXXFLAGS"
    ac_save_ldflags="$LDFLAGS"
    ac_save_libs="$LIBS"

    # Modify CXXFLAGS, LDFLAGS, LIBS
    if test "x$ac_yborm_includes" != "x" ; then
        YBORM_CXXFLAGS="-I$ac_yborm_includes"
    fi
    if test "x$ac_yborm_libs" != "x" ; then
        YBORM_LDFLAGS="-L$ac_yborm_libs"
    fi
    YBORM_LIBS="-lybutil -lyborm"
    CXXFLAGS="$ac_save_cxxflags $YBORM_CXXFLAGS"
    LDFLAGS="$ac_save_ldflags $YBORM_LDFLAGS"
    LIBS="$ac_save_libs $YBORM_LIBS"

    AC_LANG_PUSH(C++)
    AC_TRY_LINK([
#include <util/decimal.h>
],
        [decimal x(10); x.str(); ],
        [ac_yborm_present=yes],[ac_yborm_present=no])
    AC_LANG_POP(C++)

    if test "$ac_yborm_present" = "yes" ; then
        if test "x$ac_yborm_bin" = "x" ; then
            YBORM_GEN_DOMAIN="yborm_gen_domain"
            YBORM_GEN_SQL="yborm_gen_sql"
            if ! which "$YBORM_GEN_DOMAIN" > /dev/null ; then
                ac_yborm_present=no
            fi
        else
            YBORM_GEN_DOMAIN="$ac_yborm_bin/yborm_gen_domain"
            YBORM_GEN_SQL="$ac_yborm_bin/yborm_gen_sql"
            if ! test -x "$YBORM_GEN_DOMAIN" ; then
                ac_yborm_present=no
            fi
        fi
    fi

    if test "$ac_yborm_present" = "yes" ; then
        AC_MSG_RESULT([yes])
        ifelse([$1], , :, [$1])
    else
        YBORM_CXXFLAGS=""
        YBORM_LDFLAGS=""
        YBORM_LIBS=""
        YBORM_GEN_DOMAIN=""
        YBORM_GEN_SQL=""
        AC_MSG_RESULT([no])
        ifelse([$2], , :, [$2])
    fi
    # Restore CXXFLAGS, LDFLAGS, LIBS
    CXXFLAGS="$ac_save_cxxflags"
    LDFLAGS="$ac_save_ldflags"
    LIBS="$ac_save_libs"
])

dnl vim:ts=4:sts=4:sw=4:et:
