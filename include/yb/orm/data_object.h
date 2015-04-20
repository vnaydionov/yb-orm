// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__DATA_OBJECT__INCLUDED
#define YB__ORM__DATA_OBJECT__INCLUDED

/** @file
 * This is the core part of the ORM.  Mapped objects can form a graph,
 * consisting of DataObject instances interconnected with RelationObject
 * instances.
 */

#include <string>
#include <memory>
#include <iterator>
#include <vector>
#include <set>
#include <map>
#include "util/utility.h"
#include "util/exception.h"
#include "util/value_type.h"
#include "orm_config.h"
#include "schema.h"
#include "engine.h"

class TestDataObject;
class TestDataObjectSaveLoad;

namespace Yb {

class YBORM_DECL ORMError: public RunTimeError
{
public:
    ORMError(const String &msg);
};

class ObjectNotFoundByKey : public ORMError
{
public:
    ObjectNotFoundByKey(const String &msg);
};

class YBORM_DECL NullPK: public ORMError
{
public:
    NullPK(const String &table_name);
};

class YBORM_DECL CascadeDeleteError: public ORMError
{
public:
    CascadeDeleteError(const Relation &rel);
};

class YBORM_DECL CycleDetected: public ORMError
{
public:
    CycleDetected();
};

class YBORM_DECL BadTypeCast: public ORMError
{
public:
    BadTypeCast(const String &table_name,
            const String &column_name,
            const String &str_value, const String &type);
};

class YBORM_DECL StringTooLong: public ORMError
{
public:
    StringTooLong(const String &table_name,
            const String &column_name,
            int max_len, const String &value);
};

class YBORM_DECL DataObjectAlreadyInSession: public ORMError
{
public:
    DataObjectAlreadyInSession(const Key &key);
};

#define EMPTY_DATAOBJ (::Yb::DataObject::Ptr(NULL))

class DataObject;
class RelationObject;

typedef IntrusivePtr<DataObject> DataObjectPtr;
typedef IntrusivePtr<RelationObject> RelationObjectPtr;
inline DataObject *shptr_get(DataObjectPtr p) { return p.get(); }
inline RelationObject *shptr_get(RelationObjectPtr p) { return p.get(); }
typedef std::vector<DataObjectPtr> ObjectList;

class Session;

class YBORM_DECL DataObjectResultSet: public ResultSetBase<ObjectList>
{
    SqlResultSet rs_;
    std::auto_ptr<SqlResultSet::iterator> it_;
    std::vector<const Table *> tables_;
    Session &session_;

    bool fetch(ObjectList &row);
    DataObjectResultSet();
public:
    DataObjectResultSet(const SqlResultSet &rs, Session &session,
                        const Strings &tables);
    DataObjectResultSet(const DataObjectResultSet &obj);
};

YBORM_DECL const String key2str(const Key &key);

//! Session handles persisted DataObjects
/** Session class rules all over the mapped objects that should be
 * persisted in the database.  Session has associated Schema object
 * to consult to, and Engine instance to send queries to.
 */
class YBORM_DECL Session: public NonCopyable
{
    friend class ::TestDataObject;
    friend class ::TestDataObjectSaveLoad;
    typedef std::set<DataObjectPtr> Objects;
    typedef std::map<String, DataObject *> IdentityMap;

    ILogger::Ptr logger_, engine_logger_;
    Objects objects_;
    IdentityMap identity_map_;
    const Schema &schema_;
    std::auto_ptr<EngineSource> created_engine_;
    std::auto_ptr<EngineCloned> engine_;

