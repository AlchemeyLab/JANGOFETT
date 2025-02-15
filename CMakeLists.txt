cmake_minimum_required(VERSION 3.10)
project(MyGeant4Simulation VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find Geant4
set(GEANT4_CUSTOM_PATH "" CACHE PATH "Path to Geant4 installation (e.g., /path/to/geant4)")
if (GEANT4_CUSTOM_PATH STREQUAL "")
    message(FATAL_ERROR "\nERROR: The path to Geant4 is required.\n\n"
                        "To specify the path, use the following command:\n"
                        "  cmake -DGEANT4_CUSTOM_PATH=/path/to/geant4 ..\n")
endif()

find_package(Geant4 REQUIRED HINTS ${GEANT4_CUSTOM_PATH})
if (NOT Geant4_FOUND)
    message(FATAL_ERROR "\nERROR: Geant4 not found in the specified path: ${GEANT4_CUSTOM_PATH}.\n"
                        "Please ensure you provided the correct Geant4 installation path.")
endif()
include(${Geant4_USE_FILE})

# Add include directories for headers
include_directories(${CMAKE_SOURCE_DIR}/include)


# Add your source files here
set(SOURCES
    ${PROJECT_SOURCE_DIR}/src/main.cc
    ${PROJECT_SOURCE_DIR}/src/MyPrimaryGeneratorAction.cc
    ${PROJECT_SOURCE_DIR}/src/MyDetectorConstruction.cc
    ${PROJECT_SOURCE_DIR}/src/MyEventAction.cc
    ${PROJECT_SOURCE_DIR}/src/MyRunAction.cc
    ${PROJECT_SOURCE_DIR}/src/MySteppingAction.cc
)


# Add the executable
add_executable(sim ${SOURCES})

target_link_libraries(sim ${Geant4_LIBRARIES})

# Ensure DetCon.tg is copied to the build directory after the executable is built
add_custom_command(
    TARGET sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/DetCon.tg"
            "$<TARGET_FILE_DIR:sim>/DetCon.tg"
    COMMENT "Copying DetCon.tg to build directory"
)

# Define where paths.config will be written
set(CONFIG_FILE "${CMAKE_BINARY_DIR}/paths.config")

# Write paths to configuration file
file(WRITE ${CONFIG_FILE} "export CGMF_EXECUTABLE=${CGMF_EXECUTABLE}\n")
file(APPEND ${CONFIG_FILE} "export GEANT4_BUILD_PATH=${CMAKE_BINARY_DIR}\n")  # Use CMAKE_BINARY_DIR for project build directory
file(APPEND ${CONFIG_FILE} "export ROOT_BUILD_PATH=$ENV{ROOTSYS}\n")

message(STATUS "Configuration paths saved to ${CONFIG_FILE}")

