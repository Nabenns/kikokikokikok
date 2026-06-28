#include "pch.hpp"

#include "popup.hpp"
#include "drop_item.hpp"
#include "trash_item.hpp"
#include "find_item.hpp"
#include "gateway_edit.hpp"
#include "billboard_edit.hpp"
#include "lock_edit.hpp"
#include "create_blast.hpp"
#include "socialportal.hpp"
#include "megaphone.hpp"
#include "title_select.hpp"
#include "trade_request.hpp"
#include "trade_active.hpp"
#include "mailbox.hpp"
#include "guild_invite.hpp"
#include "wl_storage.hpp"

#include "storage_box.hpp"
#include "bulletin_board.hpp"
#include "fishing.hpp"
#include "cooking_oven.hpp"
#include "chemical_combiner.hpp"
#include "safe_vault.hpp"
#include "geiger_charger.hpp"
#include "magplant.hpp"
#include "vending.hpp"

#include "__dialog_return.hpp"

std::unordered_map<std::string, std::function<void(ENetEvent &, const ::hPipe &)>> dialog_return_pool
{
    {"popup", std::bind(&popup, std::placeholders::_1, std::placeholders::_2)},

    {"drop_item", std::bind(&drop_item, std::placeholders::_1, std::placeholders::_2)},
    {"trash_item", std::bind(&trash_item, std::placeholders::_1, std::placeholders::_2)},
    {"find_item", std::bind(&find_item, std::placeholders::_1, std::placeholders::_2)},

    {"gateway_edit", std::bind(&gateway_edit, std::placeholders::_1, std::placeholders::_2)},
    {"door_edit", std::bind(&gateway_edit, std::placeholders::_1, std::placeholders::_2)},
    {"sign_edit", std::bind(&gateway_edit, std::placeholders::_1, std::placeholders::_2)},

    {"billboard_edit", std::bind(&billboard_edit, std::placeholders::_1, std::placeholders::_2)},
    {"lock_edit", std::bind(&lock_edit, std::placeholders::_1, std::placeholders::_2)},

    {"create_blast", std::bind(&create_blast, std::placeholders::_1, std::placeholders::_2)},
    {"socialportal", std::bind(&socialportal, std::placeholders::_1, std::placeholders::_2)},
    {"megaphone", std::bind(&megaphone, std::placeholders::_1, std::placeholders::_2)},

    {"title_select", std::bind(&title_select, std::placeholders::_1, std::placeholders::_2)},
    {"trade_request", std::bind(&trade_request, std::placeholders::_1, std::placeholders::_2)},
    {"trade_active", std::bind(&trade_active, std::placeholders::_1, std::placeholders::_2)},
    {"mailbox_open", std::bind(&mailbox_return, std::placeholders::_1, std::placeholders::_2)},
    {"mail_send", std::bind(&mailbox_return, std::placeholders::_1, std::placeholders::_2)},
    {"mail_read", std::bind(&mailbox_return, std::placeholders::_1, std::placeholders::_2)},
    {"guild_invite", std::bind(&guild_invite, std::placeholders::_1, std::placeholders::_2)},
    {"wl_deposit", std::bind(&wl_deposit_confirm, std::placeholders::_1, std::placeholders::_2)},
    {"wl_withdraw", std::bind(&wl_withdraw_confirm, std::placeholders::_1, std::placeholders::_2)},

    {"storage_box", std::bind(&storage_box_return, std::placeholders::_1, std::placeholders::_2)},
    {"bulletin_edit", std::bind(&bulletin_edit, std::placeholders::_1, std::placeholders::_2)},
    {"fishing_wait", std::bind(&fishing_return, std::placeholders::_1, std::placeholders::_2)},
    {"cooking_oven", std::bind(&cooking_oven_return, std::placeholders::_1, std::placeholders::_2)},
    {"chemical_combiner", std::bind(&chemical_combiner_return, std::placeholders::_1, std::placeholders::_2)},
    {"safe_vault_setup", std::bind(&safe_vault_return, std::placeholders::_1, std::placeholders::_2)},
    {"safe_vault_unlock", std::bind(&safe_vault_return, std::placeholders::_1, std::placeholders::_2)},
    {"safe_vault_deposit", std::bind(&safe_vault_return, std::placeholders::_1, std::placeholders::_2)},
    {"geiger_ready", std::bind(&geiger_charger_return, std::placeholders::_1, std::placeholders::_2)},
    {"geiger_charging", std::bind(&geiger_charger_return, std::placeholders::_1, std::placeholders::_2)},
    {"magplant", std::bind(&magplant_return, std::placeholders::_1, std::placeholders::_2)},
    {"vending", std::bind(&vending_return, std::placeholders::_1, std::placeholders::_2)},
};