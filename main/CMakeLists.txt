cmake_minimum_required(VERSION 2.8)
project( DIP )
find_package( OpenCV REQUIRED )
set(MY_DIR ../include)
set(SOURCES ${MY_DIR}/saliencyMap.cpp ${MY_DIR}/saliencyMap.hpp)
set (CMAKE_CXX_STANDARD 11)
include_directories( ${OpenCV_INCLUDE_DIRS}  ${MY_DIR})
add_executable( DIP dip.cpp ${SOURCES})
target_link_libraries( DIP ${OpenCV_LIBS} )
#add_definitions(-DPRINT_QUANTIZATION) /* imshow image after quantizstion on the screen */
#add_definitions(-DPRINT_REGION_SALIENCY) /* output region based saliency map in img directory */
#add_definitions(-DPRINT_COLOR_SMOOTH) /* output saliency map after color smooth in img directory */
#add_definitions(-DPRINT_BALANCE) /* output saliency map agter balancing in in img directory */

