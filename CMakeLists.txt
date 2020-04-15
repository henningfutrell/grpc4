project(hw4)
cmake_minimum_required(VERSION 3.0)

add_compile_options(-std=c++14)

set(CLIENT_EXE fbc)
set(SERVER_EXE fbsd)
set(ROUTER_EXE router)
set(SLAVE_EXE slave)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)
find_package(Threads REQUIRED)

set(PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/protos)
set(PROTOC_GENERATED ${CMAKE_CURRENT_BINARY_DIR}/protoc-generated)
file(MAKE_DIRECTORY ${PROTOC_GENERATED})

include_directories(${PROTOC_GENERATED})

add_custom_target(
        generate_rpc_files ALL
        COMMAND protoc -I ${PROTO_FILES} --grpc_out=${PROTOC_GENERATED} --plugin=protoc-gen-grpc=`which
        grpc_cpp_plugin` timeline.proto router.proto
        COMMAND protoc -I ${PROTO_FILES} --cpp_out=${PROTOC_GENERATED} timeline.proto router.proto
)

aux_source_directory(${PROTOC_GENERATED} PROTOC_GENERATED_CC_FILES)

set(HEADERS stringmanipulation.h)

add_executable(${SERVER_EXE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tsd.cc
        ${PROTOC_GENERATED_CC_FILES}
        ${HEADERS}
        )
target_link_libraries(${SERVER_EXE}
        ${PROTOBUF_LIBRARIES}
        gRPC::grpc++_reflection
        stdc++fs
        )

add_executable(${CLIENT_EXE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tsc.cc
        ${PROTOC_GENERATED_CC_FILES}
        ${HEADERS}
        )
target_link_libraries(${CLIENT_EXE}
        ${PROTOBUF_LIBRARIES}
        gRPC::grpc++_reflection
        )

add_executable(${ROUTER_EXE}
        ${CMAKE_CURRENT_SOURCE_DIR}/router.cc
        ${PROTOC_GENERATED_CC_FILES}
        ${HEADERS}
        )
target_link_libraries(${ROUTER_EXE}
        ${PROTOBUF_LIBRARIES}
        gRPC::grpc++_reflection
        )

add_executable(${SLAVE_EXE}
        ${CMAKE_CURRENT_SOURCE_DIR}/slave.cc
        ${PROTOC_GENERATED_CC_FILES}
        ${HEADERS}
        )
target_link_libraries(${SLAVE_EXE}
        ${PROTOBUF_LIBRARIES}
        gRPC::grpc++_reflection
        )
