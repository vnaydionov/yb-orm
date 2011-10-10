// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__DATA_OBJECT__INCLUDED
#define YB__ORM__DATA_OBJECT__INCLUDED

#include <string>
#include <memory>
#include <iterator>
#include <vector>
#include <set>
#include <map>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <util/Exception.h>
#include <orm/Value.h>
#include <orm/MetaData.h>
#include <orm/Engine.h>

class TestDataObject;
class TestDataObjectSaveLoad;

namespace Yb {

class DataObject;
class RelationObject;
typedef boost::shared_ptr<DataObject> DataObjectPtr;
typedef boost::shared_ptr<RelationObject> RelationObjectPtr;
typedef std::vector<DataObjectPtr> ObjectList;

class Session: boost::noncopyable {
    friend class ::TestDataObject;
    friend class ::TestDataObjectSaveLoad;
    typedef std::set<DataObjectPtr> Objects;
    typedef std::map<Key, DataObjectPtr> IdentityMap;
    Objects objects_;
    IdentityMap identity_map_;
    const Schema *schema_;
    EngineBase *engine_;
    void add_to_identity_map(DataObjectPtr obj);
    void flush_tbl_new_keyed(const Table &tbl, Objects &keyed_objs);
    void flush_tbl_new_unkeyed(const Table &tbl, Objects &unkeyed_objs);
    void flush_new();
    void flush_update(IdentityMap &idmap_copy);
    void flush_delete(IdentityMap &idmap_copy);
public:
    Session(const Schema &schema, EngineBase *engine = NULL):
        schema_(&schema),
        engine_(engine)
    {}
    ~Session();
    const Schema &schema() const { return *schema_; }
    void save(DataObjectPtr obj);
    void detach(DataObjectPtr obj);
    DataObjectPtr get_lazy(const Key &key);
    void flush();
    EngineBase *engine() { return engine_; }
    void load_collection(ObjectList &out,
                         const std::string &table_name,
                         const Filter &filter, 
                         const StrList &order_by = StrList(),
                         int max = -1,
                         const std::string &table_alias = "");
};

class CascadeDeleteError: public BaseError {
public:
    CascadeDeleteError(const Relation &rel):
        BaseError("Cascade delete error: " +
                rel.side(0) + "-" + rel.side(1))
    {}
};

class CycleDetected: public BaseError {
public:
    CycleDetected():
        BaseError("Cycle detected in the graph of objects")
    {}
};

class FieldNotFoundInFetchedRow : public ORMError
{
public:
    FieldNotFoundInFetchedRow(const std::string &table_name, const std::string &field_name);
};

class BadTypeCast : public ORMError
{
public:
    BadTypeCast(const std::string &table_name, const std::string &field_name,
            const std::string &str_value, const std::string &type);
};

class TableDoesNotMatchRow : public ORMError
{
public:
    TableDoesNotMatchRow(const std::string &table_name, const std::string &table_name_from_row);
};

class StringTooLong : public ORMError
{
public:
    StringTooLong(const std::string &table_name, const std::string &field_name,
                  int max_len, const std::string &value);
};

enum DeletionMode { DelNormal, DelDryRun, DelUnchecked };

class DataObject: boost::noncopyable {
    // 0) Always allocate object in heap, pointer to object = object's identity.
    //      -> non-copyable.
    // 1) Store data values within object.
    // 2) Keep table metadata link.
    // 3) Fast access data values by attribute name.
    // 4) Maintain current state, change it when necessary.
    //       one of: new | ghost | dirty | sync | deleted.
    // 5) Enumerate relations, filter relations by directions:
    //             a) master  b) slave
    //    Each relation is a pointer to a RelationObject instance.

    friend class Session;
public:
    typedef DataObjectPtr Ptr;
    typedef Values::iterator iterator;
    enum Status { New, Ghost, Dirty, Sync, ToBeDeleted, Deleted };
    typedef std::set<RelationObject * > SlaveRelations;
    typedef std::set<RelationObjectPtr> MasterRelations;
private:
    const Table &table_;
    Values values_;
    Status status_;
    SlaveRelations slave_relations_;
    MasterRelations master_relations_;
    Session *session_;
    Key key_;
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
    void lazy_load(const Column &c) {
        if (!c.is_pk() && status_ == Ghost)
            load();
    }
    void set_status(Status st) { status_ = st; }
    void depth(int d) { depth_ = d; }
public:
    static void link(DataObject *master, Ptr slave,
                     const std::string &relation_name, int mode);
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
        lazy_load(table_.get_column(i));
        return values_[i];
    }
    Value &get(const std::string &name) {
        return get(table_.idx_by_name(name));
    }
    const Value get_typed_value(const Column &col, const Value &v);
    void set(int i, const Value &v);
    void set(const std::string &name, const Value &v) {
        set(table_.idx_by_name(name), v);
    }
    const Key &key();
    const ValueMap values(bool include_key=true);
    bool assigned_key();
    SlaveRelations &slave_relations() {
        return slave_relations_;
    }
    MasterRelations &master_relations() {
        return master_relations_;
    }
    void fill_from_row(const Row &r);
    void refresh_slaves_fkeys();

    void delete_object(DeletionMode mode = DelNormal, int depth = 0);
    void delete_master_relations(DeletionMode mode, int depth);
    void exclude_from_slave_relations();
    void set_free_from(RelationObject *rel);
    static void link_slave_to_master(Ptr slave, Ptr master,
                                     const std::string relation_name = "")
    {
        link(master.get(), slave, relation_name, 1);
    }
    static void link_master_to_slave(Ptr master, Ptr slave,
                                     const std::string relation_name = "")
    {
        link(master.get(), slave, relation_name, 0);
    }
    static Ptr get_master(Ptr obj, const std::string &relation_name = "");
    RelationObject *get_slaves(const std::string &relation_name = "");
    void calc_depth(int d, DataObject *parent = NULL);
};

class RelationObject: boost::noncopyable {
    // 0) Always allocate object in heap,
    //      pointer to object = object's identity.
    //      -> non-copyable.
    // 1) Store and enumerate pointers to related DataObject instances.
    //      one pointer to master, and vector of pointers to slaves.
    friend class DataObject;
public:
    typedef RelationObjectPtr Ptr;
    typedef std::set<DataObject::Ptr> SlaveObjects;
    enum Status { Incomplete, Sync };
private:
    const Relation &relation_info_;
    DataObject *master_object_;
    SlaveObjects slave_objects_;
    Status status_;

    RelationObject(const Relation &rel_info, DataObject *master)
        : relation_info_(rel_info)
        , master_object_(master)
        , status_(Incomplete)
    {}
public:
    static Ptr create_new(const Relation &rel_info, DataObject *master) {
        return Ptr(new RelationObject(rel_info, master));
    }
    const Relation &relation_info() const { return relation_info_; }
    void master_object(DataObject *obj) { master_object_ = obj; }
    DataObject *master_object() const { return master_object_; }
    SlaveObjects &slave_objects() { return slave_objects_; }
    void status(Status stat) { status_ = stat; }
    Status status() const { return status_; }
    void calc_depth(int d, DataObject *parent = NULL);
    const Key gen_fkey() const;
    size_t count_slaves();
    void lazy_load_slaves();
    void refresh_slaves_fkeys();

    void delete_master(DeletionMode mode, int depth);
    void exclude_slave(DataObject *obj);
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DATA_OBJECT__INCLUDED
