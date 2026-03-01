#if defined(UNIT_TESTS)

#include "app/piano_sensing/logging_sensor_event_handler.hpp"

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

TEST_CASE("The LoggingSensorEventHandler class") {
  SECTION("The OnNoteOn method") {
    SECTION("When called with a velocity") {
      SECTION("Should log a note-on message with sensor id and velocity") {
        RecordingLogger logger;
        midismith::adc_board::app::piano_sensing::LoggingSensorEventHandler handler(logger, 5u);

        handler.OnNoteOn(static_cast<midismith::midi::Velocity>(99u));

        REQUIRE(logger.last_level == midismith::logging::Level::Info);
        REQUIRE(logger.last_message == "noteon:5:99\n");
      }
    }
  }

  SECTION("The OnNoteOff method") {
    SECTION("When called with a release velocity") {
      SECTION("Should log a note-off message with sensor id and velocity") {
        RecordingLogger logger;
        midismith::adc_board::app::piano_sensing::LoggingSensorEventHandler handler(logger, 12u);

        handler.OnNoteOff(static_cast<midismith::midi::Velocity>(64u));

        REQUIRE(logger.last_level == midismith::logging::Level::Info);
        REQUIRE(logger.last_message == "noteoff:12:64\n");
      }
    }
  }
}

#endif
