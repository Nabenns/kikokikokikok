#include "pch.hpp"
#include "tools/create_dialog.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/database.hpp"
#include "tools/string.hpp"

#include "mailbox.hpp"

/* ═══════ mailbox dialog return ═══════ */
void mailbox_return(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    std::string btn = hPipe["buttonClicked"];

    /* ── Inbox ── */
    if (btn == "mail_inbox")
    {
        /* SELECT unread mail for this peer */
        ::hStmt hStmt{ "SELECT id, sender_name, message, item_id, item_count FROM mailbox WHERE recipient_uid = ? AND is_read = 0 ORDER BY sent_at DESC" };
        MYSQL_BIND param = make_bind_in(pPeer->user_id);
        mysql_stmt_bind_param(hStmt.pStmt, &param);

        int mail_id{};
        std::string sender_name{}, message{};
        int item_id{}, item_count{};

        MYSQL_BIND results[5] = {
            make_bind_out(mail_id),
            make_bind_out(sender_name),
            make_bind_out(message),
            make_bind_out(item_id),
            make_bind_out(item_count)
        };
        mysql_stmt_bind_result(hStmt.pStmt, results);

        mysql_stmt_execute(hStmt.pStmt);
        mysql_stmt_store_result(hStmt.pStmt);

        std::string mail_list;
        while (!mysql_stmt_fetch(hStmt.pStmt))
        {
            /* trim null bytes */
            auto trim_null = [](std::string &s) {
                if (auto pos = s.find('\0'); pos != std::string::npos) s.resize(pos);
            };
            trim_null(sender_name);
            trim_null(message);

            std::string attach_info;
            if (item_id > 0)
                attach_info = std::format("`$[Attached: {}x {}]``", item_count, item_id);

            mail_list += std::format(
                "add_button|mail_read_{}|`w{}:`` `o{} {}|noflags|0|0|\n",
                mail_id, sender_name, message, attach_info
            );
        }

        if (mail_list.empty())
            mail_list = "add_textbox|`oYour inbox is empty.``|left|\n";

        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wInbox``|left|6146|\n"
                "add_spacer|small|\n"
                "{}"
                "add_spacer|small|\n"
                "end_dialog|mailbox|Close|Back|\n"
                "add_quick_exit|\n",
                mail_list
            )
        });
    }

    /* ── Compose ── */
    else if (btn == "mail_compose")
    {
        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wSend Mail``|left|6146|\n"
                "add_spacer|small|\n"
                "add_text_input|mail_target|Recipient GrowID:||18|\n"
                "add_text_input|mail_message|Message:||200|\n"
                "add_text_input|mail_item_id|Item ID to attach (0 for none):|0|5|\n"
                "add_text_input|mail_item_count|Item count:|0|5|\n"
                "add_spacer|small|\n"
                "end_dialog|mail_send|Close|Send|\n"
                "add_quick_exit|\n"
            )
        });
    }

    /* ── Read a specific mail (claim attached item) ── */
    else if (btn.starts_with("mail_read_"))
    {
        int mail_id = std::stoi(btn.substr(10));

        /* fetch the mail */
        ::hStmt hStmt{ "SELECT sender_name, message, item_id, item_count FROM mailbox WHERE id = ? AND recipient_uid = ?" };
        MYSQL_BIND params[2] = {
            make_bind_in(mail_id),
            make_bind_in(pPeer->user_id)
        };
        mysql_stmt_bind_param(hStmt.pStmt, params);

        std::string sender_name{}, message{};
        int item_id{}, item_count{};

        MYSQL_BIND results[4] = {
            make_bind_out(sender_name),
            make_bind_out(message),
            make_bind_out(item_id),
            make_bind_out(item_count)
        };
        mysql_stmt_bind_result(hStmt.pStmt, results);

        mysql_stmt_execute(hStmt.pStmt);
        if (mysql_stmt_fetch(hStmt.pStmt) != 0)
        { on::ConsoleMessage(event.peer, "`4Mail not found."); return; }

        auto trim_null = [](std::string &s) {
            if (auto pos = s.find('\0'); pos != std::string::npos) s.resize(pos);
        };
        trim_null(sender_name);
        trim_null(message);

        /* claim attached item */
        if (item_id > 0 && item_count > 0)
        {
            modify_item_inventory(event, ::slot(item_id, item_count));
            on::ConsoleMessage(event.peer, std::format("`oYou claimed `w{}x Item #{} ``from the mail!", item_count, item_id));
        }

        /* mark as read */
        ::hStmt hUpdate{ "UPDATE mailbox SET is_read = 1 WHERE id = ?" };
        MYSQL_BIND upd_param = make_bind_in(mail_id);
        mysql_stmt_bind_param(hUpdate.pStmt, &upd_param);
        mysql_stmt_execute(hUpdate.pStmt);

        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "add_label_with_icon|big|`wMail from {}``|left|6146|\n"
                "add_spacer|small|\n"
                "add_textbox|`o{}|left|\n"
                "add_spacer|small|\n"
                "{}"
                "add_spacer|small|\n"
                "end_dialog|mailbox|Close|OK|\n"
                "add_quick_exit|\n",
                sender_name,
                message,
                (item_id > 0) ? std::format("add_textbox|`$Attached: {}x Item #{} (claimed)``|left|\n", item_count, item_id) : std::string("")
            )
        });
    }
}
