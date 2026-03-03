#if defined(UNIT_TESTS)

#include "protocol-can/can_filter_factory.hpp"

#include <catch2/catch_test_macros.hpp>

using midismith::protocol_can::CanFilterFactory;

TEST_CASE("The CanFilterFactory class") {
  SECTION("The MakeAdcFilters() method") {
    SECTION("When node_id is 1") {
      const auto filter_set = CanFilterFactory::MakeAdcFilters(1);

      SECTION("Should return 4 filters") {
        REQUIRE(filter_set.filters.size() == 4);
      }

      SECTION("Filter 0 should accept global commands (0x100, mask 0x7FF)") {
        REQUIRE(filter_set.filters[0].filter_index == 0u);
        REQUIRE(filter_set.filters[0].id == 0x100u);
        REQUIRE(filter_set.filters[0].id_mask == 0x7FFu);
      }

      SECTION("Filter 1 should accept unicast commands for node 1 (0x111, mask 0x7FF)") {
        REQUIRE(filter_set.filters[1].filter_index == 1u);
        REQUIRE(filter_set.filters[1].id == 0x111u);
        REQUIRE(filter_set.filters[1].id_mask == 0x7FFu);
      }

      SECTION("Filter 2 should accept calibration data for node 1 (0x211, mask 0x7FF)") {
        REQUIRE(filter_set.filters[2].filter_index == 2u);
        REQUIRE(filter_set.filters[2].id == 0x211u);
        REQUIRE(filter_set.filters[2].id_mask == 0x7FFu);
      }

      SECTION("Filter 3 should accept calibration ACKs for node 1 (0x221, mask 0x7FF)") {
        REQUIRE(filter_set.filters[3].filter_index == 3u);
        REQUIRE(filter_set.filters[3].id == 0x221u);
        REQUIRE(filter_set.filters[3].id_mask == 0x7FFu);
      }
    }

    SECTION("When node_id is 8 (boundary)") {
      const auto filter_set = CanFilterFactory::MakeAdcFilters(8);

      SECTION("Filter 1 should accept unicast commands for node 8 (0x118, mask 0x7FF)") {
        REQUIRE(filter_set.filters[1].id == 0x118u);
      }

      SECTION("Filter 2 should accept calibration data for node 8 (0x218, mask 0x7FF)") {
        REQUIRE(filter_set.filters[2].id == 0x218u);
      }

      SECTION("Filter 3 should accept calibration ACKs for node 8 (0x228, mask 0x7FF)") {
        REQUIRE(filter_set.filters[3].id == 0x228u);
      }
    }

    SECTION("Global command filter should be identical for all node IDs") {
      for (std::uint8_t node_id = 1; node_id <= 8; ++node_id) {
        const auto filter_set = CanFilterFactory::MakeAdcFilters(node_id);
        REQUIRE(filter_set.filters[0].id == 0x100u);
        REQUIRE(filter_set.filters[0].id_mask == 0x7FFu);
      }
    }
  }

  SECTION("The MakeMainFilters() method") {
    const auto filter_set = CanFilterFactory::MakeMainFilters();

    SECTION("Should return 4 filters") {
      REQUIRE(filter_set.filters.size() == 4);
    }

    SECTION("Filter 0 should accept real-time events (0x010, mask 0x7F0)") {
      REQUIRE(filter_set.filters[0].filter_index == 0u);
      REQUIRE(filter_set.filters[0].id == 0x010u);
      REQUIRE(filter_set.filters[0].id_mask == 0x7F0u);
    }

    SECTION("Filter 1 should accept calibration data from any ADC (0x210, mask 0x7F0)") {
      REQUIRE(filter_set.filters[1].filter_index == 1u);
      REQUIRE(filter_set.filters[1].id == 0x210u);
      REQUIRE(filter_set.filters[1].id_mask == 0x7F0u);
    }

    SECTION("Filter 2 should accept calibration ACKs from any ADC (0x220, mask 0x7F0)") {
      REQUIRE(filter_set.filters[2].filter_index == 2u);
      REQUIRE(filter_set.filters[2].id == 0x220u);
      REQUIRE(filter_set.filters[2].id_mask == 0x7F0u);
    }

    SECTION("Filter 3 should accept heartbeats from any ADC (0x710, mask 0x7F0)") {
      REQUIRE(filter_set.filters[3].filter_index == 3u);
      REQUIRE(filter_set.filters[3].id == 0x710u);
      REQUIRE(filter_set.filters[3].id_mask == 0x7F0u);
    }
  }
}

#endif  // UNIT_TESTS
