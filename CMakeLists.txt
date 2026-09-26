cmake_minimum_required(VERSION 3.25)
# 定义项目。
# storage：项目名称。
# VERSION 0.0.0：当前项目版本。
# LANGUAGES CXX：项目只启用 C++ 编译语言。
project(storage VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 定义一个布尔配置项 STORAGE_ENABLE_SANITIZERS。
# 作用：决定是否启用 ASan + UBSan。
# 默认值 OFF，普通 Debug/Release 构建不会开启 Sanitizer。
option(STORAGE_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(STORAGE_BUILD_TESTS "Build tests" ON)


# 加载我们自己写的 CMake 文件。
# 文件位置：
# cmake/project_options.cmake
include(cmake/project_options.cmake)

# 查找 Protobuf。
#
# CONFIG：
# 使用 Protobuf 官方提供的 CMake package 配置文件寻找依赖。
#
# REQUIRED：
# 如果找不到 Protobuf，则 CMake 配置直接失败。
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(blake3 CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

# 定义一个列表 PROTO_FILES。
# 保存本项目需要编译的所有 protobuf 协议文件。
set(PROTO_FILES proto/common.proto proto/control.proto proto/worker.proto)

# CMAKE_CURRENT_BINARY_DIR：
# 当前 CMake 构建目录。
#
# 假设：
# build/debug
#
# 那么最终目录大概是：
# build/debug/generated
set(PROTO_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
# 创建 generated 目录。
# 如果目录已经存在，不会报错。
file(MAKE_DIRECTORY "${PROTO_OUTPUT_DIR}")

# 创建一个空列表，
# 后续保存 protoc 生成的 *.pb.cc 文件。
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
target_link_libraries(storage_common PUBLIC spdlog::spdlog CLI11::CLI11)
storage_set_project_options(storage_common)

add_library(storage_data_plane
    src/storage/worker/file_chunk_store.cpp
    src/storage/worker/data_plane_service.cpp
    src/storage/client/data_plane_client.cpp
    src/storage/client/storage_client.cpp)
target_include_directories(storage_data_plane PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(storage_data_plane PUBLIC storage_common storage_proto BLAKE3::blake3)
storage_set_project_options(storage_data_plane)

add_library(storage_rpc src/rpc/services.cpp)
target_include_directories(storage_rpc PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(storage_rpc PUBLIC storage_common storage_proto storage_data_plane)
storage_set_project_options(storage_rpc)

add_executable(control-plane src/control/control_main.cpp)
target_link_libraries(control-plane PRIVATE storage_rpc)
storage_set_project_options(control-plane)

add_executable(worker src/worker/worker_main.cpp)
target_link_libraries(worker PRIVATE storage_rpc)
storage_set_project_options(worker)

add_executable(client src/client/client_main.cpp)
target_link_libraries(client PRIVATE storage_rpc CLI11::CLI11)
storage_set_project_options(client)

if(STORAGE_BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_executable(storage_unit_tests
        tests/unit/status_test.cpp
        tests/unit/status_or_test.cpp
        tests/unit/unique_fd_test.cpp
        tests/unit/byte_range_test.cpp
        tests/unit/checksum_test.cpp
        tests/unit/storage/worker/file_chunk_store_test.cpp)
    target_link_libraries(storage_unit_tests PRIVATE storage_data_plane GTest::gtest_main)
    storage_set_project_options(storage_unit_tests)

    add_executable(rpc_smoke_test tests/integration/rpc_smoke_test.cpp)
    target_link_libraries(rpc_smoke_test PRIVATE storage_rpc GTest::gtest_main)
    storage_set_project_options(rpc_smoke_test)

    add_executable(single_worker_put_get_test
        tests/integration/storage/single_worker_put_get_test.cpp)
    target_link_libraries(single_worker_put_get_test PRIVATE storage_rpc GTest::gtest_main)
    storage_set_project_options(single_worker_put_get_test)

    include(GoogleTest)
    gtest_discover_tests(storage_unit_tests)
    gtest_discover_tests(rpc_smoke_test)
    gtest_discover_tests(single_worker_put_get_test)
    add_test(
        NAME SingleWorkerCliE2E
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/e2e/single_worker_cli_test.sh"
                $<TARGET_FILE:worker> $<TARGET_FILE:client>)
    set_tests_properties(SingleWorkerCliE2E PROPERTIES TIMEOUT 60)
endif()
