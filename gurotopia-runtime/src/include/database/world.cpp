#include "pch.hpp"
#include "tools/ransuu.hpp"
#include "on/ConsoleMessage.hpp"
#include "database/database.hpp"

#include "world.hpp"

using namespace std::chrono;
using namespace std::literals::chrono_literals; // @note for 'ms' 's' (millisec, seconds)

world::world(const std::string& name) 
{
    this->name = name;
}

std::vector<world> worlds;

void send_action(ENetPeer& p, const std::string& action, const std::string& str) 
{
    const std::string &fmt_action = std::format("action|{}\n", action);
    std::vector<u_char> data(sizeof(int) + fmt_action.length() + str.length(), 0x00);
    
    data[0] = 3; // @note NET_MESSAGE_GAME_MESSAGE
    {
        const u_char *i8 = reinterpret_cast<const u_char*>(fmt_action.c_str());
        for (std::size_t i = 0zu; i < fmt_action.length(); ++i)
            data[sizeof(int) + i] = i8[i];
    }
    if (!str.empty())
    {
        const u_char *i8 = reinterpret_cast<const u_char*>(str.c_str());
        for (std::size_t i = 0zu; i < str.length(); ++i)
            data[sizeof(int) + fmt_action.length() + i] = i8[i];
    }
    
    enet_peer_send(&p, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
}

void send_data(ENetPeer &peer, const std::vector<u_char> &&data)
{
    ENetPacket *packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE);
    if (packet == nullptr || packet->dataLength < sizeof(::state)) return;

    enet_peer_send(&peer, 1, packet);
}

void state_visuals(ENetPeer &peer, state &&state) 
{
    ::peer *pPeer = static_cast<::peer*>(peer.data);

    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer &p) 
    {
        send_data(p, compress_state(state));
    });
}

void tile_apply_damage(ENetEvent& event, state state, block &block, u_int value)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    (block.fg == 0) ? ++block.hits[1] : ++block.hits[0];
    state.type = (value << 24) | 0x000008; // @note 0x{}000008
    state.id = 6; // @note idk exactly
    state.netid = pPeer->netid;
	state_visuals(*event.peer, std::move(state));
}

u_short modify_item_inventory(ENetEvent& event, ::slot slot)
{   
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    ::state state{.id = slot.id};
    if (slot.count < 0) state.type = (slot.count*-1 << 16) | 0x000d; // @noote 0x00{}000d
    else                state.type = (slot.count    << 24) | 0x000d; // @noote 0x{}00000d
    state_visuals(*event.peer, std::move(state));

    return pPeer->emplace(::slot(slot.id, slot.count));
}

int item_change_object(ENetEvent& event, ::slot slot, const ::pos& pos, signed uid) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    ::state state{.type = 0x0e}; // @note PACKET_ITEM_CHANGE_OBJECT

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return -1;

    auto object = std::ranges::find_if(world->objects, [&](const ::object &object) {
        return uid == 0 && object.id == slot.id && (object.pos.by_32(true) == pos.by_32(true));
    });

    if (object != world->objects.end()) // @note merge drop
    {
        object->count += slot.count;
        state.netid = 0xfffffffd;
        state.uid = object->uid;
        state.count = static_cast<float>(object->count);
        state.id =  object->id;
        state.pos = object->pos;
    }
    else if (slot.count == 0 || slot.id == 0) // @note remove drop
    {
        state.netid = pPeer->netid;
        state.uid = 0xffffffff;
        state.id = uid;
    }
    else // @note add new drop
    {
        auto it = world->objects.emplace_back(::object(slot.id, slot.count, pos, ++world->last_object_uid)); // @note a iterator ahead of time
        state.netid = 0xffffffff;
        state.uid = it.uid;
        state.count = static_cast<float>(slot.count);
        state.id = it.id;
        state.pos = pos;
    }
    state_visuals(*event.peer, std::move(state));
    return state.uid;
}

void add_drop(ENetEvent &event, ::slot im, ::pos pos)
{
    ransuu ransuu;
    item_change_object(event, {im.id, im.count},
    {
        pos.x + ransuu[{0, 16}],
        pos.y + ransuu[{0, 16}]
    });
}

