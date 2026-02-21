#include "app/composition/subsystems.hpp"
#include "app/storage/persistent_configuration.hpp"
#include "bsp/flash/internal_storage.hpp"

namespace app::composition {

ConfigContext CreateConfigSubsystem() noexcept {
  static bsp::flash::InternalStorage flash_storage;
  static app::storage::PersistentConfiguration persistent_config(flash_storage);

  persistent_config.Load();

  return ConfigContext{persistent_config};
}

}  // namespace app::composition
