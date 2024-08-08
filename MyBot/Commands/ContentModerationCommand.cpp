#include "ContentModerationCommand.h"
#include "DatabaseManager.h"
#include <regex>

namespace commands {

    dpp::slashcommand register_content_moderation_command(dpp::cluster& bot) {
        dpp::slashcommand mod_command("moderation", "Content moderation commands", bot.me.id);

        mod_command.add_option(
            dpp::command_option(dpp::co_sub_command, "add", "Add a blocked content pattern")
            .add_option(dpp::command_option(dpp::co_string, "pattern", "Text or regex pattern to block", true))
            .add_option(dpp::command_option(dpp::co_boolean, "is_regex", "Whether the pattern is a regex", false))
        );

        mod_command.add_option(
            dpp::command_option(dpp::co_sub_command, "remove", "Remove a blocked content pattern")
            .add_option(dpp::command_option(dpp::co_string, "pattern", "Text or regex pattern to remove", true))
        );

        mod_command.add_option(
            dpp::command_option(dpp::co_sub_command, "list", "List all blocked content patterns")
        );

        mod_command.set_default_permissions(dpp::p_manage_guild);

        return mod_command;
    }

    void handle_content_moderation_command(const dpp::slashcommand_t& event, dpp::cluster& bot) {
        dpp::guild_member member = event.command.member;
        dpp::snowflake guild_id = event.command.guild_id;

        if (!event.command.get_resolved_permission(event.command.usr.id).can(dpp::p_manage_guild)) {
            event.co_edit_original_response(dpp::message("You do not have permission to use this command.").set_flags(dpp::m_ephemeral));
            return;
        }

        std::string sub_command = event.command.get_command_interaction().options[0].name;

        if (sub_command == "add") {
            std::string pattern = std::get<std::string>(event.get_parameter("pattern"));
            bool is_regex = event.get_parameter("is_regex").index() == 0 ? false : std::get<bool>(event.get_parameter("is_regex"));
            if (is_regex) {
                try {
                    std::regex test_regex(pattern);
                }
                catch (const std::regex_error& e) {
                    event.reply(dpp::message("Invalid regex pattern. Please check your syntax.").set_flags(dpp::m_ephemeral));
                    return;
                }
            }
            DatabaseManager::getInstance().addBlockedContent(guild_id, pattern, is_regex);
            event.reply(dpp::message("Blocked content pattern added successfully.").set_flags(dpp::m_ephemeral));
        }
        else if (sub_command == "remove") {
            std::string pattern = std::get<std::string>(event.get_parameter("pattern"));
            bool removed = DatabaseManager::getInstance().removeBlockedContent(guild_id, pattern);
            if (removed) {
                event.reply(dpp::message("Blocked content pattern removed successfully.").set_flags(dpp::m_ephemeral));
            }
            else {
                event.reply(dpp::message("Pattern not found in the blocked content list.").set_flags(dpp::m_ephemeral));
            }
        }
        else if (sub_command == "list") {
            auto blocked_content = DatabaseManager::getInstance().getBlockedContent(guild_id);
            std::string response = "Blocked content patterns:\n";
            for (const auto& [pattern, is_regex] : blocked_content) {
                response += (is_regex ? "[Regex] " : "[Text] ") + pattern + "\n";
            }
            event.reply(dpp::message(response).set_flags(dpp::m_ephemeral));
        }
    }

    bool is_content_allowed(const std::string& content, dpp::snowflake guild_id) {
        auto blocked_content = DatabaseManager::getInstance().getBlockedContent(guild_id);
        for (const auto& [pattern, is_regex] : blocked_content) {
            if (is_regex) {
                std::regex regex_pattern(pattern);
                if (std::regex_search(content, regex_pattern)) {
                    return false;
                }
            }
            else {
                if (content.find(pattern) != std::string::npos) {
                    return false;
                }
            }
        }
        return true;
    }

} // namespace commands