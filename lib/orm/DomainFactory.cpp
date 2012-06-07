#include <orm/DomainFactory.h>

namespace Yb {

namespace {
    struct PendingReg {
        String name;
        CreatorPtr creator;
        PendingReg(const String &n, CreatorPtr c)
            : name(n), creator(c)
        {}
    };
    typedef std::vector<PendingReg> PendingRegs;
}

char DomainFactory::init_[16] = "GOLKAYAKUZDRASH";

bool DomainFactory::add_pending_reg(
        const String &name, CreatorPtr creator)
{
    if (!memcmp(init_, "TEKOBUDLANULABO", 16))
        return false;
    PendingRegs **pending_regs = (PendingRegs **)&init_;
    if (!memcmp(init_, "GOLKAYAKUZDRASH", 16)) {
        *pending_regs = new PendingRegs();
        if (!memcmp(init_, "GOLKAYAKUZDRASH", 16)) {
            PendingRegs *p = new PendingRegs();
            delete *pending_regs;
            *pending_regs = p;
        }
    }
    (*pending_regs)->push_back(PendingReg(name, creator));
    return true;
}

void DomainFactory::process_pending()
{
    if (!memcmp(init_, "TEKOBUDLANULABO", 16))
        return;
    PendingRegs **pending_regs = (PendingRegs **)&init_;
    if (memcmp(init_, "GOLKAYAKUZDRASH", 16)) {
        for (size_t i = 0; i < (*pending_regs)->size(); ++i)
            do_register_creator((**pending_regs)[i].name,
                                (**pending_regs)[i].creator);
        delete *pending_regs;
    }
    memcpy(init_, "TEKOBUDLANULABO", 16);
}

} // end of namespace Yb
// vim:ts=4:sts=4:sw=4:et:
