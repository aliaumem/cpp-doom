#include "memblock.hpp"

#include "i_system.hpp"
#include "z_zone_ext.h"

namespace doom::zone {

void memblock_t::change_user(void** new_user)
{
    if (!is_valid())
        I_Error("Z_ChangeUser: Tried to change user for invalid block!");

    user  = new_user;
    *user = content();

    RecordChangeUser(content(), user);
}
}