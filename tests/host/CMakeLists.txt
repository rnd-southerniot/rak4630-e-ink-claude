cmake_minimum_required(VERSION 3.20)
project(senseflow_eink_node_host_tests C)

# Host-native unit tests for the pure C logic of the ESP-IDF (ESP32-S3) firmware. No hardware, no
# ESP-IDF — just the system compiler + ctest, so CI runs them on every push (mirrors careflow).
# Targets the firmware/ tree (the canonical path after the P6 pio/ consolidation), NOT any pio/ tree.

enable_testing()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror)

set(REPO ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(DP_DIR ${REPO}/firmware/components/device_profile)
set(MAIN_DIR ${REPO}/firmware/main)

# ADR-006 multi-bus (v2) device-profile core: dp_serialize/deserialize roundtrip, dp_encode_payload
# exact bytes (the ADR-005 BME280 payload proven on-hardware), dp_simulate determinism.
add_executable(device_profile_test device_profile_test.c ${DP_DIR}/device_profile.c)
target_include_directories(device_profile_test PRIVATE ${DP_DIR}/include)
target_link_libraries(device_profile_test m)
add_test(NAME device_profile COMMAND device_profile_test)

# Legacy gate-ID translation map (bring-up harness).
add_executable(gate_id_map_test gate_id_map_test.c ${MAIN_DIR}/gate_id_map.c)
target_include_directories(gate_id_map_test PRIVATE ${MAIN_DIR})
add_test(NAME gate_id_map COMMAND gate_id_map_test)
