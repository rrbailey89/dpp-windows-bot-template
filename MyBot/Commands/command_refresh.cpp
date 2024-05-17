#include "command_refresh.h"
#include "DatabaseManager.h"
#include "CleanCommand.h"
#include "FactsCommandHandler.h"
#include "grantroles.h"
#include "UpdateRaidTime.h"
#include "rules_command.h"
#include "BrosnanStatus.h"
#include "WarnMember.h"
#include "MemberJoinHandler.h"
#include "CreateRolesButton.h"
#include "FunCommand.h"
#include "ModCommand.h"
#include "message_listener.h"
#include "ReminderCommand.h"
#include "AskCommand.h"

void refresh_guild_commands(dpp::cluster& bot, dpp::snowflake guild_id) {
	// Define a list of all possible guild-specific commands
	std::vector<dpp::slashcommand> all_guild_commands = {
		commands::register_clean_command(bot),
		//commands::register_facts_command(bot),
        FactsCommandHandler(bot).register_facts_command(),
		commands::register_grantroles_command(bot),
		commands::register_updateraidtime_command(bot),
		commands::register_rules_command(bot),
        message_listener::register_setdeletechannel_command(bot),
        commands::register_brosnanstatus_command(bot),
		commands::register_setwarnchannel_command(bot),
		commands::register_warn_member_command(bot),
        commands::register_setmemberjoinchannel_command(bot),
        commands::register_createrolesbutton_command(bot),
        commands::register_fun_command(bot),
        commands::register_mod_command(bot),
        commands::register_reminder_command(bot),
        commands::register_ask_command(bot)
		// Add other guild-specific commands as needed
	};

    std::cout << "Starting to populate or update the database with the current state of all commands for guild: " << guild_id << std::endl;
    for (const auto& cmd : all_guild_commands) {
        DatabaseManager::getInstance().guildCommandsPopulater(guild_id, cmd.name, true);
    }

    std::vector<dpp::slashcommand> enabled_guild_commands;
    for (auto& cmd : all_guild_commands) {
        bool enabled = DatabaseManager::getInstance().isCommandEnabledForGuild(guild_id, cmd.name);
        if (enabled) {
            enabled_guild_commands.push_back(cmd);
        }
    }
    std::cout << "Starting guild_bulk_command_create for guild: " << guild_id << std::endl;
    bot.guild_bulk_command_create(enabled_guild_commands, guild_id, [guild_id](const dpp::confirmation_callback_t& callback) {
        if (callback.is_error()) {
            std::cerr << "Error refreshing guild commands for guild " << guild_id << ": " << callback.get_error().message << std::endl;
        }
        else {
            std::cout << "Guild commands refreshed successfully for guild " << guild_id << std::endl;
        }
        });
}
