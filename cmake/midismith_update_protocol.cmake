include_guard(GLOBAL)

# Bumped when a CAN protocol change makes older peers unable to talk to a new image. Every
# packaged container declares it, and a target refuses an image that demands more than it speaks.
# Once libs/protocol owns a protocol version constant, this must be derived from it rather than
# restated here.
set(MIDISMITH_MIN_COMPATIBLE_PROTOCOL_VERSION 0)
