
# Define STM32 environment variable or define it manually here:
STM32 := $(subst \,/,${STM32})

TARGET_CHIP = STM32F030F4P6
OUTPUT = wcsm_firmware
DEBUG = 1
OPT = -Og

include ${STM32}/Build/build.mk

build: .build
clean: .clean
flash: .flash
rebuild: clean build

all: build flash
