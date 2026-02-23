#include "domain/shell/commands/config_command.hpp"

#include <cstddef>
#include <string_view>

namespace midismith::adc_board::domain::shell::commands {
namespace {

constexpr std::size_t kMaxValueSize = 64u;

std::string_view Arg(int argc, char** argv, int index) noexcept {
  if (argv == nullptr) {
    return {};
  }
  if (index < 0 || index >= argc) {
    return {};
  }
  if (argv[index] == nullptr) {
    return {};
  }
  return std::string_view(argv[index]);
}

void WriteUsage(midismith::io::WritableStreamRequirements& out) noexcept {
  out.Write("usage: config getall\r\n");
  out.Write("       config get <key>\r\n");
  out.Write("       config set <key> <value>\r\n");
  out.Write("       config save\r\n");
}

void WriteUnknownKey(midismith::io::WritableStreamRequirements& out) noexcept {
  out.Write("error: unknown key\r\n");
}

void WriteInvalidValue(midismith::io::WritableStreamRequirements& out) noexcept {
  out.Write("error: invalid value\r\n");
}

void WriteCannotReadValue(midismith::io::WritableStreamRequirements& out) noexcept {
  out.Write("error: cannot read value\r\n");
}

void WriteSaveFailed(midismith::io::WritableStreamRequirements& out) noexcept {
  out.Write("error: save failed\r\n");
}

void WriteOk(midismith::io::WritableStreamRequirements& out) noexcept {
  out.Write("ok\r\n");
}

void WriteKeyValue(midismith::io::WritableStreamRequirements& out, std::string_view key,
                   const char* value_buffer, std::size_t value_length) noexcept {
  out.Write(key);
  out.Write(": ");
  out.Write(std::string_view(value_buffer, value_length));
  out.Write("\r\n");
}

}  // namespace

void ConfigCommand::Run(int argc, char** argv,
                        midismith::io::WritableStreamRequirements& out) noexcept {
  const std::string_view op = Arg(argc, argv, 1);
  if (op.empty()) {
    WriteUsage(out);
    return;
  }

  if (op == "getall") {
    if (argc != 2) {
      WriteUsage(out);
      return;
    }

    for (std::size_t index = 0; index < provider_.KeyCount(); ++index) {
      const std::string_view key = provider_.KeyAt(index);
      if (key.empty()) {
        continue;
      }

      char value_buffer[kMaxValueSize]{};
      std::size_t value_length = 0u;
      auto status = provider_.GetValue(key, value_buffer, sizeof(value_buffer), value_length);
      if (status != midismith::adc_board::domain::config::ConfigGetStatus::kOk) {
        WriteCannotReadValue(out);
        return;
      }

      WriteKeyValue(out, key, value_buffer, value_length);
    }
    return;
  }

  if (op == "get") {
    if (argc != 3) {
      WriteUsage(out);
      return;
    }

    const std::string_view key = Arg(argc, argv, 2);
    char value_buffer[kMaxValueSize]{};
    std::size_t value_length = 0u;
    auto status = provider_.GetValue(key, value_buffer, sizeof(value_buffer), value_length);
    switch (status) {
      case midismith::adc_board::domain::config::ConfigGetStatus::kOk:
        WriteKeyValue(out, key, value_buffer, value_length);
        return;
      case midismith::adc_board::domain::config::ConfigGetStatus::kUnknownKey:
        WriteUnknownKey(out);
        return;
      case midismith::adc_board::domain::config::ConfigGetStatus::kBufferTooSmall:
      case midismith::adc_board::domain::config::ConfigGetStatus::kUnavailable:
        WriteCannotReadValue(out);
        return;
    }
    return;
  }

  if (op == "set") {
    if (argc != 4) {
      WriteUsage(out);
      return;
    }

    const std::string_view key = Arg(argc, argv, 2);
    const std::string_view value = Arg(argc, argv, 3);
    auto status = provider_.SetValue(key, value);
    switch (status) {
      case midismith::adc_board::domain::config::ConfigSetStatus::kOk:
        WriteOk(out);
        return;
      case midismith::adc_board::domain::config::ConfigSetStatus::kUnknownKey:
        WriteUnknownKey(out);
        return;
      case midismith::adc_board::domain::config::ConfigSetStatus::kInvalidValue:
        WriteInvalidValue(out);
        return;
    }
    return;
  }

  if (op == "save") {
    if (argc != 2) {
      WriteUsage(out);
      return;
    }

    auto save_result = provider_.Commit();
    if (save_result != midismith::adc_board::domain::config::TransactionResult::kSuccess) {
      WriteSaveFailed(out);
      return;
    }

    WriteOk(out);
    return;
  }

  WriteUsage(out);
}

}  // namespace midismith::adc_board::domain::shell::commands
