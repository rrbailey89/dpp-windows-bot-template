#include "DatabaseManager.h"
#include <iostream>

const std::chrono::seconds DatabaseManager::COOLDOWN_DURATION(120);

DatabaseManager::DatabaseManager(const std::string& url) : m_session(url) {
    try {
        std::cout << "Successfully connected to the database: " << m_session.getDefaultSchemaName() << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database connection error: " << err.what() << std::endl;
        throw;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error creating database session: " << ex.what() << std::endl;
        throw;
    }
}

DatabaseManager::~DatabaseManager() {
    // Clean up the database connection if needed
}

DatabaseManager& DatabaseManager::getInstance() {
    static DatabaseManager* instance = nullptr;

    if (instance == nullptr) {
        std::string username, password, host, port, database;

        std::cout << "Enter database username: ";
        std::getline(std::cin, username);

        std::cout << "Enter database password: ";
        std::getline(std::cin, password);

        std::cout << "Enter database host: ";
        std::getline(std::cin, host);

        std::cout << "Enter database port: ";
        std::getline(std::cin, port);

        std::cout << "Enter database name: ";
        std::getline(std::cin, database);

        std::string url = "mysqlx://" + username + ":" + password + "@" + host + ":" + port + "/" + database;
        instance = new DatabaseManager(url);
    }

    return *instance;
}

mysqlx::Session& DatabaseManager::getSession() {
    return m_session;
}

