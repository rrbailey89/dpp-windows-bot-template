#include "ReminderCommand.h"
#include "DatabaseManager.h"

namespace commands {
    dpp::slashcommand register_reminder_command(dpp::cluster& bot) {
        dpp::slashcommand reminder_command("reminder", "Set a reminder", bot.me.id);
        reminder_command.add_option(
            dpp::command_option(dpp::co_sub_command, "create", "Create a new reminder")
            .add_option(dpp::command_option(dpp::co_string, "text", "Reminder text", true))
            .add_option(dpp::command_option(dpp::co_string, "frequency", "Reminder frequency", true)
                .add_choice(dpp::command_option_choice("Weekly", "weekly"))
                .add_choice(dpp::command_option_choice("Monthly", "monthly")))
            .add_option(dpp::command_option(dpp::co_string, "day", "Day of the week (for weekly frequency)", true)
                .add_choice(dpp::command_option_choice("Sunday", "sunday"))
                .add_choice(dpp::command_option_choice("Monday", "monday"))
                .add_choice(dpp::command_option_choice("Tuesday", "tuesday"))
                .add_choice(dpp::command_option_choice("Wednesday", "wednesday"))
                .add_choice(dpp::command_option_choice("Thursday", "thursday"))
                .add_choice(dpp::command_option_choice("Friday", "friday"))
                .add_choice(dpp::command_option_choice("Saturday", "saturday")))
            .add_option(dpp::command_option(dpp::co_string, "time", "Reminder time (HH:MM AM/PM)", true))
            .add_option(dpp::command_option(dpp::co_channel, "channel", "Channel to send reminder", true))
        );
        reminder_command.add_option(
            dpp::command_option(dpp::co_sub_command, "remove", "Remove an existing reminder")
            .add_option(dpp::command_option(dpp::co_integer, "id", "The ID of the reminder to remove", true))
        );

        reminder_command.set_default_permissions(dpp::p_manage_guild);

        return reminder_command;
    }

    void handle_reminder_command(const dpp::slashcommand_t& event, dpp::cluster& bot) {
        try {
            // Get the subcommand
            bot.log(dpp::ll_info, "Retrieving subcommand...");

            // Log the number of options
            bot.log(dpp::ll_info, "Number of options: " + std::to_string(event.command.get_command_interaction().options.size()));

            // Log the type of the first option
            if (!event.command.get_command_interaction().options.empty()) {
                bot.log(dpp::ll_info, "Type of first option: " + std::to_string(static_cast<int>(event.command.get_command_interaction().options[0].type)));
            }

            std::string subcommand_name = event.command.get_command_interaction().options[0].name;

            bot.log(dpp::ll_info, "Subcommand: " + subcommand_name);

            if (subcommand_name == "create") {
                bot.log(dpp::ll_info, "Retrieving reminder text...");
                std::string reminder_text = std::get<std::string>(event.get_parameter("text"));
                bot.log(dpp::ll_info, "Reminder text: " + reminder_text);

                bot.log(dpp::ll_info, "Retrieving frequency...");
                std::string frequency = std::get<std::string>(event.get_parameter("frequency"));
                bot.log(dpp::ll_info, "Frequency: " + frequency);

                bot.log(dpp::ll_info, "Retrieving day...");
                std::string day = std::get<std::string>(event.get_parameter("day"));
                bot.log(dpp::ll_info, "Day: " + day);

                bot.log(dpp::ll_info, "Retrieving time...");
                std::string time = std::get<std::string>(event.get_parameter("time"));
                bot.log(dpp::ll_info, "Time: " + time);

                bot.log(dpp::ll_info, "Retrieving channel ID...");
                dpp::snowflake channel_id = std::get<dpp::snowflake>(event.get_parameter("channel"));
                bot.log(dpp::ll_info, "Channel ID: " + std::to_string(channel_id));

                // Store the reminder information in the database
                bot.log(dpp::ll_info, "Storing reminder in the database...");
                DatabaseManager::getInstance().storeReminder(event.command.guild_id, reminder_text, frequency, day, time, channel_id);
                bot.log(dpp::ll_info, "Reminder stored successfully.");

                event.reply("Reminder created successfully!");
            }
            else if (subcommand_name == "remove") {
                int64_t reminder_id = std::get<int64_t>(event.get_parameter("id"));
                bot.log(dpp::ll_info, "Attempting to remove reminder with ID: " + std::to_string(reminder_id));

                // Function to remove the reminder from the database
                bool success = DatabaseManager::getInstance().removeReminder(reminder_id);
                if (success) {
                    event.reply("Reminder removed successfully!");
                }
                else {
                    event.reply("Failed to remove the reminder. It may not exist or there was a database error.");
                }
            }
            else {
                // Unknown subcommand
                bot.log(dpp::ll_error, "Unknown subcommand: " + subcommand_name);
                event.reply("Unknown subcommand.");
            }
        }
        catch (const std::bad_variant_access& e) {
            bot.log(dpp::ll_error, std::string("Exception in reminder command: ") + e.what());
            event.reply("An error occurred while processing the command.");
        }
    }
}