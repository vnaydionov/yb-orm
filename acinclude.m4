AC_DEFUN([YB_WX],
[
    AC_SUBST(WX_CFLAGS)
    AC_SUBST(WX_LIBS)
    WX_CFLAGS=""
    WX_LIBS=""
    AC_ARG_WITH([wx-config],
        AC_HELP_STRING([--with-wx-config=BIN],
            [Configuration script for the wxWidgets C++ library]),
        [ac_wx_config="$withval"])
    if test "x$ac_wx_config" != "x" && test -x "$ac_wx_config" ; then
        AC_MSG_CHECKING([for the wxWidgets C++ library])
        WX_CFLAGS=`wx-config --cflags`
        WX_LIBS=`wx-config --libs base,xml`
        ac_save_cxxflags="$CXXFLAGS"
        ac_save_libs="$LIBS"
        CXXFLAGS="$ac_save_cxxflags $WX_CFLAGS"
        LIBS="$ac_save_libs $WX_LIBS"
        AC_LANG_PUSH(C++)
        AC_TRY_LINK([
#include <wx/mstream.h>
#include <wx/xml/xml.h>
],
        [ wxMemoryInputStream input("<aaa/>", 6); wxXmlDocument doc(input); ],
        [ have_wx="yes" ])
        AC_LANG_POP(C++)
        if test "x$have_wx" = "xyes" ; then
            AC_MSG_RESULT([yes])
            ifelse([$1], , :, [$1])
        else
            AC_MSG_RESULT([no])
            ifelse([$2], , :, [$2])
        fi
        CXXFLAGS="$ac_save_cxxflags"
        LIBS="$ac_save_libs"
    else
        ifelse([$2], , :, [$2])
    fi
])

AC_DEFUN([YB_QT],
[
    AC_SUBST(QT_CFLAGS)
    AC_SUBST(QT_LDFLAGS)
    AC_SUBST(QT_LIBS)
    QT_CFLAGS=""
    QT_LDFLAGS=""
    QT_LIBS=""
    AC_ARG_WITH([qt-includes],
        AC_HELP_STRING([--with-qt-includes=DIR],
            [Directory where the QT4 C++ includes reside]),
        [ac_qt_includes="$withval"])
    AC_ARG_WITH([qt-libs],
        AC_HELP_STRING([--with-qt-libs=DIR],
            [Directory where the QT4 C++ libraries reside]),
        [ac_qt_libs="$withval"])
    if test "x$ac_qt_includes" != "x" || test "x$ac_qt_libs" != "x" ; then
        AC_MSG_CHECKING([for the QT4 C++ libraries])
        if test "x$ac_qt_includes" != "x" ; then
            QT_CFLAGS="-I$ac_qt_includes -I$ac_qt_includes/QtCore -I$ac_qt_includes/QtXml -I$ac_qt_includes/QtSql"
        fi
        if test "x$ac_qt_libs" != "x" ; then
            QT_LDFLAGS="-L$ac_qt_libs"
        fi
        QT_LIBS="$QT_LIBS -lQtCore -lQtXml -lQtSql"
        ac_save_cxxflags="$CXXFLAGS"
        ac_save_ldflags="$LDFLAGS"
        ac_save_libs="$LIBS"
        CXXFLAGS="$ac_save_cxxflags $QT_CFLAGS"
        LDFLAGS="$ac_save_ldflags $QT_LDFLAGS"
        LIBS="$ac_save_libs $QT_LIBS"
        AC_LANG_PUSH(C++)
        AC_TRY_LINK([
#include <QtXml>
],
        [ QBuffer buf; buf.setData("<aaa/>", 6); QDomDocument doc("aaa"); ],
        [ have_qt="yes" ])
        AC_LANG_POP(C++)
        if test "x$have_qt" = "xyes" ; then
            AC_MSG_RESULT([yes])
            ifelse([$1], , :, [$1])
        else
            AC_MSG_RESULT([no])
            ifelse([$2], , :, [$2])
        fi
        CXXFLAGS="$ac_save_cxxflags"
        LDFLAGS="$ac_save_ldflags"
        LIBS="$ac_save_libs"
    else
        ifelse([$2], , :, [$2])
    fi
])

