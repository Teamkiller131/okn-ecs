cmake_minimum_required(VERSION 3.20)

set(SAMPLES_TARGET okn-ecs_sample)

add_executable(${SAMPLES_TARGET} sample_basic.cpp)
target_link_libraries(${SAMPLES_TARGET} PRIVATE okn-ecs okn-core)
target_compile_features(${SAMPLES_TARGET} PRIVATE cxx_std_23)
