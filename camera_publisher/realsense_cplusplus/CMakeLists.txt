cmake_minimum_required(VERSION 2.8.3)


ADD_DEFINITIONS(
    -std=c++11 # Or -std=c++0x
    # Other flags
)

project(camera_publisher)

find_package(OpenCV 4.1 REQUIRED)

find_package(PkgConfig)
pkg_check_modules(RS realsense2)

include_directories(
  ${OpenCV_INCLUDES}
  ${RS_INCLUDE_DIRS}
)

add_executable(camera_publisher main.cpp)
add_executable(camera_publisher_refactored main_refactored.cpp)

target_link_libraries(camera_publisher
  ${OpenCV_LIBRARIES}
  ${RS_LIBRARIES}
  pthread
)

target_link_libraries(camera_publisher_refactored
  ${OpenCV_LIBRARIES}
  ${RS_LIBRARIES}
  pthread
)

