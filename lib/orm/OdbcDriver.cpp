
#include "OdbcDriver.h"
#include <time.h>
#include <sql.h>
#include <sqlext.h>
#include <boost/lexical_cast.hpp>
#include <tiodbc.hpp>
#include <util/str_utils.hpp>

#include <iostream>

using namespace std;
using Yb::StrUtils::str_to_upper;

namespace Yb {
namespace SQL {

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
                string name = str_to_upper(stmt_.field_name(i + 1));
                string val = stmt_.field(i + 1).as_string();
                row[name] = val.empty()? Value(): Value(val);
                //cout << stmt_.field_name(i + 1) << "=\"" << val << "\"\n";
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
            stmt_.param(i + 1).set_as_string(params[i].as_string());
            //cout << "p[" << (i + 1) << "]=\"" << params[i].as_string() << "\"\n";
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
    //cout << "exec_direct: " << sql << "\n";
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
    //cout << "prepare: " << sql << "\n";
    impl_->prepare(sql);
}

void
OdbcDriver::exec(const Values &params)
{
    impl_->exec(params);
}

} // namespace SQL
} // namespace Yb

// vim:ts=4:sts=4:sw=4:et

