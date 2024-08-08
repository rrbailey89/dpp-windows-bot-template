#include "FunCommand.h"
#include <random>
#include <unordered_map>
#include "DatabaseManager.h"
#include "curl/curl.h"

namespace commands {

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    std::unordered_map<dpp::snowflake, int> game_instances;  // Map to store game instances

    dpp::slashcommand register_fun_command(dpp::cluster& bot) {
        dpp::slashcommand fun_command("fun", "Fun commands", bot.me.id);
        fun_command.add_option(
            dpp::command_option(dpp::co_sub_command, "cat", "Get a random cat picture")
        );
        fun_command.add_option(
            dpp::command_option(dpp::co_sub_command, "hug", "Get a random hug GIF")
            .add_option(dpp::command_option(dpp::co_user, "user", "The user to hug", true))

        );
        fun_command.add_option(
            dpp::command_option(dpp::co_sub_command, "guessinggame", "Play a guessing game (Different numbers for each player)")
        );
        fun_command.add_option(
            dpp::command_option(dpp::co_sub_command, "rps", "Play rock-paper-scissors")
            .add_option(dpp::command_option(dpp::co_string, "choice", "Your choice", true)
                .add_choice(dpp::command_option_choice("Rock", std::string("rock")))
                .add_choice(dpp::command_option_choice("Paper", std::string("paper")))
                .add_choice(dpp::command_option_choice("Scissors", std::string("scissors")))
            )
        );
        fun_command.add_option(
            dpp::command_option(dpp::co_sub_command, "capy", "Get a random capybara picture")
        );
        return fun_command;
    }

    void handle_fun_command(const dpp::slashcommand_t& event, dpp::cluster& bot) {

        // Get the subcommand
        std::string sub_command = event.command.get_command_interaction().options[0].name;

        if (sub_command == "cat") {
            static bool use_cataas = true;  // Flag to alternate between APIs

            if (use_cataas) {
                // Use the Cataas API
                bot.request("https://cataas.com/cat", dpp::m_get, [event, &bot](const dpp::http_request_completion_t& response) {
                    if (response.status == 200) {
                        dpp::embed embed;
                        embed.set_color(dpp::colors::blurple);
                        embed.set_title("Cat!");
                        embed.set_image("attachment://cat.jpg");

                        event.reply(
                            dpp::message()
                            .add_embed(embed)
                            .add_file("cat.jpg", response.body)
                        );
                    }
                    else {
                        event.reply("Failed to retrieve a cat picture from Cataas.");
                    }
                    });
            }
            else {
                // Use The Cat API
                bot.request("https://api.thecatapi.com/v1/images/search", dpp::m_get, [event, &bot](const dpp::http_request_completion_t& response) {
                    if (response.status == 200) {
                        nlohmann::json result = nlohmann::json::parse(response.body);
                        std::string image_url = result[0]["url"];

                        dpp::embed embed;
                        embed.set_color(dpp::colors::blurple);
                        embed.set_title("Cat!");
                        embed.set_image(image_url);

                        event.reply(
                            dpp::message().add_embed(embed)
                        );
                    }
                    else {
                        event.reply("Failed to retrieve a cat picture from The Cat API.");
                    }
                    });
            }

            use_cataas = !use_cataas;  // Toggle the flag for the next command invocation
        }

        else if (sub_command == "hug") {
            // Get the mentioned user
            dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("user"));
            dpp::user mentioned_user = bot.user_get_sync(user_id);

            // Get the user who invoked the command
            dpp::user invoker = event.command.usr;

            // Get the current hug count for the mentioned user
            int hug_count = DatabaseManager::getInstance().getUserHugCount(user_id);

            // Increment the hug count
            DatabaseManager::getInstance().incrementUserHugCount(user_id);

            // Create the message text with user mentions and hug count
            std::string message_text = mentioned_user.get_mention() + "was hugged by " + invoker.get_mention() + " they have been hugged " + std::to_string(hug_count + 1) + " times.";

            // Use the nekos.best API for hugs
            bot.request("https://nekos.best/api/v2/hug", dpp::m_get, [event, &bot, message_text](const dpp::http_request_completion_t& response) {
                if (response.status == 200) {
                    nlohmann::json result = nlohmann::json::parse(response.body);
                    std::string gif_url = result["results"][0]["url"];
                    std::string anime_name = result["results"][0]["anime_name"];

                    dpp::embed embed;
                    embed.set_color(dpp::colors::blurple);
                    embed.set_title("From the Anime: " + anime_name);
                    embed.set_image(gif_url);

                    // Send the message text and embed in a single response
                    event.reply(
                        dpp::message(message_text).add_embed(embed)
                    );
                }
                else {
                    event.reply("Failed to retrieve a hug GIF.");
                }
                });
        }

