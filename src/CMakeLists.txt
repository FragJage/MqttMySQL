#Activer le support c++11 
if(CMAKE_VERSION VERSION_LESS "3.1")
  set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
else ()
  set(CMAKE_CXX_STANDARD 11)
endif ()

#Librairie PTHREAD 
if(CMAKE_VERSION VERSION_GREATER "3.0")
  set(THREADS_PREFER_PTHREAD_FLAG ON)
endif ()
find_package(Threads)

find_library(LIBPAHO paho-mqtt3a)
add_library(paho-mqtt3a SHARED IMPORTED)
set_target_properties(paho-mqtt3a PROPERTIES
  IMPORTED_LOCATION "${LIBPAHO}"
)

find_library(LIBPAHOPP paho-mqttpp3)
add_library(paho-mqttpp3 SHARED IMPORTED)
set_target_properties(paho-mqttpp3 PROPERTIES
  IMPORTED_LOCATION "${LIBPAHOPP}"
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W -Wshadow -Wunused-variable -Wunused-parameter -Wunused-function -Wunused -Wno-system-headers -Wno-deprecated") # various warning flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s -O2") # no debug, active optimisation

include_directories(../dependencies/FragFramework/src)
include_directories(../dependencies/MqttDaemon/src)
include_directories(../dependencies/ServiceSkeleton/src)
include_directories(../dependencies/SimpleFolder/src)
include_directories(../dependencies/SimpleIni/include)
include_directories(../dependencies/SimpleLog/src)
 
#Génération de la liste des fichiers sources
set(
  FragFramework_files
  "../dependencies/FragFramework/src/Plateforms.cpp"
  "../dependencies/FragFramework/src/StringTools.cpp"
)

set(
  MqttDaemon_files
  "../dependencies/MqttDaemon/src/MqttBase.cpp"
  "../dependencies/MqttDaemon/src/MqttLogger.cpp"
  "../dependencies/MqttDaemon/src/MqttDaemon.cpp"
)

file(
  GLOB_RECURSE
  ServiceSkeleton_files
  ../dependencies/ServiceSkeleton/src/*
)

file(
  GLOB_RECURSE
  SimpleFolder_files
  ../dependencies/SimpleFolder/src/*
)

file(
  GLOB_RECURSE
  SimpleIni_files
  ../dependencies/SimpleIni/src/*
)

file(
  GLOB_RECURSE
  SimpleLog_files
  ../dependencies/SimpleLog/src/*
)

find_package(MySQL)
include_directories(${MYSQL_INCLUDE_DIR})
link_directories(${MYSQL_ADD_LIBRARY})

#Déclaration de l'exécutable
set(EXECUTABLE_OUTPUT_PATH bin/${CMAKE_BUILD_TYPE})
add_executable(
  MqttMySQL
  ${FragFramework_files}
  ${MqttDaemon_files}
  ${ServiceSkeleton_files}
  ${SimpleFolder_files}
  ${SimpleIni_files}
  ${SimpleLog_files}
  DbMysql.cpp
  MqttBridge.cpp
  MqttMySQL.cpp
  main.cpp
)

target_link_libraries(MqttMySQL paho-mqtt3a)
target_link_libraries(MqttMySQL paho-mqttpp3)

target_link_libraries(MqttMySQL ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(MqttMySQL ${MYSQL_LIBRARIES})

#Spécification WIN32
if(WIN32)
  target_link_libraries(MqttMySQL ws2_32)
endif()