void send_tile_update(ENetEvent &event, state state, block &block, world &world) 
{
    state.type = 05; // @note PACKET_SEND_TILE_UPDATE_DATA
    state.peer_state = peer_state::S_EXTENDED;
    std::vector<u_char> data = compress_state(state);

    short pos = sizeof(::state); // @note start after state bytes (as every packet has)
    data.resize(pos + 8zu); // @note {2} {2} 00 00 00 00
    
    *reinterpret_cast<short*>(&data[pos]) = block.fg; pos += sizeof(short);
    *reinterpret_cast<short*>(&data[pos]) = block.bg; pos += sizeof(short);
    pos += sizeof(short);
    
    data[pos++] = block.state[2] ;
    data[pos++] = block.state[3];
    auto item = std::ranges::find(items, block.fg, &::item::id);
    switch (item->type)
    {
        case type::LOCK:
        {
            if (!is_tile_lock(block.fg)) world.is_public = (block.state[2] & S_PUBLIC); // @note check if world lock has S_PUBLIC flag, i will change this later

            std::size_t admins = std::ranges::count_if(world.admin, std::identity{});
            data.resize(data.size() + 1zu + 1zu + 4zu + 4zu + 4zu + (admins * 4zu));

            data[pos++] = 0x03;
            data[pos++] = world.lock_state;
            *reinterpret_cast<int*>(&data[pos]) = world.owner; pos += sizeof(int);
            *reinterpret_cast<int*>(&data[pos]) = admins; pos += sizeof(int);
            /* @todo admin list */
            break;
        }
        case type::DOOR:
        case type::PORTAL:
        {
            short len = block.label.length();
            data.resize(pos + 1zu + 2zu + len + 1zu); // @note 01 {2} {} 0 0

            data[pos++] = 0x01;
            
            *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
            for (const char &c : block.label) data[pos++] = c;
            data[pos++] = '\0';
            break;
        }
        case type::SIGN:
        {
            short len = block.label.length();
            data.resize(pos + 1zu + 2zu + len + 4zu); // @note 02 {2} {} ff ff ff ff

            data[pos++] = 0x02;

            *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
            for (const char &c : block.label) data[pos++] = c;
            *reinterpret_cast<int*>(&data[pos]) = 0xffffffff; pos += sizeof(int);
            break;
        }
        case type::SEED:
        {
            data.resize(pos + 1zu + 5zu);

            data[pos++] = 0x04;
            *reinterpret_cast<int*>(&data[pos]) = (steady_clock::now() - block.tick) / 1s; pos += sizeof(int);
            data[pos++] = 0x03; // @note fruit on tree
            break;
        }
        case type::PROVIDER:
        {
            data.resize(pos + 5zu);

            data[pos++] = 0x09;
            *reinterpret_cast<int*>(&data[pos]) = (steady_clock::now() - block.tick) / 1s; pos += sizeof(int);
            break;
        }
        case DISPLAY_BLOCK:
        {
            data.resize(pos + 1zu + 4zu);
            auto display = std::ranges::find(world.displays, state.punch, &::display::pos);

            data[pos++] = 0x17;
            *reinterpret_cast<int*>(&data[pos]) = display->id; pos += sizeof(int);
            break;
        }
    }

    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&](ENetPeer& p) 
    {
        send_data(p, std::move(data));
    });
}

void remove_fire(ENetEvent &event, state state, block &block, world& world)
{
    state_visuals(*event.peer, ::state{
        .type = 0x11, // @note PACKET_SEND_PARTICLE_EFFECT
        .pos = state.punch.by_32(),
        .speed = ::pos{ 0x00000000, 0x95 }
    });

    block.state[3] &= ~S_FIRE;
    send_tile_update(event, state, block, world);

    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (++pPeer->fires_removed % 100 == 0) 
    {
        on::ConsoleMessage(event.peer, "`oI'm so good at fighting fires, I rescused this `2Highly Combustible Box``!");
        modify_item_inventory(event, {3090/*Combustible Box*/, 1});
    }

    pPeer->add_xp(event, 1);
}

