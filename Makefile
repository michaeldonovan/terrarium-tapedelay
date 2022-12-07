# Project Name
TARGET = TapeDelay

# Sources
CPP_SOURCES = src/main.cpp src/thirdparty/CHOWTape/HysteresisProcessing.cpp src/thirdparty/hiir-1.4.0/hiir/PolyphaseIir2Designer.cpp 

# Library Locations
LIBDAISY_DIR = src/thirdparty/libDaisy
DAISYSP_DIR = src/thirdparty/DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

C_INCLUDES += -Isrc/thirdparty -Isrc/thirdparty/Terrarium -Isrc/thirdparty/hiir-1.4.0 