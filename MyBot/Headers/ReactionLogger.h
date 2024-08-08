#pragma once
#include <dpp/dpp.h>

namespace commands {
    dpp::slashcommand register_setreactionchannel_command(dpp::cluster& bot);
    void handle_setreactionchannel_command(const dpp::slashcommand_t& event, dpp::cluster& bot);
}

void send_reaction_embed(dpp::cluster& bot, const dpp::message_reaction_add_t& event);