#pragma once
#include <dpp/dpp.h>

namespace commands {
    dpp::slashcommand register_ask_command(dpp::cluster& bot);
    void handle_ask_command(const dpp::slashcommand_t& event, dpp::cluster& bot);
}