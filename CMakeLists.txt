cmake_minimum_required(VERSION 3.25)

project(Tests VERSION 0.1.0 LANGUAGES CXX)

# =================================================== #
# ==================== НАСТРОЙКА ==================== #
# =================================================== #

set(EXECUTABLE_NAME ${PROJECT_NAME})

set(CPP_FILES
    src/main.cpp
    # src/main_tests.cpp
)
set(INCLUDE_DIRRECTORIES
    src/
    extern/
)

# =================================================== #
# ============ ПОДКЛЮЧЕНИЕ ЗАВИСИМОСТЕЙ ============= #
# =================================================== #

include(cmake/FindBoost.cmake)
find_package(Threads REQUIRED)
find_package(TgBot REQUIRED)

# =================================================== #
# =============== СОЗДАНИЕ БИНАРНИКОВ =============== #
# =================================================== #

add_executable(${EXECUTABLE_NAME} ${CPP_FILES})

# =================================================== #
# ================= TARGET НАСТРОЙКА ================ #
# =================================================== #

set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(${EXECUTABLE_NAME} PUBLIC
    Boost::headers
    Threads::Threads
    TgBot::TgBot
)

target_include_directories(${EXECUTABLE_NAME} PUBLIC ${INCLUDE_DIRRECTORIES})

if(MSVC)
    target_compile_options(${EXECUTABLE_NAME} PRIVATE /bigobj)
endif()