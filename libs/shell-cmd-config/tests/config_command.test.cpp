#if defined(UNIT_TESTS)
#include "shell-cmd-config/config_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <string>
#include <string_view>

#include "config/transactional_config_dictionary.hpp"
#include "io/stream_requirements.hpp"

namespace {

class StreamStub : public midismith::io::StreamRequirements {
 public:
  midismith::io::ReadResult Read(std::uint8_t&) noexcept override {
    return midismith::io::ReadResult::kNoData;
  }

  void Write(char c) noexcept override {
    output_ += c;
  }

  void Write(const char* str) noexcept override {
    output_ += str;
  }

  const std::string& GetOutput() const noexcept {
    return output_;
  }

 private:
  std::string output_;
};

class ConfigurationProviderMock final : public midismith::config::TransactionalConfigDictionary {
 public:
  std::size_t KeyCount() const noexcept override {
    return 2u;
  }

  std::string_view KeyAt(std::size_t index) const noexcept override {
    if (index == 0u) {
      return "can_board_id";
    }
    if (index == 1u) {
      return "description";
    }
    return {};
  }

  midismith::config::ConfigGetStatus GetValue(std::string_view key, char* value_buffer,
                                              std::size_t value_buffer_size,
                                              std::size_t& value_length) const noexcept override {
    if (key == "can_board_id") {
      return CopyValue(can_board_id_value, value_buffer, value_buffer_size, value_length);
    }
    if (key == "description") {
      return CopyValue(description_value, value_buffer, value_buffer_size, value_length);
    }
    return midismith::config::ConfigGetStatus::kUnknownKey;
  }

  midismith::config::ConfigSetStatus SetValue(std::string_view key,
                                              std::string_view value) noexcept override {
    last_set_key = key;
    last_set_value = value;
    return set_status;
  }

  midismith::config::TransactionResult Commit() noexcept override {
    ++commit_calls;
    return commit_result;
  }

  static midismith::config::ConfigGetStatus CopyValue(std::string_view value, char* value_buffer,
                                                      std::size_t value_buffer_size,
                                                      std::size_t& value_length) noexcept {
    if (value_buffer == nullptr || value_buffer_size < value.size()) {
      return midismith::config::ConfigGetStatus::kBufferTooSmall;
    }

    std::memcpy(value_buffer, value.data(), value.size());
    value_length = value.size();
    return midismith::config::ConfigGetStatus::kOk;
  }

  std::string_view can_board_id_value = "7";
  std::string_view description_value = "short";
  midismith::config::ConfigSetStatus set_status = midismith::config::ConfigSetStatus::kOk;
  midismith::config::TransactionResult commit_result =
      midismith::config::TransactionResult::kSuccess;
  std::string_view last_set_key{};
  std::string_view last_set_value{};
  int commit_calls = 0;
};

}  // namespace

