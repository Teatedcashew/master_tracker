cmake_minimum_required(VERSION 3.20)

# Shareable builds (-DSTANDALONE_BUILD=ON) on Windows use the static MSVC runtime,
# so friends don't need to install the Visual C++ Redistributable.
# Must be set before project() so the downloaded libraries pick it up too.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(Master_Tracker VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OFF (default): use the libraries you installed (Homebrew on the Mac). Fast, for everyday work.
# ON: download FTXUI, cpr and nlohmann_json and build them into the app, so the
#     executable runs on machines that don't have them. Used for releases.
option(STANDALONE_BUILD "Download and statically link FTXUI, cpr and nlohmann_json" OFF)

if(STANDALONE_BUILD)
  include(FetchContent)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

  # FTXUI
  set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
    GIT_TAG v7.0.3
    GIT_SHALLOW TRUE)

  # nlohmann_json
  FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

  # cpr (HTTP). Windows: build curl into the app (uses Windows' own TLS).
  # macOS / Linux: use the system's libcurl, which every Mac and almost every
  # Linux desktop already has.
  if(WIN32)
    set(CPR_USE_SYSTEM_CURL OFF CACHE BOOL "" FORCE)
    # libpsl would need the Meson build tool; the app only talks to fixed URLs.
    set(CPR_CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
    # Keep curl lean so it never goes looking for optional extras.
    foreach(opt CURL_BROTLI CURL_ZSTD USE_NGHTTP2 USE_LIBIDN2 CURL_USE_LIBSSH2 CURL_USE_LIBSSH)
      set(${opt} OFF CACHE BOOL "" FORCE)
    endforeach()
  else()
    set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "" FORCE)
  endif()
  set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(CPR_ENABLE_SSL ON CACHE BOOL "" FORCE)
  FetchContent_Declare(cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.15.1
    GIT_SHALLOW TRUE)

  FetchContent_MakeAvailable(ftxui json cpr)
else()
  find_package(cpr REQUIRED)
  find_package(nlohmann_json REQUIRED)
  find_package(ftxui REQUIRED)
endif()

# Practice programs: only built if the labs/ folder is there (it isn't on GitHub).
foreach(n 1 2 3 4 5)
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/labs/ex_${n}.cpp)
    add_executable(EX_${n} labs/ex_${n}.cpp)
    target_link_libraries(EX_${n} PRIVATE cpr::cpr nlohmann_json::nlohmann_json)
  endif()
endforeach()

add_executable(Master_Tracker_CLI master_tracker_CLI/master_tracker_CLI.cpp)

target_link_libraries(Master_Tracker_CLI
  PRIVATE
  cpr::cpr
  nlohmann_json::nlohmann_json
  ftxui::screen
  ftxui::dom
  ftxui::component
)

# Shareable Linux / MinGW builds: bake in the C++ runtime so it doesn't
# depend on the exact compiler version installed on the other machine.
if(STANDALONE_BUILD AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(WIN32)
    target_link_options(Master_Tracker_CLI PRIVATE -static)
  else()
    target_link_options(Master_Tracker_CLI PRIVATE -static-libstdc++ -static-libgcc)
  endif()
endif()
