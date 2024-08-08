#include "LevelSystem.h"
#include "DatabaseManager.h"

namespace commands {
    dpp::slashcommand register_setlevelupchannel_command(dpp::cluster& bot) {
        dpp::slashcommand command("setlevelupchannel", "Set the channel for level-up notifications", bot.me.id);
        command.add_option(dpp::command_option(dpp::co_channel, "channel", "The channel to use for level-up notifications", true));
        command.set_default_permissions(dpp::p_manage_guild);
        return command;
    }

    void handle_setlevelupchannel_command(const dpp::slashcommand_t& event, dpp::cluster& bot) {
        dpp::snowflake guild_id = event.command.guild_id;
        dpp::snowflake channel_id = std::get<dpp::snowflake>(event.get_parameter("channel"));

        // Check if the user executing the command is the guild owner
        dpp::snowflake owner_id = DatabaseManager::getInstance().getGuildOwnerId(guild_id);
        if (event.command.usr.id != owner_id) {
            event.reply("This command can only be used by the guild's owner.");
            return;
        }

        // Set the level-up channel in the database
        DatabaseManager::getInstance().setLevelUpChannelId(guild_id, channel_id);

        event.reply("Level-up notification channel set successfully.");
    }
}