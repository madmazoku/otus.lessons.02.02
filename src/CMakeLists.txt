cmake_minimum_required(VERSION 3.2)

project(otus_lessons_02_02 VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

configure_file(in.version.h version.h)

add_executable(otus_lessons_02_02_main_cli main.cpp)

add_executable(otus_lessons_02_02_test_cli test.cpp)

set_target_properties(otus_lessons_02_02_main_cli otus_lessons_02_02_test_cli PROPERTIES
	CXX_STANDARD 14
	CXX_STANDARD_REQUIRED ON
	COMPILE_OPTIONS -Wpedantic -Wall -Wextra
)

set_target_properties(otus_lessons_02_02_test_cli PROPERTIES
	COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK
	INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(otus_lessons_02_02_main_cli
	pthread
	${SDL2_LIBRARIES}
)

target_link_libraries(otus_lessons_02_02_test_cli
	pthread
	${Boost_LIBRARIES}
)

install(TARGETS otus_lessons_02_02_main_cli RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT madmazoku@gmail.com)

include(CPack)

enable_testing()

add_test(otus_lessons_02_02_test otus_lessons_02_02_test_cli)