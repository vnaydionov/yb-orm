// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
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
#include <util/Utility.h>
#include <util/Exception.h>
#include <util/Value.h>
#include <orm/MetaData.h>
#include <orm/Engine.h>

class TestDataObject;
class TestDataObjectSaveLoad;

namespace Yb {

class DataObject;
class RelationObject;

typedef IntrusivePtr<DataObject> DataObjectPtr;
typedef IntrusivePtr<RelationObject> RelationObjectPtr;
inline DataObject *shptr_get(DataObjectPtr p) { return p.get(); }
inline RelationObject *shptr_get(RelationObjectPtr p) { return p.get(); }
typedef std::vector<DataObjectPtr> ObjectList;

const String key2str(const Key &key);

class Session;

class DataObjectResultSet: public ResultSetBase<ObjectList>
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

//! Session handles persisted DataObjects
/** Session class rules all over the mapped objects that should be
 * persisted in the database.  Session has associated Schema object
 * to consult to, and Engine instance to send queries to.
 */
class Session: NonCopyable {
    friend class ::TestDataObject;
    friend class ::TestDataObjectSaveLoad;
    typedef std::set<DataObjectPtr> Objects;
    typedef std::map<String, DataObject *> IdentityMap;

    ILogger::Ptr logger_;
    Objects objects_;
    IdentityMap identity_map_;
    const Schema &schema_;
    std::auto_ptr<EngineBase> engine_;

    DataObject *add_to_identity_map(DataObject *obj, bool return_found);
    void flush_tbl_new_keyed(const Table &tbl, Objects &keyed_objs);
    void flush_tbl_new_unkeyed(const Table &tbl, Objects &unkeyed_objs);
    void flush_new();
    void flush_update(IdentityMap &idmap_copy);
    void flush_delete(IdentityMap &idmap_copy);
public:
    void debug(const String &s) { if (logger_.get()) logger_->debug(NARROW(s)); }
    Session(const Schema &schema, EngineSource *engine = NULL);
    ~Session();
    void clear();
    const Schema &schema() const { return schema_; }
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
};

class NullPK: public BaseError {
public:
    NullPK(const String &table_name):
        BaseError(_T("Null PK given for table: ") + table_name) {}
};

class CascadeDeleteError: public BaseError {
public:
    CascadeDeleteError(const Relation &rel):
        BaseError(_T("Cascade delete error: ") +
                rel.side(0) + _T("-") + rel.side(1))
    {}
};

class CycleDetected: public BaseError {
public:
    CycleDetected():
        BaseError(_T("Cycle detected in the graph of objects"))
    {}
};

class FieldNotFoundInFetchedRow : public ORMError
{
public:
    FieldNotFoundInFetchedRow(const String &table_name, const String &field_name);
};

class BadTypeCast : public ORMError
{
public:
    BadTypeCast(const String &table_name, const String &field_name,
            const String &str_value, const String &type);
};

class TableDoesNotMatchRow : public ORMError
{
public:
    TableDoesNotMatchRow(const String &table_name, const String &table_name_from_row);
};

class StringTooLong : public ORMError
{
public:
    StringTooLong(const String &table_name, const String &field_name,
                  int max_len, const String &value);
};

class DataObjectAlreadyInSession : public ORMError
{
public:
    DataObjectAlreadyInSession(const Key &key);
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
class DataObject: private NonCopyable, public RefCountBase
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
class RelationObject: private NonCopyable, public RefCountBase {
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
