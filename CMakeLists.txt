cmake_minimum_required(VERSION 3.10)
project(MyGeant4Simulation VERSION 1.0)
 
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
 
# ——— Find Geant4 —————————————————————————————————————————————————————————
set(GEANT4_CUSTOM_PATH "" CACHE PATH "Path to Geant4 installation (e.g., /path/to/geant4)")
if (GEANT4_CUSTOM_PATH STREQUAL "")
  message(FATAL_ERROR "\nERROR: The path to Geant4 is required.\n\n"
                      "To specify the path, use:\n"
                      "  cmake -DGEANT4_CUSTOM_PATH=/path/to/geant4 ..\n")
endif()
find_package(Geant4 REQUIRED HINTS ${GEANT4_CUSTOM_PATH})
include(${Geant4_USE_FILE})
 
# ——— Includes & sources ————————————————————————————————————————————————————
include_directories(${CMAKE_SOURCE_DIR}/include)
set(SOURCES
  ${PROJECT_SOURCE_DIR}/src/main.cc
  ${PROJECT_SOURCE_DIR}/src/MyPrimaryGeneratorAction.cc
  ${PROJECT_SOURCE_DIR}/src/MyDetectorConstruction.cc
  ${PROJECT_SOURCE_DIR}/src/MyEventAction.cc
  ${PROJECT_SOURCE_DIR}/src/MyRunAction.cc
  ${PROJECT_SOURCE_DIR}/src/MySteppingAction.cc
)
 
add_executable(sim ${SOURCES})
target_link_libraries(sim ${Geant4_LIBRARIES})

 
# ——— paths.config for shell‐wrapper —————————————————————————————————————————————
set(CONFIG_FILE "${CMAKE_BINARY_DIR}/paths.config")
file(WRITE  ${CONFIG_FILE}
  "export CGMF_EXECUTABLE=${CGMF_EXECUTABLE}\n"
)
file(APPEND ${CONFIG_FILE}
  "export GEANT4_BUILD_PATH=${CMAKE_BINARY_DIR}\n"
  "export ROOT_BUILD_PATH=$ENV{ROOTSYS}\n"
)
message(STATUS "Configuration paths saved to ${CONFIG_FILE}")
