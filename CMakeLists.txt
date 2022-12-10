
cmake_minimum_required(VERSION 3.23)

project(Ad 
    VERSION 0.0.1
    LANGUAGES CXX
)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8;/permissive->")
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/W4;/w14242;/w14254;/w14263;/w14265;/w14287;/we4289;/w14296;/w14311;/w14545;/w14546>")
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/w14547;/w14549;/w14555;/w14619;/w14640;/w14826;/w14905;/w14906;/w14928>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8;/permissive->")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/W4;/w14242;/w14254;/w14263;/w14265;/w14287;/we4289;/w14296;/w14311;/w14545;/w14546>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/w14547;/w14549;/w14555;/w14619;/w14640;/w14826;/w14905;/w14906;/w14928>")

add_compile_options("$<$<C_COMPILER_ID:GNU>:-Wall;-Wextra;-Wshadow;-pedantic;-Wcast-align;-Wunused>")
add_compile_options("$<$<CXX_COMPILER_ID:GNU>:-Wall;-Wextra;-Wshadow;-Wnon-virtual-dtor;-pedantic;-Wcast-align;-Wunused;-Woverloaded-virtual>")
add_compile_options("$<$<CXX_COMPILER_ID:GNU>:-Wconversion;-Wmisleading-indentation;-Wduplicated-cond;-Wduplicated-branches>")
add_compile_options("$<$<CXX_COMPILER_ID:GNU>:-Wlogical-op;-Wnull-dereference;-Wuseless-cast;-Wdouble-promotion;-Wformat=2>")


add_library(common STATIC
    src/core.cpp
)

add_executable(tests
    tests/testMain.cpp
)
target_link_libraries(tests PUBLIC common)

target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_compile_features(common
    PUBLIC cxx_std_23
)

find_package(gsl-lite)
target_link_libraries(common PUBLIC gsl::gsl-lite-v1)

find_package(date)
target_link_libraries(common PUBLIC date::date date::date-tz)

find_package(cereal)
target_link_libraries(common PUBLIC cereal::cereal)

find_package(Catch2)
target_link_libraries(common PUBLIC Catch2::Catch2)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

find_package(unofficial-sqlite3 CONFIG REQUIRED)
target_link_libraries(common PUBLIC unofficial::sqlite3::sqlite3)

find_package(fmt)
target_link_libraries(common PUBLIC fmt::fmt)

find_package(nlohmann_json)
target_link_libraries(common PUBLIC nlohmann_json::nlohmann_json)

find_package(tl-optional REQUIRED)
target_link_libraries(common PUBLIC tl::optional)

find_package(tl-expected REQUIRED)
target_link_libraries(common PUBLIC tl::expected)

find_package(clipp REQUIRED)
target_link_libraries(common PUBLIC clipp::clipp)


add_executable(day1
    day1/main.cpp
)
target_link_libraries(day1 PUBLIC common)

add_executable(day2
    day2/main.cpp
)
target_link_libraries(day2 PUBLIC common)

add_executable(day3
    day3/main.cpp
)
target_link_libraries(day3 PUBLIC common)

add_executable(day4
    day4/day4.cpp
)
target_link_libraries(day4 PUBLIC common)

add_executable(day5
    day5/day5.cpp
)
target_link_libraries(day5 PUBLIC common)

add_executable(day6
    day6/day6.cpp
)
target_link_libraries(day6 PUBLIC common)

add_executable(day7
    day7/day7.cpp
)
target_link_libraries(day7 PUBLIC common)

add_executable(day8
    day8/day8.cpp
)
target_link_libraries(day8 PUBLIC common)

add_executable(day9
    day9/day9.cpp
)
target_link_libraries(day9 PUBLIC common)

add_executable(day10
    day10/day10.cpp
)
target_link_libraries(day10 PUBLIC common)
