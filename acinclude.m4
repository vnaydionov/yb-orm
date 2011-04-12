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
                if test "x$enable_debug" = xyes ; then
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
        LIBS="$ac_save_libs -lboost_date_time"
        YB_TRY_LINK_BOOST_DATETIME([
            BOOST_LIBS="$BOOST_LIBS -lboost_date_time"
            BOOST_LIBS_R="$BOOST_LIBS_R -lboost_date_time"
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

dnl vim:ts=4:sts=4:sw=4:et:
