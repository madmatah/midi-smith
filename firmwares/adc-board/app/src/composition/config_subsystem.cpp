#include "app/composition/subsystems.hpp"
#include "app/storage/adc_board_persistent_configuration.hpp"
#include "bsp/flash/internal_storage.hpp"

namespace midismith::adc_board::app::composition {

ConfigContext CreateConfigSubsystem() noexcept {
  static midismith::adc_board::bsp::flash::InternalStorage flash_storage;
  static midismith::adc_board::app::storage::AdcBoardPersistentConfiguration persistent_config(
      flash_storage);

  persistent_config.Load();

  return ConfigContext{persistent_config};
}

}  // namespace midismith::adc_board::app::composition
