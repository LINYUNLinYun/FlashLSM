#include "flashlsm/record.h"

namespace flashlsm {

bool Record::is_tombstone() const {
    return type == RecordType::Delete;
}

}  // namespace flashlsm
