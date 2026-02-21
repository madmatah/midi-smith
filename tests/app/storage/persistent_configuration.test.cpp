#if defined(UNIT_TESTS)

#include "app/storage/persistent_configuration.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include "domain/hash/crc32.hpp"

namespace {

class FlashStorageStub final : public bsp::flash::StorageRequirements {
 public:
  static constexpr std::size_t kSectorSize = 128 * 1024;

  FlashStorageStub() noexcept {
    std::memset(storage_, 0xFF, sizeof(storage_));
  }

  const void* BaseAddress() const noexcept override {
    return storage_;
  }

  std::size_t SectorSizeBytes() const noexcept override {
    return kSectorSize;
  }

  bsp::flash::OperationResult EraseSector() noexcept override {
    if (erase_should_fail) {
      return bsp::flash::OperationResult::kError;
    }
    std::memset(storage_, 0xFF, sizeof(storage_));
    ++erase_count;
    return bsp::flash::OperationResult::kSuccess;
  }

  bsp::flash::OperationResult ProgramFlashWords(std::size_t offset_bytes, const std::uint8_t* data,
                                                std::size_t length_bytes) noexcept override {
    if (program_should_fail) {
      return bsp::flash::OperationResult::kError;
    }
    if (offset_bytes + length_bytes > kSectorSize) {
      return bsp::flash::OperationResult::kError;
    }
    std::memcpy(storage_ + offset_bytes, data, length_bytes);
    ++program_count;
    return bsp::flash::OperationResult::kSuccess;
  }

  void WriteConfig(const domain::config::UserConfig& config) {
    std::memcpy(storage_, &config, sizeof(config));
  }

  bool erase_should_fail = false;
  bool program_should_fail = false;
  int erase_count = 0;
  int program_count = 0;

 private:
  alignas(32) std::uint8_t storage_[kSectorSize]{};
};

domain::config::UserConfig CreateValidConfig(std::uint8_t board_id = 3) {
  domain::config::UserConfig config{};
  std::memset(&config, 0, sizeof(config));
  config.magic_number = domain::config::kConfigMagicNumber;
  config.version = domain::config::kConfigVersion;
  config.data.can_board_id = board_id;
  config.crc32 = domain::hash::ComputeCrc32(reinterpret_cast<const std::uint8_t*>(&config),
                                            offsetof(domain::config::UserConfig, crc32));
  return config;
}

}  // namespace

TEST_CASE("The PersistentConfiguration class") {
  FlashStorageStub flash;
  app::storage::PersistentConfiguration persistent_config(flash);

  SECTION("The Load() method") {
    SECTION("When flash is virgin (all 0xFF)") {
      SECTION("Should load default config") {
        auto status = persistent_config.Load();
        REQUIRE(status == domain::config::ConfigStatus::kVirginFlash);
        REQUIRE(persistent_config.active_config().data.can_board_id ==
                domain::config::kDefaultBoardId);
        REQUIRE(persistent_config.active_config().magic_number ==
                domain::config::kConfigMagicNumber);
      }
    }

    SECTION("When flash contains a valid config") {
      SECTION("Should load the stored values") {
        auto valid = CreateValidConfig(7);
        flash.WriteConfig(valid);

        auto status = persistent_config.Load();
        REQUIRE(status == domain::config::ConfigStatus::kValid);
        REQUIRE(persistent_config.active_config().data.can_board_id == 7);
      }
    }

    SECTION("When flash contains corrupted data") {
      SECTION("Should fall back to defaults") {
        auto config = CreateValidConfig(5);
        config.crc32 ^= 0x01u;
        flash.WriteConfig(config);

        auto status = persistent_config.Load();
        REQUIRE(status == domain::config::ConfigStatus::kInvalidCrc);
        REQUIRE(persistent_config.active_config().data.can_board_id ==
                domain::config::kDefaultBoardId);
      }
    }
  }

  SECTION("The UpdateBoardId() method") {
    SECTION("When board ID is valid") {
      SECTION("Should update the RAM config") {
        persistent_config.Load();
        REQUIRE(persistent_config.UpdateBoardId(5));
        REQUIRE(persistent_config.active_config().data.can_board_id == 5);
      }
    }

    SECTION("When board ID is 0") {
      SECTION("Should reject the update") {
        persistent_config.Load();
        REQUIRE_FALSE(persistent_config.UpdateBoardId(0));
        REQUIRE(persistent_config.active_config().data.can_board_id ==
                domain::config::kDefaultBoardId);
      }
    }

    SECTION("When board ID exceeds maximum") {
      SECTION("Should reject the update") {
        persistent_config.Load();
        REQUIRE_FALSE(persistent_config.UpdateBoardId(9));
      }
    }
  }

  SECTION("The Commit() method") {
    SECTION("Should erase and program flash") {
      persistent_config.Load();
      persistent_config.UpdateBoardId(4);

      auto result = persistent_config.Commit();
      REQUIRE(result == bsp::flash::OperationResult::kSuccess);
      REQUIRE(flash.erase_count == 1);
      REQUIRE(flash.program_count == 1);
    }

    SECTION("Should write data that can be loaded back") {
      persistent_config.Load();
      persistent_config.UpdateBoardId(6);
      persistent_config.Commit();

      app::storage::PersistentConfiguration reloaded(flash);
      auto status = reloaded.Load();
      REQUIRE(status == domain::config::ConfigStatus::kValid);
      REQUIRE(reloaded.active_config().data.can_board_id == 6);
    }

    SECTION("When erase fails") {
      SECTION("Should return error") {
        persistent_config.Load();
        flash.erase_should_fail = true;
        auto result = persistent_config.Commit();
        REQUIRE(result == bsp::flash::OperationResult::kError);
      }
    }

    SECTION("When program fails") {
      SECTION("Should return error") {
        persistent_config.Load();
        flash.program_should_fail = true;
        auto result = persistent_config.Commit();
        REQUIRE(result == bsp::flash::OperationResult::kError);
      }
    }
  }
}

#endif
