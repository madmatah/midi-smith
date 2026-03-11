#include "app/composition/subsystems.hpp"
#include "app/config/flash_layout.hpp"
#include "app/storage/main_board_persistent_configuration.hpp"
#include "bsp/board.hpp"
#include "bsp/storage/spi_flash_sector_storage.hpp"
#include "domain/config/keymap_lookup.hpp"

namespace midismith::main_board::app::composition {

ConfigContext CreateConfigSubsystem() noexcept {
  static midismith::main_board::bsp::storage::SpiFlashSectorStorage config_flash_storage(
      midismith::main_board::bsp::Board::spi_flash(),
      midismith::main_board::app::config::kMainConfigSectorIndex);
  static midismith::main_board::app::storage::MainBoardPersistentConfiguration persistent_config(
      config_flash_storage);
  persistent_config.Load();
  static midismith::main_board::domain::config::KeymapLookup keymap_lookup(
      persistent_config.active_config().data);

  return ConfigContext{keymap_lookup, persistent_config};
}

}  // namespace midismith::main_board::app::composition
