// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#include "orm/domain_factory.h"

namespace Yb {

namespace {
typedef std::pair<String, CreatorPtr> PendingReg;
typedef std::vector<PendingReg> PendingRegs;
}

NoCreator::NoCreator(const String &entity_name)
    : ORMError(_T("Domain object creator for entity '") +
               entity_name + _T("' not found"))
{}

ICreator::~ICreator() {}

int DomainFactory::init_flag_ = 0;
void *DomainFactory::pending_ = NULL;

void
DomainFactory::register_creator(const String &name, CreatorPtr creator)
{
    if (!add_pending_reg(name, creator))
        do_register_creator(name, creator);
}

DomainObjectPtr
DomainFactory::create_object(Session &session,
        const String &entity_name, LongInt id)
{
    process_pending();
    Map::const_iterator it = creator_map_.find(entity_name);
    if (it == creator_map_.end())
        throw NoCreator(entity_name);
    return it->second->create(session, id);
}

bool
DomainFactory::add_pending_reg(
        const String &name, CreatorPtr creator)
{
    if (init_flag_)
        return false;
    PendingRegs *&pending_regs =
        *reinterpret_cast<PendingRegs **> (&pending_);
    if (!pending_regs)
        pending_regs = new PendingRegs();
    pending_regs->push_back(PendingReg(name, creator));
    return true;
}

void
DomainFactory::process_pending()
{
    if (init_flag_)
        return;
    init_flag_ = 1;
    PendingRegs *&pending_regs =
        *reinterpret_cast<PendingRegs **> (&pending_);
    if (pending_regs) {
        for (size_t i = 0; i < pending_regs->size(); ++i)
            do_register_creator(
                    (*pending_regs)[i].first,
                    (*pending_regs)[i].second);
        delete pending_regs;
        pending_regs = NULL;
    }
}

void
DomainFactory::do_register_creator(const String &name, CreatorPtr creator)
{
    creator_map_.insert(Map::value_type(name, creator));
}

} // end of namespace Yb
// vim:ts=4:sts=4:sw=4:et:
