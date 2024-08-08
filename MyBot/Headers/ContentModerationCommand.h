#pragma once
#include <dpp/dpp.h>

namespace commands {
    dpp::slashcommand register_content_moderation_command(dpp::cluster& bot);
    void handle_content_moderation_command(const dpp::slashcommand_t& event, dpp::cluster& bot);
    bool is_content_allowed(const std::string& content, dpp::snowflake guild_id);
}