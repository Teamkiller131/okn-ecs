cmake_minimum_required(VERSION 3.20)
project(okn-ecs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    add_compile_options(/FS)
endif()

option(BUILD_SAMPLES "Build samples" ON)
option(BUILD_TESTS   "Build tests"   ON)

# Feature toggles
option(OKN_ECS_ENABLE_SPARSE          "Enable sparse set storage" ON)
option(OKN_ECS_ENABLE_SNAPSHOT        "Enable snapshot/rollback" ON)
option(OKN_ECS_ENABLE_REFLECTION      "Enable component/system reflection" ON)
option(OKN_ECS_ENABLE_SCRIPTING       "Enable scripting bridge" ON)
option(OKN_ECS_ENABLE_CLI             "Enable CLI tools" ON)
option(OKN_ECS_ENABLE_PROFILE_HOOKS   "Enable profile hooks" ON)
option(OKN_ECS_ENABLE_LOG_HOOKS       "Enable log hooks" ON)
option(OKN_ECS_ENABLE_MEMORY_HOOKS    "Enable memory allocator hooks" ON)

file(GLOB_RECURSE ${PROJECT_NAME}_SRC
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(${PROJECT_NAME} PUBLIC okn-core-interfaces)

# Compile definitions
if(OKN_ECS_ENABLE_SPARSE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC OKN_ECS_ENABLE_SPARSE=1)
endif()
if(OKN_ECS_ENABLE_SNAPSHOT)
    target_compile_definitions(${PROJECT_NAME} PUBLIC OKN_ECS_ENABLE_SNAPSHOT=1)
endif()
if(OKN_ECS_ENABLE_REFLECTION)
    target_compile_definitions(${PROJECT_NAME} PUBLIC OKN_ECS_ENABLE_REFLECTION=1)
endif()
if(OKN_ECS_ENABLE_SCRIPTING)
    target_compile_definitions(${PROJECT_NAME} PUBLIC OKN_ECS_ENABLE_SCRIPTING=1)
endif()
if(OKN_ECS_ENABLE_CLI)
    target_compile_definitions(${PROJECT_NAME} PUBLIC OKN_ECS_ENABLE_CLI=1)
endif()
if(OKN_ECS_ENABLE_PROFILE_HOOKS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC OKN_ECS_ENABLE_PROFILE_HOOKS=1)
endif()
if(OKN_ECS_ENABLE_LOG_HOOKS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC OKN_ECS_ENABLE_LOG_HOOKS=1)
endif()
if(OKN_ECS_ENABLE_MEMORY_HOOKS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC OKN_ECS_ENABLE_MEMORY_HOOKS=1)
endif()

# Samples
if(BUILD_SAMPLES)
    add_subdirectory(samples)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()