#include "bsp/stm32_octospi.hpp"

#include <algorithm>
#include <cstring>

namespace midismith::main_board::bsp {

static constexpr std::uint8_t kCommandWriteEnable = 0x06;
static constexpr std::uint8_t kCommandReadStatusRegister1 = 0x05;
static constexpr std::uint8_t kCommandSectorErase = 0x20;
static constexpr std::uint8_t kCommandChipErase = 0xC7;
static constexpr std::uint8_t kCommandPageProgram = 0x02;
static constexpr std::uint8_t kCommandFastReadQuadIO = 0xEB;
static constexpr std::uint8_t kCommandNormalRead = 0x03;

static constexpr std::uint8_t kStatusRegisterBusyBit = 0x01;
static constexpr std::uint8_t kStatusRegisterWriteEnableBit = 0x02;

static constexpr std::uint32_t kExternalFlashBaseAddress = 0x90000000;

Stm32Octospi::Stm32Octospi(OSPI_HandleTypeDef& hospi_handle) noexcept
    : _hospi_handle(hospi_handle) {}

bool Stm32Octospi::Read(std::uint32_t address, void* buffer, std::size_t size_bytes) noexcept {
  if (_is_memory_mapped) {
    if (address + size_bytes > chip_size_bytes()) {
      return false;
    }
    std::memcpy(buffer, reinterpret_cast<void*>(kExternalFlashBaseAddress + address), size_bytes);
    return true;
  }

  OSPI_RegularCmdTypeDef read_command = {0};
  read_command.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  read_command.FlashId = HAL_OSPI_FLASH_ID_1;
  read_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  read_command.Instruction = kCommandNormalRead;
  read_command.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
  read_command.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
  read_command.Address = address;
  read_command.DataMode = HAL_OSPI_DATA_1_LINE;
  read_command.NbData = size_bytes;
  read_command.DummyCycles = 0;

  if (HAL_OSPI_Command(&_hospi_handle, &read_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return false;
  }
  if (HAL_OSPI_Receive(&_hospi_handle, static_cast<std::uint8_t*>(buffer),
                       HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return false;
  }

  return true;
}

bool Stm32Octospi::Write(std::uint32_t address, const void* data, std::size_t size_bytes) noexcept {
  if (_is_memory_mapped) {
    return false;
  }

  std::size_t remaining_bytes_to_write = size_bytes;
  std::uint32_t current_address = address;
  const std::uint8_t* source_data_pointer = static_cast<const std::uint8_t*>(data);

  while (remaining_bytes_to_write > 0) {
    std::size_t page_offset_bytes = current_address % page_size_bytes();
    std::size_t bytes_to_program =
        std::min(remaining_bytes_to_write, page_size_bytes() - page_offset_bytes);

    if (!EnableWriteOperations()) {
      return false;
    }

    OSPI_RegularCmdTypeDef program_command = {0};
    program_command.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    program_command.FlashId = HAL_OSPI_FLASH_ID_1;
    program_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    program_command.Instruction = kCommandPageProgram;
    program_command.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
    program_command.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
    program_command.Address = current_address;
    program_command.DataMode = HAL_OSPI_DATA_1_LINE;
    program_command.NbData = bytes_to_program;

    if (HAL_OSPI_Command(&_hospi_handle, &program_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
        HAL_OK) {
      return false;
    }
    if (HAL_OSPI_Transmit(&_hospi_handle, const_cast<std::uint8_t*>(source_data_pointer),
                          HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      return false;
    }
    if (!WaitUntilReady()) {
      return false;
    }

    remaining_bytes_to_write -= bytes_to_program;
    current_address += bytes_to_program;
    source_data_pointer += bytes_to_program;
  }

  return true;
}

bool Stm32Octospi::EraseSector(std::uint32_t address) noexcept {
  if (_is_memory_mapped) {
    return false;
  }
  if (!EnableWriteOperations()) {
    return false;
  }

  OSPI_RegularCmdTypeDef erase_command = {0};
  erase_command.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  erase_command.FlashId = HAL_OSPI_FLASH_ID_1;
  erase_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  erase_command.Instruction = kCommandSectorErase;
  erase_command.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
  erase_command.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
  erase_command.Address = address;
  erase_command.DataMode = HAL_OSPI_DATA_NONE;

  if (HAL_OSPI_Command(&_hospi_handle, &erase_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return false;
  }
  return WaitUntilReady();
}

bool Stm32Octospi::EraseChip() noexcept {
  if (_is_memory_mapped) {
    return false;
  }
  if (!EnableWriteOperations()) {
    return false;
  }

  OSPI_RegularCmdTypeDef erase_command = {0};
  erase_command.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  erase_command.FlashId = HAL_OSPI_FLASH_ID_1;
  erase_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  erase_command.Instruction = kCommandChipErase;
  erase_command.AddressMode = HAL_OSPI_ADDRESS_NONE;
  erase_command.DataMode = HAL_OSPI_DATA_NONE;

  if (HAL_OSPI_Command(&_hospi_handle, &erase_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return false;
  }
  return WaitUntilReady();
}

bool Stm32Octospi::EnableDirectAccess() noexcept {
  if (_is_memory_mapped) {
    return true;
  }

  OSPI_RegularCmdTypeDef write_configuration = {0};
  OSPI_MemoryMappedTypeDef memory_mapped_configuration = {0};

  write_configuration.OperationType = HAL_OSPI_OPTYPE_WRITE_CFG;
  write_configuration.FlashId = HAL_OSPI_FLASH_ID_1;
  write_configuration.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  write_configuration.Instruction = kCommandPageProgram;
  write_configuration.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
  write_configuration.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
  write_configuration.DataMode = HAL_OSPI_DATA_1_LINE;
  write_configuration.DummyCycles = 0;

  if (HAL_OSPI_Command(&_hospi_handle, &write_configuration, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return false;
  }

  OSPI_RegularCmdTypeDef read_configuration = {0};
  read_configuration.OperationType = HAL_OSPI_OPTYPE_READ_CFG;
  read_configuration.FlashId = HAL_OSPI_FLASH_ID_1;
  read_configuration.Instruction = kCommandFastReadQuadIO;
  read_configuration.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  read_configuration.AddressMode = HAL_OSPI_ADDRESS_4_LINES;
  read_configuration.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
  read_configuration.DataMode = HAL_OSPI_DATA_4_LINES;
  read_configuration.DummyCycles = 6;

  if (HAL_OSPI_Command(&_hospi_handle, &read_configuration, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return false;
  }

  memory_mapped_configuration.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;
  if (HAL_OSPI_MemoryMapped(&_hospi_handle, &memory_mapped_configuration) != HAL_OK) {
    return false;
  }

  _is_memory_mapped = true;
  return true;
}

bool Stm32Octospi::WaitUntilReady() noexcept {
  OSPI_RegularCmdTypeDef status_command = {0};
  OSPI_AutoPollingTypeDef auto_polling_configuration = {0};

  status_command.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  status_command.FlashId = HAL_OSPI_FLASH_ID_1;
  status_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  status_command.Instruction = kCommandReadStatusRegister1;
  status_command.AddressMode = HAL_OSPI_ADDRESS_NONE;
  status_command.DataMode = HAL_OSPI_DATA_1_LINE;
  status_command.NbData = 1;

  if (HAL_OSPI_Command(&_hospi_handle, &status_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return false;
  }

  auto_polling_configuration.Match = 0x00;
  auto_polling_configuration.Mask = kStatusRegisterBusyBit;
  auto_polling_configuration.MatchMode = HAL_OSPI_MATCH_MODE_AND;
  auto_polling_configuration.Interval = 0x10;
  auto_polling_configuration.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_OSPI_AutoPolling(&_hospi_handle, &auto_polling_configuration,
                           HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return false;
  }

  return true;
}

bool Stm32Octospi::EnableWriteOperations() noexcept {
  OSPI_RegularCmdTypeDef write_enable_command = {0};
  write_enable_command.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  write_enable_command.FlashId = HAL_OSPI_FLASH_ID_1;
  write_enable_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  write_enable_command.Instruction = kCommandWriteEnable;
  write_enable_command.AddressMode = HAL_OSPI_ADDRESS_NONE;
  write_enable_command.DataMode = HAL_OSPI_DATA_NONE;

  if (HAL_OSPI_Command(&_hospi_handle, &write_enable_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    return false;
  }

  OSPI_RegularCmdTypeDef status_command = {0};
  status_command.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
  status_command.FlashId = HAL_OSPI_FLASH_ID_1;
  status_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
  status_command.Instruction = kCommandReadStatusRegister1;
  status_command.AddressMode = HAL_OSPI_ADDRESS_NONE;
  status_command.DataMode = HAL_OSPI_DATA_1_LINE;
  status_command.NbData = 1;

  if (HAL_OSPI_Command(&_hospi_handle, &status_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return false;
  }

  OSPI_AutoPollingTypeDef auto_polling_configuration = {0};
  auto_polling_configuration.Match = kStatusRegisterWriteEnableBit;
  auto_polling_configuration.Mask = kStatusRegisterWriteEnableBit;
  auto_polling_configuration.MatchMode = HAL_OSPI_MATCH_MODE_AND;
  auto_polling_configuration.Interval = 0x10;
  auto_polling_configuration.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_OSPI_AutoPolling(&_hospi_handle, &auto_polling_configuration,
                           HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return false;
  }

  return true;
}

}  // namespace midismith::main_board::bsp
