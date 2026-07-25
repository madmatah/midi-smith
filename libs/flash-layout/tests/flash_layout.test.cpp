#if defined(UNIT_TESTS)

#include "flash-layout/flash_layout.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

namespace {

using midismith::flash_layout::BankOf;
using midismith::flash_layout::IsSectorAligned;
using midismith::flash_layout::kApplicationConfigAddress;
using midismith::flash_layout::kApplicationConfigBank;
using midismith::flash_layout::kApplicationConfigSector;
using midismith::flash_layout::kApplicationLoadAddress;
using midismith::flash_layout::kBankOneAddress;
using midismith::flash_layout::kBootJournalAddress;
using midismith::flash_layout::kBootloaderAddress;
using midismith::flash_layout::kStagingAddress;
using midismith::flash_layout::SectorOf;

constexpr std::uint32_t kFirstBankTwoAddress = 0x08100000;
constexpr std::uint32_t kLastBankOneAddress = 0x080FFFFF;

}  // namespace

TEST_CASE("The BankOf function") {
  SECTION("When the address is the last byte of bank 1") {
    SECTION("Should report bank 1") {
      REQUIRE(BankOf(kLastBankOneAddress) == 1);
    }
  }

  SECTION("When the address is the first byte of bank 2") {
    SECTION("Should report bank 2") {
      REQUIRE(BankOf(kFirstBankTwoAddress) == 2);
    }
  }
}

TEST_CASE("The SectorOf function") {
  SECTION("When the address is the start of a bank") {
    SECTION("Should report sector 0, because sector numbers are relative to their bank") {
      REQUIRE(SectorOf(kBankOneAddress) == 0);
      REQUIRE(SectorOf(kFirstBankTwoAddress) == 0);
    }
  }

  SECTION("When the address is the last sector of bank 1") {
    SECTION("Should report sector 7, the index the HAL erase call takes") {
      REQUIRE(SectorOf(0x080E0000) == 7);
    }
  }

  SECTION("When the address is inside a sector rather than at its start") {
    SECTION("Should report the sector that contains it") {
      REQUIRE(SectorOf(0x080E0001) == 7);
      REQUIRE(SectorOf(kLastBankOneAddress) == 7);
    }
  }
}

TEST_CASE("The IsSectorAligned function") {
  SECTION("When the address starts a sector") {
    SECTION("Should accept it") {
      REQUIRE(IsSectorAligned(0x080E0000));
    }
  }

  SECTION("When the address is one byte into a sector") {
    SECTION("Should reject it, because erasing it would destroy the previous sector") {
      REQUIRE_FALSE(IsSectorAligned(0x080E0001));
    }
  }
}

TEST_CASE("The declared flash map") {
  SECTION("When the application configuration region is located") {
    SECTION("Should resolve to bank 1 sector 7, the pair the erase call names") {
      REQUIRE(kApplicationConfigAddress == 0x080E0000);
      REQUIRE(kApplicationConfigBank == 1);
      REQUIRE(kApplicationConfigSector == 7);
    }
  }

  SECTION("When the regions an application writes are located") {
    SECTION("Should all sit in the bank the application does not execute from") {
      REQUIRE(BankOf(kApplicationLoadAddress) == 2);
      REQUIRE(BankOf(kStagingAddress) == 1);
      REQUIRE(BankOf(kBootJournalAddress) == 1);
      REQUIRE(BankOf(kApplicationConfigAddress) == 1);
    }
  }

  SECTION("When the bootloader is located") {
    SECTION("Should start where the core fetches the reset vector") {
      REQUIRE(kBootloaderAddress == kBankOneAddress);
    }
  }
}

#endif
