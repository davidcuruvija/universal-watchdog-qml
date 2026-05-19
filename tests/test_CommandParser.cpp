#include <catch2/catch_test_macros.hpp>
#include "../include/CommandParser.h"

TEST_CASE("CommandParser: add process") {
    CommandParser parser;
    auto cmd = parser.parse("add process 1234");
    REQUIRE(cmd.type == ParsedCommand::Type::ADD_PROCESS);
    REQUIRE(cmd.args.size() == 1);
    REQUIRE(cmd.args[0] == "1234");
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

TEST_CASE("CommandParser: sort cpu", "[commandParser][sort]") {
    CommandParser parser;
    auto cmd = parser.parse("sort cpu");
    REQUIRE(cmd.type == ParsedCommand::Type::SORT);
    REQUIRE(cmd.args[0] == "cpu");
}

TEST_CASE("CommandParser: sort memory", "[commandParser][sort]") {
    CommandParser parser;
    auto cmd = parser.parse("sort memory");
    REQUIRE(cmd.type == ParsedCommand::Type::SORT);
    REQUIRE(cmd.args[0] == "memory");
}

TEST_CASE("CommandParser: sort name", "[commandParser][sort]") {
    CommandParser parser;
    auto cmd = parser.parse("sort name");
    REQUIRE(cmd.type == ParsedCommand::Type::SORT);
    REQUIRE(cmd.args[0] == "name");
}

TEST_CASE("CommandParser: sort case insensitive", "[commandParser][sort]") {
    CommandParser parser;
    REQUIRE(parser.parse("SORT CPU").type == ParsedCommand::Type::SORT);
}

TEST_CASE("CommandParser: sort without field is UNKNOWN", "[commandParser][sort]") {
    CommandParser parser;
    auto cmd = parser.parse("sort");
    REQUIRE(cmd.type == ParsedCommand::Type::UNKNOWN);
    REQUIRE_FALSE(cmd.errorMessage.empty());
}

TEST_CASE("CommandParser: sort with invalid field is UNKNOWN", "[commandParser][sort]") {
    CommandParser parser;
    auto cmd = parser.parse("sort invalid");
    REQUIRE(cmd.type == ParsedCommand::Type::UNKNOWN);
    REQUIRE_FALSE(cmd.errorMessage.empty());
}
