#include "FactsCommandHandler.h"
#include "pierce_brosnan_facts.h"
#include <random>

FactsCommandHandler::FactsCommandHandler(dpp::cluster& bot) : m_bot(bot) {}

dpp::slashcommand FactsCommandHandler::register_facts_command() {
    dpp::slashcommand facts_command("facts", "Get interesting facts!", m_bot.me.id);

    facts_command.add_option(
        dpp::command_option(dpp::co_sub_command, "cat", "Get a random cat fact!")
    );
    facts_command.add_option(
        dpp::command_option(dpp::co_sub_command, "dog", "Get a random dog fact!")
    );
    facts_command.add_option(
        dpp::command_option(dpp::co_sub_command, "chucknorris", "Get a random Chuck Norris fact!")
    );
    facts_command.add_option(
        dpp::command_option(dpp::co_sub_command, "number", "Get a random number fact!")
    );
    facts_command.add_option(
        dpp::command_option(dpp::co_sub_command, "useless", "Get a random useless fact!")
    );
    facts_command.add_option(
        dpp::command_option(dpp::co_sub_command, "piercebrosnan", "Get a random Pierce Brosnan fact!")
    );

    return facts_command;
}

// Function to handle the response from the cat fact API
void FactsCommandHandler::handleCommand(const dpp::slashcommand_t& event) {
    std::string subcommand = event.command.get_command_interaction().options[0].name;

    if (subcommand == "cat") {
        handleCatFact(event);
    }
    else if (subcommand == "dog") {
        handleDogFact(event);
    }
    else if (subcommand == "chucknorris") {
        handleChuckNorrisFact(event);
    }
    else if (subcommand == "number") {
        handleNumberFact(event);
    }
    else if (subcommand == "useless") {
        handleUselessFact(event);
    }
    else if (subcommand == "piercebrosnan") {
        handlePierceBrosnanFact(event);
    }
    else {
        event.reply("Unknown subcommand.");
    }
}

void FactsCommandHandler::handleCatFact(const dpp::slashcommand_t& event) {
    m_bot.request("https://catfact.ninja/fact", dpp::http_method::m_get, [event](const dpp::http_request_completion_t& response) {
        try {
            std::cout << "Response status: " << response.status << std::endl;

            if (response.status == 200) {
                nlohmann::json data = nlohmann::json::parse(response.body);
                std::string fact = data["fact"].get<std::string>();

                dpp::embed embed;
                embed.set_title("Random Cat Fact")
                    .set_description(fact)
                    .set_color(0x0099ff);

                event.reply(dpp::message().add_embed(embed));
            }
            else {
                std::cout << "Error: Unexpected status code " << response.status << std::endl;
                event.reply("Failed to fetch a cat fact.");
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error parsing JSON: " << e.what() << std::endl;
            event.reply("Failed to parse the cat fact response.");
        }
        },
        "",
        "text/plain",
        {},
        "1.0");
}

// Function to handle the response from the dog fact API
void FactsCommandHandler::handleDogFact(const dpp::slashcommand_t& event) {
    m_bot.request("https://dog-api.kinduff.com/api/facts", dpp::http_method::m_get, [event](const dpp::http_request_completion_t& response) {
        try {
            if (response.status == 200) {
                nlohmann::json data = nlohmann::json::parse(response.body);
                std::string fact = data["facts"][0].get<std::string>();

                dpp::embed embed;
                embed.set_title("Random Dog Fact")
                    .set_description(fact)
                    .set_color(0x0099ff);

                event.reply(dpp::message().add_embed(embed));
            }
            else {
                std::cout << "Error: Unexpected status code " << response.status << std::endl;
                event.reply("Failed to fetch a dog fact.");
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error parsing JSON: " << e.what() << std::endl;
            event.reply("Failed to parse the dog fact response.");
        }
        },
        "",
        "text/plain",
        {},
        "1.0");
}

// Function to handle the response from the Chuck Norris fact API
void FactsCommandHandler::handleChuckNorrisFact(const dpp::slashcommand_t& event) {
    m_bot.request("https://api.chucknorris.io/jokes/random", dpp::http_method::m_get, [event](const dpp::http_request_completion_t& response) {
        if (response.status == 200) {
            nlohmann::json data = nlohmann::json::parse(response.body);
            std::string fact = data["value"].get<std::string>();

            dpp::embed embed;
            embed.set_title("Random Chuck Norris Fact")
                .set_description(fact)
                .set_color(0x0099ff);

            event.reply(dpp::message().add_embed(embed));
        }
        else {
            event.reply("Failed to fetch a Chuck Norris fact.");
        }
        },
        "",
        "text/plain",
        {},
        "1.0");
}

void FactsCommandHandler::handleNumberFact(const dpp::slashcommand_t& event) {
    m_bot.request("http://numbersapi.com/random", dpp::http_method::m_get, [event](const dpp::http_request_completion_t& response) {
        if (response.status == 200) {
            dpp::embed embed;
            embed.set_title("Random Number Fact")
                .set_description(response.body)
                .set_color(0x0099ff);

            event.reply(dpp::message().add_embed(embed));
        }
        else {
            event.reply("Failed to fetch a number fact.");
        }
        },
        "",
        "text/plain",
        {},
        "1.0");
}

void FactsCommandHandler::handleUselessFact(const dpp::slashcommand_t& event) {
    m_bot.request("https://uselessfacts.jsph.pl/api/v2/facts/random", dpp::http_method::m_get, [event](const dpp::http_request_completion_t& response) {
        if (response.status == 200) {
            nlohmann::json data = nlohmann::json::parse(response.body);
            std::string fact = data["text"].get<std::string>();

            dpp::embed embed;
            embed.set_title("Random Useless Fact")
                .set_description(fact)
                .set_color(0x0099ff);

            event.reply(dpp::message().add_embed(embed));
        }
        else {
            event.reply("Failed to fetch a useless fact.");
        }
        },
        "",
        "text/plain",
        {},
        "1.0");
}

void FactsCommandHandler::handlePierceBrosnanFact(const dpp::slashcommand_t& event) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, num_pierce_brosnan_facts_count - 1);

    int index = dis(gen);
    std::string fact = pierce_brosnan_facts[index];

    dpp::embed embed;
    embed.set_title("Random Pierce Brosnan Fact")
        .set_description(fact)
        .set_color(0x0099ff);

    event.reply(dpp::message().add_embed(embed));
}