void generate_world(world &world, const std::string& name)
{
    ransuu ransuu;
    u_short main_door = ransuu[{2, 100 * 60 / 100 - 4}];
    std::vector<block> blocks(100 * 60, block{0, 0});
    
    /* use name hash for deterministic randomness per world */
    std::size_t seed = 0;
    for (char c : name) seed = seed * 31 + c;
    std::srand((unsigned int)seed);

    /* grass surface height varies slightly */
    int base_surface = 37;
    int surface[100];
    for (int x = 0; x < 100; ++x)
    {
        surface[x] = base_surface + (int)(std::sin(x * 0.3f) * 3 + std::sin(x * 0.7f) * 1.5f);
        if (surface[x] < 34) surface[x] = 34;
        if (surface[x] > 42) surface[x] = 42;
    }
    
    for (std::size_t i = 0zu; i < blocks.size(); ++i)
    {
        ::block &block = blocks[i];
        int x = (int)(i % 100);
        int y = (int)(i / 100);
        
        if (y >= surface[x])
        {
            block.bg = 14; // @note cave background
            if (y > surface[x] && y < surface[x] + 12 && ransuu[{0, 38}] <= 1)
                block.fg = 10; // rock vein
            else if (y > surface[x] + 12 && y < surface[x] + 16 && ransuu[{0, 8}] < 3)
                block.fg = 4; // lava
            else if (y >= surface[x] + 16)
                block.fg = 8; // bedrock
            else
                block.fg = 2; // dirt
        }
        
        if (i == cord(main_door, surface[main_door] - 1))
            block.fg = 6, block.label = "EXIT"; // @note main door
        else if (i == cord(main_door, surface[main_door]))
            block.fg = 8; // @note bedrock (below main door)
    }

    /* plant trees on grass surface */
    for (int x = 3; x < 97; ++x)
    {
        int sx = surface[x];
        if (x == main_door || x == main_door - 1 || x == main_door + 1) continue;
        if (std::rand() % 3 == 0) // 33% chance per column
        {
            int tree_fg = (std::rand() % 5 < 3) ? 15 : // grass seed
                          (std::rand() % 3 == 0) ? 16 : 17; // random tree type
            blocks[cord(x, sx - 1)].fg = tree_fg;
            blocks[cord(x, sx - 1)].bg = 14;
        }
    }

    /* add random decorations: torches, flowers */
    for (int i = 0; i < 8; ++i)
    {
        int dx = std::rand() % 96 + 2;
        int sy = surface[dx] - 1;
        if (blocks[cord(dx, sy)].fg == 0)
            blocks[cord(dx, sy)].fg = (std::rand() % 2) ? 28 : 30; // random decoration
    }

    /* START world gets special features */
    if (name == "START")
    {
        /* spawn area with signs */
        int sx = main_door;
        int sy = surface[sx] - 1;
        blocks[cord(sx + 2, sy)].fg = 62; // sign
        blocks[cord(sx + 2, sy)].label = "Welcome to Gurotopia!";
        blocks[cord(sx + 3, sy)].fg = 62; // sign
        blocks[cord(sx + 3, sy)].label = "By: Gurotopia Team";
        /* path blocks */
        for (int px = sx - 4; px <= sx + 4; ++px)
            if (blocks[cord(px, sy)].fg == 0)
                blocks[cord(px, sy)].fg = 40; // wood block path
        /* donation box */
        blocks[cord(sx + 4, sy - 1)].fg = 1276; // donation box item
    }

    world.blocks = std::move(blocks);
    world.name = std::move(name);
}

bool door_mover(world &world, const ::pos &pos)
{
    std::vector<block> &blocks = world.blocks;

    if (blocks[cord(pos.x, pos.y)].fg != 0 ||
        blocks[cord(pos.x, (pos.y + 1))].fg != 0) return false;

    for (std::size_t i = 0zu; i < blocks.size(); ++i)
    {
        if (blocks[i].fg == 6)
        {
            blocks[i].fg = 0; // remove main door
            blocks[cord(i % 100, (i / 100 + 1))].fg = 0; // remove bedrock below
            break;
        }
    }
    blocks[cord(pos.x, pos.y)].fg = 6;
    blocks[cord(pos.x, (pos.y + 1))].fg = 8;
    return true;
}

