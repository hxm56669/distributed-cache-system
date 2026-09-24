cmake_minimum_required(VERSION 3.25)
project(storage VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(STORAGE_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(STORAGE_BUILD_TESTS "Build tests" ON)

include(cmake/project_options.cmake)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

set(PROTO_FILES proto/common.proto proto/control.proto proto/worker.proto)
set(PROTO_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${PROTO_OUTPUT_DIR}")

set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)
foreach(PROTO_FILE IN LISTS PROTO_FILES)
    get_filename_component(PROTO_NAME "${PROTO_FILE}" NAME_WE)
    list(APPEND PROTO_SRCS "${PROTO_OUTPUT_DIR}/${PROTO_NAME}.pb.cc")
    list(APPEND PROTO_HDRS "${PROTO_OUTPUT_DIR}/${PROTO_NAME}.pb.h")
    list(APPEND GRPC_SRCS "${PROTO_OUTPUT_DIR}/${PROTO_NAME}.grpc.pb.cc")
    list(APPEND GRPC_HDRS "${PROTO_OUTPUT_DIR}/${PROTO_NAME}.grpc.pb.h")
    add_custom_command(
        OUTPUT "${PROTO_OUTPUT_DIR}/${PROTO_NAME}.pb.cc"
               "${PROTO_OUTPUT_DIR}/${PROTO_NAME}.pb.h"
               "${PROTO_OUTPUT_DIR}/${PROTO_NAME}.grpc.pb.cc"
               "${PROTO_OUTPUT_DIR}/${PROTO_NAME}.grpc.pb.h"
        COMMAND protobuf::protoc
        ARGS --cpp_out=${PROTO_OUTPUT_DIR}
             --grpc_out=${PROTO_OUTPUT_DIR}
             --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
             -I "${CMAKE_CURRENT_SOURCE_DIR}/proto"
             "${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}"
        VERBATIM)
endforeach()

add_library(storage_proto ${PROTO_SRCS} ${GRPC_SRCS})
target_include_directories(storage_proto PUBLIC "${PROTO_OUTPUT_DIR}")
target_link_libraries(storage_proto PUBLIC protobuf::libprotobuf gRPC::grpc++)
storage_set_project_options(storage_proto)

add_library(storage_common
    src/common/status.cpp
    src/common/logging.cpp
    src/common/config.cpp
    src/storage/types/byte_range.cpp
    src/storage/types/checksum.cpp)
target_include_directories(storage_common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(storage_common PUBLIC spdlog::spdlog)
storage_set_project_options(storage_common)

add_library(storage_rpc src/rpc/services.cpp)
target_include_directories(storage_rpc PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(storage_rpc PUBLIC storage_common storage_proto)
storage_set_project_options(storage_rpc)

add_executable(control-plane src/control/control_main.cpp)
target_link_libraries(control-plane PRIVATE storage_rpc)
storage_set_project_options(control-plane)

add_executable(worker src/worker/worker_main.cpp)
target_link_libraries(worker PRIVATE storage_rpc)
storage_set_project_options(worker)

add_executable(client src/client/client_main.cpp)
target_link_libraries(client PRIVATE storage_rpc)
storage_set_project_options(client)

if(STORAGE_BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_executable(storage_unit_tests
        tests/unit/status_test.cpp
        tests/unit/status_or_test.cpp
        tests/unit/unique_fd_test.cpp
        tests/unit/byte_range_test.cpp
        tests/unit/checksum_test.cpp)
    target_link_libraries(storage_unit_tests PRIVATE storage_common GTest::gtest_main)
    storage_set_project_options(storage_unit_tests)

    add_executable(rpc_smoke_test tests/integration/rpc_smoke_test.cpp)
    target_link_libraries(rpc_smoke_test PRIVATE storage_rpc GTest::gtest_main)
    storage_set_project_options(rpc_smoke_test)

    include(GoogleTest)
    gtest_discover_tests(storage_unit_tests)
    gtest_discover_tests(rpc_smoke_test)
endif()
