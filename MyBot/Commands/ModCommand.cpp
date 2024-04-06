#include "ModCommand.h"

namespace commands {

    dpp::slashcommand register_mod_command(dpp::cluster& bot) {
        dpp::slashcommand mod_command("mod", "Moderation commands", bot.me.id);

        mod_command.add_option(
            dpp::command_option(dpp::co_sub_command, "kick", "Kick a user from the server")
            .add_option(dpp::command_option(dpp::co_user, "user", "The user to kick", true))
            .add_option(dpp::command_option(dpp::co_string, "reason", "Reason for kicking the user", false))
        );

        mod_command.add_option(
            dpp::command_option(dpp::co_sub_command, "ban", "Ban a user from the server")
            .add_option(dpp::command_option(dpp::co_user, "user", "The user to ban", true))
            .add_option(dpp::command_option(dpp::co_integer, "delete_days", "Number of days worth of messages to delete (0-7)", false))
            .add_option(dpp::command_option(dpp::co_string, "reason", "Reason for banning the user", false))
        );

        mod_command.add_option(
            dpp::command_option(dpp::co_sub_command, "timeout", "Timeout a user")
            .add_option(dpp::command_option(dpp::co_user, "user", "The user to timeout", true))
            .add_option(dpp::command_option(dpp::co_integer, "duration", "Duration of the timeout in seconds", true))
            .add_option(dpp::command_option(dpp::co_string, "reason", "Reason for the timeout", false))
        );

        mod_command.set_default_permissions(dpp::p_kick_members | dpp::p_ban_members | dpp::p_moderate_members);

        return mod_command;
    }

    void handle_mod_command(const dpp::slashcommand_t& event, dpp::cluster& bot) {
        if (event.command.get_command_interaction().options[0].name == "kick") {
            dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("user"));

            std::string reason = "No reason provided";
            if (std::holds_alternative<std::string>(event.get_parameter("reason"))) {
                reason = std::get<std::string>(event.get_parameter("reason"));
            }

            bot.set_audit_reason(reason);

            bot.guild_member_kick(event.command.guild_id, user_id, [event, &bot, reason](const dpp::confirmation_callback_t& cc) {
                if (cc.is_error()) {
                    event.reply(dpp::message("Failed to kick user: " + cc.get_error().message).set_flags(dpp::m_ephemeral));
                }
                else {
                    event.reply(dpp::message("User kicked successfully.").set_flags(dpp::m_ephemeral));
                    bot.log(dpp::ll_info, "User kicked. Reason: " + reason);
                }
                });
        }
        else if (event.command.get_command_interaction().options[0].name == "ban") {
            dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("user"));

            uint32_t delete_seconds = 0;
            if (std::holds_alternative<int64_t>(event.get_parameter("delete_days"))) {
                int64_t delete_days = std::get<int64_t>(event.get_parameter("delete_days"));
                delete_days = std::max(delete_days, static_cast<int64_t>(0)); // Ensure delete_days is not negative
                delete_days = std::min(delete_days, static_cast<int64_t>(7)); // Ensure delete_days is not greater than 7
                delete_seconds = static_cast<uint32_t>(delete_days * 24 * 60 * 60); // Convert days to seconds
            }

            std::string reason = "No reason provided";
            if (std::holds_alternative<std::string>(event.get_parameter("reason"))) {
                reason = std::get<std::string>(event.get_parameter("reason"));
            }

            bot.set_audit_reason(reason);

            bot.guild_ban_add(event.command.guild_id, user_id, delete_seconds, [event, &bot, reason](const dpp::confirmation_callback_t& cc) {
                if (cc.is_error()) {
                    event.reply(dpp::message("Failed to ban user: " + cc.get_error().message).set_flags(dpp::m_ephemeral));
                }
                else {
                    event.reply(dpp::message("User banned successfully.").set_flags(dpp::m_ephemeral));
                    bot.log(dpp::ll_info, "User banned. Reason: " + reason);
                }
                });
        }
        else if (event.command.get_command_interaction().options[0].name == "timeout") {
            dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("user"));
            int64_t duration = std::get<int64_t>(event.get_parameter("duration"));

            std::string reason = "No reason provided";
            if (std::holds_alternative<std::string>(event.get_parameter("reason"))) {
                reason = std::get<std::string>(event.get_parameter("reason"));
            }

            bot.set_audit_reason(reason);

            time_t timeout_timestamp = time(nullptr) + duration;

            bot.guild_member_timeout(event.command.guild_id, user_id, timeout_timestamp, [event, &bot, reason](const dpp::confirmation_callback_t& cc) {
                if (cc.is_error()) {
                    event.reply(dpp::message("Failed to timeout user: " + cc.get_error().message).set_flags(dpp::m_ephemeral));
                }
                else {
                    event.reply(dpp::message("User timed out successfully.").set_flags(dpp::m_ephemeral));
                    bot.log(dpp::ll_info, "User timed out. Reason: " + reason);
                }
                });
        }
    }
}