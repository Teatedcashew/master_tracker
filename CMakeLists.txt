cmake_minimum_required(VERSION 3.14)
project(Master_Tracker)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(cpr REQUIRED)

find_package(nlohmann_json REQUIRED)

find_package(ftxui REQUIRED)

add_executable(EX_1 labs/ex_1.cpp)

add_executable(EX_2 labs/ex_2.cpp)

add_executable(EX_3 labs/ex_3.cpp)

add_executable(EX_4 labs/ex_4.cpp)

add_executable(EX_5 labs/ex_5.cpp)

add_executable(Master_Tracker_CLI master_tracker_CLI/master_tracker_CLI.cpp)

target_link_libraries(EX_1
  PRIVATE

  cpr::cpr
  nlohmann_json::nlohmann_json
)

target_link_libraries(EX_2
  PRIVATE

  cpr::cpr
  nlohmann_json::nlohmann_json
)

target_link_libraries(EX_3
  PRIVATE

  cpr::cpr
  nlohmann_json::nlohmann_json
)

target_link_libraries(EX_4
  PRIVATE

  cpr::cpr
  nlohmann_json::nlohmann_json
)

target_link_libraries(EX_5
  PRIVATE

  cpr::cpr
  nlohmann_json::nlohmann_json
)

target_link_libraries(EX_5
  PRIVATE

  cpr::cpr
  nlohmann_json::nlohmann_json
)

target_link_libraries(Master_Tracker_CLI
  PRIVATE

  cpr::cpr
  nlohmann_json::nlohmann_json

  ftxui::screen
  ftxui::dom
  ftxui::component
)