dnl YB_ODBC([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Test for the ODBC library
dnl Defines
dnl   ODBC_LIBS to the set of flags required to link against ODBC
AC_DEFUN([YB_ODBC],
[
    AC_SUBST(ODBC_CFLAGS)
    AC_SUBST(ODBC_LIBS)
    AC_MSG_CHECKING([whether we can use ODBC library])

    AC_ARG_WITH([odbc-includes],
        AC_HELP_STRING([--with-odbc-includes=DIR],
            [Directory where ODBC header files reside]),
        [ac_odbc_includes="$withval"])
    AC_ARG_WITH([odbc-lib],
        AC_HELP_STRING([--with-odbc-lib=LIB],
            [Library implementing ODBC interface]),
        [ac_odbc_lib="$withval"])
    
    if test "x$ac_odbc_includes" != "x"; then
        ODBC_CFLAGS="-I $ac_odbc_includes"
    else
        ODBC_CFLAGS=""
    fi
    if test "x$ac_odbc_lib" != "x"; then
        ODBC_LIBS="$ac_odbc_lib"
    else
        if test "$build_os" = "mingw32"; then
            ODBC_LIBS="-lodbc32"
        else
            ODBC_LIBS="-lodbc"
        fi
    fi

    ac_save_libs="$LIBS"
    ac_save_cflags="$CFLAGS"
    LIBS="$ac_save_libs $ODBC_LIBS"
    CFLAGS="$ac_save_cflags $ODBC_CFLAGS"

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
        have_odbc="yes"
        ifelse([$1], , :, [$1])
        ],
        [ 
        AC_MSG_RESULT([no])
        ODBC_CFLAGS=""
        ODBC_LIBS=""
        have_odbc="no"
        ifelse([$2], , :, [$2])
        ])

    AC_LANG_POP(C)
    LIBS="$ac_save_libs"
    CFLAGS="$ac_save_cflags"
])

dnl YB_EXECINFO([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Test for the execinfo interface: whether it is in libc or
dnl in a separate library.
dnl Defines
dnl   EXECINFO_LIBS to the set of flags required to link against execinfo
AC_DEFUN([YB_EXECINFO],
[
    AC_SUBST(EXECINFO_LIBS)
    AC_MSG_CHECKING([whether we can access execinfo])

    ac_save_libs="$LIBS"
    AC_LANG_PUSH(C)

    EXECINFO_LIBS=""
    LIBS="$ac_save_libs $EXECINFO_LIBS"
    AC_TRY_LINK(
        [
#include <execinfo.h>
        ],
        [
    void *addrlist[10];
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void *));
    char **symbollist = backtrace_symbols(addrlist, addrlen);
        ], 
        [
        AC_MSG_RESULT([yes])
        ifelse([$1], , :, [$1])
        ],
        [ 
        EXECINFO_LIBS="-lexecinfo"
        LIBS="$ac_save_libs $EXECINFO_LIBS"
        AC_TRY_LINK(
            [
    #include <execinfo.h>
            ],
            [
        void *addrlist[10];
        int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void *));
        char **symbollist = backtrace_symbols(addrlist, addrlen);
            ], 
            [
            AC_MSG_RESULT([yes, -lexecinfo])
            ifelse([$1], , :, [$1])
            ],
            [
            AC_MSG_RESULT([no])
            EXECINFO_LIBS=""
            ifelse([$2], , :, [$2])
            ])
        ])

    AC_LANG_POP(C)
    LIBS="$ac_save_libs"
])

