#include "ReactionLogger.h"
#include "DatabaseManager.h"
#include <iostream>

namespace commands {
    dpp::slashcommand register_setreactionchannel_command(dpp::cluster& bot) {
        dpp::slashcommand command("setreactionchannel", "Set the channel for reaction tracking", bot.me.id);
        command.add_option(dpp::command_option(dpp::co_channel, "channel", "The channel to use for reaction tracking", true));
        command.set_default_permissions(dpp::p_manage_guild);
        return command;
    }

    void handle_setreactionchannel_command(const dpp::slashcommand_t& event, dpp::cluster& bot) {
        dpp::snowflake guild_id = event.command.guild_id;
        dpp::snowflake channel_id = std::get<dpp::snowflake>(event.get_parameter("channel"));

        // Check if the user executing the command is the guild owner
        dpp::snowflake owner_id = DatabaseManager::getInstance().getGuildOwnerId(guild_id);
        if (event.command.usr.id != owner_id) {
            event.reply("This command can only be used by the guild's owner.");
            return;
        }

        // Set the reaction tracking channel in the database
        DatabaseManager::getInstance().setReactionTrackingChannel(guild_id, channel_id);

        event.reply("Reaction tracking channel set successfully.");
    }
}

void send_reaction_embed(dpp::cluster& bot, const dpp::message_reaction_add_t& event) {
    // Check if the reaction is from a guild
    if (!event.reacting_guild) {
        return; // Ignore reactions from DMs or other non-guild contexts
    }

    // Get the reaction tracking channel for this guild
    dpp::snowflake tracking_channel_id = DatabaseManager::getInstance().getReactionTrackingChannel(event.reacting_guild->id);

    // If no tracking channel is set, don't attempt to log the reaction
    if (tracking_channel_id == 0) {
        return;
    }

    // Ignore reactions from bots
    if (event.reacting_user.is_bot()) {
        return;
    }

    dpp::snowflake user_id = event.reacting_user.id;
    std::string emoji_name = event.reacting_emoji.name;

    std::string message_link = dpp::utility::message_url(event.reacting_guild->id, event.channel_id, event.message_id);

    std::string nickname_or_username = event.reacting_member.get_nickname().empty() ? event.reacting_user.username : event.reacting_member.get_nickname();

    dpp::embed reaction_embed;
    reaction_embed.set_title("Reaction Received from " + nickname_or_username)
        .set_color(0x00ff00)
        .add_field("User Reacted", "<@" + std::to_string(user_id) + ">", true)
        .add_field("Emoji Used", emoji_name, true)
        .add_field("Message Link", message_link, false);

    bot.message_create(dpp::message(tracking_channel_id, "").add_embed(reaction_embed), [](const dpp::confirmation_callback_t& response) {
        if (response.is_error()) {
            std::cerr << "Error sending embed: " << response.get_error().message << "\n";
        }
        else {
            std::cout << "Embed sent successfully\n";
        }
        });
}