#pragma once

#include <dpp/dpp.h>

class FactsCommandHandler {
public:
    FactsCommandHandler(dpp::cluster& bot);
    void handleCommand(const dpp::slashcommand_t& event);
    dpp::slashcommand register_facts_command();

private:
    dpp::cluster& m_bot;
    void handleCatFact(const dpp::slashcommand_t& event);
    void handleDogFact(const dpp::slashcommand_t& event);
    void handleChuckNorrisFact(const dpp::slashcommand_t& event);
    void handleNumberFact(const dpp::slashcommand_t& event);
    void handleUselessFact(const dpp::slashcommand_t& event);
    void handlePierceBrosnanFact(const dpp::slashcommand_t& event);
};