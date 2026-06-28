#include "pch.hpp"

#include "popup.hpp"

#include "commands/gameplay.hpp"
#include "commands/achievements.hpp"
#include "action/trade.hpp"
#include "action/mailbox.hpp"
#include "action/wl_storage.hpp"
#include "on/ConsoleMessage.hpp"
#include "on/SetBux.hpp"

/* local helper — find an online peer by netID (static in other TUs) */
static ::peer* find_peer_by_netid_popup(short netid, ENetPeer** out_enet = nullptr)
{
    for (ENetPeer &p : std::span(host->peers, host->peerCount))
    {
        if (p.state == ENET_PEER_STATE_CONNECTED && p.data)
        {
            ::peer *pp = static_cast<::peer*>(p.data);
            if (pp->netid == netid)
            {
                if (out_enet) *out_enet = &p;
                return pp;
            }
        }
    }
    return nullptr;
}

void popup(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    /* ═══════ EXISTING HANDLERS ═══════ */

    if (hPipe["buttonClicked"] == "my_worlds")
    {
        auto section = [](const auto& range) 
        {
            std::string result;
            for (const auto &name : range)
                if (!name.empty())
                    result.append(std::format("add_button|{0}|{0}|noflags|0|0|\n", name));
            return result;
        };
        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "start_custom_tabs|\n"
                "add_custom_button|myWorldsUiTab_0|image:interface/large/btn_tabs2.rttex;image_size:228,92;frame:1,0;width:0.15;|\n"
                "add_custom_button|myWorldsUiTab_1|image:interface/large/btn_tabs2.rttex;image_size:228,92;frame:0,1;width:0.15;|\n"
                "add_custom_button|myWorldsUiTab_2|image:interface/large/btn_tabs2.rttex;image_size:228,92;frame:0,2;width:0.15;|\n"
                "end_custom_tabs|\n"
                "add_label|big|Locked Worlds|left|0|\n"
                "add_spacer|small|\n"
                "add_textbox|Place a World Lock in a world to lock it. Break your World Lock to unlock a world.|left|\n"
                "add_spacer|small|\n"
                "{}\n"
                "add_spacer|small|\n"
                "end_dialog|worlds_list||Back|\n"
                "add_quick_exit|\n",
                section(pPeer->my_worlds)
            )
        });
    }
    else if (hPipe["buttonClicked"] == "billboard_edit")
    {
        auto item = std::ranges::find(items, pPeer->billboard.id, &::item::id);

        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wTrade Billboard``|left|8282|\n"
                "add_spacer|small|\n"
                "{}"
                "add_item_picker|billboard_item|`wSelect Billboard Item``|Choose an item to put on your billboard!|\n"
                "add_spacer|small|\n"
                "add_checkbox|billboard_toggle|`$Show Billboard``|{}\n"
                "add_checkbox|billboard_buying_toggle|`$Is Buying``|{}\n"
                "add_text_input|setprice|Price of item:|{}|5|\n"
                "add_checkbox|chk_peritem|World Locks per Item|{}\n"
                "add_checkbox|chk_perlock|Items per World Lock|{}\n"
                "add_spacer|small|\n"
                "end_dialog|billboard_edit|Close|Update|\n",
                (pPeer->billboard.id == 0) ? 
                    "" : 
                    std::format("add_label_with_icon|small|`w{}``|left|{}|\n", item->raw_name, pPeer->billboard.id),
                to_char(pPeer->billboard.show),
                to_char(pPeer->billboard.isBuying),
                pPeer->billboard.price,
                to_char(pPeer->billboard.perItem),
                to_char(!pPeer->billboard.perItem)
            )
        });
    }
    else if (hPipe["buttonClicked"] == "seed_diary_customization")
    {
        send_varlist(event.peer, { "OnDialogRequestRML", "show_seed_diary_ui" });
    }

    /* ═══════ SELF WRENCH BUTTONS ═══════ */

    else if (hPipe["buttonClicked"] == "bonus")
    {
        command::daily(event, "");
    }
    else if (hPipe["buttonClicked"] == "goals")
    {
        command::quest_list(event, "");
    }
    else if (hPipe["buttonClicked"] == "title_edit")
    {
        command::title(event, "");
    }
    else if (hPipe["buttonClicked"] == "alist")
    {
        achievements(event, "");
    }
    else if (hPipe["buttonClicked"] == "pets")
    {
        command::pet_info(event, "");
    }
    else if (hPipe["buttonClicked"] == "open_worldlock_storage")
    {
        action::wl_storage(event, "");
    }
    else if (hPipe["buttonClicked"] == "wardrobe_customization")
    {
        /* build clothing slot list from peer's clothing array */
        std::string slots_text;
        static constexpr std::array<const char*, 10zu> slot_names = {
            "Hair", "Shirt", "Pants", "Feet", "Face",
            "Hand", "Back", "Mask", "Necklace", "Anches"
        };
        for (std::size_t i = 0; i < pPeer->clothing.size(); ++i)
        {
            auto item = std::ranges::find(items, (short)pPeer->clothing[i], &::item::id);
            std::string item_name = (item != items.end()) ? item->raw_name : "Empty";
            slots_text += std::format("add_textbox|`w{}:`` `o{}|left|\n", slot_names[i], item_name);
        }
        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wWardrobe``|left|5988\n"
                "add_spacer|small|\n"
                "{}"
                "add_spacer|small|\n"
                "add_textbox|`oCustomize your wardrobe from the inventory screen.``|left|\n"
                "add_quick_exit|\n"
                "end_dialog|wardrobe|Close||\n",
                slots_text
            )
        });
    }
    else if (hPipe["buttonClicked"] == "set_online_status")
    {
        send_varlist(event.peer, {
            "OnDialogRequest",
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wOnline Status``|left|2416\n"
            "add_spacer|small|\n"
            "add_textbox|`oSet your online status. Other players will see this.``|left|\n"
            "add_spacer|small|\n"
            "add_button|status_online|`2Online``|noflags|0|0|\n"
            "add_button|status_away|`eAway``|noflags|0|0|\n"
            "add_button|status_dnd|`4Do Not Disturb``|noflags|0|0|\n"
            "add_spacer|small|\n"
            "add_quick_exit|\n"
            "end_dialog|online_status|Close|Cancel|\n"
        });
    }
    else if (hPipe["buttonClicked"] == "open_personlize_profile")
    {
        u_short lvl = pPeer->level.front();
        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_player_info|`{}{}``|{}|{}|{}\n"
                "add_spacer|small|\n"
                "add_textbox|`oGems: `w{}``|left|\n"
                "add_textbox|`oLevel: `w{}``|left|\n"
                "add_textbox|`oTitle: `w{}``|left|\n"
                "add_textbox|`oWorld: `w{}``|left|\n"
                "add_spacer|small|\n"
                "add_quick_exit|\n"
                "end_dialog|profile|Close|Continue|\n",
                pPeer->prefix, pPeer->growid,
                std::to_string(lvl), pPeer->level.back(), 50 * (lvl * lvl + 2),
                pPeer->gems, lvl,
                pPeer->title.empty() ? "None" : pPeer->title,
                pPeer->recent_worlds.back()
            )
        });
    }
    else if (hPipe["buttonClicked"] == "emojis")
    {
        send_varlist(event.peer, {
            "OnDialogRequest",
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wGrowmojis``|left|758\n"
            "add_spacer|small|\n"
            "add_textbox|`oAvailable growmojis:``|left|\n"
            "add_spacer|small|\n"
            "add_textbox|`w:)`` `w:(`` `w:D`` `w:P`` `w;P`` `w<3`` `w:O`` `w:oo`` `w:cry`` `wlove`` `wcool`` `wmad`` `wshock`` `wrolleyes`` `wslant``|left|\n"
            "add_spacer|small|\n"
            "add_textbox|`oType these in chat to use growmojis!``|left|\n"
            "add_quick_exit|\n"
            "end_dialog|growmojis|Close|OK|\n"
        });
    }
    else if (hPipe["buttonClicked"] == "marvelous_missions")
    {
        send_varlist(event.peer, {
            "OnDialogRequest",
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wMarvelous Missions``|left|3418\n"
            "add_spacer|small|\n"
            "add_textbox|`oMarvelous Missions are special tasks that reward you with unique items.``|left|\n"
            "add_spacer|small|\n"
            "add_textbox|`oNo active missions. Check back later!``|left|\n"
            "add_spacer|small|\n"
            "add_quick_exit|\n"
            "end_dialog|marvelous_missions|Close|OK|\n"
        });
    }
    else if (hPipe["buttonClicked"] == "trade_scan")
    {
        /* scan for active trades in current world */
        std::string trade_list;
        int count = 0;
        peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& peer) {
            ::peer *pp = static_cast<::peer*>(peer.data);
            if (pp->user_id != pPeer->user_id)
            {
                trade_list += std::format("add_textbox|`w{}`` - `oAvailable for trade``|left|\n", pp->growid);
                ++count;
            }
        });
        if (count == 0)
            trade_list = "add_textbox|`oNo other players in this world.``|left|\n";

        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wTrade Scan``|left|5988\n"
                "add_spacer|small|\n"
                "add_textbox|`oPlayers in `w{}`` available for trade:``|left|\n"
                "add_spacer|small|\n"
                "{}"
                "add_spacer|small|\n"
                "add_quick_exit|\n"
                "end_dialog|trade_scan|Close|OK|\n",
                pPeer->recent_worlds.back(),
                trade_list
            )
        });
    }
    else if (hPipe["buttonClicked"] == "wrench_customization")
    {
        send_varlist(event.peer, {
            "OnDialogRequest",
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wWrench Customization``|left|5988\n"
            "add_spacer|small|\n"
            "add_textbox|`oCustomize your wrench menu appearance.``|left|\n"
            "add_spacer|small|\n"
            "add_textbox|`oNo customization options available yet.``|left|\n"
            "add_spacer|small|\n"
            "add_quick_exit|\n"
            "end_dialog|wrench_custom|Close|OK|\n"
        });
    }
    else if (hPipe["buttonClicked"] == "notebook_edit")
    {
        send_varlist(event.peer, {
            "OnDialogRequest",
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wNotebook``|left|5732\n"
            "add_spacer|small|\n"
            "add_text_input|notebook_content|Your notes:||500|\n"
            "add_spacer|small|\n"
            "add_quick_exit|\n"
            "end_dialog|notebook_save|Close|Save|\n"
        });
    }
    else if (hPipe["buttonClicked"] == "renew_pvp_license")
    {
        /* check if peer already has PvP license (item 2422) */
        bool has_license = false;
        for (const ::slot &s : pPeer->slots)
        {
            if (s.id == 2422 && s.count > 0)
            {
                has_license = true;
                break;
            }
        }
        if (has_license)
        {
            on::ConsoleMessage(event.peer, "You already have a Card Battle License!");
        }
        else
        {
            modify_item_inventory(event, ::slot(2422, 1));
            on::ConsoleMessage(event.peer, "`2You received a Card Battle License!``");
        }
    }

    /* ═══════ OTHER PLAYER WRENCH BUTTONS ═══════ */

    else if (hPipe["buttonClicked"] == "trade")
    {
        /* action::trade parses netID from header itself */
        action::trade(event, "");
    }
    else if (hPipe["buttonClicked"] == "sendpm")
    {
        short netid = (short)atoi(hPipe["netID"].c_str());
        ENetPeer *target_enet = nullptr;
        ::peer *target_peer = find_peer_by_netid_popup(netid, &target_enet);
        if (!target_peer || !target_enet)
        {
            on::ConsoleMessage(event.peer, "`4Player not found or offline.");
            return;
        }
        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wSend Message to {}``|left|6146\n"
                "add_spacer|small|\n"
                "add_text_input|pm_content|Message:||200|\n"
                "add_spacer|small|\n"
                "add_quick_exit|\n"
                "end_dialog|pm_send_{}|Close|Send|\n",
                target_peer->growid,
                target_peer->user_id
            )
        });
    }
    else if (hPipe["buttonClicked"] == "friend_add")
    {
        short netid = (short)atoi(hPipe["netID"].c_str());
        ENetPeer *target_enet = nullptr;
        ::peer *target_peer = find_peer_by_netid_popup(netid, &target_enet);
        if (!target_peer || !target_enet)
        {
            on::ConsoleMessage(event.peer, "`4Player not found or offline.");
            return;
        }
        command::friend_add(event, std::format(" {}", target_peer->growid));
    }
    else if (hPipe["buttonClicked"] == "ignore_player")
    {
        short netid = (short)atoi(hPipe["netID"].c_str());
        ENetPeer *target_enet = nullptr;
        ::peer *target_peer = find_peer_by_netid_popup(netid, &target_enet);
        if (!target_peer || !target_enet)
        {
            on::ConsoleMessage(event.peer, "`4Player not found or offline.");
            return;
        }
        command::ignore(event, std::format(" {}", target_peer->growid));
    }
    else if (hPipe["buttonClicked"] == "show_clothes")
    {
        short netid = (short)atoi(hPipe["netID"].c_str());
        ENetPeer *target_enet = nullptr;
        ::peer *target_peer = find_peer_by_netid_popup(netid, &target_enet);
        if (!target_peer || !target_enet)
        {
            on::ConsoleMessage(event.peer, "`4Player not found or offline.");
            return;
        }
        static constexpr std::array<const char*, 10zu> slot_names = {
            "Hair", "Shirt", "Pants", "Feet", "Face",
            "Hand", "Back", "Mask", "Necklace", "Anches"
        };
        std::string clothes_text;
        for (std::size_t i = 0; i < target_peer->clothing.size(); ++i)
        {
            auto item = std::ranges::find(items, (short)target_peer->clothing[i], &::item::id);
            std::string item_name = (item != items.end()) ? item->raw_name : "Empty";
            clothes_text += std::format("add_textbox|`w{}:`` `o{}|left|\n", slot_names[i], item_name);
        }
        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`w{}'s Clothes``|left|5988\n"
                "add_spacer|small|\n"
                "{}"
                "add_spacer|small|\n"
                "add_quick_exit|\n"
                "end_dialog|show_clothes|Close|OK|\n",
                target_peer->growid,
                clothes_text
            )
        });
    }
    else if (hPipe["buttonClicked"] == "report_player")
    {
        short netid = (short)atoi(hPipe["netID"].c_str());
        ENetPeer *target_enet = nullptr;
        ::peer *target_peer = find_peer_by_netid_popup(netid, &target_enet);
        if (!target_peer || !target_enet)
        {
            on::ConsoleMessage(event.peer, "`4Player not found or offline.");
            return;
        }
        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wReport {}``|left|5644\n"
                "add_spacer|small|\n"
                "add_textbox|`oSelect a category for your report:``|left|\n"
                "add_spacer|small|\n"
                "add_button|report_scam|`oScamming``|noflags|0|0|\n"
                "add_button|report_harass|`oHarassment``|noflags|0|0|\n"
                "add_button|report_cheat|`oCheating``|noflags|0|0|\n"
                "add_button|report_inappropriate|`oInappropriate Behavior``|noflags|0|0|\n"
                "add_spacer|small|\n"
                "add_text_input|report_details|Additional details:||500|\n"
                "add_spacer|small|\n"
                "add_quick_exit|\n"
                "end_dialog|report_submit_{}|Close|Submit|\n",
                target_peer->growid,
                target_peer->user_id
            )
        });
    }
}