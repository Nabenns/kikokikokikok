#include "pch.hpp"
#include <map>
#include "tools/create_dialog.hpp"
#include "safe_vault.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "database/peer.hpp"
#include "database/items.hpp"
#include "proton/Variant.hpp"

/* @note safe vault — password-protected storage per world position */
static std::map<std::string, std::string> sv_passwords; /* key -> password */
static std::map<std::string, std::vector<::slot>> sv_storage; /* key -> items */

static std::string sv_key(const std::string& wname, short tx, short ty)
{
    return std::format("{}_{}_{}", wname, tx, ty);
}

void action::safe_vault(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (!pPeer) return;

    ::hPipe hPipe{ header };
    const short tilex = (short)atoi(hPipe["tilex"].c_str());
    const short tiley = (short)atoi(hPipe["tiley"].c_str());

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    std::string key = sv_key(world->name, tilex, tiley);
    bool has_pass = sv_passwords.count(key) > 0;

    if (has_pass)
    {
        /* ask for password */
        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wSafe Vault``|left|5476|\\n"
                "add_spacer|small|\\n"
                "add_textbox|`oThis safe is locked. Enter the password to access it.``|left|\\n"
                "add_spacer|small|\\n"
                "add_text_input|sv_password|Password||20|\\n"
                "embed_data|tilex|{}\\n"
                "embed_data|tiley|{}\\n"
                "add_spacer|small|\\n"
                "add_quick_exit|\\n"
                "end_dialog|safe_vault_unlock|Close|Unlock|\\n",
                tilex, tiley
            )
        });
    }
    else
    {
        /* no password set — show setup dialog */
        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wSafe Vault - Setup``|left|5476|\\n"
                "add_spacer|small|\\n"
                "add_textbox|`oSet a password for your safe vault.``|left|\\n"
                "add_spacer|small|\\n"
                "add_text_input|sv_password|Set Password||20|\\n"
                "embed_data|tilex|{}\\n"
                "embed_data|tiley|{}\\n"
                "add_spacer|small|\\n"
                "add_quick_exit|\\n"
                "end_dialog|safe_vault_setup|Close|Set Password|\\n",
                tilex, tiley
            )
        });
    }
}
