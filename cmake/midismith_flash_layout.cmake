include_guard(GLOBAL)

# Single source of truth for the flash map shared by both STM32H743 firmware packages.
# The application is linked here and the .msfw container declares it; the packaging step
# rejects any binary whose ELF disagrees.
set(MIDISMITH_APPLICATION_LOAD_ADDRESS 0x08000000)

# The application slot the field-update mechanism will copy into. Enforced at build time so
# an image can never outgrow the slot it is destined for.
set(MIDISMITH_APPLICATION_SLOT_SIZE_BYTES 393216)

# Bumped when a CAN protocol change makes older peers unable to talk to a new image.
set(MIDISMITH_MIN_COMPATIBLE_PROTOCOL_VERSION 0)
