cmake_minimum_required(VERSION 3.20)

set(LIBDAISY_DIR src/thirdparty/libDaisy)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/${LIBDAISY_DIR}/cmake/toolchains/stm32h750xx.cmake)

set(TARGET "Terrarium-TapeDelay")

project(${TARGET})

add_executable(${TARGET} src/main.cpp)

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED On
    )

target_include_directories(${TARGET} PRIVATE 
                            src
                            src/thirdparty)

add_subdirectory(${LIBDAISY_DIR})
add_subdirectory(src/thirdparty/DaisySP)
add_subdirectory(src/mbdsp)

add_dependencies(${TARGET} DaisySP daisy mbdsp)

target_link_libraries(${TARGET} PRIVATE 
                        DaisySP 
                        daisy 
                        mbdsp
                    )

option(PGM_DEVICE interface/stlink.cfg)
option(CHIPSET stm32h7x)
add_custom_target(program openocd -f ${PGM_DEVICE} -f target/${CHIPSET}.cfg)