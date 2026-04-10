cmake_minimum_required(VERSION 3.25)
project(model VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies (via vcpkg) ──────────────────────────────────────────────────
find_package(OpenSSL       REQUIRED)
find_package(fetch         REQUIRED)
find_package(nlohmann_json REQUIRED)

# ── Library target ────────────────────────────────────────────────────────────
add_library(model
    src/client.cpp
    src/chat.cpp
    src/history.cpp
    src/session.cpp
    src/providers/openai.cpp
    src/providers/anthropic.cpp
    src/providers/google.cpp
)

add_library(model::model ALIAS model)

target_include_directories(model
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(model
    PUBLIC  nlohmann_json::nlohmann_json
    PRIVATE fetch::fetch
)

# ── Samples ───────────────────────────────────────────────────────────────────
option(MODEL_BUILD_SAMPLES "Build sample programs" ON)
if(MODEL_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS model EXPORT modelTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/model DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT modelTargets
    NAMESPACE   model::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/model
)

configure_package_config_file(
    cmake/model-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/model-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/model
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/model-config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/model
)