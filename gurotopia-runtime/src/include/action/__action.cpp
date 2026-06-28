#include "pch.hpp"

#include "protocol.hpp"
#include "tankIDName.hpp"
#include "refresh_item_data.hpp"
#include "enter_game.hpp"

#include "dialog_return.hpp"
#include "friends.hpp"

#include "join_request.hpp"
#include "quit_to_exit.hpp"
#include "respawn.hpp"
#include "setSkin.hpp"
#include "input.hpp"
#include "drop.hpp"
#include "info.hpp"
#include "trash.hpp"
#include "wrench.hpp"
#include "itemfavourite.hpp"
#include "inventoryfavuitrigger.hpp"
#include "store.hpp"
#include "storenavigate.hpp"
#include "buy.hpp"
#include "trade.hpp"
#include "mailbox.hpp"
#include "wl_storage.hpp"

#include "storage_box.hpp"
#include "bulletin_board.hpp"
#include "fishing.hpp"
#include "cooking_oven.hpp"
#include "chemical_combiner.hpp"
#include "safe_vault.hpp"
#include "geiger_charger.hpp"
#include "magplant.hpp"
#include "donation_box.hpp"

#include "quit.hpp"

#include "__action.hpp"

std::unordered_map<std::string, std::function<void(ENetEvent&, const std::string&)>> action_pool
{
    {"protocol", std::bind(&action::protocol, std::placeholders::_1, std::placeholders::_2)},
    {"tankIDName", std::bind(&action::tankIDName, std::placeholders::_1, std::placeholders::_2)},
    {"action|refresh_item_data", std::bind(&action::refresh_item_data, std::placeholders::_1, std::placeholders::_2)}, 
    {"action|enter_game", std::bind(&action::enter_game, std::placeholders::_1, std::placeholders::_2)},
    
    {"action|dialog_return", std::bind(&action::dialog_return, std::placeholders::_1, std::placeholders::_2)},
    {"action|friends", std::bind(&action::friends, std::placeholders::_1, std::placeholders::_2)},

    {"action|join_request", std::bind(&action::join_request, std::placeholders::_1, std::placeholders::_2, "")},
    {"action|quit_to_exit", std::bind(&action::quit_to_exit, std::placeholders::_1, std::placeholders::_2, false)},
    {"action|respawn", std::bind(&action::respawn, std::placeholders::_1, std::placeholders::_2)},
    {"action|respawn_spike", std::bind(&action::respawn, std::placeholders::_1, std::placeholders::_2)},
    {"action|setSkin", std::bind(&action::setSkin, std::placeholders::_1, std::placeholders::_2)},
    {"action|input", std::bind(&action::input, std::placeholders::_1, std::placeholders::_2)},
    {"action|drop", std::bind(&action::drop, std::placeholders::_1, std::placeholders::_2)},
    {"action|info", std::bind(&action::info, std::placeholders::_1, std::placeholders::_2)},
    {"action|trash", std::bind(&action::trash, std::placeholders::_1, std::placeholders::_2)},
    {"action|wrench", std::bind(&action::wrench, std::placeholders::_1, std::placeholders::_2)},
    {"action|itemfavourite", std::bind(&action::itemfavourite, std::placeholders::_1, std::placeholders::_2)},
    {"action|inventoryfavuitrigger", std::bind(&action::inventoryfavuitrigger, std::placeholders::_1, std::placeholders::_2)},
    {"action|store", std::bind(&action::store, std::placeholders::_1, std::placeholders::_2)},
    {"action|storenavigate", std::bind(&action::storenavigate, std::placeholders::_1, std::placeholders::_2)},
    {"action|buy", std::bind(&action::buy, std::placeholders::_1, std::placeholders::_2, "")},
    {"action|trade", std::bind(&action::trade, std::placeholders::_1, std::placeholders::_2, "")},
    {"action|mailbox", std::bind(&action::mailbox, std::placeholders::_1, std::placeholders::_2)},
    {"action|wl_storage", std::bind(&action::wl_storage, std::placeholders::_1, std::placeholders::_2)},

    {"action|storage_box", std::bind(&action::storage_box, std::placeholders::_1, std::placeholders::_2)},
    {"action|bulletin_board", std::bind(&action::bulletin_board, std::placeholders::_1, std::placeholders::_2)},
    {"action|fishing", std::bind(&action::fishing, std::placeholders::_1, std::placeholders::_2)},
    {"action|cooking_oven", std::bind(&action::cooking_oven, std::placeholders::_1, std::placeholders::_2)},
    {"action|chemical_combiner", std::bind(&action::chemical_combiner, std::placeholders::_1, std::placeholders::_2)},
    {"action|safe_vault", std::bind(&action::safe_vault, std::placeholders::_1, std::placeholders::_2)},
    {"action|geiger_charger", std::bind(&action::geiger_charger, std::placeholders::_1, std::placeholders::_2)},
    {"action|magplant", std::bind(&action::magplant, std::placeholders::_1, std::placeholders::_2)},
    {"action|donation_box", std::bind(&action::donation_box, std::placeholders::_1, std::placeholders::_2)},

    {"action|quit", std::bind(&action::quit, std::placeholders::_1, std::placeholders::_2)}
};