void blast::thermonuclear(world &world, const std::string& name)
{
    ransuu ransuu;
    u_short main_door = ransuu[{2, 100 * 60 / 100 - 4}];
    std::vector<block> blocks(100 * 60, block{0, 0});
    
    for (std::size_t i = 0zu; i < blocks.size(); ++i)
    {
        blocks[i].fg = (i >= cord(0, 54)) ? 8 : 0;

        if (i == cord(main_door, 36)) blocks[i].fg = 6;
        else if (i == cord(main_door, 37)) blocks[i].fg = 8;
    }
    world.blocks = std::move(blocks);
    world.name = std::move(name);
}

/* ── world persistence ── */

void world::mysql_save()
{
    if (this->name.empty()) return;

    /* serialize blocks: fg:bg:s3:s4:label;fg:bg:s3:s4:label;... */
    std::string blocks_str;
    blocks_str.reserve(this->blocks.size() * 12);
    for (std::size_t i = 0; i < this->blocks.size(); ++i)
    {
        if (i > 0) blocks_str += ';';
        const ::block &b = this->blocks[i];
        blocks_str += std::format("{}:{}:{}:{}", b.fg, b.bg, (int)b.state[2], (int)b.state[3]);
        if (!b.label.empty())
        {
            blocks_str += ':';
            for (char c : b.label)
                blocks_str += (c == ';' || c == ':' || c == ',') ? '?' : c; // @note sanitize delimiters
        }
    }

    /* serialize objects: id:count:x:y:uid;... */
    std::string objects_str;
    for (std::size_t i = 0; i < this->objects.size(); ++i)
    {
        if (i > 0) objects_str += ';';
        objects_str += std::format("{}:{}:{}:{}:{}", 
            this->objects[i].id, this->objects[i].count,
            this->objects[i].pos.x, this->objects[i].pos.y,
            this->objects[i].uid);
    }
    /* append last_object_uid as special entry */
    if (!objects_str.empty()) objects_str += ';';
    objects_str += std::format("0:0:0:0:{}", this->last_object_uid);

    /* serialize doors: dest:id:password:x:y;... */
    std::string doors_str;
    for (std::size_t i = 0; i < this->doors.size(); ++i)
    {
        if (i > 0) doors_str += ';';
        doors_str += std::format("{}:{}:{}:{}:{}", 
            this->doors[i].dest, this->doors[i].id,
            this->doors[i].password, this->doors[i].pos.x, this->doors[i].pos.y);
    }

    /* serialize displays: id:x:y;... */
    std::string displays_str;
    for (std::size_t i = 0; i < this->displays.size(); ++i)
    {
        if (i > 0) displays_str += ';';
        displays_str += std::format("{}:{}:{}", 
            this->displays[i].id, this->displays[i].pos.x, this->displays[i].pos.y);
    }

    /* escape strings for SQL */
    std::string esc_blocks(2 * blocks_str.size() + 1, '\0');
    std::string esc_objects(2 * objects_str.size() + 1, '\0');
    std::string esc_doors(2 * doors_str.size() + 1, '\0');
    std::string esc_displays(2 * displays_str.size() + 1, '\0');

    mysql_real_escape_string(db, esc_blocks.data(),   blocks_str.c_str(),   blocks_str.size());
    mysql_real_escape_string(db, esc_objects.data(),  objects_str.c_str(),  objects_str.size());
    mysql_real_escape_string(db, esc_doors.data(),    doors_str.c_str(),    doors_str.size());
    mysql_real_escape_string(db, esc_displays.data(), displays_str.c_str(), displays_str.size());

    esc_blocks.resize(strlen(esc_blocks.c_str()));
    esc_objects.resize(strlen(esc_objects.c_str()));
    esc_doors.resize(strlen(esc_doors.c_str()));
    esc_displays.resize(strlen(esc_displays.c_str()));

    std::string query = std::format(
        "INSERT INTO world (name, owner, is_public, lock_state, minimum_entry_level, blocks, objects, doors, displays) "
        "VALUES ('{}', {}, {}, {}, {}, '{}', '{}', '{}', '{}') "
        "ON DUPLICATE KEY UPDATE owner=VALUES(owner), is_public=VALUES(is_public), lock_state=VALUES(lock_state), "
        "minimum_entry_level=VALUES(minimum_entry_level), blocks=VALUES(blocks), objects=VALUES(objects), "
        "doors=VALUES(doors), displays=VALUES(displays)",
        this->name, this->owner, (int)this->is_public, (int)this->lock_state, (int)this->minimum_entry_level,
        esc_blocks, esc_objects, esc_doors, esc_displays
    );

    if (mysql_query(db, query.c_str()))
        fprintf(stderr, "[world save] %s\n", mysql_error(db));
}

