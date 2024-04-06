#pragma once
#include <dpp/dpp.h>

namespace commands {
    dpp::slashcommand register_mod_command(dpp::cluster& bot);
    void handle_mod_command(const dpp::slashcommand_t& event, dpp::cluster& bot);
}