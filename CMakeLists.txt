cmake_minimum_required(VERSION 3.20)
project(QuickLineups CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)

add_executable(quicklineups
    src/main.cc
    src/controllers/LineupController.cc
    src/controllers/AdminController.cc
    src/services/LineupStore.cc
)

target_link_libraries(quicklineups PRIVATE Drogon::Drogon)

file(COPY ${CMAKE_SOURCE_DIR}/config.json DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/data        DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/static      DESTINATION ${CMAKE_BINARY_DIR})