TEST_CASE("The ConfigCommand class", "[domain][shell][commands]") {
  ConfigurationProviderMock provider;
  midismith::shell_cmd_config::ConfigCommand command(provider);
  StreamStub stream;

  SECTION("Name()") {
    SECTION("Should return config") {
      REQUIRE(command.Name() == "config");
    }
  }

  SECTION("Run()") {
    SECTION("When called with no operation") {
      SECTION("Should print usage") {
        char* argv[] = {const_cast<char*>("config")};
        command.Run(1, argv, stream);
        REQUIRE(stream.GetOutput().find("usage: config getall") != std::string::npos);
      }
    }

    SECTION("When called with 'getall'") {
      SECTION("With correct number of arguments") {
        SECTION("Should print all key values") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("getall")};
          command.Run(2, argv, stream);
          REQUIRE(stream.GetOutput() == "can_board_id: 7\r\ndescription: short\r\n");
        }
      }

      SECTION("With extra arguments") {
        SECTION("Should print usage") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("getall"),
                          const_cast<char*>("extra")};
          command.Run(3, argv, stream);
          REQUIRE(stream.GetOutput().find("usage: config getall") != std::string::npos);
        }
      }

      SECTION("When a key is empty") {
        SECTION("Should skip that key") {}
      }

      SECTION("When a value cannot be read") {
        SECTION("Should print an error") {
          provider.description_value =
              "a value that is intentionally much larger than the local output buffer capacity "
              "used by the command";
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("getall")};
          command.Run(2, argv, stream);
          REQUIRE(stream.GetOutput() == "can_board_id: 7\r\nerror: cannot read value\r\n");
        }
      }
    }

    SECTION("When called with 'get'") {
      SECTION("With correct key") {
        SECTION("Should print the key value") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("get"),
                          const_cast<char*>("can_board_id")};
          command.Run(3, argv, stream);
          REQUIRE(stream.GetOutput() == "can_board_id: 7\r\n");
        }
      }

      SECTION("With unknown key") {
        SECTION("Should print an error") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("get"),
                          const_cast<char*>("unknown")};
          command.Run(3, argv, stream);
          REQUIRE(stream.GetOutput() == "error: unknown key\r\n");
        }
      }

      SECTION("When state is unavailable") {
        SECTION("Should print an error") {}
      }

      SECTION("With wrong number of arguments") {
        SECTION("Should print usage") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("get")};
          command.Run(2, argv, stream);
          REQUIRE(stream.GetOutput().find("usage: config getall") != std::string::npos);
        }
      }
    }

    SECTION("When called with 'set'") {
      SECTION("With valid key and value") {
        SECTION("Should print ok and update provider") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("set"),
                          const_cast<char*>("can_board_id"), const_cast<char*>("5")};
          command.Run(4, argv, stream);
          REQUIRE(stream.GetOutput() == "ok\r\n");
          REQUIRE(provider.last_set_key == "can_board_id");
          REQUIRE(provider.last_set_value == "5");
        }
      }

      SECTION("With invalid value") {
        SECTION("Should print an error") {
          provider.set_status = midismith::config::ConfigSetStatus::kInvalidValue;
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("set"),
                          const_cast<char*>("can_board_id"), const_cast<char*>("bad")};
          command.Run(4, argv, stream);
          REQUIRE(stream.GetOutput() == "error: invalid value\r\n");
        }
      }

      SECTION("With wrong number of arguments") {
        SECTION("Should print usage") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("set"),
                          const_cast<char*>("key")};
          command.Run(3, argv, stream);
          REQUIRE(stream.GetOutput().find("usage: config getall") != std::string::npos);
        }
      }
    }

    SECTION("When called with 'save'") {
      SECTION("With correct number of arguments") {
        SECTION("When commit succeeds") {
          SECTION("Should print ok") {
            char* argv[] = {const_cast<char*>("config"), const_cast<char*>("save")};
            command.Run(2, argv, stream);
            REQUIRE(stream.GetOutput() == "ok\r\n");
            REQUIRE(provider.commit_calls == 1);
          }
        }

        SECTION("When commit fails") {
          SECTION("Should print error") {
            provider.commit_result = midismith::config::TransactionResult::kFailure;
            char* argv[] = {const_cast<char*>("config"), const_cast<char*>("save")};
            command.Run(2, argv, stream);
            REQUIRE(stream.GetOutput() == "error: save failed\r\n");
          }
        }
      }

      SECTION("With extra arguments") {
        SECTION("Should print usage") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("save"),
                          const_cast<char*>("extra")};
          command.Run(3, argv, stream);
          REQUIRE(stream.GetOutput().find("usage: config getall") != std::string::npos);
        }
      }
    }

    SECTION("When called with unknown operation") {
      SECTION("Should print usage") {
        char* argv[] = {const_cast<char*>("config"), const_cast<char*>("unknown")};
        command.Run(2, argv, stream);
        REQUIRE(stream.GetOutput().find("usage: config getall") != std::string::npos);
      }
    }

    SECTION("Branch coverage for Arg() helper") {
      SECTION("When argv is nullptr") {
        SECTION("Should return empty string_view") {
          command.Run(2, nullptr, stream);
          REQUIRE(stream.GetOutput().find("usage: config getall") != std::string::npos);
        }
      }

      SECTION("When argv[index] is nullptr") {
        SECTION("Should return empty string_view") {
          char* argv[] = {const_cast<char*>("config"), nullptr};
          command.Run(2, argv, stream);
          REQUIRE(stream.GetOutput().find("usage: config getall") != std::string::npos);
        }
      }
    }
  }
}
#endif
