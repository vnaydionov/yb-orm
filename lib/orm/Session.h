
#ifndef YB__ORM__SESSION__INCLUDED
#define YB__ORM__SESSION__INCLUDED

#include <map>
#include <vector>
#include <set>
#include <string>
#include <stdexcept>
#include <boost/utility.hpp>
#include "Value.h"
#include "Filters.h"

class TestSession;
class TestOdbcSession;

namespace Yb {
namespace SQL {

typedef std::vector<std::string> FieldList;
typedef std::set<std::string> FieldSet;

class DBError : public std::logic_error
{
public:
    DBError(const std::string &msg);
};

class GenericDBError: public DBError
{
public:
    GenericDBError(const std::string &err);
};

class NoDataFound : public DBError
{
public:
    NoDataFound(const std::string &msg = "");
};

class BadSQLOperation : public DBError
{
public:
    BadSQLOperation(const std::string &msg);
};

class BadOperationInMode : public DBError
{
public:
    BadOperationInMode(const std::string &msg);
};

class StrList
{
    std::string str_list_;
    template <typename T>
    static const std::string container_to_str(const T &cont)
    {
        std::string r;
        typename T::const_iterator it = cont.begin(), end = cont.end();
        if (it != end)
            r = *it;
        for (++it; it != end; ++it)
            r += ", " + *it;
        return r;
    }
public:
    StrList()
    {}
    StrList(const FieldSet &fs)
        : str_list_(container_to_str<FieldSet>(fs))
    {}
    StrList(const FieldList &fl)
        : str_list_(container_to_str<FieldList>(fl))
    {}
    template <typename T>
    StrList(const T &fl)
	: str_list_(container_to_str<T>(fl))
    {}
    StrList(const std::string &s)
        : str_list_(s)
    {}
    StrList(const char *s)
        : str_list_(s)
    {}
    const std::string &get_str() const { return str_list_; }
};


typedef std::set<std::string> FieldSet;

typedef std::map<std::string, Value> Row;
typedef std::auto_ptr<Row> RowPtr;
typedef std::vector<Row> Rows;
typedef std::auto_ptr<Rows> RowsPtr;

class Session : private boost::noncopyable
{
    friend class ::TestSession;
    friend class ::TestOdbcSession;

public:
    enum mode { READ_ONLY, MANUAL, FORCE_SELECT_UPDATE }; 
    Session(mode work_mode);
    virtual ~Session();

    // SQL operators
    RowsPtr select(const StrList &what,
            const StrList &from, const Filter &where = Filter(),
            const StrList &group_by = StrList(), const Filter &having = Filter(),
            const StrList &order_by = StrList(), int max_rows = -1,
            bool for_update = false);
    const std::vector<long long> insert(const std::string &table_name,
            const Rows &rows, const FieldSet &exclude_fields = FieldSet(),
            bool collect_new_ids = false);
    void update(const std::string &table_name,
            const Rows &rows, const FieldSet &key_fields,
            const FieldSet &exclude_fields = FieldSet(),
            const Filter &where = Filter());
    void delete_from(const std::string &table_name, const Filter &where);
    void exec_proc(const std::string &proc_code);
    void commit();
    void rollback();

    // Convenience utility methods
    RowPtr select_row(const StrList &what, const StrList &from, const Filter &where);
    RowPtr select_row(const StrList &from, const Filter &where);
    const Value select1(const std::string &what,
            const std::string &from, const Filter &where);
    long long get_curr_value(const std::string &seq_name);
    long long get_next_value(const std::string &seq_name);

    /* Use cases:
    SQL::Session db;
    int count = db.select1("count(1)", "t_manager", SQL::FilterEq("hidden", Value(1))).asInt();
    RowPtr row = db.select_row("v_ui_contract", SQL::FilterEq("contract_id", Value(contract_id)));
    */

    bool is_touched() const { return touched_; }

    virtual const boost::posix_time::ptime fix_dt_hook(const boost::posix_time::ptime &t);

private:
    virtual RowsPtr on_select(const StrList &what,
            const StrList &from, const Filter &where,
            const StrList &group_by, const Filter &having,
            const StrList &order_by, int max_rows,
            bool for_update) = 0;
    virtual const std::vector<long long> on_insert(
            const std::string &table_name,
            const Rows &rows, const FieldSet &exclude_fields,
            bool collect_new_ids) = 0;
    virtual void on_update(const std::string &table_name,
            const Rows &rows, const FieldSet &key_fields,
            const FieldSet &exclude_fields, const Filter &where) = 0;
    virtual void on_delete(const std::string &table_name, const Filter &where) = 0;
    virtual void on_exec_proc(const std::string &proc_code) = 0;
    virtual void on_commit() = 0;
    virtual void on_rollback() = 0;

private:
    bool touched_;
    mode mode_;
};

} // namespace SQL
} // namespace Yb

// vim:ts=4:sts=4:sw=4:et

#endif // YB__ORM__SESSION__INCLUDED