        else if (sub_command == "guessinggame") {
            dpp::snowflake user_id = event.command.usr.id;  // Get the user ID

            // Check if the user has an ongoing game instance
            if (game_instances.find(user_id) == game_instances.end()) {
                // Generate a new random number for the user
                std::random_device rd;
                std::mt19937 rng(rd());
                std::uniform_int_distribution<int> dist(1, 100);
                game_instances[user_id] = dist(rng);

                // Output the secret number to the console (for debugging purposes)
                std::cout << "New game started for user " << user_id << ". Secret number: " << game_instances[user_id] << std::endl;

                // Send a message prompting the user to guess the number
                event.reply("I have selected a number between 1 and 100. Try to guess it by typing your guess in the chat!");
            }
            else {
                event.reply("You already have an ongoing guessing game. Type your guess in the chat to continue playing.");
            }
        }

        else if (sub_command == "capy") {
            CURL* curl;
            CURLcode res;
            std::string readBuffer;

            curl = curl_easy_init();
            if (curl) {
                // Disable SSL certificate verification
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

                curl_easy_setopt(curl, CURLOPT_URL, "https://api.capy.lol/v1/capybara?json=true");
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

                res = curl_easy_perform(curl);
                curl_easy_cleanup(curl);

                if (res == CURLE_OK) {
                    try {
                        nlohmann::json result = nlohmann::json::parse(readBuffer);
                        if (result["success"].get<bool>()) {
                            nlohmann::json data = result["data"];
                            std::string image_url = data["url"].get<std::string>();
                            std::string alt_text = data.value("alt", "");

                            // Ensure the alt text is no longer than 256 characters
                            if (alt_text.length() > 256) {
                                alt_text = alt_text.substr(0, 253) + "...";
                            }

                            dpp::embed embed;
                            embed.set_color(dpp::colors::blurple);
                            embed.set_title(alt_text.empty() ? "Capybara!" : alt_text);
                            embed.set_image(image_url);

                            event.reply(
                                dpp::message().add_embed(embed)
                            );
                        }
                        else {
                            event.reply("Failed to retrieve a capybara picture.");
                        }
                    }
                    catch (const std::exception& e) {
                        event.reply("Failed to parse the capybara API response.");
                    }
                }
                else {
                    event.reply("Failed to retrieve a capybara picture.");
                }
            }
            else {
                event.reply("Failed to initialize CURL.");
            }
            }

        else if (sub_command == "rps") {
                std::string player_choice_str = std::get<std::string>(event.get_parameter("choice"));

                // Map player's choice to corresponding integer
                std::unordered_map<std::string, int> choice_map = {
                    {"rock", 0},
                    {"paper", 1},
                    {"scissors", 2}
                };

                int player_choice = choice_map[player_choice_str];

                // Generate bot's choice randomly
                std::random_device rd;
                std::mt19937 rng(rd());
                std::uniform_int_distribution<int> dist(0, 2);
                int bot_choice = dist(rng);

                // Map bot's choice to corresponding string
                std::unordered_map<int, std::string> reverse_choice_map = {
                    {0, "rock"},
                    {1, "paper"},
                    {2, "scissors"}
                };

                // Determine the outcome
                std::string outcome;
                if (player_choice == bot_choice) {
                    outcome = "It's a tie!";
                }
                else if ((player_choice == 0 && bot_choice == 2) ||
                    (player_choice == 1 && bot_choice == 0) ||
                    (player_choice == 2 && bot_choice == 1)) {
                    outcome = "You win!";
                }
                else {
                    outcome = "You lose!";
                }

                // Reply with the outcome
                event.reply("You chose: " + player_choice_str + "\n" +
                    "Bot chose: " + reverse_choice_map[bot_choice] + "\n" +
                    outcome);
            }
    }

    void handle_message(const dpp::message_create_t& event, dpp::cluster& bot) {
        dpp::snowflake user_id = event.msg.author.id;  // Get the user ID

        // Check if the user has an ongoing game instance
        if (game_instances.find(user_id) != game_instances.end()) {
            try {
                int player_guess = std::stoi(event.msg.content);
                int secret_number = game_instances[user_id];  // Get the secret number for the user

                // Check the player's guess
                if (player_guess == secret_number) {
                    bot.message_create(dpp::message(event.msg.channel_id, "Congratulations! You guessed the correct number: " + std::to_string(secret_number)));
                    game_instances.erase(user_id);  // Remove the game instance for the user
                }
                else if (player_guess < secret_number) {
                    bot.message_create(dpp::message(event.msg.channel_id, "Too low! Try again."));
                }
                else {
                    bot.message_create(dpp::message(event.msg.channel_id, "Too high! Try again."));
                }
            }
            catch (const std::invalid_argument&) {
                // Ignore invalid input (non-numeric guesses)
            }
        }
    }
}
