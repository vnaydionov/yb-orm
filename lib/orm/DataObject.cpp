#include <orm/DataObject.h>

using namespace std;

namespace Yb {

void DataObject::load() {
    YB_ASSERT(session_ != NULL);
    ///
    status_ = Sync;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
