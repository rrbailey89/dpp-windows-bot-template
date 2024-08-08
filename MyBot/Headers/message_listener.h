#pragma once
#include <dpp/dpp.h>

class message_listener {
public:
    message_listener(dpp::cluster& bot, dpp::cache<dpp::message>& message_cache);
    void setup();
    static dpp::slashcommand register_setdeletechannel_command(dpp::cluster& bot);

private:
    dpp::cluster& bot;
    dpp::cache<dpp::message>& message_cache;

    void get_emoji_from_openai(const std::string& message, const dpp::message& original_message);
    void on_message_create(const dpp::message_create_t& event);
    void on_message_delete(const dpp::message_delete_t& event);
    void on_message_delete_bulk(const dpp::message_delete_bulk_t& event);
    void on_message_update(const dpp::message_update_t& event);
    void on_slashcommand(const dpp::slashcommand_t& event);
    void handle_leveling(const dpp::message_create_t& event);

};