bool world::mysql_load()
{
    if (this->name.empty()) return false;

    std::string query = std::format("SELECT owner, is_public, lock_state, minimum_entry_level, blocks, objects, doors, displays FROM world WHERE name = '{}' LIMIT 1", this->name);
    if (mysql_query(db, query.c_str()))
    {
        fprintf(stderr, "[world load] %s\n", mysql_error(db));
        return false;
    }

    MYSQL_RES *result = mysql_store_result(db);
    if (!result) return false;

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) { mysql_free_result(result); return false; } // @note world not in DB

    this->owner = row[0] ? std::atoi(row[0]) : 0;
    this->is_public = row[1] ? std::atoi(row[1]) : 0;
    this->lock_state = row[2] ? std::atoi(row[2]) : 0;
    this->minimum_entry_level = row[3] ? std::atoi(row[3]) : 1;

    /* load blocks */
    if (row[4] && row[4][0])
    {
        std::string blocks_str(row[4]);
        std::vector<std::string> entries = readch(blocks_str, ';');
        this->blocks.clear();
        this->blocks.reserve(entries.size());
        for (const auto& entry : entries)
        {
            std::vector<std::string> parts = readch(entry, ':');
            ::block b;
            if (parts.size() >= 1) b.fg = (short)std::stoi(parts[0]);
            if (parts.size() >= 2) b.bg = (short)std::stoi(parts[1]);
            if (parts.size() >= 3) b.state[2] = (u_char)std::stoi(parts[2]);
            if (parts.size() >= 4) b.state[3] = (u_char)std::stoi(parts[3]);
            if (parts.size() >= 5) b.label = parts[4];
            this->blocks.push_back(std::move(b));
        }
    }

    /* load objects */
    if (row[5] && row[5][0])
    {
        std::string objects_str(row[5]);
        std::vector<std::string> entries = readch(objects_str, ';');
        this->objects.clear();
        for (const auto& entry : entries)
        {
            std::vector<std::string> parts = readch(entry, ':');
            if (parts.size() >= 5)
            {
                u_short oid = (u_short)std::stoi(parts[0]);
                u_int uid = (u_int)std::stoul(parts[4]);
                if (oid == 0) { this->last_object_uid = uid; continue; } // @note special entry: last_object_uid
                this->objects.emplace_back(::object(
                    oid, (u_short)std::stoi(parts[1]),
                    ::pos(std::stof(parts[2]), std::stof(parts[3])),
                    uid
                ));
                if (uid > this->last_object_uid) this->last_object_uid = uid;
            }
        }
    }

    /* load doors */
    if (row[6] && row[6][0])
    {
        std::string doors_str(row[6]);
        std::vector<std::string> entries = readch(doors_str, ';');
        this->doors.clear();
        for (const auto& entry : entries)
        {
            std::vector<std::string> parts = readch(entry, ':');
            if (parts.size() >= 5)
                this->doors.emplace_back(::door(
                    parts[0], parts[1], parts[2],
                    ::pos(std::stof(parts[3]), std::stof(parts[4]))
                ));
        }
    }

    /* load displays */
    if (row[7] && row[7][0])
    {
        std::string displays_str(row[7]);
        std::vector<std::string> entries = readch(displays_str, ';');
        this->displays.clear();
        for (const auto& entry : entries)
        {
            std::vector<std::string> parts = readch(entry, ':');
            if (parts.size() >= 3)
                this->displays.emplace_back(::display(
                    (u_int)std::stoul(parts[0]),
                    ::pos(std::stof(parts[1]), std::stof(parts[2]))
                ));
        }
    }

    mysql_free_result(result);
    printf("[world load] '%s' loaded: %zu blocks, %zu objects, %zu doors, %zu displays, last_uid=%u\n",
        this->name.c_str(), this->blocks.size(), this->objects.size(),
        this->doors.size(), this->displays.size(), this->last_object_uid);
    return true;
}
