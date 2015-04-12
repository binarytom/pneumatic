cmake_minimum_required(VERSION 2.8)
PROJECT(pneumatic)
add_definitions(-std=gnu++11)
#set(Boost_USE_STATIC_LIBS        ON) # only find static libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
set(BOOST_INCLUDEDIR "/usr/include")
set(BOOST_LIBRARYDIR "/usr/lib/x86_64-linux-gnu")
set(BOOST_ROOT "/home/tom/local")
file(STRINGS source.files source_files)
add_definitions(-DBOOST_LOG_DYN_LINK)
find_package(Boost COMPONENTS date_time filesystem system regex signals thread log program_options iostreams)
find_package(Threads REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  add_executable(pneumatic ${source_files})
  target_link_libraries(
	  pneumatic
	  ${CMAKE_THREAD_LIBS_INIT}
	  ${Boost_FILESYSTEM_LIBRARY}
	  ${Boost_SYSTEM_LIBRARY}
	  ${Boost_REGEX_LIBRARY}
	  ${Boost_LIBRARIES}
	  ${Boost_LOG_LIBRARY}
	  ${Boost_THREAD_LIBRARY}
  )
endif()