dnl YB_SQLITE3([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Test for the SQLite3 library
dnl Defines
dnl   SQLITE3_CFLAGS to the set of flags required to include SQLite3 headers
dnl   SQLITE3_LIBS to the set of flags required to link against SQLite3
AC_DEFUN([YB_SQLITE3],
[
    AC_SUBST(SQLITE3_CFLAGS)
    AC_SUBST(SQLITE3_LIBS)
    AC_MSG_CHECKING([whether we can use SQLite3 library])

    SQLITE3_CFLAGS=`pkg-config sqlite3 --cflags`
    SQLITE3_LIBS=`pkg-config sqlite3 --libs`
    ac_save_cflags="$CFLAGS"
    ac_save_libs="$LIBS"
    CFLAGS="$ac_save_cflags $SQLITE3_CFLAGS"
    LIBS="$ac_save_libs $SQLITE3_LIBS"

    AC_LANG_PUSH(C)
    AC_TRY_LINK(
        [
#include <stdlib.h>
#include <sqlite3.h>
        ],
        [
        sqlite3 *conn;
        sqlite3_open("dfsfsdf", &conn);
        return 0;
        ], 
        [
        AC_MSG_RESULT([yes])
        have_sqlite3="yes"
        ifelse([$1], , :, [$1])
        ],
        [ 
        AC_MSG_RESULT([no])
        SQLITE3_CFLAGS=""
        SQLITE3_LIBS=""
        have_sqlite3="no"
        ifelse([$2], , :, [$2])
        ])

    AC_LANG_POP(C)
    CFLAGS="$ac_save_cflags"
    LIBS="$ac_save_libs"
])

dnl YB_SOCI([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Test for the SOCI library
dnl Defines
dnl   SOCI_CXXFLAGS to the set of flags required to include SOCI headers
dnl   SOCI_LIBS to the set of flags required to link against SOCI
AC_DEFUN([YB_SOCI],
[
    AC_SUBST(SOCI_CXXFLAGS)
    AC_SUBST(SOCI_LIBS)
    AC_MSG_CHECKING([whether we can use SOCI library])

    AC_ARG_WITH([soci-includes],
        AC_HELP_STRING([--with-soci-includes=DIR],
            [Place where SOCI includes are]),
        [ac_soci_includes="$withval"],[ac_soci_includes=""])
    AC_ARG_WITH([soci-libs],
        AC_HELP_STRING([--with-soci-libs=DIR],
            [Place where SOCI library is]),
        [ac_soci_libs="$withval"],[ac_soci_libs=""])

    SOCI_CXXFLAGS=-I/usr/local/include/soci
    SOCI_LIBS=-lsoci_core
    if test "x$ac_soci_includes" != "x" ; then
        SOCI_CXXFLAGS="-I$ac_soci_includes"
    fi
    if test "x$ac_soci_libs" != "x" ; then
        SOCI_LIBS="-L$ac_soci_libs -lsoci_core"
    fi
    ac_save_cxxflags="$CXXFLAGS"
    ac_save_libs="$LIBS"
    CXXFLAGS="$ac_save_cxxflags $SOCI_CXXFLAGS"
    LIBS="$ac_save_libs $SOCI_LIBS"

    AC_LANG_PUSH(C++)
    AC_TRY_LINK(
        [
#include <soci.h>
        ],
        [
        soci::session sql("mysql", "db=xxx");
        return 0;
        ], 
        [
        AC_MSG_RESULT([yes])
        have_soci="yes"
        ifelse([$1], , :, [$1])
        ],
        [ 
        AC_MSG_RESULT([no])
        SOCI_CXXFLAGS=""
        SOCI_LIBS=""
        have_soci="no"
        ifelse([$2], , :, [$2])
        ])

    AC_LANG_POP(C++)
    CXXFLAGS="$ac_save_cxxflags"
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

AC_DEFUN([YB_TEST_DB], 
[
    AC_SUBST(YBORM_DBTYPE)
    AC_SUBST(YBORM_DB)
    AC_SUBST(YBORM_USER)
    AC_SUBST(YBORM_PASSWD)
    AC_SUBST(YBORM_URL)
    YBORM_DBTYPE=""
    YBORM_DB=""
    YBORM_USER=""
    YBORM_PASSWD=""
    YBORM_URL=""
    AC_ARG_WITH([test-dbtype],
        AC_HELP_STRING([--with-test-dbtype=SQLITE|MYSQL|POSTGRES|INTERBASE|ORACLE],
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
    AC_ARG_WITH([test-db-url],
        AC_HELP_STRING([--with-test-db-url=dialect+driver://user:password@database or like],
            [Specify the URL to connect to the test database]),
        [YBORM_URL="$withval";
        YBORM_DBTYPE=`echo "$withval" | awk '{sub(/[[:+]].*/, ""); print toupper($][0)}'`
        ],[YBORM_URL=""])
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
    AC_SUBST(YBORM_GEN)
    YBORM_CXXFLAGS=""
    YBORM_LDFLAGS=""
    YBORM_LIBS=""
    YBORM_GEN=""

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
    AC_ARG_WITH([yborm-root],
        AC_HELP_STRING([--with-yborm-root=DIR],
            [Place where is the root of YB.ORM]),
        [
        ac_yborm_bin="$withval/bin"
        ac_yborm_libs="$withval/lib"
        ac_yborm_includes="$withval/include/yb"
        ]
        ,[])

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
    CXXFLAGS="$ac_save_cxxflags $YBORM_CXXFLAGS $WX_CFLAGS $QT_CFLAGS"
    LDFLAGS="$ac_save_ldflags $YBORM_LDFLAGS $QT_LDFLAGS"
    LIBS="$ac_save_libs $YBORM_LIBS $EXECINFO_LIBS $WX_LIBS $QT_LIBS"

    AC_LANG_PUSH(C++)
    AC_TRY_LINK([
#include <util/decimal.h>
],
        [Yb::Decimal x(10); x.str(); ],
        [ac_yborm_present=yes],[ac_yborm_present=no])
    AC_LANG_POP(C++)

    if test "$ac_yborm_present" = "yes" ; then
        if test "x$ac_yborm_bin" = "x" ; then
            YBORM_GEN="yborm_gen"
            if ! which "$YBORM_GEN" > /dev/null ; then
                ac_yborm_present=no
            fi
        else
            YBORM_GEN="$ac_yborm_bin/yborm_gen"
            if ! test -x "$YBORM_GEN" ; then
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
        YBORM_GEN=""
        AC_MSG_RESULT([no])
        ifelse([$2], , :, [$2])
    fi
    # Restore CXXFLAGS, LDFLAGS, LIBS
    CXXFLAGS="$ac_save_cxxflags"
    LDFLAGS="$ac_save_ldflags"
    LIBS="$ac_save_libs"
])

dnl YB_CHECK_UNICODE([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Test for using of wide Unicode strings by default
AC_DEFUN([YB_CHECK_UNICODE],
[
    AC_MSG_CHECKING([whether we are using Unicode strings])
    AC_TRY_COMPILE([],
        [
#if defined(YB_USE_WX)
#if wxUSE_UNICODE
#error Unicode detected
#endif
#elif defined(YB_USE_QT)
#error Unicode detected
#elif defined(YB_USE_UNICODE)
#error Unicode detected
#endif
],
        [have_unicode=no],[have_unicode=yes])
    if test "x$have_unicode" = "xyes" ; then
        AC_MSG_RESULT([yes])
        ifelse([$1], , :, [$1])
    else
        AC_MSG_RESULT([no])
        ifelse([$2], , :, [$2])
    fi
])

dnl vim:ts=4:sts=4:sw=4:et:
