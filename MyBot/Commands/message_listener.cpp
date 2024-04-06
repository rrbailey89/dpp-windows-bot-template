#include "message_listener.h"
#include "db_access.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <set>
#include <random>
#include <chrono>
#include <dpp/dpp.h>

unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
std::default_random_engine generator(seed);
std::uniform_int_distribution<int> distribution(1, 100);

message_listener::message_listener(dpp::cluster& bot, dpp::cache<dpp::message>& message_cache)
    : bot(bot), message_cache(message_cache) {}

dpp::slashcommand message_listener::register_setdeletechannel_command(dpp::cluster& bot) {
    dpp::slashcommand setdeletechannel_command("setdeletechannel", "Set the channel for deleted messages", bot.me.id);
    setdeletechannel_command.add_option(dpp::command_option(dpp::co_channel, "channel", "Choose a channel", true));
    return setdeletechannel_command;
}

void message_listener::setup() {
    bot.on_message_create([this](const dpp::message_create_t& event) {
        on_message_create(event);
        });

    bot.on_message_delete([this](const dpp::message_delete_t& event) {
        on_message_delete(event);
        });

    bot.on_message_delete_bulk([this](const dpp::message_delete_bulk_t& event) {
        on_message_delete_bulk(event);
        });

    bot.on_message_update([this](const dpp::message_update_t& event) {
        on_message_update(event);
        });

    bot.on_slashcommand([this](const dpp::slashcommand_t& event) {
        on_slashcommand(event);
        });
}

void message_listener::get_emoji_from_openai(const std::string& message, const dpp::message& original_message) {
    // OpenAI API setup
    const std::string endpoint = "https://api.openai.com/v1/chat/completions";
    const std::string api_key = get_openai_api_key(); // Replace with your actual OpenAI API key

    // Prepare the JSON payload
    nlohmann::json request_body = {
        {"model", "gpt-3.5-turbo"},
        {"temperature", 0.7},
        {"max_tokens", 256},
        {"top_p", 1},
        {"frequency_penalty", 0},
        {"presence_penalty", 0},
        {"messages", {
            {{"role", "system"}, {"content", "You are only capable of responding to the message with a single emoji that best represents the message."}},
            {{"role", "user"}, {"content", message}}
        }}
    };

    // Setup the headers
    std::multimap<std::string, std::string> headers = {
        {"Authorization", "Bearer " + api_key},
        {"Content-Type", "application/json"}
    };

    // Making the POST request to the OpenAI API
    std::cout << "Sending request to OpenAI API..." << std::endl;
    bot.request(endpoint, dpp::http_method::m_post,
        [this, original_message](const dpp::http_request_completion_t& response) {
            if (response.status == 200) { // HTTP OK
                std::cout << "Received response from OpenAI API. Status: " << response.status << std::endl;
                nlohmann::json response_json = nlohmann::json::parse(response.body);
                std::cout << "OpenAI API Response: " << response_json.dump(4) << std::endl;
                std::string emoji = response_json["choices"][0]["message"]["content"].get<std::string>();
                bot.message_add_reaction(original_message, emoji); // React to the original message with the emoji
            }
            else {
                std::cerr << "Error from OpenAI API. Status: " << response.status << ". Body: " << response.body << std::endl;
            }
        },
        request_body.dump(), "application/json", headers
    );
}

void message_listener::on_message_create(const dpp::message_create_t& event) {
    // Store the message in the cache
    dpp::message* msg_copy = new dpp::message(event.msg);
    message_cache.store(msg_copy);    
    if (event.msg.author.id == bot.me.id || event.msg.author.is_bot()) {
        return;
    }

    std::vector<std::string> keywords = { "pierce brosnan", "yoshi p", "yoship", "yoshi-p", "japan", "raid", "shion", "saskia", "erik", "amia", "opal", "lief", "dyna", "bot", "cat", "broken", "lol", "lmao", "a8s" };
    std::string message_content = event.msg.content;
    std::transform(message_content.begin(), message_content.end(), message_content.begin(), ::tolower);

    std::istringstream iss(message_content);
    std::set<std::string> tokens{ std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{} };

    bool trigger_word_found = false;

    for (const auto& keyword : keywords) {
        std::string lowercase_keyword = keyword;
        std::transform(lowercase_keyword.begin(), lowercase_keyword.end(), lowercase_keyword.begin(), ::tolower);

        if ((lowercase_keyword.find(' ') != std::string::npos && message_content.find(lowercase_keyword) != std::string::npos) ||
            (tokens.find(lowercase_keyword) != tokens.end())) {

            trigger_word_found = true;

            int random_value = distribution(generator);

            if (random_value <= 40) {
                if (lowercase_keyword == "cat") {
                    bot.message_add_reaction(event.msg, "\U0001F431"); // Cat emoji
                }
                else if (lowercase_keyword == "broken") {
                    bot.message_add_reaction(event.msg, "\U0001F914"); // Thinking emoji
                }
                else if (lowercase_keyword == "lol" || lowercase_keyword == "lmao") {
                    bot.message_add_reaction(event.msg, "\U0001F923"); // ROFL emoji
                }
                else if (lowercase_keyword == "saskia") {
                    bot.message_add_reaction(event.msg, "\U0001F49C"); // Purple Heart emoji
                }
                else if (lowercase_keyword == "yoship" || lowercase_keyword == "yoshi p" || lowercase_keyword == "yoshi-p") {
                    bot.message_add_reaction(event.msg, "\:wine31:1223785508702781460"); // Yoshi-P emoji
                }
                else {
                    bot.message_add_reaction(event.msg, "\U0001F916"); // Robot emoji
                }
            }
        }
    }

    if (!trigger_word_found) {
        int random_value = distribution(generator);

        if (random_value <= 30) {
            std::cout << "Triggering OpenAI API for message: " << message_content << std::endl;
            get_emoji_from_openai(message_content, event.msg);
        }
    }
}

