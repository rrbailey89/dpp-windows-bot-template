#include "AskCommand.h"
#include "DatabaseManager.h"
#include "fstream"
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <regex>
#include <curl/curl.h>

namespace commands {

    dpp::slashcommand register_ask_command(dpp::cluster& bot) {
        dpp::slashcommand ask_command("ask", "Ask a question to the OpenAI API", bot.me.id);
        ask_command.add_option(
            dpp::command_option(dpp::co_sub_command, "question", "Ask a question")
            .add_option(dpp::command_option(dpp::co_string, "query", "The question to ask", true))
        );
        ask_command.add_option(
            dpp::command_option(dpp::co_sub_command, "image", "Generate an image using the DALL-E 3 model")
            .add_option(dpp::command_option(dpp::co_string, "prompt", "The prompt for image generation", true))
        );
        return ask_command;
    }

    static size_t writeCallback(char* ptr, size_t size, size_t nmemb, std::string* data) {
        data->append(ptr, size * nmemb);
        return size * nmemb;
    }

    void handle_ask_command(const dpp::slashcommand_t& event, dpp::cluster& bot) {
        // Get the subcommand
        std::string sub_command = event.command.get_command_interaction().options[0].name;

        if (sub_command == "question") {
            std::string question = std::get<std::string>(event.get_parameter("query"));
            dpp::snowflake user_id = event.command.usr.id;

            // OpenAI API setup
            const std::string api_key = DatabaseManager::getInstance().getOpenAIApiKey(); // Replace with your actual OpenAI API key

            // Setup the headers
            std::multimap<std::string, std::string> headers = {
                {"Authorization", "Bearer " + api_key},
                {"Content-Type", "application/json"},
                {"OpenAI-Beta", "assistants=v2"}
            };
            event.thinking();

            // Check if a conversation thread exists for the user in the database
            std::string thread_id = DatabaseManager::getInstance().getConversationThreadIdForUser(user_id);

            auto check_run_status = [&bot, headers, event](const std::string& thread_id, const std::string& run_id) {
                std::string run_status_endpoint = "https://api.openai.com/v1/threads/" + thread_id + "/runs/" + run_id;
                auto run_completed = std::make_shared<std::atomic<bool>>(false);
                auto mtx = std::make_shared<std::mutex>();
                auto cv = std::make_shared<std::condition_variable>();

                std::thread([&bot, run_status_endpoint, thread_id, run_id, headers, event, run_completed, mtx, cv]() {
                    while (!run_completed->load()) {
                        std::unique_lock<std::mutex> lock(*mtx);
                        cv->wait_for(lock, std::chrono::seconds(2), [run_completed]() { return run_completed->load(); });

                        if (run_completed->load()) {
                            break;
                        }

                        std::cout << "Checking run status for run_id: " << run_id << std::endl;
                        bot.request(run_status_endpoint, dpp::http_method::m_get,
                            [thread_id, run_id, &bot, headers, event, run_completed, cv](const dpp::http_request_completion_t& response) mutable {
                                if (response.status == 200) {
                                    nlohmann::json response_json = nlohmann::json::parse(response.body);
                                    std::string status = response_json["status"];
                                    std::cout << "Run status: " << status << std::endl;
                                    if (status == "completed" || status == "failed") {
                                        run_completed->store(true); // Set run_completed to true when run is completed or failed
                                        cv->notify_all(); // Notify the condition variable

                                        if (status == "completed") {
                                            // List messages after run completes
                                            std::string list_messages_endpoint = "https://api.openai.com/v1/threads/" + thread_id + "/messages";
                                            std::cout << "Run completed, sending request to list messages..." << std::endl;
                                            bot.request(list_messages_endpoint, dpp::http_method::m_get,
                                                [event](const dpp::http_request_completion_t& response) {
                                                    if (response.status == 200) {
                                                        std::cout << "Received response for list messages. Status: " << response.status << std::endl;
                                                        nlohmann::json response_json = nlohmann::json::parse(response.body);
                                                        std::cout << "List Messages Response: " << response_json.dump(4) << std::endl;

                                                        // Log the response to a file
                                                        std::ofstream log_file("openai_response.log", std::ios::app);
                                                        if (log_file.is_open()) {
                                                            log_file << "List Messages Response:\n" << response_json.dump(4) << "\n\n";
                                                            log_file.close();
                                                        }
                                                        else {
                                                            std::cerr << "Failed to open log file." << std::endl;
                                                        }

                                                        // Extract the assistant's response from the message list
                                                        std::string assistant_response;
                                                        for (const auto& message : response_json["data"]) {
                                                            if (message["role"] == "assistant" && !message["content"].empty()) {
                                                                assistant_response = message["content"][0]["text"]["value"].get<std::string>();
                                                                break;
                                                            }
                                                        }

                                                        // Check if the assistant's response is not empty before sending it to the user
                                                        if (!assistant_response.empty()) {
                                                            event.edit_original_response(dpp::message(assistant_response));
                                                        }
                                                        else {
                                                            std::cerr << "Empty assistant response received." << std::endl;
                                                            event.edit_original_response(dpp::message("Sorry, I couldn't generate a response to your question."));
                                                        }
                                                    }
                                                    else {
                                                        std::cerr << "Error from OpenAI API. Status: " << response.status << ". Body: " << response.body << std::endl;
                                                        event.edit_original_response(dpp::message("An error occurred while processing your request."));
                                                    }
                                                },
                                                "", "application/json", headers, "1.0"
                                            );
                                        }
                                        else {
                                            std::cerr << "Run failed." << std::endl;
                                            event.edit_original_response(dpp::message("An error occurred while processing your request."));
                                        }
                                    }
                                }
                                else {
                                    run_completed->store(true); // Set run_completed to true when there is an error
                                    cv->notify_all(); // Notify the condition variable
                                    std::cerr << "Error from OpenAI API. Status: " << response.status << ". Body: " << response.body << std::endl;
                                    event.edit_original_response(dpp::message("An error occurred while processing your request."));
                                }
                            },
                            "", "application/json", headers, "1.0"
                        );
                    }
                    }).detach();
                };

            if (thread_id.empty()) {
                // Create a new thread if it doesn't exist
                std::string create_thread_endpoint = "https://api.openai.com/v1/threads";
                std::cout << "Sending request to create thread..." << std::endl;
                bot.request(create_thread_endpoint, dpp::http_method::m_post,
                    [event, &bot, user_id, question, headers, check_run_status](const dpp::http_request_completion_t& response) {
                        if (response.status == 200) { // HTTP OK
                            nlohmann::json response_json = nlohmann::json::parse(response.body);
                            std::string thread_id = response_json["id"].get<std::string>();

                            // Store the thread_id in the database for the user
                            DatabaseManager::getInstance().storeConversationThreadIdForUser(user_id, thread_id);

                            // Log the response to a file
                            std::ofstream log_file("openai_response.log", std::ios::app);
                            if (log_file.is_open()) {
                                log_file << "Create Thread Response:\n" << response_json.dump(4) << "\n\n";
                                log_file.close();
                            }
                            else {
                                std::cerr << "Failed to open log file." << std::endl;
                            }

                            // Add the user's question to the thread
                            std::string add_message_endpoint = "https://api.openai.com/v1/threads/" + thread_id + "/messages";
                            nlohmann::json message_body = {
                                {"role", "user"},
                                {"content", question}
                            };
                            bot.request(add_message_endpoint, dpp::http_method::m_post,
                                [event, &bot, user_id, thread_id, headers, check_run_status](const dpp::http_request_completion_t& response) {
                                    if (response.status == 200) { // HTTP OK
                                        nlohmann::json response_json = nlohmann::json::parse(response.body);

                                        // Log the response to a file
                                        std::ofstream log_file("openai_response.log", std::ios::app);
                                        if (log_file.is_open()) {
                                            log_file << "Add Message Response:\n" << response_json.dump(4) << "\n\n";
                                            log_file.close();
                                        }
                                        else {
                                            std::cerr << "Failed to open log file." << std::endl;
                                        }

                                        // Run the thread
                                        std::string run_thread_endpoint = "https://api.openai.com/v1/threads/" + thread_id + "/runs";
                                        nlohmann::json run_body = {
                                            {"assistant_id", "asst_u0ap3h8EWbamvzjZVmZJxrfj"} // Replace with your actual assistant ID
                                        };
                                        bot.request(run_thread_endpoint, dpp::http_method::m_post,
                                            [event, &bot, user_id, thread_id, headers, check_run_status](const dpp::http_request_completion_t& response) {
                                                if (response.status == 200) { // HTTP OK
                                                    nlohmann::json response_json = nlohmann::json::parse(response.body);
                                                    std::string run_id = response_json["id"].get<std::string>();

                                                    // Log the response to a file
                                                    std::ofstream log_file("openai_response.log", std::ios::app);
                                                    if (log_file.is_open()) {
                                                        log_file << "Run Thread Response:\n" << response_json.dump(4) << "\n\n";
                                                        log_file.close();
                                                    }
                                                    else {
                                                        std::cerr << "Failed to open log file." << std::endl;
                                                    }

                                                    // Check the run status periodically
                                                    check_run_status(thread_id, run_id);
                                                }
                                                else {
                                                    std::cerr << "Error from OpenAI API. Status: " << response.status << ". Body: " << response.body << std::endl;
                                                    event.edit_original_response(dpp::message("An error occurred while processing your request."));
                                                }
                                            },
                                            run_body.dump(), "application/json", headers, "1.0"
                                        );
                                    }
                                    else {
                                        std::cerr << "Error from OpenAI API. Status: " << response.status << ". Body: " << response.body << std::endl;
                                        event.edit_original_response(dpp::message("An error occurred while processing your request."));
                                    }
                                },
                                message_body.dump(), "application/json", headers, "1.0"
                            );
                        }
                        else {
                            std::cerr << "Error from OpenAI API. Status: " << response.status << ". Body: " << response.body << std::endl;
                            event.edit_original_response(dpp::message("An error occurred while processing your request."));
                        }
                    },
                    "", "application/json", headers, "1.0"
                );
            }
            else {
                // Add the user's question to the existing thread
                std::string add_message_endpoint = "https://api.openai.com/v1/threads/" + thread_id + "/messages";
                nlohmann::json message_body = {
                    {"role", "user"},
                    {"content", question}
                };
                bot.request(add_message_endpoint, dpp::http_method::m_post,
                    [event, &bot, user_id, thread_id, headers, check_run_status](const dpp::http_request_completion_t& response) {
                        if (response.status == 200) { // HTTP OK
                            nlohmann::json response_json = nlohmann::json::parse(response.body);

                            // Log the response to a file
                            std::ofstream log_file("openai_response.log", std::ios::app);
                            if (log_file.is_open()) {
                                log_file << "Add Message Response:\n" << response_json.dump(4) << "\n\n";
                                log_file.close();
                            }
                            else {
                                std::cerr << "Failed to open log file." << std::endl;
                            }

                            // Run the thread
                            std::string run_thread_endpoint = "https://api.openai.com/v1/threads/" + thread_id + "/runs";
                            nlohmann::json run_body = {
                                {"assistant_id", "asst_u0ap3h8EWbamvzjZVmZJxrfj"} // Replace with your actual assistant ID
                            };
                            bot.request(run_thread_endpoint, dpp::http_method::m_post,
                                [event, &bot, user_id, thread_id, headers, check_run_status](const dpp::http_request_completion_t& response) {
                                    if (response.status == 200) { // HTTP OK
                                        nlohmann::json response_json = nlohmann::json::parse(response.body);
                                        std::string run_id = response_json["id"].get<std::string>();

                                        // Log the response to a file
                                        std::ofstream log_file("openai_response.log", std::ios::app);
                                        if (log_file.is_open()) {
                                            log_file << "Run Thread Response:\n" << response_json.dump(4) << "\n\n";
                                            log_file.close();
                                        }
                                        else {
                                            std::cerr << "Failed to open log file." << std::endl;
                                        }

                                        // Check the run status periodically
                                        check_run_status(thread_id, run_id);
                                    }
                                    else {
                                        std::cerr << "Error from OpenAI API. Status: " << response.status << ". Body: " << response.body << std::endl;
                                        event.edit_original_response(dpp::message("An error occurred while processing your request."));
                                    }
                                },
                                run_body.dump(), "application/json", headers, "1.0"
                            );
                        }
                        else {
                            std::cerr << "Error from OpenAI API. Status: " << response.status << ". Body: " << response.body << std::endl;
                            event.edit_original_response(dpp::message("An error occurred while processing your request."));
                        }
                    },
                    message_body.dump(), "application/json", headers, "1.0"
                );
}
}
else if (sub_command == "image") {
    std::string prompt = std::get<std::string>(event.get_parameter("prompt"));

    // OpenAI DALL-E 3 API setup
    const std::string endpoint = "https://api.openai.com/v1/images/generations";
    const std::string api_key = DatabaseManager::getInstance().getOpenAIApiKey();

    nlohmann::json request_body = {
        {"model", "dall-e-3"},
        {"prompt", prompt},
        {"n", 1},
        {"size", "1024x1024"}
    };

    std::string request_body_str = request_body.dump();

    // Initialize libcurl
    CURL* curl = curl_easy_init();
    if (curl) {
        // Set the request URL
        curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());

        // Set the request method to POST
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // Set the request headers
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set the request body
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body_str.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request_body_str.length());

        // Set the response callback
        std::string response_body;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

        // Disable SSL certificate verification
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        // Perform the request
        event.thinking();

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            // Request successful, parse the response
            nlohmann::json response_json = nlohmann::json::parse(response_body);

            std::string image_url;
            if (response_json.contains("data") && response_json["data"].is_array()) {
                for (const auto& element : response_json["data"]) {
                    if (element.contains("url")) {
                        image_url = element["url"].get<std::string>();
                        break;
                    }
                }
            }

            if (!image_url.empty()) {
                // Download the image
                CURL* download_curl = curl_easy_init();
                if (download_curl) {
                    std::string image_data;
                    curl_easy_setopt(download_curl, CURLOPT_URL, image_url.c_str());
                    curl_easy_setopt(download_curl, CURLOPT_WRITEFUNCTION, writeCallback);
                    curl_easy_setopt(download_curl, CURLOPT_WRITEDATA, &image_data);
                    curl_easy_setopt(download_curl, CURLOPT_SSL_VERIFYPEER, 0L);
                    curl_easy_setopt(download_curl, CURLOPT_SSL_VERIFYHOST, 0L);

                    CURLcode download_res = curl_easy_perform(download_curl);
                    if (download_res == CURLE_OK) {
                        // Image downloaded successfully, reupload it to Discord
                        dpp::message msg;
                        msg.add_file("generated_image.png", image_data);
                        msg.content = "Prompt: \"" + prompt + "\"";
                        event.edit_original_response(msg);
                    }
                    else {
                        std::cerr << "Error downloading image: " << curl_easy_strerror(download_res) << std::endl;
                        event.edit_original_response(dpp::message("An error occurred while downloading the generated image."));
                    }

                    curl_easy_cleanup(download_curl);
                }
            }
            else {
                std::cerr << "No image URL found in the response." << std::endl;
                event.edit_original_response(dpp::message("Sorry, I couldn't generate an image for your prompt."));
            }
        }
        else {
            std::cerr << "Error in libcurl request: " << curl_easy_strerror(res) << std::endl;
            event.edit_original_response(dpp::message("An error occurred while processing your request."));
        }

        // Clean up
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    }
}

} // namespace commands