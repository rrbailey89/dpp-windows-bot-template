#pragma once
#include <dpp/dpp.h>

namespace commands {
    dpp::slashcommand register_setlevelupchannel_command(dpp::cluster& bot);
    void handle_setlevelupchannel_command(const dpp::slashcommand_t& event, dpp::cluster& bot);
}