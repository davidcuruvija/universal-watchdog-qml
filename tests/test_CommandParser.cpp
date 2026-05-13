#include <catch2/catch_test_macros.hpp>
#include "../include/CommandParser.h"

TEST_CASE("CommandParser: add process") {
    CommandParser parser;
    auto cmd = parser.parse("add process 1234");
    REQUIRE(cmd.type == ParsedCommand::Type::ADD_PROCESS);
    REQUIRE(cmd.args.size() == 1);
    REQUIRE(cmd.args[0] == "1234");
}

TEST_CASE("CommandParser: add thread") {
    CommandParser parser;
    auto cmd = parser.parse("add thread worker-1 2000");
    REQUIRE(cmd.type == ParsedCommand::Type::ADD_THREAD);
    REQUIRE(cmd.args[0] == "worker-1");
    REQUIRE(cmd.args[1] == "2000");
}

TEST_CASE("CommandParser: remove") {
    CommandParser parser;
    auto cmd = parser.parse("remove firefox (pid=1234)");
    REQUIRE(cmd.type == ParsedCommand::Type::REMOVE);
    REQUIRE(cmd.args[0] == "firefox (pid=1234)");
}

TEST_CASE("CommandParser: case insensitive") {
    CommandParser parser;
    REQUIRE(parser.parse("HELP").type == ParsedCommand::Type::HELP);
    REQUIRE(parser.parse("Help").type == ParsedCommand::Type::HELP);
    REQUIRE(parser.parse("ADD PROCESS 1").type == ParsedCommand::Type::ADD_PROCESS);
}

TEST_CASE("CommandParser: unknown") {
    CommandParser parser;
    auto cmd = parser.parse("blabla");
    REQUIRE(cmd.type == ParsedCommand::Type::UNKNOWN);
    REQUIRE_FALSE(cmd.errorMessage.empty());
}

TEST_CASE("CommandParser: empty") {
    CommandParser parser;
    auto cmd = parser.parse("");
    REQUIRE(cmd.type == ParsedCommand::Type::UNKNOWN);
}
