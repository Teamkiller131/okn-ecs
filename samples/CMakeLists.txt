cmake_minimum_required(VERSION 3.20)

set(SAMPLES_TARGET okn-ecs_samples)

file(GLOB_RECURSE ${SAMPLES_TARGET}_SRC
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

if(${SAMPLES_TARGET}_SRC)
    add_executable(${SAMPLES_TARGET} ${${SAMPLES_TARGET}_SRC})
    target_link_libraries(${SAMPLES_TARGET} PRIVATE okn-ecs)
    target_compile_features(${SAMPLES_TARGET} PRIVATE cxx_std_23)
endif()