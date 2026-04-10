vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL             https://github.com/carbon-os/model
    REF             71f3e9c0936faa7c0b67f8f1ca84c579af750c25
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

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")