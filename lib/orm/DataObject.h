#ifndef YB__ORM__DATA_OBJECT__INCLUDED
#define YB__ORM__DATA_OBJECT__INCLUDED

#include <string>
#include <memory>
#include <iterator>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <util/Exception.h>
#include <orm/Value.h>
#include <orm/MetaData.h>

namespace Yb {

class Session;
class RelationObject;

class CascadeDeleteError: public BaseError {
public:
    CascadeDeleteError(const Relation &rel):
        BaseError("Cascade delete error: " +
                rel.side(0) + "-" + rel.side(1))
    {}
};

class DataObject {
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
    typedef std::auto_ptr<DataObject> Ptr;
    typedef Values::iterator iterator;
    enum Status { New, Ghost, Dirty, Sync, Deleted };
    typedef std::vector<RelationObject * > SlaveRelations;
    typedef std::vector<boost::shared_ptr<RelationObject> > MasterRelations;
private:
    const Table &table_;
    Values values_;
    Status status_;
    SlaveRelations slave_relations_;
    MasterRelations master_relations_;
    Session *session_;

    DataObject(const Table &table, Status status)
        : table_(table)
        , values_(table.size())
        , status_(status)
        , session_(NULL)
    {}
    void load();
    void lazy_load(const Column &c) {
        if (!c.is_pk() && status_ == Ghost)
            load();
    }
    // non-copyable
    DataObject(const DataObject &);
    DataObject &operator=(const DataObject &);
public:
    static Ptr create_new(const Table &table) {
        return Ptr(new DataObject(table, New));
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
    Session *session() const { return session_; }
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
    }
    void set(const std::string &name, const Value &v) {
        set(table_.idx_by_name(name), v);
    }
    SlaveRelations &slave_relations() {
        return slave_relations_;
    }
    MasterRelations &master_relations() {
        return master_relations_;
    }

    bool could_be_deleted() const;
    void delete_object();
    void delete_master_relations();
    void exclude_from_slave_relations();
    void set_free_from(RelationObject *rel);
};

class RelationObject {
    // 0) Always allocate object in heap,
    //      pointer to object = object's identity.
    //      -> non-copyable.
    // 1) Store and enumerate pointers to related DataObject instances.
    //      one pointer to master, and vector of pointers to slaves.
public:
    typedef std::auto_ptr<RelationObject> Ptr;
    typedef std::vector<DataObject * > SlaveObjects;
    enum Status { Incomplete, Sync };
private:
    const Relation &relation_info_;
    DataObject *master_object_;
    SlaveObjects slave_objects_;
    Status status_;

    RelationObject(const Relation &rel_info)
        : relation_info_(rel_info)
        , master_object_(NULL)
        , status_(Incomplete)
    {}
    // non-copyable
    RelationObject(const RelationObject &);
    RelationObject &operator=(const RelationObject &);
public:
    static Ptr create_new(const Relation &rel_info) {
        return Ptr(new RelationObject(rel_info));
    }
    const Relation &relation_info() const { return relation_info_; }
    void master_object(DataObject *obj) { master_object_ = obj; }
    DataObject *master_object() const { return master_object_; }
    SlaveObjects &slave_objects() { return slave_objects_; }
    void status(Status stat) { status_ = stat; }
    Status status() const { return status_; }

    bool could_master_be_deleted() const;
    void delete_master();
    void exclude_slave(DataObject *obj);
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DATA_OBJECT__INCLUDED
