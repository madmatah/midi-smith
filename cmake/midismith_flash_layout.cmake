include_guard(GLOBAL)

# Update-slot geometry declared to the packaging tool. Both firmware packages carry the same
# STM32H743 and will share one bootloader, so the slot they are installed into is a monorepo
# level fact rather than a per-package one.
#
# The linker scripts are not yet generated from these values; until they are, the packaging step
# is what reconciles them, by reading the ELF and refusing any binary linked elsewhere.
set(MIDISMITH_APPLICATION_LOAD_ADDRESS 0x08000000)

# The application slot the field-update mechanism will copy into. Enforced at build time so an
# image can never outgrow the slot it is destined for.
set(MIDISMITH_APPLICATION_SLOT_SIZE_BYTES 393216)
