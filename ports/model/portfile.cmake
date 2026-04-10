vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL             https://github.com/carbon-os/model
    REF             9b925a59c8b4ce00fa5f30b07b174d94f688632a
    HEAD_REF        main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DMODEL_BUILD_SAMPLES=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    CONFIG_PATH lib/cmake/model
)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")