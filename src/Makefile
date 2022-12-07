# Project Name
TARGET = TapeDelay

# Sources
CPP_SOURCES = main.cpp CHOWTape/HysteresisProcessing.cpp hiir-1.4.0/hiir/PolyphaseIir2Designer.cpp 

# Library Locations
LIBDAISY_DIR = ./libDaisy
DAISYSP_DIR = ./DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

C_INCLUDES += -ITerrarium -Ihiir-1.4.0 