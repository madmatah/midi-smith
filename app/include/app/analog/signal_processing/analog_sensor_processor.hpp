#pragma once

#include "app/analog/signal_processing/analog_sensor_processor_workflow.hpp"

namespace app::analog::signal_processing {

class AnalogSensorProcessor {
 public:
  using LinearizerConfiguration = workflow::LinearizerConfiguration;
  using KeyActionHandler = workflow::KeyActionHandler;

  void Reset() noexcept {
    workflow_.Reset();
  }

  template <typename ContextT>
  float Transform(float input, const ContextT& context) noexcept {
    return workflow_.Transform(input, context);
  }

  void SetLinearizerConfiguration(const LinearizerConfiguration* configuration) noexcept {
    WorkflowControlSurface::SetLinearizerConfiguration(workflow_, configuration);
  }

  void SetTelemetryCapture(app::telemetry::SensorRttStreamCapture* capture) noexcept {
    WorkflowControlSurface::SetTelemetryCapture(workflow_, capture);
  }

  void SetNoteOnKeyActionHandler(KeyActionHandler* handler) noexcept {
    WorkflowControlSurface::SetNoteOnKeyActionHandler(workflow_, handler);
  }

  void SetNoteOffKeyActionHandler(KeyActionHandler* handler) noexcept {
    WorkflowControlSurface::SetNoteOffKeyActionHandler(workflow_, handler);
  }

 private:
  using Workflow = workflow::ProcessorWorkflow;
  using WorkflowControlSurface = workflow::ControlSurface;

  Workflow workflow_{};
};

}  // namespace app::analog::signal_processing
