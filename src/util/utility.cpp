// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBUTIL_SOURCE

#include "util/utility.h"

namespace Yb {

RefCountBase::RefCountBase()
    : ref_count_(0)
{}

RefCountBase::~RefCountBase()
{}

void RefCountBase::add_ref()
{
    ++ref_count_;
}

void RefCountBase::release()
{
    if (!--ref_count_)
        delete this;
}

NonCopyable::NonCopyable()
{}

NonCopyable::~NonCopyable()
{}

} // end of namespace Yb

// vim:ts=4:sts=4:sw=4:et:
