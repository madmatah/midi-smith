#include "domain/shell/command_parser.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("The CommandParser class", "[shell]") {
  SECTION("The ParseInPlace() method") {
    SECTION("When called with a simple command") {
      char line[] = "help";
      char* argv[4];
      int argc = domain::shell::CommandParser::ParseInPlace(line, 4, argv);

      SECTION("Should return 1 argument") {
        REQUIRE(argc == 1);
      }
      SECTION("Should correctly set argv[0]") {
        REQUIRE(std::string(argv[0]) == "help");
      }
    }

    SECTION("When called with multiple arguments and spaces") {
      char line[] = "  cmd   arg1  arg2  ";
      char* argv[4];
      int argc = domain::shell::CommandParser::ParseInPlace(line, 4, argv);

      SECTION("Should return 3 arguments") {
        REQUIRE(argc == 3);
      }
      SECTION("Should correctly set argv[0]") {
        REQUIRE(std::string(argv[0]) == "cmd");
      }
      SECTION("Should correctly set argv[1]") {
        REQUIRE(std::string(argv[1]) == "arg1");
      }
      SECTION("Should correctly set argv[2]") {
        REQUIRE(std::string(argv[2]) == "arg2");
      }
    }

    SECTION("When called with more arguments than max_args") {
      char line[] = "cmd arg1 arg2 arg3";
      char* argv[2];
      int argc = domain::shell::CommandParser::ParseInPlace(line, 2, argv);

      SECTION("Should return -1 indicating error") {
        REQUIRE(argc == -1);
      }
    }

    SECTION("When called with an empty string") {
      char line[] = "   ";
      char* argv[4];
      int argc = domain::shell::CommandParser::ParseInPlace(line, 4, argv);

      SECTION("Should return 0 arguments") {
        REQUIRE(argc == 0);
      }
    }
  }
}
