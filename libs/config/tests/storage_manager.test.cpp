#if defined(UNIT_TESTS)

#include "config/storage_manager.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include "config/storable_config.hpp"

namespace {

struct TestData {
  std::uint8_t value;
};

using TestConfig = midismith::config::StorableConfig<TestData, 0x5354474Du, 2>;

TestConfig CreateDefaultTestConfig() noexcept {
  TestConfig config{};
  std::memset(&config, 0, sizeof(config));
  config.magic_number = TestConfig::kMagicNumber;
  config.version = TestConfig::kVersion;
  config.data.value = 11;
  midismith::config::ConfigValidator<TestConfig>::StampCrc(config);
  return config;
}

TestConfig CreateTestConfig(std::uint8_t value,
                            std::uint16_t version = TestConfig::kVersion) noexcept {
  auto config = CreateDefaultTestConfig();
  config.version = version;
  config.data.value = value;
  midismith::config::ConfigValidator<TestConfig>::StampCrc(config);
  return config;
}

class FlashStorageStub final : public midismith::bsp::storage::FlashSectorStorageRequirements {
 public:
  static constexpr std::size_t kSectorSize = 4096;

  FlashStorageStub() noexcept { std::memset(storage_, 0xFF, sizeof(storage_)); }

  std::size_t SectorSizeBytes() const noexcept override { return kSectorSize; }

  midismith::bsp::storage::StorageOperationResult Read(
      std::size_t offset_bytes, std::uint8_t* buffer,
      std::size_t length_bytes) const noexcept override {
    if (offset_bytes + length_bytes > kSectorSize) {
      return midismith::bsp::storage::StorageOperationResult::kError;
    }
    std::memcpy(buffer, storage_ + offset_bytes, length_bytes);
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  midismith::bsp::storage::StorageOperationResult EraseSector() noexcept override {
    if (erase_should_fail) {
      return midismith::bsp::storage::StorageOperationResult::kError;
    }

    std::memset(storage_, 0xFF, sizeof(storage_));
    ++erase_count;
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  midismith::bsp::storage::StorageOperationResult Write(
      std::size_t offset_bytes, const std::uint8_t* data,
      std::size_t length_bytes) noexcept override {
    if (write_should_fail) {
      return midismith::bsp::storage::StorageOperationResult::kError;
    }
    if (offset_bytes + length_bytes > kSectorSize) {
      return midismith::bsp::storage::StorageOperationResult::kError;
    }

    std::memcpy(storage_ + offset_bytes, data, length_bytes);
    ++write_count;
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  void WriteConfig(const TestConfig& config) noexcept {
    std::memcpy(storage_, &config, sizeof(config));
  }

  const std::uint8_t* storage() const noexcept { return storage_; }

  bool erase_should_fail = false;
  bool write_should_fail = false;
  int erase_count = 0;
  int write_count = 0;

 private:
  alignas(32) std::uint8_t storage_[kSectorSize]{};
};

}  // namespace

TEST_CASE("The StorageManager class") {
  auto default_config = CreateDefaultTestConfig();
  FlashStorageStub flash;
  midismith::config::StorageManager<TestConfig> storage_manager(flash, default_config);

  SECTION("The Load method") {
    SECTION("When flash is virgin") {
      TestConfig ram_config{};
      auto status = storage_manager.Load(ram_config);

      REQUIRE(status == midismith::config::ConfigStatus::kVirginFlash);
      REQUIRE(ram_config.data.value == default_config.data.value);
    }

    SECTION("When flash has invalid magic") {
      auto config = CreateTestConfig(22);
      config.magic_number = 0xDEADBEEFu;
      midismith::config::ConfigValidator<TestConfig>::StampCrc(config);
      flash.WriteConfig(config);

      TestConfig ram_config{};
      auto status = storage_manager.Load(ram_config);

      REQUIRE(status == midismith::config::ConfigStatus::kInvalidMagic);
      REQUIRE(ram_config.data.value == default_config.data.value);
    }

    SECTION("When flash has invalid CRC") {
      auto config = CreateTestConfig(33);
      config.crc32 ^= 0x01u;
      flash.WriteConfig(config);

      TestConfig ram_config{};
      auto status = storage_manager.Load(ram_config);

      REQUIRE(status == midismith::config::ConfigStatus::kInvalidCrc);
      REQUIRE(ram_config.data.value == default_config.data.value);
    }

    SECTION("When flash has a newer version") {
      auto config = CreateTestConfig(44, TestConfig::kVersion + 1);
      flash.WriteConfig(config);

      TestConfig ram_config{};
      auto status = storage_manager.Load(ram_config);

      REQUIRE(status == midismith::config::ConfigStatus::kNewerVersion);
      REQUIRE(ram_config.data.value == default_config.data.value);
    }

    SECTION("When flash has a valid config") {
      auto config = CreateTestConfig(55);
      flash.WriteConfig(config);

      TestConfig ram_config{};
      auto status = storage_manager.Load(ram_config);

      REQUIRE(status == midismith::config::ConfigStatus::kValid);
      REQUIRE(ram_config.data.value == 55);
    }

    SECTION("When flash has an older config") {
      auto config = CreateTestConfig(66, TestConfig::kVersion - 1);
      flash.WriteConfig(config);

      TestConfig ram_config{};
      auto status = storage_manager.Load(ram_config);

      REQUIRE(status == midismith::config::ConfigStatus::kOlderVersion);
      REQUIRE(ram_config.data.value == 66);
    }
  }

  SECTION("The Save method") {
    SECTION("When data is unchanged") {
      auto config = CreateDefaultTestConfig();
      flash.WriteConfig(config);

      auto result = storage_manager.Save(config);

      REQUIRE(result == midismith::bsp::storage::StorageOperationResult::kSuccess);
      REQUIRE(flash.erase_count == 0);
      REQUIRE(flash.write_count == 0);
    }

    SECTION("When data is changed") {
      auto config = CreateTestConfig(77);

      auto result = storage_manager.Save(config);

      REQUIRE(result == midismith::bsp::storage::StorageOperationResult::kSuccess);
      REQUIRE(flash.erase_count == 1);
      REQUIRE(flash.write_count == 1);
    }

    SECTION("When erase fails") {
      auto config = CreateTestConfig(77);
      flash.erase_should_fail = true;

      auto result = storage_manager.Save(config);

      REQUIRE(result == midismith::bsp::storage::StorageOperationResult::kError);
      REQUIRE(flash.write_count == 0);
    }

    SECTION("When write fails") {
      auto config = CreateTestConfig(77);
      flash.write_should_fail = true;

      auto result = storage_manager.Save(config);

      REQUIRE(result == midismith::bsp::storage::StorageOperationResult::kError);
      REQUIRE(flash.erase_count == 1);
    }

    SECTION("When RAM CRC is wrong") {
      auto config = CreateTestConfig(88);
      config.crc32 = 0;

      auto result = storage_manager.Save(config);
      REQUIRE(result == midismith::bsp::storage::StorageOperationResult::kSuccess);

      TestConfig saved{};
      std::memcpy(&saved, flash.storage(), sizeof(saved));
      REQUIRE(midismith::config::ConfigValidator<TestConfig>::Validate(saved) ==
              midismith::config::ConfigStatus::kValid);
      REQUIRE(saved.data.value == 88);
    }
  }
}

#endif
