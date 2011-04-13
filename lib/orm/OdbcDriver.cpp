#include "OdbcDriver.h"
#include <time.h>
#include <tiodbc.hpp>
#include <boost/lexical_cast.hpp>
#include <util/str_utils.hpp>

//#define DUMP_ODBC
#ifdef DUMP_ODBC
#include <iostream>
#endif

using namespace std;
using Yb::StrUtils::str_to_upper;

namespace Yb {

SqlDriver::~SqlDriver()
{}

class OdbcDriverImpl
{
    tiodbc::connection conn_;
    tiodbc::statement stmt_;

public:
    OdbcDriverImpl() {}
    ~OdbcDriverImpl() {}

    void open(const string &dsn, const string &user, const string &passwd)
    {
        if (!conn_.connect(dsn, user, passwd, 10, false))
            throw DBError(conn_.last_error());
    }

    void commit()
    {
        if (!conn_.commit())
            throw DBError(conn_.last_error());
    }

    void rollback()
    {
        if (!conn_.rollback())
            throw DBError(conn_.last_error());
    }

    void exec_direct(const string &sql)
    {
        if (!stmt_.execute_direct(conn_, sql))
            throw DBError(stmt_.last_error());
    }

    RowsPtr fetch_rows(int max_rows)
    {
        RowsPtr rows(new Rows);
        for (int count = 0; max_rows < 0? 1: count < max_rows; ++count) {
            if (!stmt_.fetch_next())
                break;
            int col_count = stmt_.count_columns();
            Row row;
            for (int i = 0; i < col_count; ++i) {
                tiodbc::field_impl f = stmt_.field(i + 1);
                string name = str_to_upper(f.get_name());
                Value v;
                if (f.get_type() == SQL_DATE ||
                        f.get_type() == SQL_TIMESTAMP ||
                        f.get_type() == SQL_TYPE_TIMESTAMP)
                {
                    TIMESTAMP_STRUCT ts = f.as_date_time();
                    if (ts.year != 0 || f.is_null() != 1) {
                        v = Value(mk_datetime(ts.year, ts.month, ts.day,
                                    ts.hour, ts.minute, ts.second));
                    }
                }
                else {
                    string val = f.as_string();
                    if (f.is_null() != 1)
                        v = Value(val);
                }
                row[name] = v;
#ifdef DUMP_ODBC
                cout << f.get_name()
                    << "=" << v.sql_str() << "\n";
#endif
            }
            rows->push_back(row);
        }
        return rows;
    }

    void prepare(const string &sql)
    {
        if (!stmt_.prepare(conn_, sql))
            throw DBError(stmt_.last_error());
    }

    void exec(const Values &params)
    {
        for (int i = 0; i < params.size(); ++i) {
            if (params[i].get_type() == Value::DATETIME) {
                TIMESTAMP_STRUCT ts;
                memset(&ts, 0, sizeof(ts));
                DateTime t = params[i].as_date_time();
                ts.year = t.date().year();
                ts.month = t.date().month();
                ts.day = t.date().day();
                ts.hour = t.time_of_day().hours();
                ts.minute = t.time_of_day().minutes();
                ts.second = t.time_of_day().seconds();
                ts.fraction = 0; // TODO
                stmt_.param(i + 1).set_as_date_time(
                        ts,
                        params[i].is_null());
            }
            else {
                stmt_.param(i + 1).set_as_string(
                        params[i].is_null()?
                            string(): params[i].as_string(),
                        params[i].is_null());
            }
#ifdef DUMP_ODBC
            cout << "p[" << (i + 1) << "]=\"" << params[i].sql_str() << "\"\n";
#endif
        }
        if (!stmt_.execute())
            throw DBError(stmt_.last_error());
    }
};

OdbcDriver::OdbcDriver()
    : impl_ (new OdbcDriverImpl())
{}

OdbcDriver::~OdbcDriver()
{
    delete impl_;
}

void
OdbcDriver::open(const string &db, const string &user, const string &passwd)
{
    impl_->open(db, user, passwd);
}

void
OdbcDriver::commit()
{
    impl_->commit();
}

void
OdbcDriver::rollback()
{
    impl_->rollback();
}

void
OdbcDriver::exec_direct(const string &sql)
{
#ifdef DUMP_ODBC
    cout << "exec_direct: " << sql << "\n";
#endif
    impl_->exec_direct(sql);
}

RowsPtr
OdbcDriver::fetch_rows(int max_rows)
{
    return impl_->fetch_rows(max_rows);
}

void
OdbcDriver::prepare(const string &sql)
{
#ifdef DUMP_ODBC
    cout << "prepare: " << sql << "\n";
#endif
    impl_->prepare(sql);
}

void
OdbcDriver::exec(const Values &params)
{
    impl_->exec(params);
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
