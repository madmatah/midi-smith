import socket
import mido
import argparse

# Constants for default values
DEFAULT_RTT_HOST = '127.0.0.1'
DEFAULT_RTT_PORT = 60000
DEFAULT_MIDI_PORT_NAME = 'RTT to MIDI'
DEFAULT_TRANSPOSE_SEMITONES = 60
RTT_RECEIVE_BUFFER_SIZE_BYTES = 1024


def open_midi_output_port(port_name):
    """Opens a MIDI output port, attempting to create a virtual one if available."""
    try:
        return mido.open_output(port_name, virtual=True)
    except NotImplementedError:
        return mido.open_output(port_name)


def connect_to_rtt_server(host, port):
    """Establishes a TCP connection to the RTT server."""
    rtt_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    rtt_socket.connect((host, port))
    print(f"Connected to RTT on {host}:{port}")
    return rtt_socket


def convert_rtt_stream_to_midi(rtt_socket, midi_output_port, transpose_offset_semitones):
    """Receives data from RTT and sends corresponding MIDI messages."""
    receive_buffer = ""
    prefix_to_midi_type = {
        "noteon": "note_on",
        "noteoff": "note_off"
    }

    try:
        while True:
            received_data = rtt_socket.recv(RTT_RECEIVE_BUFFER_SIZE_BYTES).decode('utf-8')
            if not received_data:
                break

            receive_buffer += received_data
            while "\n" in receive_buffer:
                command_line, receive_buffer = receive_buffer.split("\n", 1)
                command_line = command_line.strip()

                for prefix, midi_type in prefix_to_midi_type.items():
                    if command_line.startswith(f"{prefix}:"):
                        _, sensor_id, velocity = command_line.split(":")
                        note_number = transpose_offset_semitones + int(sensor_id)

                        midi_message = mido.Message(midi_type, note=note_number, velocity=int(velocity))
                        midi_output_port.send(midi_message)
                        print(f"{midi_message}")
                        break

    except KeyboardInterrupt:
        print("\nStopping MIDI conversion...")


def main():
    parser = argparse.ArgumentParser(description="Create a virtual MIDI output port and convert RTT sensor data to MIDI messages.")
    parser.add_argument("--host", default=DEFAULT_RTT_HOST, help=f"RTT host address (default: {DEFAULT_RTT_HOST})")
    parser.add_argument("--port", type=int, default=DEFAULT_RTT_PORT, help=f"RTT port (default: {DEFAULT_RTT_PORT})")
    parser.add_argument("--transpose", type=int, default=DEFAULT_TRANSPOSE_SEMITONES, help=f"MIDI transpose offset (default: {DEFAULT_TRANSPOSE_SEMITONES})")
    parser.add_argument("--midi-port", default=DEFAULT_MIDI_PORT_NAME, help=f"MIDI output port name (default: {DEFAULT_MIDI_PORT_NAME})")

    args = parser.parse_args()

    midi_output_port = open_midi_output_port(args.midi_port)
    rtt_socket = None

    try:
        rtt_socket = connect_to_rtt_server(args.host, args.port)
        convert_rtt_stream_to_midi(rtt_socket, midi_output_port, args.transpose)
    except Exception as error:
        print(f"Error: {error}")
    finally:
        if rtt_socket:
            rtt_socket.close()
        midi_output_port.close()
        print("Ports closed.")


if __name__ == "__main__":
    main()
