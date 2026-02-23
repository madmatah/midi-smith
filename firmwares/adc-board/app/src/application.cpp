#include "app/application.hpp"

#include "app/composition/subsystems.hpp"
#include "app/config/analog_acquisition.hpp"
#include "app/config/config.hpp"
#include "app/telemetry/sensor_rtt_telemetry_defaults.hpp"
#include "bsp/board.hpp"
#include "bsp/memory_sections.hpp"
#include "bsp/rtt_logger.hpp"
#include "bsp/serial/uart_stream.hpp"
#include "usart.h"

namespace midismith::adc_board::app {

void Application::init() noexcept {
  midismith::adc_board::bsp::Board::init();
}

void Application::create_tasks() noexcept {
  static midismith::bsp::RttLogger logger;
  BSP_AXI_SRAM static midismith::adc_board::app::telemetry::SensorRttStreamCapture
      sensor_rtt_capture;
  sensor_rtt_capture.SetOutputHz(
      ::midismith::adc_board::app::telemetry::DefaultSensorRttTelemetryOutputHz());

  // Console Stream (USART1)
  alignas(32) BSP_AXI_SRAM_NOCACHE static midismith::adc_board::bsp::serial::UartStream<256, 1024>
      console_stream(huart1);
  (void) console_stream.StartRxDma();

  midismith::adc_board::app::composition::ConsoleContext console{console_stream};
  midismith::adc_board::app::composition::ConfigContext config =
      midismith::adc_board::app::composition::CreateConfigSubsystem();

  midismith::adc_board::app::composition::AdcControlContext adc_control =
      midismith::adc_board::app::composition::CreateAnalogSubsystem(sensor_rtt_capture, logger);
  midismith::adc_board::app::composition::AdcStateContext adc_state =
      midismith::adc_board::app::composition::CreateAdcStateContext();
  midismith::adc_board::app::composition::SensorsContext sensors =
      midismith::adc_board::app::composition::CreateSensorsContext();

  midismith::adc_board::app::composition::SensorRttTelemetryControlContext sensor_rtt =
      midismith::adc_board::app::composition::CreateSensorRttTelemetrySubsystem(sensors, adc_state,
                                                                                sensor_rtt_capture);
  midismith::adc_board::app::composition::CreateShellSubsystem(console, config, adc_control,
                                                               sensors, sensor_rtt);
}

}  // namespace midismith::adc_board::app
