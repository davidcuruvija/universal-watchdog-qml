#include "../include/CommandParser.h"
#include <sstream>
#include <algorithm>
#include <cctype>

std::string CommandParser::toLower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::vector<std::string> CommandParser::tokenize(const std::string& input) const {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

ParsedCommand CommandParser::parse(const std::string& input) const {
    ParsedCommand cmd;
    cmd.rawInput = input;
    
    auto tokens = tokenize(input);
    if (tokens.empty()) {
        cmd.type = ParsedCommand::Type::UNKNOWN;
        return cmd;
    }

    std::string verb = toLower(tokens[0]);

    if (verb == "help") {
        cmd.type = ParsedCommand::Type::HELP;
    } else if (verb == "quit" || verb == "exit") {
        cmd.type = ParsedCommand::Type::QUIT;
    } else if (verb == "list") {
        cmd.type = ParsedCommand::Type::LIST;
    } else if (verb == "watch") {
        cmd.type = ParsedCommand::Type::WATCH;
    } else if (verb == "sort") {
        if (tokens.size() < 2) {
            cmd.type = ParsedCommand::Type::UNKNOWN;
            cmd.errorMessage = "Usage: sort cpu|memory|name";
            return cmd;
        }
        std::string field = toLower(tokens[1]);
        if (field != "cpu" && field != "memory" && field != "name") {
            cmd.type = ParsedCommand::Type::UNKNOWN;
            cmd.errorMessage = "Unknown sort field '" + field + "'. Use: cpu, memory, name";
            return cmd;
        }
        cmd.type = ParsedCommand::Type::SORT;
        cmd.args.push_back(field);
    } else if (verb == "config") {
        cmd.type = ParsedCommand::Type::CONFIG;
    } else if (verb == "clear") {
        cmd.type = ParsedCommand::Type::CLEAR;
    } else if (verb == "add") {
        if (tokens.size() < 3) {
            cmd.type = ParsedCommand::Type::UNKNOWN;
            cmd.errorMessage = "Usage: add process <pid>";
            return cmd;
        }
        std::string target = toLower(tokens[1]);
        if (target == "process") {
            cmd.type = ParsedCommand::Type::ADD_PROCESS;
            cmd.args.push_back(tokens[2]);
            // check if tokens[2] is valid pid
            try {
                std::stoi(tokens[2]);
            } catch(...) {
                cmd.type = ParsedCommand::Type::UNKNOWN;
                cmd.errorMessage = "Invalid PID format: " + tokens[2];
            }
        } else {
            cmd.type = ParsedCommand::Type::UNKNOWN;
            cmd.errorMessage = "Unknown target for 'add'. Use 'process'.";
        }
    } else if (verb == "remove") {
        if (tokens.size() < 2) {
            cmd.type = ParsedCommand::Type::UNKNOWN;
            cmd.errorMessage = "Usage: remove <name>";
            return cmd;
        }
        cmd.type = ParsedCommand::Type::REMOVE;
        std::string name = tokens[1];
        for (size_t i = 2; i < tokens.size(); ++i) {
            name += " " + tokens[i];
        }
        cmd.args.push_back(name);
    } else if (verb == "kill") {
        if (tokens.size() < 2) {
            cmd.type = ParsedCommand::Type::UNKNOWN;
            cmd.errorMessage = "Usage: kill <name>";
            return cmd;
        }
        cmd.type = ParsedCommand::Type::KILL;
        std::string name = tokens[1];
        for (size_t i = 2; i < tokens.size(); ++i) name += " " + tokens[i];
        cmd.args.push_back(name);
    } else if (verb == "restart") {
        if (tokens.size() < 2) {
            cmd.type = ParsedCommand::Type::UNKNOWN;
            cmd.errorMessage = "Usage: restart <name>";
            return cmd;
        }
        cmd.type = ParsedCommand::Type::RESTART;
        std::string name = tokens[1];
        for (size_t i = 2; i < tokens.size(); ++i) name += " " + tokens[i];
        cmd.args.push_back(name);
    } else {
        cmd.type = ParsedCommand::Type::UNKNOWN;
        cmd.errorMessage = "Unknown command: '" + verb + "'. Type 'help' for available commands.";
    }

    return cmd;
}
