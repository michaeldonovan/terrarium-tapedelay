# Project Name
TARGET = TapeDelay

# Sources
CPP_SOURCES = src/main.cpp  src/mbdsp/thirdparty/hiir-1.4.0/hiir/PolyphaseIir2Designer.cpp 

# Library Location
LIBDAISY_DIR = src/thirdparty/libDaisy
DAISYSP_DIR = src/thirdparty/DaisySP

#APP_TYPE = BOOT_QSPI

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

C_INCLUDES += -Isrc/thirdparty -Isrc/mbdsp/thirdparty/hiir-1.4.0 -Isrc/mbdsp -Isrc/mbdsp/thirdparty -I$(LIBDAISY_DIR) -I$(DAISYSP_DIR)