    DataObject *add_to_identity_map(DataObject *obj, bool return_found);
    void flush_tbl_new_keyed(const Table &tbl, Objects &keyed_objs);
    void flush_tbl_new_unkeyed(const Table &tbl, Objects &unkeyed_objs);
    void flush_new();
    void flush_update(IdentityMap &idmap_copy);
    void flush_delete(IdentityMap &idmap_copy);
    void clone_engine(EngineSource *src_engine);
public:
    void set_logger(ILogger::Ptr logger);
    void debug(const String &s) { if (logger_.get()) logger_->debug(NARROW(s)); }
    Session(const Schema &schema, EngineSource *engine = NULL);
    Session(const Schema &schema, const String &connection_url);
    Session(const Schema &schema, const String &driver_name,
            const String &dialect_name, void *raw_connection);
    ~Session();
    void clear();
    const Schema &schema() const { return schema_; }
    void create_schema(bool ignore_errors = false) {
        engine_->create_schema(schema_, ignore_errors);
    }
    void drop_schema(bool ignore_errors = false) {
        engine_->drop_schema(schema_, ignore_errors);
    }
    //! Save a detached or new DataObject into Session
    void save(DataObjectPtr obj);
    //!
    DataObjectPtr save_or_update(DataObjectPtr obj);
    //! Tell session to release a DataObject
    void detach(DataObjectPtr obj);
    /** Get pointer to a DataObject by key provided
     * If there is an object with such key in the identity_map_
     * it is returned, otherwise a new Ghost DataObject is created,
     * placed in the identity_map_ and returned.
     */
    DataObjectPtr get_lazy(const Key &key);
    void flush();
    void commit();
    void rollback();
    EngineBase *engine() { return engine_.get(); }
    void load_collection(ObjectList &out,
                         const Expression &tables,
                         const Expression &filter,
                         const Expression &order_by = Expression(),
                         bool for_update_flag = false);
    DataObjectResultSet load_collection(
            const Expression &tables, const Expression &filter,
            const Expression &order_by = Expression(),
            bool for_update_flag = false);
    DataObjectResultSet load_collection(
            const Strings &tables, const SelectExpr &select_expr);
};

enum DeletionMode { DelNormal, DelDryRun, DelUnchecked };

//! Represents an instance of a mapped class.
/** Some facts about DataObject class <ul>
  <li>Object is always allocated in heap, pointer to object = object's identity.
      -> non-copyable.
  <li>Store data values within object.
  <li>Keep link to the table metadata.
  <li>Fast access data values by attribute name.
  <li>Maintain current state, change it when necessary.
      one of: new | ghost | dirty | sync | deleted.
  <li>Enumerate relations, filter relations by directions:
      a) master  b) slave
  <li>Each relation is a pointer to a RelationObject instance.
</ul>
*/
class YBORM_DECL DataObject
    : private NonCopyable, public RefCountBase
{
    friend class Session;
public:
    typedef DataObjectPtr Ptr;
    typedef Values::iterator iterator;
    enum Status { New, Ghost, Dirty, Sync, ToBeDeleted, Deleted };
    typedef std::map<const Relation *, RelationObject * > SlaveRelations;
    typedef std::map<const Relation *, RelationObjectPtr> MasterRelations;
private:
    const Table &table_;
    Values values_;
    Status status_;
    SlaveRelations slave_relations_;
    MasterRelations master_relations_;
    Session *session_;
    Key key_;
    String key_str_;
    bool assigned_key_;
    int depth_;

    DataObject(const Table &table, Status status)
        : table_(table)
        , values_(table.size())
        , status_(status)
        , session_(NULL)
        , assigned_key_(false)
        , depth_(0)
    {}
    void update_key();
    void load();
    void lazy_load(const Column *c = NULL) {
        if ((!c || !c->is_pk()) && status_ == Ghost)
            load();
    }
    void set_status(Status st) { status_ = st; }
    void depth(int d) { depth_ = d; }
    void populate_all_master_relations();
public:
    static void link(DataObject *master, Ptr slave,
                     const String &relation_name, int mode);
    static void link(DataObject *master, Ptr slave,
                     const Relation &r);
    static Ptr create_new(const Table &table, Status status = New) {
        return Ptr(new DataObject(table, status));
    }
    ~DataObject();
    const Table &table() const { return table_; }

    Status status() const { return status_; }
    Session *session() const { return session_; }
    void set_session(Session *session);
    void forget_session();
    iterator begin() {
        if (status_ == Ghost)
            load();
        return values_.begin();
    }
    iterator end() { return values_.end(); }
    size_t size() const { return values_.size(); }
    int depth() const { return depth_; }
    Value &get(int i) {
        lazy_load(&table_[i]);
        return values_[i];
    }
    Value &get(const String &name) {
        return get(table_.idx_by_name(name));
    }
    void touch();
    void set(int i, const Value &v);
    void set(const String &name, const Value &v) {
        set(table_.idx_by_name(name), v);
    }
    const Key &key();
    const String &key_str();
    Key fk_value_for(const Relation &r);
    const Values &raw_values() const { return values_; }
    bool assigned_key();
    SlaveRelations &slave_relations() {
        return slave_relations_;
    }
    MasterRelations &master_relations() {
        return master_relations_;
    }
    size_t fill_from_row(Row &r, size_t pos = 0);
    void refresh_slaves_fkeys();
    void refresh_master_fkeys();

    void delete_object(DeletionMode mode = DelNormal, int depth = 0);
    void delete_master_relations(DeletionMode mode, int depth);
    void exclude_from_slave_relations();
    void set_free_from(RelationObject *rel);
    static void link_slave_to_master(Ptr slave, Ptr master,
                                     const String relation_name = _T(""))
    {
        link(shptr_get(master), slave, relation_name, 1);
    }
    static void link_master_to_slave(Ptr master, Ptr slave,
                                     const String relation_name = _T(""))
    {
        link(shptr_get(master), slave, relation_name, 0);
    }
    static Ptr get_master(Ptr obj, const String &relation_name = _T(""));
    static bool has_master(Ptr obj, const String &relation_name = _T(""));
    RelationObject *get_slaves(const Relation &r);
    RelationObject *get_slaves(const String &relation_name = _T(""));
    void calc_depth(int d, DataObject *parent = NULL);
    void dump_tree(std::ostream &out, int level = 0);
};

//! Represents an instance of 1-to-many relation.
/** Some facts about RelationObject class <ul>
  <li>Object is always allocated in heap, pointer to object = object's identity.
      -> non-copyable.
  <li>Store and enumerate pointers to related DataObject instances:
  <li>one pointer to master,
  <li>and vector of pointers to slaves.
</ul>
*/
class YBORM_DECL RelationObject
    : private NonCopyable, public RefCountBase
{
    friend class DataObject;
public:
    typedef RelationObjectPtr Ptr;
    typedef std::vector<DataObject::Ptr> SlaveObjects;
    typedef std::map<DataObject *, int> SlaveObjectsOrder;
    enum Status { Incomplete, Sync };
private:
    const Relation &relation_info_;
    DataObject *master_object_;
    SlaveObjects slave_objects_;
    SlaveObjectsOrder slave_order_;
    Status status_;

    RelationObject(const Relation &rel_info, DataObject *master)
        : relation_info_(rel_info)
        , master_object_(master)
        , status_(Incomplete)
    {}
    void add_slave(DataObject::Ptr slave);
    void remove_slave(DataObject::Ptr slave);
public:
    static Ptr create_new(const Relation &rel_info, DataObject *master) {
        return Ptr(new RelationObject(rel_info, master));
    }
    const Relation &relation_info() const { return relation_info_; }
    void master_object(DataObject *obj) { master_object_ = obj; }
    DataObject *master_object() const { return master_object_; }
    SlaveObjects &slave_objects() { return slave_objects_; }
    SlaveObjects::iterator find(DataObject *obj);
    void status(Status stat) { status_ = stat; }
    Status status() const { return status_; }
    void calc_depth(int d, DataObject *parent = NULL);
    const Key gen_fkey() const;
    size_t count_slaves();
    void lazy_load_slaves();
    void refresh_slaves_fkeys();

    void delete_master(DeletionMode mode, int depth);
    void exclude_slave(DataObject *obj);
    void dump_tree(std::ostream &out, int level);
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DATA_OBJECT__INCLUDED
