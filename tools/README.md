# Midi Smith tools

This directory contains some tools that were useful during the development of the project.

## Overview

- `rtt_dsp`: real-time RTT scope with configurable DSP filters (PyQtGraph UI).
- `rtt_scope`: real-time RTT signal visualizer (VisPy UI).
- `rtt2midi`: bridge RTT note events to MIDI output.
- `adc_freq`: ADC timing/frequency calculator for different sampling cycles.

## Requirements

- `uv` installed (`uv --version`)
- Linux GUI dependencies for Qt/VisPy apps

On Ubuntu, install the Qt xcb dependency once:

```bash
sudo apt update
sudo apt install libxcb-cursor0
```

## rtt_dsp

This tool can be used to visualize sensor metrics at each stage of the DSP workflow.

You need to start the vscode task "Debug adc-board" to enable the RTT channel.

Then, open a terminal on adc-board and activate the RTT metric streaming for a specific sensor :

```
tio /dev/ttyACM0
adc-board> adc on
adc-board> sensor_rtt 2
```

On your computer, launch rtt_dsp :

```bash
./tools/rtt_dsp
```


## rtt_scope

This tool is a scope to visualize sensor metrics.
You might prefer using rtt_dsp that has more features.

```bash
/path/to/midi-smith/tools/rtt_scope --help
```

## rtt2midi

This tool creates a MIDI input device and generate noteOn / noteOff events based on RTT console logs.
It allows to "play notes" with only an ADC board.

Example usage:

```bash
uv run --directory . --only-group rtt-midi --frozen python tools/rtt2midi.py \
  --host 127.0.0.1 --port 60000 --transpose 60 --midi-port "RTT to MIDI"
```

Sensor 1 will be midi note 60, sensor 2 -> 61, sensor 3 -> 62...etc

## adc_freq


```bash
uv run --directory . --frozen python tools/adc_freq.py --help
```

Example usage:

```bash
uv run --directory . --frozen python tools/adc_freq.py --clock 7 --resolution 16 --ranks 8
```
