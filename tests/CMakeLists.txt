cmake_minimum_required(VERSION 3.20)

set(TESTS_TARGET okn-ecs_tests)

file(GLOB_RECURSE ${TESTS_TARGET}_SRC
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

if(${TESTS_TARGET}_SRC)
    add_executable(${TESTS_TARGET} ${${TESTS_TARGET}_SRC})
    target_link_libraries(${TESTS_TARGET} PRIVATE okn-ecs)
    target_compile_features(${TESTS_TARGET} PRIVATE cxx_std_23)
    add_test(NAME ${TESTS_TARGET} COMMAND ${TESTS_TARGET})
endif()