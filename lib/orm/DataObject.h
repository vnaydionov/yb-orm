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

namespace Yb {

class DataObject;

class SessionV2: boost::noncopyable {
    typedef boost::shared_ptr<DataObject> DataObjectPtr;
    typedef std::set<DataObjectPtr> Objects;
    typedef std::map<Key, DataObjectPtr> IdentityMap;
    Objects objects_;
    IdentityMap identity_map_;
    const Schema *schema_;
public:
    SessionV2(const Schema &schema):
        schema_(&schema) {}
    ~SessionV2();
    void save(DataObjectPtr obj);
    void detach(DataObjectPtr obj);
    DataObjectPtr get_lazy(const Key &key);
};


class RelationObject;

class CascadeDeleteError: public BaseError {
public:
    CascadeDeleteError(const Relation &rel):
        BaseError("Cascade delete error: " +
                rel.side(0) + "-" + rel.side(1))
    {}
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
public:
    typedef boost::shared_ptr<DataObject> Ptr;
    typedef Values::iterator iterator;
    enum Status { New, Ghost, Dirty, Sync, ToBeDeleted, Deleted };
    typedef std::set<RelationObject * > SlaveRelations;
    typedef std::set<boost::shared_ptr<RelationObject> > MasterRelations;
private:
    const Table &table_;
    Values values_;
    Status status_;
    SlaveRelations slave_relations_;
    MasterRelations master_relations_;
    SessionV2 *session_;
    Key key_;
    bool assigned_key_;

    DataObject(const Table &table, Status status)
        : table_(table)
        , values_(table.size())
        , status_(status)
        , session_(NULL)
        , assigned_key_(false)
    {}
    void update_key();
    void load();
    void lazy_load(const Column &c) {
        if (!c.is_pk() && status_ == Ghost)
            load();
    }
    static void link(DataObject *master, DataObject *slave,
                     const std::string &relation_name, int mode);
public:
    static Ptr create_new(const Table &table, Status status = New) {
        return Ptr(new DataObject(table, status));
    }
    ~DataObject();
    /*
    static Ptr create_from_row(const Table &table, const ResultSetItem &row) {
        Ptr obj(new DataObject(table, Sync));
        ///
        return obj;
    }
    */
    const Table &table() const { return table_; }
    
    Status status() const { return status_; }
    SessionV2 *session() const { return session_; }
    void set_session(SessionV2 *session);
    void forget_session();
    iterator begin() {
        if (status_ == Ghost)
            load();
        return values_.begin();
    }
    iterator end() { return values_.end(); }
    size_t size() const { return values_.size(); }
    Value &get(int i) {
        lazy_load(table_.get_column(i));
        return values_[i];
    }
    Value &get(const std::string &name) {
        return get(table_.idx_by_name(name));
    }
    void set(int i, const Value &v) {
        lazy_load(table_.get_column(i));
        values_[i] = v;
        update_key();
    }
    void set(const std::string &name, const Value &v) {
        set(table_.idx_by_name(name), v);
    }
    const Key &key();
    bool assigned_key();
    SlaveRelations &slave_relations() {
        return slave_relations_;
    }
    MasterRelations &master_relations() {
        return master_relations_;
    }

    void delete_object(DeletionMode mode = DelNormal);
    void delete_master_relations(DeletionMode mode = DelNormal);
    void exclude_from_slave_relations();
    void set_free_from(RelationObject *rel);
    void link_to_master(Ptr master, const std::string relation_name = "") {
        link(master.get(), this, relation_name, 1);
    }
    void link_to_slave(Ptr slave, const std::string relation_name = "") {
        link(this, slave.get(), relation_name, 0);
    }
    Ptr go_to_master(const std::string relation_name = "");
    RelationObject *go_to_slaves(const std::string relation_name = "");
};

class RelationObject: boost::noncopyable {
    // 0) Always allocate object in heap,
    //      pointer to object = object's identity.
    //      -> non-copyable.
    // 1) Store and enumerate pointers to related DataObject instances.
    //      one pointer to master, and vector of pointers to slaves.
public:
    typedef boost::shared_ptr<RelationObject> Ptr;
    typedef std::set<DataObject * > SlaveObjects;
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

    void delete_master(DeletionMode mode = DelNormal);
    void exclude_slave(DataObject *obj);
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DATA_OBJECT__INCLUDED
