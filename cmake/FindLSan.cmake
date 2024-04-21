# [cmake_documentation] FindLSan.cmake
#
# The module defines the following variables:
# @arg __LSan_FOUND__: `TRUE` if the compiler supports leak sanitizer
# [/cmake_documentation]

set(flag_candidates
    # GNU/Clang
    "-g -fsanitize=leak"
    # MSVC uses
    "/fsanitize=leak"
)

include(Helpers)
check_compiler_flags_list("${flag_candidates}" "LeakSanitizer" "LSan")

if(LSan_DETECTED)
    set(LSan_SUPPORTED "LeakSanitizer is supported by compiler")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LSan DEFAULT_MSG LSan_SUPPORTED)