void message_listener::on_message_delete(const dpp::message_delete_t& event) {
    dpp::message* deleted_message = message_cache.find(event.id);
    if (deleted_message != nullptr) {
        dpp::embed embed;
        embed.set_author(deleted_message->author.username, "", deleted_message->author.get_avatar_url())
            .set_color(0xFF0000) // Red color
            .add_field("Deleted Message Info", "**Message sent by <@" + std::to_string(deleted_message->author.id) + "> in <#" + std::to_string(deleted_message->channel_id) + "> has been deleted.**", false);

        if (!deleted_message->content.empty()) {
            embed.add_field("Content", deleted_message->content, false);
        }

        bool hasImageAttachment = std::any_of(deleted_message->attachments.begin(), deleted_message->attachments.end(), [](const dpp::attachment& att) {
            return att.url.find(".png") != std::string::npos || att.url.find(".jpg") != std::string::npos || att.url.find(".jpeg") != std::string::npos || att.url.find(".gif") != std::string::npos;
            });

        if (hasImageAttachment) {
            dpp::embed_footer footer;
            footer.set_text("An image was deleted with this post, attempting to upload below, Message ID: " + std::to_string(deleted_message->id));
            embed.set_footer(footer);
        }

        embed.set_timestamp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));

        // Query the database to get the correct channel ID for this guild
        uint64_t channel_id = get_message_delete_channel_id_for_guild(deleted_message->guild_id);

        if (channel_id > 0) { // Ensure a valid channel was found
            bot.message_create(dpp::message(channel_id, embed), [this, deleted_message, hasImageAttachment, channel_id](const dpp::confirmation_callback_t& response) {
                if (hasImageAttachment) {
                    for (auto& attachment : deleted_message->attachments) {
                        if (!attachment.url.empty()) {
                            bot.message_create(dpp::message(channel_id, attachment.url));
                        }
                    }
                }
                });
        }
        else {
            std::cerr << "No delete channel set for guild: " << deleted_message->guild_id << std::endl;
        }

        message_cache.remove(deleted_message);
    }
}

void message_listener::on_message_delete_bulk(const dpp::message_delete_bulk_t& event) {
    uint64_t channel_id = 0; // This will be set based on the first found message
    bool channel_id_set = false;

    int message_count = 1;
    for (auto id : event.deleted) {
        dpp::message* deleted_message = message_cache.find(id);
        if (deleted_message != nullptr) {
            if (!channel_id_set) {
                // Retrieve the correct channel ID for this guild from the database
                // This assumes all messages in the bulk delete event are from the same guild
                channel_id = get_message_delete_channel_id_for_guild(deleted_message->guild_id);
                if (channel_id == 0) {
                    std::cerr << "No delete channel set for guild: " << deleted_message->guild_id << std::endl;
                    return; // Early exit if no channel is configured for the guild
                }
                channel_id_set = true;
            }

            dpp::embed embed;
            embed.set_author(deleted_message->author.username, "", deleted_message->author.get_avatar_url())
                .set_color(0xFF0000) // Red color
                .add_field("Bulk message deleted", "Message " + std::to_string(message_count) + " of " + std::to_string(event.deleted.size()), false)
                .add_field("Content", deleted_message->content, false)
                .set_timestamp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));

            // Send the embed to the dynamically determined channel ID
            bot.message_create(dpp::message(channel_id, embed));

            message_cache.remove(deleted_message); // Remove the message from cache after processing

            ++message_count;
        }
    }

    if (channel_id_set && event.deleted.size() > 0) {
        std::cout << "Processed bulk message delete. Messages deleted: " << event.deleted.size() << std::endl;
    }
    else {
        std::cerr << "Bulk delete event with no accessible messages in cache or no channel configured." << std::endl;
    }
}

void message_listener::on_message_update(const dpp::message_update_t& event) {
    dpp::message* cached_message = message_cache.find(event.msg.id);
    if (cached_message != nullptr) {
        // Update the content of the cached message
        cached_message->content = event.msg.content;
        // Update any other relevant fields of the cached message
    }
}

void message_listener::on_slashcommand(const dpp::slashcommand_t& event) {
    if (event.command.get_command_name() == "setdeletechannel") {
        // Fetch the user's permissions in the guild
        if (event.command.get_resolved_permission(event.command.usr.id).can(dpp::p_manage_guild)) {

            // Check if the user has the permission to manage the guild
            dpp::snowflake channel_id = std::get<dpp::snowflake>(event.get_parameter("channel"));
            dpp::snowflake guild_id = event.command.guild_id;
            std::string guild_name = event.command.get_guild().name;

            // Perform your logic to set the message delete channel here
            set_message_delete_channel(guild_id, guild_name, channel_id); // Adjust your function call accordingly

            event.reply("Delete channel set successfully.");
        }
        else {
            event.reply("You do not have permission to use this command.");
        }
    }
}