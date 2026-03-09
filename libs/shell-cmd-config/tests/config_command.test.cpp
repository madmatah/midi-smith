#if defined(UNIT_TESTS)
#include "shell-cmd-config/config_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <optional>
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

  const std::string& output() const noexcept {
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
    if (index == 1u && expose_empty_second_key_) {
      return {};
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
      if (forced_get_status_.has_value()) {
        return *forced_get_status_;
      }
      return CopyValue(can_board_id_value_, value_buffer, value_buffer_size, value_length);
    }
    if (key == "description") {
      if (forced_get_status_.has_value()) {
        return *forced_get_status_;
      }
      return CopyValue(description_value_, value_buffer, value_buffer_size, value_length);
    }
    return midismith::config::ConfigGetStatus::kUnknownKey;
  }

  midismith::config::ConfigSetStatus SetValue(std::string_view key,
                                              std::string_view value) noexcept override {
    last_set_key_ = key;
    last_set_value_ = value;
    return set_status_;
  }

  midismith::config::TransactionResult Commit() noexcept override {
    ++commit_calls_;
    return commit_result_;
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

  std::string_view can_board_id_value_ = "7";
  std::string_view description_value_ = "short";
  bool expose_empty_second_key_ = false;
  std::optional<midismith::config::ConfigGetStatus> forced_get_status_;
  midismith::config::ConfigSetStatus set_status_ = midismith::config::ConfigSetStatus::kOk;
  midismith::config::TransactionResult commit_result_ =
      midismith::config::TransactionResult::kSuccess;
  std::string_view last_set_key_{};
  std::string_view last_set_value_{};
  int commit_calls_ = 0;
};

}  // namespace

