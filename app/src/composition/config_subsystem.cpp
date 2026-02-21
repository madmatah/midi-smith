#include "app/composition/subsystems.hpp"
#include "app/storage/adc_board_persistent_configuration.hpp"
#include "bsp/flash/internal_storage.hpp"

namespace app::composition {

ConfigContext CreateConfigSubsystem() noexcept {
  static bsp::flash::InternalStorage flash_storage;
  static app::storage::AdcBoardPersistentConfiguration persistent_config(flash_storage);

  persistent_config.Load();

  return ConfigContext{persistent_config};
}

}  // namespace app::composition
