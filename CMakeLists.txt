cmake_minimum_required (VERSION 2.8)

# Pour trouver le package FindMySQL.cmake
SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR} )

#Déclaration du projet
project (MqttMySQL)

add_subdirectory(src)
