CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
Project(ledserver)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

find_package( ZLIB REQUIRED )
FIND_PACKAGE(Wt REQUIRED Wt HTTP)
FIND_PACKAGE(ixwebsocket REQUIRED)
#FIND_PACKAGE(ws2811 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(ws2811 REQUIRED IMPORTED_TARGET libws2811)

ADD_DEFINITIONS(-DHPDF_DLL)

ADD_EXECUTABLE(ledserver.wt
   src/main.cpp
   include/main.h
)

file(GLOB SRC_FILES    
    "src/*.cpp"
    "include/*.h"
)

target_sources(ledserver.wt PUBLIC
    ${SRC_FILES}
)

# if ( ZLIB_FOUND )
#     include_directories( ${ZLIB_INCLUDE_DIRS} )
#     target_link_libraries( ledserver.wt ${ZLIB_LIBRARIES} )
# endif( ZLIB_FOUND )

include_directories( ${ZLIB_INCLUDE_DIRS} )
TARGET_LINK_LIBRARIES(ledserver.wt
   ${ZLIB_LIBRARIES}
   ixwebsocket
   ${ZLIB_LIBRARIES};
   PkgConfig::ws2811
   Wt::Wt
   Wt::HTTP
)