#pragma once
#include <string>
#include <vector>

/**
 * @brief Represents a parsed user command with its type and arguments.
 */
struct ParsedCommand {
    enum class Type {
        ADD_PROCESS,    // add process <pid>
        ADD_THREAD,     // add thread <name> [timeout_ms]
        REMOVE,         // remove <name>
        KILL,           // kill <name>
        RESTART,        // restart <name>
        LIST,           // list
        WATCH,          // watch
        CONFIG,         // config
        CLEAR,          // clear
        HELP,           // help
        QUIT,           // quit | exit
        UNKNOWN         // unrecognized input
    };

    Type type = Type::UNKNOWN;
    std::vector<std::string> args;   // parsed arguments
    std::string rawInput;            // original input string
    std::string errorMessage;        // set when type == UNKNOWN
};

/**
 * @brief Parses raw user input strings into structured commands.
 *
 * Handles tokenization, command recognition, and argument validation.
 * Stateless — each parse() call is independent.
 */
class CommandParser {
public:
    /**
     * @brief Parse a raw input line into a ParsedCommand.
     * @param input The user's raw terminal input.
     * @return A ParsedCommand with type, args, and optional error message.
     */
    ParsedCommand parse(const std::string& input) const;

private:
    std::vector<std::string> tokenize(const std::string& input) const;
    std::string toLower(const std::string& str) const;
};
