#include "pch.hpp"

#include "database.hpp"

MYSQL *db;
std::mutex db_mutex;

void db_check_connection()
{
    bool reconnected = false;
    if (mysql_ping(db))
    {
        fprintf(stderr, "[db] connection lost, reconnecting...\n");

        const char *db_host = std::getenv("GURO_DB_HOST") ? std::getenv("GURO_DB_HOST") : "127.0.0.1";
        const char *db_user = std::getenv("GURO_DB_USER") ? std::getenv("GURO_DB_USER") : "root";
        const char *db_pass = std::getenv("GURO_DB_PASS") ? std::getenv("GURO_DB_PASS") : "ukEhT<ZM3~&t)jI{";
        const char *db_name = std::getenv("GURO_DB_NAME") ? std::getenv("GURO_DB_NAME") : "gurotopia";
        const int   db_port = std::getenv("GURO_DB_PORT") ? std::atoi(std::getenv("GURO_DB_PORT")) : 3306;

        mysql_close(db);
        db = mysql_init(NULL);

        my_bool reconnect = 1;
        mysql_options(db, MYSQL_OPT_RECONNECT, &reconnect);

        if (!mysql_real_connect(db, db_host, db_user, db_pass, db_name, db_port, NULL, 0))
        {
            fprintf(stderr, "[db] reconnect failed: %s\n", mysql_error(db));
        }
        else
        {
            printf("[db] reconnected to SQL server on %s:%d\n", db->host, db->port);
            reconnected = true;
        }
    }
    if (reconnected)
    {
        // re-select DB just in case
        const char *db_name = std::getenv("GURO_DB_NAME") ? std::getenv("GURO_DB_NAME") : "gurotopia";
        mysql_select_db(db, db_name);
    }
}

void create_table_if_not_exist()
{
    const char* query = R"(
        CREATE TABLE IF NOT EXISTS peer (
            uid INT AUTO_INCREMENT PRIMARY KEY,
            growid VARCHAR(18) UNIQUE,
            password VARCHAR(18),
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            gems INT DEFAULT 0,
            level SMALLINT UNSIGNED DEFAULT 1,
            xp SMALLINT UNSIGNED DEFAULT 0,
            role TINYINT UNSIGNED DEFAULT 0,
            skin_color INT UNSIGNED DEFAULT 2527912447,
            hair_color INT DEFAULT -1,
            slot_size SMALLINT DEFAULT 16,
            clothing TEXT,
            inventory TEXT
        );
    )";

    if (mysql_query(db, query))
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }

    /* migrate existing table: add columns if they don't exist */
    const char* migrations[] = {
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS gems INT DEFAULT 0",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS level SMALLINT UNSIGNED DEFAULT 1",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS xp SMALLINT UNSIGNED DEFAULT 0",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS role TINYINT UNSIGNED DEFAULT 0",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS skin_color INT UNSIGNED DEFAULT 2527912447",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS hair_color INT DEFAULT -1",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS slot_size SMALLINT DEFAULT 16",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS clothing TEXT",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS inventory TEXT",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS banned TINYINT DEFAULT 0",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS muted TINYINT DEFAULT 0",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS last_daily TIMESTAMP NULL",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS title VARCHAR(32) DEFAULT ''",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS friends TEXT",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS guild_id INT DEFAULT 0",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS stats TEXT",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS pet_id SMALLINT DEFAULT 0",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS pet_name VARCHAR(32) DEFAULT ''",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS pet_level SMALLINT DEFAULT 0",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS pet_xp INT DEFAULT 0",
        "ALTER TABLE peer ADD COLUMN IF NOT EXISTS quests TEXT",
        nullptr
    };

    for (int i = 0; migrations[i]; ++i)
    {
        if (mysql_query(db, migrations[i]))
            fprintf(stderr, "%s\n", mysql_error(db)); // @note harmless if column already exists
    }

    /* world table */
    const char* world_query = R"(
        CREATE TABLE IF NOT EXISTS world (
            name VARCHAR(18) PRIMARY KEY,
            owner INT DEFAULT 0,
            is_public TINYINT DEFAULT 0,
            lock_state TINYINT UNSIGNED DEFAULT 0,
            minimum_entry_level TINYINT UNSIGNED DEFAULT 1,
            blocks LONGTEXT,
            objects LONGTEXT,
            doors LONGTEXT,
            displays LONGTEXT,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
        );
    )";

    if (mysql_query(db, world_query))
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }

    /* guild table */
    const char* guild_query = R"(
        CREATE TABLE IF NOT EXISTS guild (
            id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(16) UNIQUE,
            leader INT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            motd TEXT,
            vault TEXT
        );
    )";

    if (mysql_query(db, guild_query))
        fprintf(stderr, "%s\n", mysql_error(db));

    /* mailbox table */
    const char* mail_query = R"(
        CREATE TABLE IF NOT EXISTS mailbox (
            id INT AUTO_INCREMENT PRIMARY KEY,
            recipient_uid INT NOT NULL,
            sender_name VARCHAR(18),
            message TEXT,
            item_id SMALLINT DEFAULT 0,
            item_count SMALLINT DEFAULT 0,
            sent_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            is_read TINYINT DEFAULT 0
        );
    )";

    if (mysql_query(db, mail_query))
        fprintf(stderr, "%s\n", mysql_error(db));
}

