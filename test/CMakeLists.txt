find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(Catch2 3 CONFIG QUIET)
if(NOT Catch2_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.10.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()

add_executable(test_baseline test_baseline.cpp test_helpers.cpp)
target_link_libraries(test_baseline PRIVATE decoder JPEG::JPEG PNG::PNG Catch2::Catch2WithMain)
