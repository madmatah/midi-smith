#if defined(UNIT_TESTS)

#include "app/messaging/main_board_inbound_sensor_event_logging_handler.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdarg>
#include <cstdio>
#include <string>

namespace {

class RecordingLogger final : public midismith::logging::LoggerRequirements {
 public:
  void vlogf(midismith::logging::Level level, const char* fmt,
             std::va_list* args) noexcept override {
    last_level = level;

    char buffer[128]{};
    std::vsnprintf(buffer, sizeof(buffer), fmt, *args);
    last_message = buffer;
  }

  midismith::logging::Level last_level = midismith::logging::Level::Debug;
  std::string last_message;
};

}  // namespace

TEST_CASE("The MainBoardInboundSensorEventLoggingHandler class", "[main-board][app][messaging]") {
  RecordingLogger logger;
  midismith::main_board::app::messaging::MainBoardInboundSensorEventLoggingHandler handler(logger);

  SECTION("When the sensor event is kNoteOn") {
    const midismith::protocol::SensorEvent event = {
        .type = midismith::protocol::SensorEventType::kNoteOn,
        .sensor_id = 9u,
        .velocity = 101u,
    };

    handler.OnSensorEvent(event);

    REQUIRE(logger.last_level == midismith::logging::Level::Info);
    REQUIRE(logger.last_message == "noteon:9:101\n");
  }

  SECTION("When the sensor event is kNoteOff") {
    const midismith::protocol::SensorEvent event = {
        .type = midismith::protocol::SensorEventType::kNoteOff,
        .sensor_id = 4u,
        .velocity = 67u,
    };

    handler.OnSensorEvent(event);

    REQUIRE(logger.last_level == midismith::logging::Level::Info);
    REQUIRE(logger.last_message == "noteoff:4:67\n");
  }
}

#endif
