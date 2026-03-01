#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include "io/stream_format.hpp"
#include "shell/command_requirements.hpp"
#include "stats/stats_provider_requirements.hpp"

namespace midismith::shell_cmd_stats {

template <typename RequestT>
class NoArgsRequestParser {
 public:
  bool operator()(std::string_view command_name, int argc, char** argv, RequestT& request,
                  midismith::io::WritableStreamRequirements& out) const noexcept {
    (void) argv;
    (void) request;
    if (argc > 1) {
      out.Write("usage: ");
      out.Write(command_name);
      out.Write("\r\n");
      return false;
    }
    return true;
  }
};

namespace detail {

class ShellStatsVisitor final : public midismith::stats::StatsVisitorRequirements {
 public:
  explicit ShellStatsVisitor(midismith::io::WritableStreamRequirements& out) noexcept : out_(out) {}

  void OnMetric(std::string_view metric_name, std::uint32_t value) noexcept override {
    WriteMetricPrefix(metric_name);
    midismith::io::WriteUint32(out_, value);
    out_.Write("\r\n");
  }

  void OnMetric(std::string_view metric_name, std::int32_t value) noexcept override {
    WriteMetricPrefix(metric_name);
    midismith::io::WriteInt32(out_, value);
    out_.Write("\r\n");
  }

  void OnMetric(std::string_view metric_name, std::uint64_t value) noexcept override {
    WriteMetricPrefix(metric_name);
    midismith::io::WriteUint64(out_, value);
    out_.Write("\r\n");
  }

  void OnMetric(std::string_view metric_name, std::int64_t value) noexcept override {
    WriteMetricPrefix(metric_name);
    midismith::io::WriteInt64(out_, value);
    out_.Write("\r\n");
  }

  void OnMetric(std::string_view metric_name, bool value) noexcept override {
    WriteMetricPrefix(metric_name);
    midismith::io::WriteBool(out_, value);
    out_.Write("\r\n");
  }

  void OnMetric(std::string_view metric_name, std::string_view value_text) noexcept override {
    WriteMetricPrefix(metric_name);
    out_.Write(value_text);
    out_.Write("\r\n");
  }

 private:
  void WriteMetricPrefix(std::string_view metric_name) noexcept {
    out_.Write("  ");
    out_.Write(metric_name);
    out_.Write(": ");
  }

  midismith::io::WritableStreamRequirements& out_;
};

}  // namespace detail

template <typename RequestT, std::size_t kProviderCount,
          typename RequestParserT = NoArgsRequestParser<RequestT>>
class GenericStatsCommand final : public midismith::shell::CommandRequirements {
 public:
  using ProviderRequirements = midismith::stats::StatsProviderRequirements<RequestT>;

  GenericStatsCommand(std::string_view name, std::string_view help,
                      std::span<ProviderRequirements*, kProviderCount> providers,
                      RequestParserT request_parser = RequestParserT{}) noexcept
      : name_(name), help_(help), providers_(providers), request_parser_(request_parser) {}

  std::string_view Name() const noexcept override {
    return name_;
  }

  std::string_view Help() const noexcept override {
    return help_;
  }

  void Run(int argc, char** argv,
           midismith::io::WritableStreamRequirements& out) noexcept override {
    static_assert(std::is_invocable_r_v<bool, RequestParserT, std::string_view, int, char**,
                                        RequestT&, midismith::io::WritableStreamRequirements&>,
                  "RequestParserT must be callable as "
                  "bool(std::string_view,int,char**,RequestT&,WritableStreamRequirements&).");

    RequestT request{};
    if (!request_parser_(name_, argc, argv, request, out)) {
      return;
    }

    bool has_rendered_provider = false;
    for (ProviderRequirements* provider : providers_) {
      if (provider == nullptr) {
        continue;
      }

      if (has_rendered_provider) {
        out.Write("\r\n");
      }

      out.Write('[');
      out.Write(provider->Category());
      out.Write("]\r\n");

      detail::ShellStatsVisitor visitor(out);
      const midismith::stats::StatsPublishStatus status = provider->ProvideStats(request, visitor);
      if (status == midismith::stats::StatsPublishStatus::kUnavailable) {
        out.Write("  error: unavailable\r\n");
      } else if (status == midismith::stats::StatsPublishStatus::kPartial) {
        out.Write("  partial: true\r\n");
      }

      has_rendered_provider = true;
    }
  }

 private:
  std::string_view name_;
  std::string_view help_;
  std::span<ProviderRequirements*, kProviderCount> providers_;
  RequestParserT request_parser_;
};

}  // namespace midismith::shell_cmd_stats
