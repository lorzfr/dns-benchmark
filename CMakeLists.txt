cmake_minimum_required(VERSION 3.16)

project(dnsbenchmark VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
include(GNUInstallDirs)
find_package(Threads REQUIRED)

add_library(dnsbenchmark_lib
    src/benchmark.cpp
    src/cli.cpp
    src/constants.cpp
    src/dns_protocol.cpp
    src/output.cpp
    src/transport_posix.cpp
    src/tui.cpp
)

target_include_directories(dnsbenchmark_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(dnsbenchmark_lib
    PUBLIC
        Threads::Threads
)

add_executable(dnsbenchmark
    src/main.cpp
)

target_link_libraries(dnsbenchmark
    PRIVATE
        dnsbenchmark_lib
)

install(
    TARGETS dnsbenchmark
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    FILES README.md
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/${PROJECT_NAME}
)

if(BUILD_TESTING)
    add_executable(dnsbenchmark_unit_tests
        tests/unit_tests.cpp
    )

    target_link_libraries(dnsbenchmark_unit_tests
        PRIVATE
            dnsbenchmark_lib
            Threads::Threads
    )

    add_test(
        NAME dnsbenchmark_unit_tests
        COMMAND dnsbenchmark_unit_tests
    )

    if(UNIX)
        add_executable(dnsbenchmark_integration_tests
            tests/integration_tests.cpp
        )

        target_link_libraries(dnsbenchmark_integration_tests
            PRIVATE
                dnsbenchmark_lib
                Threads::Threads
        )

        add_test(
            NAME dnsbenchmark_integration_tests
            COMMAND dnsbenchmark_integration_tests
        )
    endif()
endif()

if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VENDOR "lorz")
    set(CPACK_PACKAGE_CONTACT "lorz")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Native Linux DNS resolver benchmark")
    string(CONCAT CPACK_PACKAGE_DESCRIPTION
        "dnsbenchmark is a native C++ DNS resolver benchmark CLI for Linux.\n"
        "It sends raw UDP DNS A-record queries, measures latency and reliability,\n"
        "and can export raw benchmark rows to CSV."
    )
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-linux")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")

    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "lorz")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

    add_custom_target(
        package_deb
        COMMAND "${CMAKE_CPACK_COMMAND}" -G DEB --config "${CMAKE_BINARY_DIR}/CPackConfig.cmake"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Building Debian package"
    )
endif()

include(CPack)
