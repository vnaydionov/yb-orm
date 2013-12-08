// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#include "util/string_utils.h"
#include "orm/data_object.h"
#include <algorithm>
#include <iostream>
#if 0
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#endif

using namespace std;

namespace Yb {

StringTooLong::StringTooLong(
        const String &table_name, const String &field_name,
        int max_len, const String &value)
    : ORMError(_T("Can't set value of ") + table_name + _T(".") + field_name +
            _T(" with '") + value + _T("', having max length ") +
            to_string(max_len))
{}

BadTypeCast::BadTypeCast(
        const String &table_name, const String &field_name,
        const String &str_value, const String &type)
    : ORMError(_T("Can't cast field ") + table_name + _T(".") + field_name +
            _T(" = \"") + str_value + _T("\" to type ") + type)
{}

bool DataObjectResultSet::fetch(ObjectList &row)
{
    if (!it_.get())
        it_.reset(new SqlResultSet::iterator(rs_.begin()));
    if (rs_.end() == *it_)
        return false;
    ObjectList new_row;
    Row &cur = **it_;
    size_t pos = 0;
    for (int i = 0; i < tables_.size(); ++i) {
        DataObject::Ptr d = DataObject::create_new
            (*tables_[i], DataObject::Sync);
        pos = d->fill_from_row(cur, pos);
        DataObject::Ptr e = session_.save_or_update(d);
        new_row.push_back(e);
    }
    row.swap(new_row);
    ++*it_;

    return true;
}

DataObjectResultSet::DataObjectResultSet(const SqlResultSet &rs, Session &session,
                                         const Strings &tables)
    : rs_(rs)
    , session_(session)
{
    const Schema &schema = session.schema();
    Strings::const_iterator i = tables.begin(), iend = tables.end();
    for (; i != iend; ++i)
        tables_.push_back(&schema.table(*i));
}

DataObjectResultSet::DataObjectResultSet(const DataObjectResultSet &obj)
    : rs_(obj.rs_)
    , tables_(obj.tables_)
    , session_(obj.session_)
{
    YB_ASSERT(!obj.it_.get());
}

const String key2str(const Key &key)
{
    std::ostringstream out;
    out << "Key('" << NARROW(key.first) << "', {";
    ValueMap::const_iterator i = key.second.begin(),
        iend = key.second.end();
    for (; i != iend; ++i)
        out << "'" << NARROW(i->first) << "': "
            << NARROW(i->second.sql_str()) << ", ";
    out << "}";
    return WIDEN(out.str());
}

DataObjectAlreadyInSession::DataObjectAlreadyInSession(const Key &key)
    : ORMError(_T("DataObject is already registered in the identity map: ") + key2str(key))
{}

Session::Session(const Schema &schema, EngineSource *engine)
    : schema_(schema)
{
    if (engine) {
        engine_.reset(engine->clone().release());
        if (engine_->logger())
            logger_.reset(engine_->logger()->new_logger("orm").release());
    }
}

Session::~Session() { clear(); }

void Session::clear()
{
    Objects::iterator i = objects_.begin(), iend = objects_.end();
    for (; i != iend; ++i)
        (*i)->forget_session();
    Objects empty_objects;
    objects_.swap(empty_objects);
    IdentityMap empty_map;
    identity_map_.swap(empty_map);
    if (engine_.get())
        engine_->rollback();
}

DataObject *Session::add_to_identity_map(DataObject *obj, bool return_found)
{
    if (obj->assigned_key()) {
        const String &key = obj->key_str();
        IdentityMap::iterator i = identity_map_.find(key);
        if (i != identity_map_.end()) {
            if (return_found)
                return i->second;
            throw DataObjectAlreadyInSession(obj->key());
        }
        identity_map_[key] = obj;
    }
    return obj;
}

void Session::save(DataObjectPtr obj0)
{
    DataObject *obj = add_to_identity_map(shptr_get(obj0), false);
    if (obj == shptr_get(obj0)) {
        objects_.insert(obj0);
        obj->set_session(this);
    }
}

DataObjectPtr Session::save_or_update(DataObjectPtr obj0)
{
    DataObject *obj = add_to_identity_map(shptr_get(obj0), true);
    if (obj == shptr_get(obj0)) {
        objects_.insert(obj0);
        obj->set_session(this);
        return obj0;
    }
    const Table &table = obj->table();
    for (size_t i = 0; i < table.size(); ++i)
        if (!table[i].is_pk())
            obj->values_[i] = obj0->values_[i];
    obj->status_ = obj0->status_;
    return DataObjectPtr(obj);
}

void Session::detach(DataObjectPtr obj)
{
    if (obj->assigned_key()) {
        const String &key = obj->key_str();
        IdentityMap::iterator i = identity_map_.find(key);
        if (i != identity_map_.end())
            identity_map_.erase(i);
    }
    Objects::iterator i = objects_.find(obj);
    if (i != objects_.end()) {
        objects_.erase(i);
        obj->forget_session();
    }
}

void Session::load_collection(ObjectList &out,
                              const Expression &from,
                              const Filter &filter, 
                              const Expression &order_by,
                              bool for_update_flag)
{
    DataObjectResultSet rs = load_collection(
            from, filter, order_by, for_update_flag);
    DataObjectResultSet::iterator i = rs.begin(), iend = rs.end();
    for (; i != iend; ++i)
        out.push_back((*i)[0]);
}

DataObjectResultSet Session::load_collection(
        const Expression &from,
        const Filter &filter,
        const Expression &order_by,
        bool for_update_flag)
{
    Strings tables;
    find_all_tables(from, tables);
    ExpressionList cols;
    Strings::const_iterator i = tables.begin(), iend = tables.end();
    for (; i != iend; ++i) {
        const Table &table = schema_.table(*i);
        Columns::const_iterator j = table.begin(), jend = table.end();
        for (; j != jend; ++j)
            cols << ColumnExpr(*i, j->name());
    }
    SqlResultSet rs = engine_->select_iter(
            SelectExpr(cols).from_(from).where_(filter)
                .order_by_(order_by).for_update(for_update_flag));
    return DataObjectResultSet(rs, *this, tables);
}

DataObject::Ptr Session::get_lazy(const Key &key)
{
    String key_str = key2str(key);
    IdentityMap::iterator i = identity_map_.find(key_str);
    if (i != identity_map_.end())
        return DataObject::Ptr(i->second);
    bool empty = empty_key(key);
    //if (empty)
    //    throw NullPK(key.first);
    DataObjectPtr new_obj =
        DataObject::create_new(schema_[key.first], DataObject::Ghost);
    ValueMap::const_iterator j = key.second.begin(), jend = key.second.end();
    for (; j != jend; ++j)
        new_obj->set(j->first, j->second);
    objects_.insert(new_obj);
    new_obj->set_session(this);
    identity_map_[key_str] = shptr_get(new_obj);
    return new_obj;
}

typedef map<String, Rows> RowsByTable;
typedef map<String, RowsData> RowsDataByTable;

template <class Row__, class Rows__>
void add_row_to_rows_by_table(map<String, Rows__> &rows_by_table,
                              const String &tbl_name, const Row__ &row)
{
    typename map<String, Rows__>::iterator k = rows_by_table.find(tbl_name);
    if (k == rows_by_table.end()) {
        Rows__ rows(1);
        rows[0] = row;
        rows_by_table.insert(make_pair(tbl_name, rows));
    }
    else {
        Rows__ &rows = k->second;
        rows.push_back(row);
    }
}

void Session::flush_tbl_new_keyed(const Table &tbl, Objects &keyed_objs)
{
    bool sql_seq = engine_->get_dialect()->has_sequences();
    bool use_autoinc = !sql_seq &&
        (tbl.autoinc() || !str_empty(tbl.seq_name()));
    RowsData rows;
    rows.reserve(keyed_objs.size());
    Objects::iterator i, iend = keyed_objs.end();
    for (i = keyed_objs.begin(); i != iend; ++i) {
        (*i)->refresh_master_fkeys();
        rows.push_back(&(*i)->raw_values());
        add_to_identity_map(shptr_get(*i), true);
    }
    engine_->insert(tbl, rows, false);
}

void Session::flush_tbl_new_unkeyed(const Table &tbl, Objects &unkeyed_objs)
{
    bool sql_seq = engine_->get_dialect()->has_sequences();
    bool use_seq = sql_seq && !str_empty(tbl.seq_name());
    bool use_autoinc = !sql_seq &&
        (tbl.autoinc() || !str_empty(tbl.seq_name()));
    Objects::iterator i, iend = unkeyed_objs.end();
    if (use_seq) {
        String pk = tbl.get_surrogate_pk();
        for (i = unkeyed_objs.begin(); i != iend; ++i)
            (*i)->set(pk, Value(engine_->get_next_value(tbl.seq_name())));
    }
    RowsData rows;
    rows.reserve(unkeyed_objs.size());
    for (i = unkeyed_objs.begin(); i != iend; ++i) {
        (*i)->refresh_master_fkeys();
        rows.push_back(&(*i)->raw_values());
    }
    vector<LongInt> ids = engine_->insert(tbl, rows, use_autoinc);
    if (use_autoinc) {
        String pk = tbl.get_surrogate_pk();
        i = unkeyed_objs.begin();
        for (int p = 0; i != iend; ++i, ++p)
            (*i)->set(pk, Value(ids[p]));
    }
    // Refresh fk values for all slaves and
    // add flushed objects to the identity_map_
    for (i = unkeyed_objs.begin(); i != iend; ++i) {
        (*i)->refresh_slaves_fkeys();
        add_to_identity_map(shptr_get(*i), false);
    }
}

void Session::flush_new()
{
    SqlDialect *dialect = engine_->get_dialect();
    bool sql_seq = dialect->has_sequences();
    Objects::iterator i = objects_.begin(), iend = objects_.end();
    for (; i != iend; ++i)
        if ((*i)->status() == DataObject::New)
            (*i)->depth(-1);
    for (i = objects_.begin(); i != iend; ++i)
        if ((*i)->status() == DataObject::New)
            (*i)->calc_depth(0);
    int max_depth = -1;
    typedef map<String, Objects> ObjectsByTable;
    typedef map<int, ObjectsByTable> GroupsByDepth;
    GroupsByDepth groups_by_depth;
    for (i = objects_.begin(); i != iend; ++i)
        if ((*i)->status() == DataObject::New)
    {
        int d = (*i)->depth();
        if (d > max_depth)
            max_depth = d;
        GroupsByDepth::iterator k = groups_by_depth.find(d);
        if (k == groups_by_depth.end()) {
            groups_by_depth.insert(pair<int, ObjectsByTable> 
                                   (d, ObjectsByTable()));
            k = groups_by_depth.find(d);
        }
        ObjectsByTable &objs_by_table = k->second;
        const String &tbl_name = (*i)->table().name();
        ObjectsByTable::iterator q = objs_by_table.find(tbl_name);
        if (q == objs_by_table.end()) {
            objs_by_table.insert(pair<String, Objects>
                                 (tbl_name, Objects()));
            q = objs_by_table.find(tbl_name);
        }
        Objects &objs = q->second;
        objs.insert(*i);
    }
    for (int d = 0; d <= max_depth; ++d) {
        GroupsByDepth::iterator k = groups_by_depth.find(d);
        if (k == groups_by_depth.end())
            continue;
        ObjectsByTable &objs_by_table = k->second;
        ObjectsByTable::iterator j = objs_by_table.begin(), jend = objs_by_table.end();
        for (; j != jend; ++j) {
            const Table &tbl = schema_[j->first];
            Objects &objs = j->second, unkeyed_objs, keyed_objs;
            Objects::iterator l, lend = objs.end();
            for (l = objs.begin(); l != lend; ++l) {
                if ((*l)->assigned_key())
                    keyed_objs.insert(*l);
                else
                    unkeyed_objs.insert(*l);
            }
            flush_tbl_new_keyed(tbl, keyed_objs);
            flush_tbl_new_unkeyed(tbl, unkeyed_objs);
        }
    }
    for (i = objects_.begin(); i != iend; ++i)
        if ((*i)->status() == DataObject::New)
            (*i)->set_status(DataObject::Ghost);
}

void Session::flush_update(IdentityMap &idmap_copy)
{
    RowsDataByTable rows_by_table;
    IdentityMap::iterator i = idmap_copy.begin(), iend = idmap_copy.end();
    for (; i != iend; ++i)
        if (i->second->status() == DataObject::Dirty) {
            const String &tbl_name = i->second->table().name();
            i->second->refresh_master_fkeys();
            add_row_to_rows_by_table(rows_by_table, tbl_name,
                                     &i->second->raw_values());
            i->second->set_status(DataObject::Ghost);
        }
    RowsDataByTable::iterator j = rows_by_table.begin(), jend = rows_by_table.end();
    for (; j != jend; ++j)
        engine_->update(schema_[j->first], j->second);
}

void Session::flush_delete(IdentityMap &idmap_copy)
{
    typedef vector<Key> Keys;
    typedef map<String, Keys> KeysByTable;
    typedef map<int, KeysByTable> GroupsByDepth;
    int max_depth = -1;
    GroupsByDepth groups_by_depth;
    IdentityMap::iterator i = idmap_copy.begin(), iend = idmap_copy.end();
    for (; i != iend; ++i)
        if (i->second->status() == DataObject::ToBeDeleted)
    {
        int d = i->second->depth();
        if (d > max_depth)
            max_depth = d;
        GroupsByDepth::iterator k = groups_by_depth.find(d);
        if (k == groups_by_depth.end()) {
            groups_by_depth.insert(pair<int, KeysByTable> 
                                   (d, KeysByTable()));
            k = groups_by_depth.find(d);
        }
        KeysByTable &keys_by_table = k->second;
        const String &tbl_name = i->second->table().name();
        KeysByTable::iterator q = keys_by_table.find(tbl_name);
        if (q == keys_by_table.end()) {
            keys_by_table.insert(pair<String, Keys> (tbl_name, Keys()));
            q = keys_by_table.find(tbl_name);
        }
        Keys &keys = q->second;
        keys.push_back(i->second->key());
        i->second->set_status(DataObject::Deleted);
    }

    for (int d = max_depth; d >= 0; --d) {
        debug(_T("flush_delete: depth: ") + to_string(d));
        GroupsByDepth::iterator k = groups_by_depth.find(d);
        if (k == groups_by_depth.end())
            continue;
        KeysByTable &keys_by_table = k->second;
        KeysByTable::iterator j = keys_by_table.begin(),
            jend = keys_by_table.end();
        for (; j != jend; ++j) {
            debug(_T("flush_delete: table: ") + j->first);
            engine_->delete_from(schema_[j->first], j->second);
        }
    }
}

void Session::flush()
{
    debug(_T("flush started"));
    try {
        IdentityMap idmap_copy = identity_map_;
        flush_new();
        flush_update(idmap_copy);
        flush_delete(idmap_copy);
        // Delete the deleted objects
        Objects obj_copy = objects_;
        Objects::iterator i = obj_copy.begin(), iend = obj_copy.end();
        for (; i != iend; ++i)
            if ((*i)->status() == DataObject::Deleted) {
                IdentityMap::iterator k = identity_map_.find((*i)->key_str());
                if (k != identity_map_.end())
                    identity_map_.erase(k);
                objects_.erase(objects_.find(*i));
            }
        debug(_T("flush finished OK"));
    }
    catch (...) {
        debug(_T("flush finished with an ERROR"));
        throw;
    }
}

void Session::commit()
{
    flush();
    engine_->commit();
}

void Session::rollback()
{
    //purge();
    engine_->rollback();
}

void DataObject::set_session(Session *session)
{
    YB_ASSERT(session && (!session_ || session_ == session));
    session_ = session;
}

void DataObject::forget_session()
{
    YB_ASSERT(session_);
    session_ = NULL;
}

void DataObject::touch()
{
    if (status_ == Sync)
        status_ = Dirty;
}
    
void DataObject::set(int i, const Value &v)
{
    const Column &c = table_[i];
    if (c.is_ro() && !c.is_pk())
        throw ReadOnlyColumn(table_.name(), c.name());
    lazy_load(&c);
    Value new_v = v;
    new_v.fix_type(c.type());
    bool equal = values_[i] == new_v;
    if (c.is_pk() && session_ != NULL
        && !equal && !values_[i].is_null())
    {
        throw ReadOnlyColumn(table_.name(), c.name());
    }
    if (c.type() == Value::STRING) {
        const String &s = new_v.read_as<String>();
        if (c.size() && c.size() < s.size())
            throw StringTooLong(table_.name(), c.name(), c.size(), s);
    }
    if (!equal) {
        values_[i].swap(new_v);
        if (c.is_pk())
            update_key();
        else
            touch();
    }
}

void DataObject::update_key()
{
    assigned_key_ = table_.mk_key(values_, key_);
    key_str_ = key2str(key_);
}

const Key &DataObject::key()
{
    if (str_empty(key_.first))
        update_key();
    return key_;
}

const String &DataObject::key_str()
{
    if (str_empty(key_.first))
        update_key();
    return key_str_;
}

bool DataObject::assigned_key()
{
    if (str_empty(key_.first))
        update_key();
    return assigned_key_;
}

void DataObject::load()
{
    YB_ASSERT(session_ != NULL);
    ExpressionList cols;
    Columns::const_iterator it = table_.begin(), end = table_.end();
    for (; it != end; ++it)
        cols << ColumnExpr(table_.name(), it->name());
    KeyFilter f(key());
    RowsPtr result = session_->engine()->select
        (cols, Expression(table_.name()), f);
    if (result->size() != 1)
        throw ObjectNotFoundByKey(table_.name() + _T("(")
                                  + f.get_sql() + _T(")"));
    fill_from_row(*result->begin());
}

void DataObject::calc_depth(int d, DataObject *parent)
{
    if (d > depth_) {
        depth_ = d;
        MasterRelations::iterator i = master_relations_.begin(),
            iend = master_relations_.end();
        for (; i != iend; ++i)
            i->second->calc_depth(d + 1, parent);
    }
}

void DataObject::link(DataObject *master, DataObject::Ptr slave,
                      const Relation &r)
{
    slave->lazy_load();
    // Find existing relation in slave's relations
    RelationObject *ro = NULL;
    SlaveRelations::iterator i = slave->slave_relations().find(&r);
    if (i != slave->slave_relations().end())
        ro = i->second;
    if (ro && ro->master_object() != master) {
        ro->remove_slave(slave);
        ro = NULL;
    }
    // Try to find relation object in master's relations
    if (!ro) {
        MasterRelations::iterator j = master->master_relations().find(&r);
        if (j != master->master_relations().end())
            ro = shptr_get(j->second);
    }
    // Create one if it doesn't exist, master will own it
    if (!ro) {
        RelationObject::Ptr new_ro = RelationObject::create_new(r, master);
        master->master_relations().insert(make_pair(&r, new_ro));
        ro = shptr_get(new_ro);
    }
    // Register slave in the relation
    ro->add_slave(slave);
    slave->calc_depth(master->depth() + 1, master);
    if (master->assigned_key()) {
        Key pkey = master->key();
        const Strings &fkey_parts = r.fk_fields();
        for (int i = 0; i < fkey_parts.size(); ++i)
            slave->set(fkey_parts[i], pkey.second[i].second);
    }
    else if (slave->status() == DataObject::Sync &&
        (master->status() == DataObject::New ||
         slave->fk_value_for(ro->relation_info()) != master->key()))
    {
        slave->touch();
    }
}

void DataObject::link(DataObject *master, DataObject::Ptr slave,
                      const String &relation_name, int mode)
{
    // Find relation in metadata.
    const Schema &schema(master->table().schema());
    const Relation *r = schema.find_relation
        (master->table().class_name(), relation_name,
         slave->table().class_name(), mode);
    link(master, slave, *r);
}

Key DataObject::fk_value_for(const Relation &r)
{
    const Table &master_tbl = r.table(0),
        &slave_tbl = table_;
    const Strings &parts = r.fk_fields();
    Key fkey;
    fkey.second.reserve(master_tbl.pk_fields().size());
    fkey.first = master_tbl.name();
    Strings::const_iterator i = parts.begin(), iend = parts.end(),
        j = master_tbl.pk_fields().begin(),
        jend = master_tbl.pk_fields().end();
    for (; i != iend && j != jend; ++i, ++j)
        fkey.second.push_back(make_pair(*j, get(*i)));
    return fkey;
}

DataObject::Ptr DataObject::get_master(
    DataObject::Ptr obj, const String &relation_name)
{
    YB_ASSERT(shptr_get(obj));
    YB_ASSERT(obj->session_);
    // Find relation in metadata.
    const Schema &schema(obj->table_.schema());
    const Relation *r = schema.find_relation
        (obj->table_.class_name(), relation_name, _T(""), 1);
    YB_ASSERT(r != NULL);
    // Find FK value.
    Key fkey = obj->fk_value_for(*r);
    DataObject::Ptr master = obj->session_->get_lazy(fkey);
    link(shptr_get(master), obj, *r);
    return master;
}

bool DataObject::has_master(
    DataObject::Ptr obj, const String &relation_name)
{
    try {
        Ptr p = get_master(obj, relation_name);
        return true;
    }
    catch (const NullPK &) { }
    return false;
}

RelationObject *DataObject::get_slaves(const Relation &r)
{
    // Try to find relation object in master's relations
    RelationObject *ro = NULL;
    MasterRelations::iterator j = master_relations_.find(&r);
    if (j != master_relations_.end())
        ro = shptr_get(j->second);
    // Create one if it doesn't exist, master will own it
    if (!ro) {
        RelationObject::Ptr new_ro = RelationObject::create_new(r, this);
        master_relations_.insert(make_pair(&r, new_ro));
        ro = shptr_get(new_ro);
    }
    return ro;
}

RelationObject *DataObject::get_slaves(
    const String &relation_name)
{
    // Find relation in metadata.
    const Schema &schema(table_.schema());
    const Relation *r = schema.find_relation
        (table_.class_name(), relation_name, _T(""), 0);
    YB_ASSERT(r != NULL);
    return get_slaves(*r);
}

void
DataObject::populate_all_master_relations()
{
    Schema::RelMap::const_iterator
        it = table_.schema().rels_lower_bound(table_.class_name()),
        end = table_.schema().rels_upper_bound(table_.class_name());
    for (; it != end; ++it) {
        if (it->second->get_table(0) == &table_) {
            //session_->debug(_T("populate: ") +
            //        it->second->get_table(0)->name() + _T(" > ") +
            //        it->second->get_table(1)->name());
            RelationObject *ro = get_slaves(*it->second);
            // TODO: fix code duplication: DomainObject.h:123
            /*
            if (ro->master_object()->status() != DataObject::New &&
                ro->status() != RelationObject::Sync)
            {
                ro->lazy_load_slaves();
            }
            */
        }
    }
}

DataObject::~DataObject()
{}

size_t DataObject::fill_from_row(Row &r, size_t pos)
{
    size_t i = 0;
    for (; i < table_.size(); ++i) {
        values_[i].swap(r[pos + i].second);
        values_[i].fix_type(table_[i].type());
    }
    update_key();
    status_ = Sync;
    return pos + i;
}

void DataObject::refresh_slaves_fkeys()
{
    MasterRelations::iterator i = master_relations_.begin(),
        iend = master_relations_.end();
    for (; i != iend; ++i)
        i->second->refresh_slaves_fkeys();
}

void DataObject::refresh_master_fkeys()
{
    SlaveRelations::iterator i = slave_relations_.begin(),
        iend = slave_relations_.end();
    for (; i != iend; ++i)
        i->second->refresh_slaves_fkeys();
}

void DataObject::delete_object(DeletionMode mode, int depth)
{
    if (status_ == ToBeDeleted || status_ == Deleted)
        return;
    if (mode != DelUnchecked) {
        populate_all_master_relations();
    }
    if (session_) {
        std::ostringstream out;
        out << "delete_object mode=" << mode << " depth=" << depth
            << " status=" << status_ << "\n";
        dump_tree(out);
        session_->debug(WIDEN(out.str()));
    }
    if (mode != DelUnchecked) {
        delete_master_relations(DelDryRun, depth + 1);
    }
    if (mode != DelDryRun) {
        delete_master_relations(DelUnchecked, depth + 1);
        exclude_from_slave_relations();
        if (status_ == New) {
            status_ = Deleted;
        }
        else {
            //depth_ = depth; // why the hell I did that?
            status_ = ToBeDeleted;
        }
    }
}

void DataObject::delete_master_relations(DeletionMode mode, int depth)
{
    MasterRelations::iterator i = master_relations_.begin(),
        iend = master_relations_.end();
    for (; i != iend; ++i)
        i->second->delete_master(mode, depth);
    if (mode != DelDryRun) {
        MasterRelations empty;
        master_relations_.swap(empty);
    }
}

void DataObject::exclude_from_slave_relations()
{
    SlaveRelations::iterator i = slave_relations_.begin(),
        iend = slave_relations_.end();
    for (; i != iend; ++i)
        i->second->exclude_slave(this);
}

void DataObject::set_free_from(RelationObject *rel)
{
    const Strings &parts = rel->relation_info().fk_fields();
    Strings::const_iterator i = parts.begin(), end = parts.end();
    for (; i != end; ++i)
        set(*i, Value());
}

void DataObject::dump_tree(std::ostream &out, int level)
{
    for (int i = 0; i < level*4; ++i)
        out << " ";
    out << NARROW(table_.name()) << "(";
    for (int i = 0; i < values_.size(); ++i) {
        if (i)
            out << ", ";
        String s = _T("NULL");
        if (!values_[i].is_null())
            s = values_[i].as_string();
        out << NARROW(table_[i].name()) << ":"
            << NARROW(s);
    }
    out << ")\n";
    MasterRelations::iterator i = master_relations_.begin(),
        iend = master_relations_.end();
    for (; i != iend; ++i)
        i->second->dump_tree(out, level + 1);
}

void RelationObject::add_slave(DataObject::Ptr slave)
{
    pair<SlaveObjectsOrder::iterator, bool> r
        = slave_order_.insert(make_pair(shptr_get(slave), slave_objects_.size()));
    if (r.second) {
        slave_objects_.push_back(slave);
        slave->slave_relations().insert(make_pair(&relation_info(), this));
    }
}

void RelationObject::remove_slave(DataObject::Ptr slave)
{
    SlaveObjectsOrder::iterator it = slave_order_.find(shptr_get(slave));
    if (it != slave_order_.end()) {
        slave->slave_relations().erase(&relation_info());
        slave_objects_.erase(slave_objects_.begin() + it->second);
        slave_order_.erase(it);
    }
}

RelationObject::SlaveObjects::iterator RelationObject::find(DataObject *obj)
{
    SlaveObjectsOrder::iterator it = slave_order_.find(obj);
    if (slave_order_.end() == it)
        return slave_objects_.end();
    return slave_objects_.begin() + it->second;
}

void RelationObject::calc_depth(int d, DataObject *parent)
{
    SlaveObjects::iterator i = slave_objects_.begin(),
        iend = slave_objects_.end();
    for (; i != iend; ++i) {
        if (parent && shptr_get(*i) == parent)
            throw CycleDetected();
        (*i)->calc_depth(d, parent);
    }
}

void RelationObject::delete_master(DeletionMode mode, int depth)
{
    if (mode == DelDryRun && master_object_->session())
        lazy_load_slaves();
    if (relation_info_.cascade() == Relation::Nullify) {
        if (mode != DelDryRun) {
            SlaveObjects::iterator i = slave_objects_.begin(),
                iend = slave_objects_.end();
            for (; i != iend; ++i)
                (*i)->set_free_from(this);
            SlaveObjects empty;
            slave_objects_.swap(empty);
        }
    }
    else if (relation_info_.cascade() == Relation::Delete) {
        SlaveObjects slaves_copy = slave_objects_;
        SlaveObjects::iterator i = slaves_copy.begin(),
            iend = slaves_copy.end();
        for (; i != iend; ++i)
            (*i)->delete_object(mode, depth);
        if (mode != DelDryRun)
            YB_ASSERT(!slave_objects_.size());
    }
    else {
        if (slave_objects_.size() != 0)
            throw CascadeDeleteError(relation_info_);
    }
}

const Key RelationObject::gen_fkey() const
{
    const Table &master_tbl = relation_info_.table(0),
        &slave_tbl = relation_info_.table(1);
    const Strings &parts = relation_info_.fk_fields();
    Key fkey;
    fkey.second.reserve(master_tbl.pk_fields().size());
    fkey.first = slave_tbl.name();
    Strings::const_iterator i = parts.begin(), iend = parts.end(),
        j = master_tbl.pk_fields().begin(),
        jend = master_tbl.pk_fields().end();
    for (; i != iend && j != jend; ++i, ++j)
        fkey.second.push_back(make_pair(*i, master_object_->get(*j)));
    return fkey;
}

size_t RelationObject::count_slaves()
{
    if (status_ == Sync)
        return slave_objects_.size();
    YB_ASSERT(master_object_->session());
    Session &session = *master_object_->session();
    const Table &master_tbl = relation_info_.table(0),
        &slave_tbl = relation_info_.table(1);
    KeyFilter f(gen_fkey());
    Expression cols(_T("COUNT(*) RCNT"));
    RowsPtr result = session.engine()->
        select(cols, Expression(slave_tbl.name()), f);
    if (result->size() != 1)
        throw ObjectNotFoundByKey(_T("COUNT(*) FOR ") + slave_tbl.name() + _T("(")
                                  + f.get_sql() + _T(")"));
    return result->begin()->begin()->second.as_longint();
}

void RelationObject::lazy_load_slaves()
{
    if (status_ != Incomplete)
        return;
    YB_ASSERT(master_object_->session());
    Session &session = *master_object_->session();
    const Table &master_tbl = relation_info_.table(0),
        &slave_tbl = relation_info_.table(1);
    ExpressionList cols;
    Columns::const_iterator j = slave_tbl.begin(), jend = slave_tbl.end();
    for (; j != jend; ++j)
        cols << ColumnExpr(slave_tbl.name(), j->name());
    SelectExpr select_expr = SelectExpr(cols)
        .from_(Expression(slave_tbl.name()))
        .where_(KeyFilter(gen_fkey()));
    if (relation_info_.has_attr(1, _T("order-by")))
        select_expr.order_by_(
                Expression(relation_info_.attr(1, _T("order-by"))));
    SqlResultSet rs = session.engine()->select_iter(select_expr);
    SqlResultSet::iterator k = rs.begin(), kend = rs.end();
    for (; k != kend; ++k) {
        Key pkey;
        bool assigned_key = slave_tbl.mk_key(*k, pkey);
        DataObject::Ptr o = session.get_lazy(pkey);
        if (o->status() == DataObject::Ghost)
            o->fill_from_row(*k);
        if (o->status() != DataObject::ToBeDeleted
                && o->status() != DataObject::Deleted)
            DataObject::link(master_object_, o, relation_info_);
    }
    status_ = Sync;
}

void RelationObject::refresh_slaves_fkeys()
{
    const Table &master_tbl = relation_info_.table(0),
        &slave_tbl = relation_info_.table(1);
    const Strings &parts = relation_info_.fk_fields();
    SlaveObjects::iterator k = slave_objects_.begin(),
        kend = slave_objects_.end();
    for (; k != kend; ++k) {
        Strings::const_iterator i = parts.begin(), iend = parts.end(),
            j = master_tbl.pk_fields().begin(), jend = master_tbl.pk_fields().end();
        for (; i != iend && j != jend; ++i, ++j)
            (*k)->set(*i, master_object_->get(*j));
    }
}

void RelationObject::exclude_slave(DataObject *obj)
{
    SlaveObjects::iterator i = slave_objects_.begin(),
        iend = slave_objects_.end();
    for (; i != iend; ++i) {
        if (shptr_get(*i) == obj)
            break;
    }
    if (i != iend)
        slave_objects_.erase(i);
}

void RelationObject::dump_tree(std::ostream &out, int level)
{
    for (int i = 0; i < level*4; ++i)
        out << " ";
    out << "(cascade:" << relation_info_.cascade()
        << ", status:" << status_ 
        << ", in_session:" << (master_object_->session() != NULL)
        << ")[\n";
    SlaveObjects::iterator i = slave_objects_.begin(),
        iend = slave_objects_.end();
    for (; i != iend; ++i) {
        (*i)->dump_tree(out, level + 1);
    }
    for (int i = 0; i < level*4; ++i)
        out << " ";
    out << "]\n";
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
