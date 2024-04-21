#pragma once
#include "mysqlx/xdevapi.h"
#include <string>
#include <dpp/dpp.h>
#include <vector>
#include <tuple>

class DatabaseManager {
public:
    static DatabaseManager& getInstance();
    static const std::chrono::seconds COOLDOWN_DURATION;
    mysqlx::Session& getSession();
    std::string getBotTokenFromDb();
    void deleteOldConversationMessages(int hours);
    void updateGuildInfo(dpp::snowflake guild_id, const std::string& guild_name, dpp::snowflake owner_id, int member_count);
    void updateGuildChannelInfo(dpp::snowflake guild_id, dpp::snowflake channel_id, const std::string& channel_name, const std::string& channel_type);
    void updateGuildRoleInfo(dpp::snowflake guild_id, dpp::snowflake role_id, const std::string& role_name);
    void removeGuildInfo(dpp::snowflake guild_id);
    void guildCommandsPopulater(dpp::snowflake guild_id, const std::string& command, bool enabled);
    bool isCommandEnabledForGuild(dpp::snowflake guild_id, const std::string& command);
    int getBlameCount();
    std::vector<std::tuple<int, std::string, std::string, std::string, std::string, dpp::snowflake>> getDueRemindersWithId();
    void incrementBlameCount();
    dpp::snowflake getWarnChannelIdForGuild(dpp::snowflake guild_id);
    dpp::snowflake getBotOwnerId();
    void addGuildRule(dpp::snowflake guild_id, const std::string& rule);
    std::vector<std::string> getGuildRules(dpp::snowflake guild_id);
    void removeGuildRule(dpp::snowflake guild_id, int rule_number);
    dpp::snowflake getGuildOwnerId(dpp::snowflake guild_id);
    void setMessageDeleteChannel(dpp::snowflake guild_id, const std::string& guild_name, dpp::snowflake channel_id);
    std::string getOpenAIApiKey();
    void setWarnChannelForGuild(dpp::snowflake guild_id, dpp::snowflake channel_id);
    dpp::snowflake getMessageDeleteChannelIdForGuild(dpp::snowflake guild_id);
    std::chrono::system_clock::time_point getLastUsedTime(dpp::snowflake user_id, const std::string& command);
    void updateCooldown(dpp::snowflake user_id, const std::string& command);
    void setCommandEnabledOrDisabledForGuild(dpp::snowflake guild_id, const std::string& command, bool enabled);
    std::string getGuildName(dpp::snowflake guild_id);
    void storeUserJoinDate(dpp::snowflake guild_id, dpp::snowflake user_id, int64_t join_date);
    std::string getUserJoinDate(dpp::snowflake guild_id, dpp::snowflake user_id);
    void setMemberJoinChannelForGuild(dpp::snowflake guild_id, dpp::snowflake channel_id);
    dpp::snowflake getMemberJoinChannelForGuild(dpp::snowflake guild_id);
    int getUserHugCount(dpp::snowflake user_id);
    void incrementUserHugCount(dpp::snowflake user_id);
    void storeReminder(dpp::snowflake guild_id, const std::string& reminder_text, const std::string& frequency, const std::string& day, const std::string& time, dpp::snowflake channel_id);
    void updateReminderLastSent(int reminder_id);
    bool removeReminder(int64_t reminder_id);
    void storeConversationMessage(dpp::snowflake user_id, const nlohmann::json& message);
    std::vector<nlohmann::json> getConversation(dpp::snowflake user_id);
    std::string unescapeJson(const std::string& json);


private:
    DatabaseManager(const std::string& url);
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    mysqlx::Session m_session;

};