void mysql_connect()
{
    db = mysql_init(NULL);

    // Enable auto-reconnect so the connection survives MariaDB restarts / timeouts.
    // Without this, a single stale connection = "Server has gone away" forever.
    my_bool reconnect = 1;
    mysql_options(db, MYSQL_OPT_RECONNECT, &reconnect);

    const char *db_host   = std::getenv("GURO_DB_HOST")   ? std::getenv("GURO_DB_HOST")   : "127.0.0.1";
    const char *db_user   = std::getenv("GURO_DB_USER")   ? std::getenv("GURO_DB_USER")   : "root";
    const char *db_pass   = std::getenv("GURO_DB_PASS")   ? std::getenv("GURO_DB_PASS")   : "ukEhT<ZM3~&t)jI{";
    const char *db_name   = std::getenv("GURO_DB_NAME")   ? std::getenv("GURO_DB_NAME")   : "gurotopia";
    const int   db_port   = std::getenv("GURO_DB_PORT")   ? std::atoi(std::getenv("GURO_DB_PORT")) : 3306;

    if (!mysql_real_connect(db, db_host, db_user, db_pass, NULL, db_port, NULL, 0))
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }
    else printf("connected to SQL server on %s:%d\n", db->host, db->port);

    mysql_query(db, std::format("CREATE DATABASE IF NOT EXISTS {}", db_name).c_str());
    mysql_select_db(db, db_name);

    create_table_if_not_exist();
}

hStmt::hStmt(const char *query) : m_lock(db_mutex)
{
    db_check_connection();

    this->pStmt = mysql_stmt_init(db);
    if (!pStmt) 
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }
    if (mysql_stmt_prepare(pStmt, query, (u_long)strlen(query)))
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }
}
hStmt::~hStmt() 
{
    if (mysql_stmt_close(pStmt))
    {
        fprintf(stderr, "%s\n", mysql_error(db));
    }
}


MYSQL_BIND make_bind_in(const signed &buffer)
{
    return { .buffer = (void*)&buffer, .buffer_type = MYSQL_TYPE_LONG };
}
MYSQL_BIND make_bind_in(const unsigned &buffer)
{
    return { .buffer = (void*)&buffer, .buffer_type = MYSQL_TYPE_LONG, .is_unsigned = true };
}
MYSQL_BIND make_bind_in(const long &buffer)
{
    return { .buffer = (void*)&buffer, .buffer_type = MYSQL_TYPE_LONG };
}
MYSQL_BIND make_bind_in(const long long &buffer)
{
    return { .buffer = (void*)&buffer, .buffer_type = MYSQL_TYPE_LONGLONG };
}
MYSQL_BIND make_bind_in(const float &buffer)
{
    return { .buffer = (void*)&buffer, .buffer_type = MYSQL_TYPE_FLOAT };
}
MYSQL_BIND make_bind_in(const std::string &buffer)
{
    return { .buffer = (void*)buffer.c_str(), .buffer_length = (u_long)buffer.size(), .buffer_type = MYSQL_TYPE_STRING };
}

MYSQL_BIND make_bind_out(signed &buffer)
{
    return { .buffer = &buffer, .buffer_type = MYSQL_TYPE_LONG };
}
MYSQL_BIND make_bind_out(unsigned &buffer)
{
    return { .buffer = &buffer, .buffer_type = MYSQL_TYPE_LONG, .is_unsigned = true };
}
MYSQL_BIND make_bind_out(long &buffer)
{
    return { .buffer = &buffer, .buffer_type = MYSQL_TYPE_LONG };
}
MYSQL_BIND make_bind_out(long long &buffer)
{
    return { .buffer = &buffer, .buffer_type = MYSQL_TYPE_LONGLONG };
}
MYSQL_BIND make_bind_out(float &buffer)
{
    return { .buffer = &buffer, .buffer_type = MYSQL_TYPE_FLOAT };
}
MYSQL_BIND make_bind_out(std::string &buffer)
{
    buffer.resize(1024, '\0');

    return { .buffer = buffer.data(), .buffer_length = (u_long)buffer.size(), .buffer_type = MYSQL_TYPE_STRING };
}
