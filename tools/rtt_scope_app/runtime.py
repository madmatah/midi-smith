import struct
import traceback

from tools.rtt_common.sensor_rtt_protocol import KIND_DATA


def build_update_callback(scope, client, decoder, data_format):
    def update_callback(_event):
        try:
            processed = False

            if data_format == "sensor_frame":
                raw_data = client.receive_aligned_data(frame_size_bytes=1)
                if raw_data:
                    frames = decoder.feed(raw_data)
                    if decoder.latest_schema is not None:
                        scope.configure_metrics(decoder.latest_schema)

                    if scope.metric_names:
                        seqs = []
                        timestamps_us = []
                        sensor_ids = []
                        values_matrix = []

                        for frame in frames:
                            if frame.kind != KIND_DATA:
                                continue
                            values_by_name = frame.values_by_name or {}
                            values_matrix.append(
                                [values_by_name.get(name, 0.0) for name in scope.metric_names]
                            )
                            seqs.append(frame.seq)
                            timestamps_us.append(frame.timestamp_us)
                            sensor_ids.append(frame.sensor_id or 0)

                        if values_matrix:
                            scope.process_incoming_sensor_frames(
                                seqs,
                                timestamps_us,
                                sensor_ids,
                                values_matrix,
                            )
                            processed = True
            else:
                raw_data = client.receive_aligned_data(frame_size_bytes=4)
                if raw_data:
                    value_count = len(raw_data) // 4
                    if value_count > 0:
                        format_character = "f" if data_format == "float32" else "I"
                        unpacked_values = struct.unpack(
                            "<" + format_character * value_count,
                            raw_data[: value_count * 4],
                        )
                        scope.process_incoming_values(unpacked_values)
                        processed = True

            if not processed or scope.view_offset > 0:
                scope.update_ui_status(client.is_connected, client.total_bytes_received)
            elif processed:
                scope.update_ui_status(client.is_connected, client.total_bytes_received)

        except Exception as error:
            print(f"Error in update loop: {error}")
            traceback.print_exc()

    return update_callback
