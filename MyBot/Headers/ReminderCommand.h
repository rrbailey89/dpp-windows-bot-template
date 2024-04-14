#pragma once
#include <dpp/dpp.h>

namespace commands {
    dpp::slashcommand register_reminder_command(dpp::cluster& bot);
    void handle_reminder_command(const dpp::slashcommand_t& event, dpp::cluster& bot);
}