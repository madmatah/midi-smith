#pragma once

#include <cstddef>

#include "domain/io/stream_requirements.hpp"
#include "domain/shell/command_dispatcher.hpp"
#include "domain/shell/command_parser.hpp"
#include "domain/shell/line_editor.hpp"

namespace domain::shell {

struct ShellConfig {
  const char* prompt = "shell> ";
};

template <std::size_t kLineBufferSize, std::size_t kMaxCommands, std::size_t kMaxArgs>
class ShellEngine {
 public:
  ShellEngine(domain::io::StreamRequirements& stream, const ShellConfig& config) noexcept
      : _stream(stream), _config(config) {}

  void Init() noexcept {
    ShowPrompt();
  }

  void SetPrompt(const char* prompt) noexcept {
    _config.prompt = prompt;
  }

  bool RegisterCommand(CommandRequirements& command) noexcept {
    return _dispatcher.Register(command);
  }

  bool Poll() noexcept {
    bool line_ready = false;
    bool did_rx = _editor.Poll(_stream, _stream, line_ready, OnCompletion, this);

    if (line_ready) {
      char* line = _editor.GetLine();
      char* argv[kMaxArgs];
      int argc = CommandParser::ParseInPlace(line, kMaxArgs, argv);

      if (argc > 0) {
        _dispatcher.Dispatch(argc, argv, _stream);
      } else if (argc < 0) {
        _stream.Write("error: too many arguments\r\n");
      }

      _editor.Reset();
      ShowPrompt();
      return true;
    }

    return did_rx;
  }

 private:
  static void OnCompletion(char* buffer, std::size_t& cursor, std::size_t max_size,
                           domain::io::WritableStreamRequirements& writer,
                           void* user_data) noexcept {
    auto* self = static_cast<ShellEngine*>(user_data);
    self->HandleCompletion(buffer, cursor, max_size, writer);
  }

  void HandleCompletion(char* buffer, std::size_t& cursor, std::size_t max_size,
                        domain::io::WritableStreamRequirements& writer) noexcept {
    if (IsCompletingArgument(buffer, cursor)) {
      return;
    }

    buffer[cursor] = '\0';
    std::string_view prefix(buffer);

    CommandRequirements* matches[kMaxCommands];
    std::size_t count = _dispatcher.FindCompletions(prefix, matches, kMaxCommands);

    if (count == 1) {
      std::string_view name = matches[0] ? matches[0]->Name() : "help";
      std::string_view remaining = name.substr(prefix.length());

      for (char c : remaining) {
        if (cursor < max_size - 1) {
          buffer[cursor++] = c;
          writer.Write(c);
        }
      }

      if (cursor < max_size - 1) {
        buffer[cursor++] = ' ';
        writer.Write(' ');
      }
      buffer[cursor] = '\0';
    } else if (count > 1) {
      writer.Write("\r\n");
      for (std::size_t i = 0; i < count; ++i) {
        writer.Write(matches[i] ? matches[i]->Name().data() : "help");
        writer.Write("  ");
      }
      writer.Write("\r\n");
      ShowPrompt();
      writer.Write(std::string_view(buffer, cursor));
    }
  }

  bool IsCompletingArgument(const char* buffer, std::size_t cursor) const noexcept {
    for (std::size_t i = 0; i < cursor; ++i) {
      if (buffer[i] == ' ') {
        return true;
      }
    }
    return false;
  }


 private:
  void ShowPrompt() noexcept {
    _stream.Write(_config.prompt);
  }

  domain::io::StreamRequirements& _stream;
  ShellConfig _config;
  LineEditor<kLineBufferSize> _editor;
  CommandDispatcher<kMaxCommands> _dispatcher;
};

}  // namespace domain::shell
