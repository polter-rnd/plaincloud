cmake_minimum_required(VERSION 3.23...3.26)

# Setup vcpkg
if(DEFINED ENV{VCPKG_INSTALLATION_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "$ENV{VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING ""
    )
endif()

project(
    "slimlog"
    VERSION 0.1.0
    LANGUAGES CXX
)

# Compiler / language configuration.
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

# Set build type if none specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE
        "Release"
        CACHE STRING "Choose the type of build." FORCE
    )
    # Set the possible values of build type for cmake-gui
    set_property(
        CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
    )
endif()

# build shared option
option(SLIMLOG_BUILD_SHARED "Build shared library" OFF)

# install options
option(SLIMLOG_SYSTEM_INCLUDES "Include as system headers (skip for clang-tidy)." OFF)

# CMake include files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Compilation database is required for static code analysis
set(CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE INTERNAL ""
)

# Specify all standard include directories so that IWYU will able to check them
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

# To generate fancy report of used features
include(FeatureSummary)
include(Helpers)

# Option for enabling / disabling code formatters
option(ENABLE_FORMATTERS "Code formatters" OFF)
add_feature_info("CodeFormatting" ENABLE_FORMATTERS "formatting and checking code style")
if(ENABLE_FORMATTERS)
    set(CLANG_FORMAT_MIN_VERSION 15.0)
    include(CodeFormatting)
    add_code_format_targets(
        CHECK_TARGET formatcheck
        FORMAT_TARGET format
        SOURCE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src
        EXCLUDE_DIRS ${CMAKE_BINARY_DIR}
    )
endif()

# Reset all static analyzers by default
unset(CMAKE_CXX_CPPCHECK CACHE)
unset(CMAKE_CXX_CLANG_TIDY CACHE)
unset(CMAKE_CXX_INCLUDE_WHAT_YOU_USE CACHE)

# Option for enabling / disabling static analyzers
option(ENABLE_ANALYZERS "Static code analyzers" OFF)
add_feature_info("StaticCodeAnalysis" ENABLE_ANALYZERS "code linting and static analysis")
if(ENABLE_ANALYZERS)
    set(CPPCHECK_MIN_VERSION 2.10)
    set(CLANG_TIDY_MIN_VERSION 17.0)
    set(CLANG_FORMAT_MIN_VERSION 17.0)
    set(IWYU_MIN_VERSION 0.23)
    include(StaticCodeAnalysis)
    enable_static_code_analysis(
        CLANG_TIDY_EXTRA_ARGS -header-filter=.*
        IWYU_EXTRA_ARGS --cxx17ns --quoted_includes_first --no_fwd_decls --verbose=3
                        --mapping_file=${CMAKE_CURRENT_SOURCE_DIR}/.iwyu-mappings
        CPPCHECK_EXTRA_ARGS --enable=warning,performance,portability,information,missingInclude
                            --inline-suppr --suppress=missingIncludeSystem --check-level=exhaustive
    )
endif()

# Option for enabling / disabling sanitizers
option(ENABLE_SANITIZERS "Code sanitizers" OFF)
add_feature_info("CodeSanitizing" ENABLE_SANITIZERS "address/leak/UB/thread sanitizers")
if(ENABLE_SANITIZERS)
    include(CodeSanitizing)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Coverage" OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_package_switchable(
        Gcovr
        OPTION ENABLE_COVERAGE
        PURPOSE "Code coverage reports by gcovr"
    )
    add_feature_info("GcovrCodeCoverage" ENABLE_COVERAGE "code test coverage")

    if(ENABLE_COVERAGE)
        include(GcovrCodeCoverage)
        add_gcovr_coverage_target(
            HTML COBERTURA COVERALLS SONARQUBE
            COVERAGE_TARGET coverage
            COVERAGE_INIT_TARGET coverage-clean
        )
    endif()
endif()

# fmtlib is recommended for better formatting performance
set(FMT_OPTION ENABLE_FMTLIB)
set(FMT_PURPOSE "Implementation of C++20 std::format")
if(ENABLE_FMTLIB AND ENABLE_FMTLIB_HO)
    message(FATAL_ERROR "ENABLE_FMTLIB and ENABLE_FMTLIB_HO are mutually exclusive")
elseif(ENABLE_FMTLIB)
    option(ENABLE_FMTLIB_HO "${FMT_PURPOSE} (header-only)" OFF)
else()
    option(ENABLE_FMTLIB ${FMT_PURPOSE} OFF)
    set(FMT_OPTION ENABLE_FMTLIB_HO)
    set(FMT_PURPOSE "${FMT_PURPOSE} (header-only)")
endif()
find_package_switchable(
    fmt
    MIN_VERSION 9.1.0
    OPTION ${FMT_OPTION}
    DEFAULT ON
    PURPOSE ${FMT_PURPOSE}
)

# Include library targets
add_subdirectory(src)

# Option for building documentation
find_package_switchable(
    Doxygen
    OPTION BUILD_DOCS
    DEFAULT OFF
    PURPOSE "Build Doxygen documentation"
)
add_feature_info("DoxygenDocumentation" BUILD_DOCS "build documentation")

if(BUILD_DOCS)
    find_package(Doxygen REQUIRED)

    # Write documentation for CMake files
    include(CMakeDocumentation)
    parse_cmake_documentation(INCLUDES "cmake/*")
    set(cmake_doxygen_file ${PROJECT_BINARY_DIR}/cmake.dox)
    write_cmake_documentation("${cmake_doxygen_file}" SORTED)

    # Create docs target
    set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/docs)
    set(DOXYGEN_PROJECT_NAME "SlimLog")
    doxygen_add_docs(docs ${PROJECT_SOURCE_DIR} COMMENT "Generating API documentation with Doxygen")
endif()

# Summary of enabled and disabled features
feature_summary(WHAT ALL)

# Summary of available options
message(STATUS "Options summary:")
message("")
dump_option_variables("^ENABLE_|^BUILD_|^SANITIZE_|^ANALYZE_|^FORMAT_")
message("")
