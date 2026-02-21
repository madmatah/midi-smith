#if defined(UNIT_TESTS)

#include "domain/config/transactional_config_dictionary.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstring>

namespace {

class TransactionalConfigDictionaryStub final
    : public domain::config::TransactionalConfigDictionary {
 public:
  std::size_t KeyCount() const noexcept override {
    return 1u;
  }

  std::string_view KeyAt(std::size_t index) const noexcept override {
    if (index == 0u) {
      return "can_board_id";
    }
    return {};
  }

  domain::config::ConfigGetStatus GetValue(std::string_view key, char* value_buffer,
                                           std::size_t value_buffer_size,
                                           std::size_t& value_length) const noexcept override {
    if (key != "can_board_id") {
      return domain::config::ConfigGetStatus::kUnknownKey;
    }
    if (value_buffer == nullptr || value_buffer_size < 1u) {
      return domain::config::ConfigGetStatus::kBufferTooSmall;
    }

    value_buffer[0] = '1';
    value_length = 1u;
    return domain::config::ConfigGetStatus::kOk;
  }

  domain::config::ConfigSetStatus SetValue(std::string_view key,
                                           std::string_view value) noexcept override {
    if (key != "can_board_id") {
      return domain::config::ConfigSetStatus::kUnknownKey;
    }
    if (value.empty()) {
      return domain::config::ConfigSetStatus::kInvalidValue;
    }

    last_value_ = value;
    return domain::config::ConfigSetStatus::kOk;
  }

  domain::config::TransactionResult Commit() noexcept override {
    commit_called_ = true;
    return domain::config::TransactionResult::kSuccess;
  }

  std::string_view last_value() const noexcept {
    return last_value_;
  }

  bool commit_called() const noexcept {
    return commit_called_;
  }

 private:
  std::string_view last_value_{};
  bool commit_called_ = false;
};

}  // namespace

TEST_CASE("The TransactionalConfigDictionary class") {
  TransactionalConfigDictionaryStub dictionary;

  SECTION("The GetValue() method") {
    SECTION("When key exists and buffer is large enough") {
      SECTION("Should return kOk and write the value") {
        std::array<char, 4> value_buffer{};
        std::size_t value_length = 0u;

        auto status = dictionary.GetValue("can_board_id", value_buffer.data(), value_buffer.size(),
                                          value_length);

        REQUIRE(status == domain::config::ConfigGetStatus::kOk);
        REQUIRE(value_length == 1u);
        REQUIRE(std::strncmp(value_buffer.data(), "1", 1) == 0);
      }
    }
  }

  SECTION("The SetValue() method") {
    SECTION("When key exists and value is valid") {
      SECTION("Should return kOk and store the value") {
        auto status = dictionary.SetValue("can_board_id", "8");

        REQUIRE(status == domain::config::ConfigSetStatus::kOk);
        REQUIRE(dictionary.last_value() == "8");
      }
    }
  }

  SECTION("The Commit() method") {
    SECTION("When called") {
      SECTION("Should return kSuccess") {
        auto result = dictionary.Commit();

        REQUIRE(result == domain::config::TransactionResult::kSuccess);
        REQUIRE(dictionary.commit_called());
      }
    }
  }
}

#endif
