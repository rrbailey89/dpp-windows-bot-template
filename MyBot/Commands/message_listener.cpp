#include "message_listener.h"
#include "DatabaseManager.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <set>
#include <random>
#include <chrono>
#include <dpp/dpp.h>
#include "FunCommand.h"
#include <cmath>
#include <optional>
#include "ContentModerationCommand.h"


unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
std::default_random_engine generator(seed);
std::uniform_int_distribution<int> distribution(1, 100);

int calculate_required_messages(int level) {
    return static_cast<int>(10 * std::pow(1.5, level - 1));
}

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
    const std::string api_key = DatabaseManager::getInstance().getOpenAIApiKey(); // Replace with your actual OpenAI API key

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
    
    commands::handle_message(event, bot);

    handle_leveling(event);

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

        if (random_value <= 15) {
            std::cout << "Triggering OpenAI API for message: " << message_content << std::endl;
            get_emoji_from_openai(message_content, event.msg);
        }
    }
    if (event.msg.guild_id != 0) {  // Check if the message is from a guild
        if (!commands::is_content_allowed(event.msg.content, event.msg.guild_id)) {
            bot.message_delete(event.msg.id, event.msg.channel_id);
            dpp::message response("Your message was removed because it contains blocked content.");
            response.set_flags(dpp::m_ephemeral);
            bot.direct_message_create(event.msg.author.id, response);
            return;  // Exit early as the message was deleted
        }
    }
}

void message_listener::on_message_delete(const dpp::message_delete_t& event) {
    dpp::message* deleted_message = message_cache.find(event.id);
    if (deleted_message != nullptr) {
        dpp::embed embed;
        embed.set_author(deleted_message->author.username, "", deleted_message->author.get_avatar_url())
            .set_color(0xFF0000)  // Red color
            .add_field("Deleted Message Info", "**Message sent by <@" + std::to_string(deleted_message->author.id) + "> in <#" + std::to_string(deleted_message->channel_id) + "> has been deleted.**", false);

        if (!deleted_message->content.empty()) {
            embed.add_field("Content", deleted_message->content, false);
        }

        if (!deleted_message->attachments.empty()) {
            dpp::embed_footer footer;
            footer.set_text("An image was deleted with this post, attempting to upload below, Message ID: " + std::to_string(deleted_message->id));
            embed.set_footer(footer);
        }

        embed.set_timestamp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));

        uint64_t channel_id = DatabaseManager::getInstance().getMessageDeleteChannelIdForGuild(deleted_message->guild_id);

        if (channel_id > 0) {  // Ensure a valid channel was found
            bot.message_create(dpp::message(channel_id, embed), [this, deleted_message, channel_id](const dpp::confirmation_callback_t& response) {
                for (auto& attachment : deleted_message->attachments) {
                    if (!attachment.url.empty()) {
                        // Download the image from the attachment URL
                        bot.request(attachment.url, dpp::http_method::m_get,
                            [this, channel_id, attachment](const dpp::http_request_completion_t& response) {
                                if (response.status == 200) {  // HTTP OK
                                    std::string ext = attachment.filename.substr(attachment.filename.find_last_of('.'));
                                    std::string file_name = "deleted_image" + ext;
                                    // Upload the downloaded image as a new attachment
                                    dpp::message msg(channel_id, "");
                                    msg.add_file(file_name, response.body);
                                    bot.message_create(msg);
                                }
                            }
                        );
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
                channel_id = DatabaseManager::getInstance().getMessageDeleteChannelIdForGuild(deleted_message->guild_id);
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

            // Handle image attachments if any
            bool hasImageAttachment = std::any_of(deleted_message->attachments.begin(), deleted_message->attachments.end(), [](const dpp::attachment& att) {
                return att.url.find(".png") != std::string::npos || att.url.find(".jpg") != std::string::npos || att.url.find(".jpeg") != std::string::npos || att.url.find(".gif") != std::string::npos;
                });

            if (hasImageAttachment) {
                dpp::embed_footer footer;
                footer.set_text("This message had image(s) attached, attempting to re-upload.");
                embed.set_footer(footer);
            }

            bot.message_create(dpp::message(channel_id, embed), [this, deleted_message, channel_id, hasImageAttachment](const dpp::confirmation_callback_t& response) {
                if (hasImageAttachment) {
                    // Attempt to re-upload the image attachments
                    for (const auto& attachment : deleted_message->attachments) {
                        if (!attachment.url.empty()) {
                            bot.request(attachment.url, dpp::http_method::m_get,
                                [this, channel_id, attachment](const dpp::http_request_completion_t& response) {
                                    if (response.status == 200) {  // HTTP OK
                                        std::string ext = attachment.filename.substr(attachment.filename.find_last_of('.'));
                                        std::string file_name = "restored_image" + ext;
                                        dpp::message msg(channel_id, "");
                                        msg.add_file(file_name, response.body);
                                        bot.message_create(msg);
                                    }
                                }
                            );
                        }
                    }
                }
                });

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
        // Fetch the user's 
        // 
        // 
        // 
        // 
        // s in the guild
        if (event.command.get_resolved_permission(event.command.usr.id).can(dpp::p_manage_guild)) {

            // Check if the user has the permission to manage the guild
            dpp::snowflake channel_id = std::get<dpp::snowflake>(event.get_parameter("channel"));
            dpp::snowflake guild_id = event.command.guild_id;
            std::string guild_name = event.command.get_guild().name;

            // Perform your logic to set the message delete channel here
            DatabaseManager::getInstance().setMessageDeleteChannel(guild_id, guild_name, channel_id); // Adjust your function call accordingly

            event.reply("Delete channel set successfully.");
        }
        else {
            event.reply("You do not have permission to use this command.");
        }
    }
}

void message_listener::handle_leveling(const dpp::message_create_t& event) {
    if (event.msg.author.is_bot()) return; // Ignore bot messages

    dpp::snowflake guild_id = event.msg.guild_id;
    dpp::snowflake user_id = event.msg.author.id;

    DatabaseManager::getInstance().incrementUserMessageCount(guild_id, user_id);
    int total_messages = DatabaseManager::getInstance().getTotalUserMessageCount(guild_id, user_id);
    int current_level = DatabaseManager::getInstance().getUserLevel(guild_id, user_id);

    if (current_level == 0) {
        // First message, set to level 1
        current_level = 1;
        DatabaseManager::getInstance().setUserLevel(guild_id, user_id, current_level);

        auto level_up_channel_id = DatabaseManager::getInstance().getLevelUpChannelId(guild_id);
        if (level_up_channel_id.has_value()) {
            std::string level_up_message = "<@" + std::to_string(user_id) + "> has started their journey at level 1!";
            bot.message_create(dpp::message(level_up_channel_id.value(), level_up_message));
        }
    }

    int required_messages = calculate_required_messages(current_level + 1);

    if (total_messages >= required_messages) {
        // Level up!
        int new_level = current_level + 1;
        DatabaseManager::getInstance().setUserLevel(guild_id, user_id, new_level);

        auto level_up_channel_id = DatabaseManager::getInstance().getLevelUpChannelId(guild_id);
        if (level_up_channel_id.has_value()) {
            std::string level_up_message = "<@" + std::to_string(user_id) + "> has reached level " + std::to_string(new_level) + "!";
            bot.message_create(dpp::message(level_up_channel_id.value(), level_up_message));
        }
    }
}