std::string DatabaseManager::getBotTokenFromDb() {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT token FROM bot_config WHERE name = 'bot_token' LIMIT 1;";
        mysqlx::SqlResult result = session.sql(sql).execute();
        if (result.count() > 0) {
            mysqlx::Row row = result.fetchOne();
            return row[0].get<std::string>();
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return "";
}

void DatabaseManager::deleteOldConversationMessages(int hours) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "DELETE FROM conversation_messages WHERE created_at < DATE_SUB(NOW(), INTERVAL ? HOUR)";
        session.sql(sql).bind(hours).execute();
        std::cout << "Old conversation messages deleted successfully" << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

void DatabaseManager::updateGuildInfo(dpp::snowflake guild_id, const std::string& guild_name, dpp::snowflake owner_id, int member_count) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "INSERT INTO guild_info (guild_id, guild_name, owner_id, member_count) "
            "VALUES (?, ?, ?, ?) "
            "ON DUPLICATE KEY UPDATE guild_name = VALUES(guild_name), owner_id = VALUES(owner_id), member_count = VALUES(member_count)";
        session.sql(sql)
            .bind(static_cast<long long>(guild_id))
            .bind(guild_name)
            .bind(static_cast<long long>(owner_id))
            .bind(member_count)
            .execute();
        std::cout << "Guild info updated successfully" << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

void DatabaseManager::updateGuildChannelInfo(dpp::snowflake guild_id, dpp::snowflake channel_id, const std::string& channel_name, const std::string& channel_type) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "INSERT INTO guild_channels (channel_id, guild_id, channel_name, channel_type) "
            "VALUES (?, ?, ?, ?) "
            "ON DUPLICATE KEY UPDATE channel_name = VALUES(channel_name), channel_type = VALUES(channel_type)";
        session.sql(sql)
            .bind(static_cast<long long>(channel_id))
            .bind(static_cast<long long>(guild_id))
            .bind(channel_name)
            .bind(channel_type)
            .execute();
        std::cout << "Guild channel information updated successfully" << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

void DatabaseManager::updateGuildRoleInfo(dpp::snowflake guild_id, dpp::snowflake role_id, const std::string& role_name) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "INSERT INTO guild_roles (guild_id, role_id, role_name) "
            "VALUES (?, ?, ?) "
            "ON DUPLICATE KEY UPDATE role_name = VALUES(role_name)";
        session.sql(sql)
            .bind(static_cast<long long>(guild_id))
            .bind(static_cast<long long>(role_id))
            .bind(role_name)
            .execute();
        std::cout << "Guild role information updated successfully" << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

void DatabaseManager::removeGuildInfo(dpp::snowflake guild_id) {
    try {
        mysqlx::Session& session = getSession();
        mysqlx::Table table = session.getSchema("DiscordBotTestServer").getTable("guild_info");
        table.remove()
            .where("guild_id = :guild_id")
            .bind("guild_id", static_cast<long long>(guild_id))
            .execute();
        std::cout << "Guild info deleted successfully" << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

void DatabaseManager::guildCommandsPopulater(dpp::snowflake guild_id, const std::string& command, bool enabled) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "INSERT IGNORE INTO guild_commands (guild_id, command, enabled) VALUES (?, ?, ?)";
        session.sql(sql)
            .bind(static_cast<long long>(guild_id))
            .bind(command)
            .bind(enabled)
            .execute();
        std::cout << "Command populated successfully" << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

bool DatabaseManager::isCommandEnabledForGuild(dpp::snowflake guild_id, const std::string& command) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT enabled FROM guild_commands WHERE guild_id = ? AND command = ? LIMIT 1";
        mysqlx::RowResult result = session.sql(sql)
            .bind(static_cast<long long>(guild_id))
            .bind(command)
            .execute();
        if (result.count() > 0) {
            mysqlx::Row row = result.fetchOne();
            return row[0].get<bool>();
        }
        return false; // Default to false if no record found
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return false;
}

int DatabaseManager::getBlameCount() {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT count FROM blame_count WHERE id = 1;";
        mysqlx::RowResult result = session.sql(sql).execute();
        if (result.count() > 0) {
            mysqlx::Row row = result.fetchOne();
            return row[0].get<int>();
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return 0;
}

std::vector<std::tuple<int, std::string, std::string, std::string, std::string, dpp::snowflake>> DatabaseManager::getDueRemindersWithId() {
    std::vector<std::tuple<int, std::string, std::string, std::string, std::string, dpp::snowflake>> dueReminders;
    try {
        mysqlx::Session& session = getSession();
        std::time_t now = std::time(nullptr);
        std::tm localtime;
        localtime_s(&localtime, &now);
        std::ostringstream timeStream;
        timeStream << std::put_time(&localtime, "%I:%M %p");
        std::string currentTime = timeStream.str();
        std::string currentDay = localtime.tm_wday == 0 ? "sunday" :
            localtime.tm_wday == 1 ? "monday" :
            localtime.tm_wday == 2 ? "tuesday" :
            localtime.tm_wday == 3 ? "wednesday" :
            localtime.tm_wday == 4 ? "thursday" :
            localtime.tm_wday == 5 ? "friday" : "saturday";
        std::string sql = "SELECT id, reminder_text, frequency, day, time, channel_id FROM reminders "
            "WHERE (frequency = 'weekly' AND day = ? AND STR_TO_DATE(CONCAT(CURDATE(), ' ', time), '%Y-%m-%d %l:%i %p') <= NOW() AND (last_sent IS NULL OR DATE(last_sent) <> CURDATE())) "
            "OR (frequency = 'monthly' AND MONTH(last_sent) <> MONTH(CURDATE()) AND STR_TO_DATE(CONCAT(CURDATE(), ' ', time), '%Y-%m-%d %l:%i %p') <= NOW()) "
            "OR (frequency = 'monthly' AND last_sent IS NULL AND STR_TO_DATE(CONCAT(CURDATE(), ' ', time), '%Y-%m-%d %l:%i %p') <= NOW())";
        mysqlx::SqlStatement stmt = session.sql(sql);
        stmt.bind(currentDay);
        mysqlx::RowResult result = stmt.execute();
        for (mysqlx::Row row : result) {
            uint64_t channelIdUint64 = row[5].get<uint64_t>();
            dueReminders.emplace_back(
                row[0].get<int>(), // id
                row[1].get<std::string>(), // reminder_text
                row[2].get<std::string>(), // frequency
                row[3].get<std::string>(), // day
                row[4].get<std::string>(), // time
                dpp::snowflake(channelIdUint64)
            );
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return dueReminders;
}

void DatabaseManager::incrementBlameCount() {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "UPDATE blame_count SET count = count + 1 WHERE id = 1;";
        mysqlx::SqlStatement stmt = session.sql(sql);
        mysqlx::SqlResult result = stmt.execute();

        std::cout << "Blame count incremented successfully" << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

dpp::snowflake DatabaseManager::getWarnChannelIdForGuild(dpp::snowflake guild_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT channel_id FROM warn_channel_ids WHERE guild_id = ? LIMIT 1;";
        mysqlx::SqlStatement stmt = session.sql(sql);
        stmt.bind(static_cast<uint64_t>(guild_id));

        mysqlx::RowResult result = stmt.execute();

        if (result.count() > 0) {
            mysqlx::Row row = result.fetchOne();
            uint64_t channelIdUint64 = row[0].get<uint64_t>();
            return dpp::snowflake(channelIdUint64);
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return 0;
}

dpp::snowflake DatabaseManager::getBotOwnerId() {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT token FROM bot_config WHERE name = 'bot_owner_id' LIMIT 1;";
        mysqlx::SqlStatement stmt = session.sql(sql);
        mysqlx::RowResult result = stmt.execute();

        if (result.count() > 0) {
            mysqlx::Row row = result.fetchOne();
            std::string ownerIdStr = row[0].get<std::string>();
            return dpp::snowflake(ownerIdStr);
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return 0;
}

void DatabaseManager::addGuildRule(dpp::snowflake guild_id, const std::string& rule) {
    try {
        mysqlx::Session& session = getSession();
        mysqlx::Table table = session.getSchema("DiscordBotTestServer").getTable("guild_rules");
        mysqlx::Result result = table.insert("guild_id", "rule")
            .values(static_cast<uint64_t>(guild_id), rule)
            .execute();

        std::cout << "Guild rule inserted successfully, affected rows: " << result.getAffectedItemsCount() << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

std::vector<std::string> DatabaseManager::getGuildRules(dpp::snowflake guild_id) {
    std::vector<std::string> rules;
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT rule FROM guild_rules WHERE guild_id = ?;";
        mysqlx::SqlStatement stmt = session.sql(sql).bind(static_cast<uint64_t>(guild_id));
        mysqlx::RowResult result = stmt.execute();

        for (mysqlx::Row row : result) {
            rules.push_back(row[0].get<std::string>());
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return rules;
}

void DatabaseManager::removeGuildRule(dpp::snowflake guild_id, int rule_number) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql_select_id = "SELECT id FROM guild_rules WHERE guild_id = ? ORDER BY id LIMIT ?, 1";
        mysqlx::SqlStatement selectStmt = session.sql(sql_select_id)
            .bind(static_cast<uint64_t>(guild_id))
            .bind(rule_number - 1);
        mysqlx::RowResult selectResult = selectStmt.execute();

        if (selectResult.count() == 0) {
            std::cerr << "Rule not found or invalid rule number." << std::endl;
            return;
        }
        int rule_id = selectResult.fetchOne()[0].get<int>();

        if (rule_id > 0) {
            std::string sql_delete = "DELETE FROM guild_rules WHERE id = ?";
            mysqlx::SqlStatement deleteStmt = session.sql(sql_delete).bind(rule_id);
            mysqlx::SqlResult deleteResult = deleteStmt.execute();
            std::cout << "Rule successfully removed, affected rows: " << deleteResult.getAffectedItemsCount() << std::endl;
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

dpp::snowflake DatabaseManager::getGuildOwnerId(dpp::snowflake guild_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT owner_id FROM guild_info WHERE guild_id = ?";
        mysqlx::SqlStatement stmt = session.sql(sql).bind(static_cast<uint64_t>(guild_id));
        mysqlx::RowResult result = stmt.execute();

        if (result.count() > 0) {
            mysqlx::Row row = result.fetchOne();
            uint64_t owner_id_uint64 = row[0].get<uint64_t>();
            return dpp::snowflake(owner_id_uint64);
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return 0;
}

void DatabaseManager::setMessageDeleteChannel(dpp::snowflake guild_id, const std::string& guild_name, dpp::snowflake channel_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "REPLACE INTO message_delete_channels (guild_id, guild_name, channel_id) VALUES (?, ?, ?);";
        mysqlx::SqlStatement stmt = session.sql(sql)
            .bind(static_cast<uint64_t>(guild_id))
            .bind(guild_name)
            .bind(static_cast<uint64_t>(channel_id));
        mysqlx::SqlResult result = stmt.execute();

        std::cout << "Message delete channel updated successfully, affected rows: " << result.getAffectedItemsCount() << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

std::string DatabaseManager::getOpenAIApiKey() {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT token FROM bot_config WHERE name = 'openai_api_key' LIMIT 1;";
        mysqlx::RowResult result = session.sql(sql).execute();

        if (result.count() > 0) {
            mysqlx::Row row = result.fetchOne();
            return row[0].get<std::string>();
        }
        else {
            std::cerr << "OpenAI API key not found in the database." << std::endl;
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }

    return "";
}

void DatabaseManager::setWarnChannelForGuild(dpp::snowflake guild_id, dpp::snowflake channel_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "INSERT INTO warn_channel_ids (guild_id, channel_id) VALUES (?, ?) ON DUPLICATE KEY UPDATE channel_id = VALUES(channel_id);";
        mysqlx::SqlStatement stmt = session.sql(sql)
            .bind(static_cast<uint64_t>(guild_id))
            .bind(static_cast<uint64_t>(channel_id));
        mysqlx::SqlResult result = stmt.execute();

        std::cout << "Warn channel set successfully, affected rows: " << result.getAffectedItemsCount() << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

dpp::snowflake DatabaseManager::getMessageDeleteChannelIdForGuild(dpp::snowflake guild_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT channel_id FROM message_delete_channels WHERE guild_id = ?;";
        mysqlx::RowResult result = session.sql(sql).bind(static_cast<uint64_t>(guild_id)).execute();
        if (result.count() > 0) {
            mysqlx::Row row = result.fetchOne();
            uint64_t channel_id_uint64 = row[0].get<uint64_t>();
            return dpp::snowflake(channel_id_uint64);
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return 0;
}

std::chrono::system_clock::time_point DatabaseManager::getLastUsedTime(dpp::snowflake user_id, const std::string& command) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT last_used FROM user_cooldowns WHERE user_id = ? AND command = ?;";
        mysqlx::RowResult result = session.sql(sql).bind(static_cast<uint64_t>(user_id)).bind(command).execute();
        if (result.count() > 0) {
            mysqlx::Row row = result.fetchOne();
            return std::chrono::system_clock::from_time_t(row[0].get<int64_t>());
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return std::chrono::system_clock::time_point();
}

void DatabaseManager::updateCooldown(dpp::snowflake user_id, const std::string& command) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "INSERT INTO user_cooldowns (user_id, command, last_used) VALUES (?, ?, ?) ON DUPLICATE KEY UPDATE last_used = VALUES(last_used);";
        session.sql(sql)
            .bind(static_cast<uint64_t>(user_id))
            .bind(command)
            .bind(static_cast<int64_t>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())))
            .execute();
        std::cout << "Cooldown updated successfully" << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

void DatabaseManager::setCommandEnabledOrDisabledForGuild(dpp::snowflake guild_id, const std::string& command, bool enabled) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "INSERT INTO guild_commands (guild_id, command, enabled) VALUES (?, ?, ?) ON DUPLICATE KEY UPDATE enabled = VALUES(enabled);";
        session.sql(sql)
            .bind(static_cast<uint64_t>(guild_id))
            .bind(command)
            .bind(enabled)
            .execute();
        std::cout << "Command status updated successfully" << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

std::string DatabaseManager::getGuildName(dpp::snowflake guild_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT guild_name FROM guild_info WHERE guild_id = ?;";
        mysqlx::RowResult result = session.sql(sql).bind(static_cast<uint64_t>(guild_id)).execute();
        if (result.count() > 0) {
            mysqlx::Row row = result.fetchOne();
            return row[0].get<std::string>();
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return "";
}

void DatabaseManager::storeUserJoinDate(dpp::snowflake guild_id, dpp::snowflake user_id, int64_t join_date) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "INSERT INTO guild_member_join_dates (guild_id, user_id, join_date) VALUES (?, ?, FROM_UNIXTIME(?)) ON DUPLICATE KEY UPDATE join_date = VALUES(join_date);";
        session.sql(sql)
            .bind(static_cast<uint64_t>(guild_id))
            .bind(static_cast<uint64_t>(user_id))
            .bind(join_date)
            .execute();
        std::cout << "Join date stored successfully" << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

std::string DatabaseManager::getUserJoinDate(dpp::snowflake guild_id, dpp::snowflake user_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT DATE_FORMAT(join_date, '%Y-%m-%d %H:%i:%s') AS join_date FROM guild_member_join_dates WHERE guild_id = ? AND user_id = ? LIMIT 1;";
        mysqlx::RowResult result = session.sql(sql)
            .bind(static_cast<uint64_t>(guild_id))
            .bind(static_cast<uint64_t>(user_id))
            .execute();
        if (result.count() > 0) {
            mysqlx::Row row = result.fetchOne();
            return row[0].get<std::string>();
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return "";
}

void DatabaseManager::setMemberJoinChannelForGuild(dpp::snowflake guild_id, dpp::snowflake channel_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "INSERT INTO guild_member_join_channels (guild_id, channel_id) VALUES (?, ?) ON DUPLICATE KEY UPDATE channel_id = VALUES(channel_id);";
        session.sql(sql)
            .bind(static_cast<uint64_t>(guild_id))
            .bind(static_cast<uint64_t>(channel_id))
            .execute();
        std::cout << "Member join channel set successfully" << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

dpp::snowflake DatabaseManager::getMemberJoinChannelForGuild(dpp::snowflake guild_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT channel_id FROM guild_member_join_channels WHERE guild_id = ? LIMIT 1;";
        mysqlx::RowResult result = session.sql(sql).bind(static_cast<uint64_t>(guild_id)).execute();
        if (result.count() > 0) {
            mysqlx::Row row = result.fetchOne();
            uint64_t channel_id_uint64 = row[0].get<uint64_t>();
            return dpp::snowflake(channel_id_uint64);
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return 0;
}

int DatabaseManager::getUserHugCount(dpp::snowflake user_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT hug_count FROM user_hug_counts WHERE user_id = ?;";
        mysqlx::RowResult result = session.sql(sql).bind(static_cast<uint64_t>(user_id)).execute();
        if (result.count() > 0) {
            mysqlx::Row row = result.fetchOne();
            return row[0].get<int>();
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return 0;
}

void DatabaseManager::incrementUserHugCount(dpp::snowflake user_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "INSERT INTO user_hug_counts (user_id, hug_count) VALUES (?, 1) ON DUPLICATE KEY UPDATE hug_count = hug_count + 1;";
        session.sql(sql).bind(static_cast<uint64_t>(user_id)).execute();
        std::cout << "User hug count incremented successfully" << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

void DatabaseManager::storeReminder(dpp::snowflake guild_id, const std::string& reminder_text, const std::string& frequency, const std::string& day, const std::string& time, dpp::snowflake channel_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "INSERT INTO reminders (guild_id, reminder_text, frequency, day, time, channel_id) "
            "VALUES (?, ?, ?, ?, ?, ?) "
            "ON DUPLICATE KEY UPDATE channel_id = VALUES(channel_id);";
        session.sql(sql)
            .bind(static_cast<uint64_t>(guild_id))
            .bind(reminder_text)
            .bind(frequency)
            .bind(day)
            .bind(time)
            .bind(static_cast<uint64_t>(channel_id))
            .execute();
        std::cout << "Reminder stored successfully" << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

void DatabaseManager::updateReminderLastSent(int reminder_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "UPDATE reminders SET last_sent = NOW() WHERE id = ?";
        mysqlx::SqlStatement stmt = session.sql(sql);
        stmt.bind(reminder_id);
        mysqlx::SqlResult result = stmt.execute();

        if (result.getAffectedItemsCount() > 0) {
            std::cout << "Reminder last_sent updated successfully" << std::endl;
        }
        else {
            std::cerr << "Failed to update reminder last_sent" << std::endl;
        }
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

bool DatabaseManager::removeReminder(int64_t reminder_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "DELETE FROM reminders WHERE id = ?;";
        mysqlx::SqlResult result = session.sql(sql).bind(reminder_id).execute();
        return result.getAffectedItemsCount() > 0;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return false;
}

void DatabaseManager::storeConversationMessage(dpp::snowflake user_id, const nlohmann::json& message) {
    try {
        mysqlx::Session& session = getSession();
        std::string message_str = message.dump();
        std::string sql = "INSERT INTO conversation_messages (user_id, message) VALUES (?, ?)";
        mysqlx::SqlStatement stmt = session.sql(sql);
        stmt.bind(static_cast<uint64_t>(user_id), message_str);
        stmt.execute();
        std::cout << "Message stored successfully" << std::endl;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
}

std::string DatabaseManager::unescapeJson(const std::string& json) {
    std::string unescaped_json;
    bool in_code_block = false;
    bool is_escaped = false;

    for (size_t i = 0; i < json.length(); ++i) {
        if (json[i] == '`' && i + 2 < json.length() && json[i + 1] == '`' && json[i + 2] == '`') {
            in_code_block = !in_code_block;
            unescaped_json += "```";
            i += 2;
        }
        else if (!in_code_block && json[i] == '\\') {
            if (is_escaped) {
                unescaped_json += '\\';
                is_escaped = false;
            }
            else {
                is_escaped = true;
            }
        }
        else {
            if (is_escaped) {
                switch (json[i]) {
                case 'n':
                    unescaped_json += '\n';
                    break;
                case 't':
                    unescaped_json += '\t';
                    break;
                case 'r':
                    unescaped_json += '\r';
                    break;
                case '\\':
                    unescaped_json += '\\';
                    break;
                case '"':
                    unescaped_json += '"';
                    break;
                default:
                    unescaped_json += '\\';
                    unescaped_json += json[i];
                    break;
                }
                is_escaped = false;
            }
            else {
                unescaped_json += json[i];
            }
        }
    }

    // Replace newline characters with escaped newline characters
    size_t pos = 0;
    while ((pos = unescaped_json.find('\n', pos)) != std::string::npos) {
        unescaped_json.replace(pos, 1, "\\n");
        pos += 2;
    }

    return unescaped_json;
}

std::vector<nlohmann::json> DatabaseManager::getConversation(dpp::snowflake user_id) {
    try {
        mysqlx::Session& session = getSession();
        std::string sql = "SELECT JSON_UNQUOTE(message) AS message FROM conversation_messages WHERE user_id = ? ORDER BY created_at;";
        mysqlx::SqlStatement stmt = session.sql(sql);
        stmt.bind(static_cast<uint64_t>(user_id));
        auto result = stmt.execute();

        std::vector<nlohmann::json> conversation;
        while (result.hasData()) {
            try {
                auto row = result.fetchOne();
                std::string message_json = row[0].get<std::string>();

                std::string unescaped_json = unescapeJson(message_json);

                std::cout << "Retrieved JSON: " << unescaped_json << std::endl;

                nlohmann::json message = nlohmann::json::parse(unescaped_json);
                std::cout << "Parsed JSON: " << message.dump() << std::endl;

                conversation.push_back(message);
            }
            catch (const std::exception& e) {
                std::cerr << "Exception occurred: " << e.what() << std::endl;
            }
        }

        return conversation;
    }
    catch (const mysqlx::Error& err) {
        std::cerr << "Database operation error: " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }
    return {};
}