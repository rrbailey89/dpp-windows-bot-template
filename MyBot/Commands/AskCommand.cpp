#include "AskCommand.h"
#include "DatabaseManager.h"
#include "fstream"

namespace commands {

    dpp::slashcommand register_ask_command(dpp::cluster& bot) {
        dpp::slashcommand ask_command("ask", "Ask a question to the OpenAI API", bot.me.id);
        ask_command.add_option(
            dpp::command_option(dpp::co_string, "question", "The question to ask", true)
        );
        return ask_command;
    }

    void handle_ask_command(const dpp::slashcommand_t& event, dpp::cluster& bot) {
        std::string question = std::get<std::string>(event.get_parameter("question"));
        dpp::snowflake user_id = event.command.usr.id;

        std::vector<nlohmann::json> conversation = DatabaseManager::getInstance().getConversation(user_id);

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
            {"messages", conversation}
        };

        request_body["messages"].push_back({ {"role", "user"}, {"content", question} });

        // Setup the headers
        std::multimap<std::string, std::string> headers = {
            {"Authorization", "Bearer " + api_key},
            {"Content-Type", "application/json"}
        };
        event.thinking();

        // Making the POST request to the OpenAI API
        std::cout << "Sending request to OpenAI API..." << std::endl;
        bot.request(endpoint, dpp::http_method::m_post,
            [event, &bot, user_id](const dpp::http_request_completion_t& response) {
                if (response.status == 200) { // HTTP OK
                    std::cout << "Received response from OpenAI API. Status: " << response.status << std::endl;
                    nlohmann::json response_json = nlohmann::json::parse(response.body);
                    std::cout << "OpenAI API Response: " << response_json.dump(4) << std::endl;

                    // Log the response to a file
                    std::ofstream log_file("openai_response.log", std::ios::app);
                    if (log_file.is_open()) {
                        log_file << "OpenAI API Response:\n" << response_json.dump(4) << "\n\n";
                        log_file.close();
                    }
                    else {
                        std::cerr << "Failed to open log file." << std::endl;
                    }

                    std::string answer = response_json["choices"][0]["message"]["content"].get<std::string>();

                    // Add the user's question and the assistant's answer to the conversation history
                    nlohmann::json user_message = { {"role", "user"}, {"content", std::get<std::string>(event.get_parameter("question"))} };
                    nlohmann::json assistant_message = { {"role", "assistant"}, {"content", answer} };
                    DatabaseManager::getInstance().storeConversationMessage(user_id, user_message);
                    DatabaseManager::getInstance().storeConversationMessage(user_id, assistant_message);

                    // Edit the original "thinking" response with the actual answer
                    event.edit_original_response(dpp::message(answer));
                }
                else {
                    std::cerr << "Error from OpenAI API. Status: " << response.status << ". Body: " << response.body << std::endl;
                    event.edit_original_response(dpp::message("An error occurred while processing your request."));
                }
            },
            request_body.dump(), "application/json", headers, "1.0"
        );
    }
}