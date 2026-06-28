#pragma once

/* Auto-save worlds every 5 minutes */
extern void autosave_worlds();

/* Backup database */
extern void backup_database();

/* Anti-cheat: detect speed hack in movement */
extern bool anticheat_speedhack(ENetEvent& event, state& state);

/* Rate limiter: returns true if packet should be dropped */
extern bool rate_limit_check(ENetEvent& event);