TEST_CASE("The ConfigCommand class", "[domain][shell][commands]") {
  ConfigurationProviderMock provider;
  midismith::shell_cmd_config::ConfigCommand command(provider);
  StreamStub stream;

  SECTION("The Name() method") {
    SECTION("When called") {
      SECTION("Should return 'config'") {
        REQUIRE(command.Name() == "config");
      }
    }
  }

  SECTION("The Help() method") {
    SECTION("When called") {
      SECTION("Should return the expected help string") {
        REQUIRE(command.Help() == "Manage persistent configuration (getall/get/set/save)");
      }
    }
  }

  SECTION("Run()") {
    SECTION("When called with no operation") {
      SECTION("Should print usage") {
        char* argv[] = {const_cast<char*>("config")};
        command.Run(1, argv, stream);
        REQUIRE(stream.output().find("usage: config getall") != std::string::npos);
      }
    }

    SECTION("When called with 'getall'") {
      SECTION("With correct number of arguments") {
        SECTION("Should print all key values") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("getall")};
          command.Run(2, argv, stream);
          REQUIRE(stream.output() == "can_board_id: 7\r\ndescription: short\r\n");
        }
      }

      SECTION("With extra arguments") {
        SECTION("Should print usage") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("getall"),
                          const_cast<char*>("extra")};
          command.Run(3, argv, stream);
          REQUIRE(stream.output().find("usage: config getall") != std::string::npos);
        }
      }

      SECTION("When a key is empty") {
        SECTION("Should skip that key") {
          provider.expose_empty_second_key_ = true;
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("getall")};
          command.Run(2, argv, stream);
          REQUIRE(stream.output() == "can_board_id: 7\r\n");
        }
      }

      SECTION("When a value cannot be read") {
        SECTION("Should print an error") {
          provider.description_value_ =
              "a value that is intentionally much larger than the local output buffer capacity "
              "used by the command";
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("getall")};
          command.Run(2, argv, stream);
          REQUIRE(stream.output() == "can_board_id: 7\r\nerror: cannot read value\r\n");
        }
      }
    }

    SECTION("When called with 'get'") {
      SECTION("With correct key") {
        SECTION("Should print the key value") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("get"),
                          const_cast<char*>("can_board_id")};
          command.Run(3, argv, stream);
          REQUIRE(stream.output() == "can_board_id: 7\r\n");
        }
      }

      SECTION("With unknown key") {
        SECTION("Should print an error") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("get"),
                          const_cast<char*>("unknown")};
          command.Run(3, argv, stream);
          REQUIRE(stream.output() == "error: unknown key\r\n");
        }
      }

      SECTION("When state is unavailable") {
        SECTION("Should print an error") {
          provider.forced_get_status_ = midismith::config::ConfigGetStatus::kUnavailable;
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("get"),
                          const_cast<char*>("can_board_id")};
          command.Run(3, argv, stream);
          REQUIRE(stream.output() == "error: cannot read value\r\n");
        }
      }

      SECTION("With wrong number of arguments") {
        SECTION("Should print usage") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("get")};
          command.Run(2, argv, stream);
          REQUIRE(stream.output().find("usage: config getall") != std::string::npos);
        }
      }
    }

    SECTION("When called with 'set'") {
      SECTION("With valid key and value") {
        SECTION("Should print ok and update provider") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("set"),
                          const_cast<char*>("can_board_id"), const_cast<char*>("5")};
          command.Run(4, argv, stream);
          REQUIRE(stream.output() == "ok\r\n");
          REQUIRE(provider.last_set_key_ == "can_board_id");
          REQUIRE(provider.last_set_value_ == "5");
        }
      }

      SECTION("With invalid value") {
        SECTION("Should print an error") {
          provider.set_status_ = midismith::config::ConfigSetStatus::kInvalidValue;
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("set"),
                          const_cast<char*>("can_board_id"), const_cast<char*>("bad")};
          command.Run(4, argv, stream);
          REQUIRE(stream.output() == "error: invalid value\r\n");
        }
      }

      SECTION("With wrong number of arguments") {
        SECTION("Should print usage") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("set"),
                          const_cast<char*>("key")};
          command.Run(3, argv, stream);
          REQUIRE(stream.output().find("usage: config getall") != std::string::npos);
        }
      }
    }

    SECTION("When called with 'save'") {
      SECTION("With correct number of arguments") {
        SECTION("When commit succeeds") {
          SECTION("Should print ok") {
            char* argv[] = {const_cast<char*>("config"), const_cast<char*>("save")};
            command.Run(2, argv, stream);
            REQUIRE(stream.output() == "ok\r\n");
            REQUIRE(provider.commit_calls_ == 1);
          }
        }

        SECTION("When commit fails") {
          SECTION("Should print error") {
            provider.commit_result_ = midismith::config::TransactionResult::kFailure;
            char* argv[] = {const_cast<char*>("config"), const_cast<char*>("save")};
            command.Run(2, argv, stream);
            REQUIRE(stream.output() == "error: save failed\r\n");
          }
        }
      }

      SECTION("With extra arguments") {
        SECTION("Should print usage") {
          char* argv[] = {const_cast<char*>("config"), const_cast<char*>("save"),
                          const_cast<char*>("extra")};
          command.Run(3, argv, stream);
          REQUIRE(stream.output().find("usage: config getall") != std::string::npos);
        }
      }
    }

    SECTION("When called with unknown operation") {
      SECTION("Should print usage") {
        char* argv[] = {const_cast<char*>("config"), const_cast<char*>("unknown")};
        command.Run(2, argv, stream);
        REQUIRE(stream.output().find("usage: config getall") != std::string::npos);
      }
    }

    SECTION("When argv content is invalid for operation parsing") {
      SECTION("When argv is nullptr") {
        SECTION("Should return empty string_view") {
          command.Run(2, nullptr, stream);
          REQUIRE(stream.output().find("usage: config getall") != std::string::npos);
        }
      }

      SECTION("When argv[index] is nullptr") {
        SECTION("Should return empty string_view") {
          char* argv[] = {const_cast<char*>("config"), nullptr};
          command.Run(2, argv, stream);
          REQUIRE(stream.output().find("usage: config getall") != std::string::npos);
        }
      }
    }
  }
}
#endif
