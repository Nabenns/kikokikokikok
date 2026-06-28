#pragma once

#include <string>

/* return JSON list of all online players */
extern std::string online_json();

/* broadcast a message to all connected players */
extern void broadcast_message(const std::string& admin_growid, const std::string& message);

/* set a player's role + write audit log */
extern std::string setrole_from_web(int target_uid, int new_role, int admin_uid);

/* write to audit_log table */
extern void audit_log_write(int admin_uid, const std::string& action, int target_uid, const std::string& detail);

/* return paginated JSON of all items from memory (items vector) */
extern std::string items_json(int page);

/* return JSON of all store items from memory (shouhin_tachi) */
extern std::string store_json();

/* return JSON of gServer_data config */
extern std::string config_json();

/* escape a string for JSON (quotes, backslashes, control chars) */
extern std::string json_escape(const std::string& s);