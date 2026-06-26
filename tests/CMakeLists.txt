cmake_minimum_required(VERSION 3.20)

set(TESTS_TARGET okn-ecs_tests)

find_package(doctest CONFIG QUIET)
if(NOT doctest_FOUND)
    set(doctest_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows/share/doctest")
    find_package(doctest CONFIG QUIET)
endif()

file(GLOB_RECURSE ${TESTS_TARGET}_SRC
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

if(${TESTS_TARGET}_SRC)
    add_executable(${TESTS_TARGET} ${${TESTS_TARGET}_SRC})
    target_link_libraries(${TESTS_TARGET} PRIVATE okn-ecs doctest::doctest)
    target_compile_features(${TESTS_TARGET} PRIVATE cxx_std_23)
    add_test(NAME ${TESTS_TARGET} COMMAND ${TESTS_TARGET})
endif()