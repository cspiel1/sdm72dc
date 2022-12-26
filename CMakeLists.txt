cmake_minimum_required(VERSION 3.10)

# set the project name
project(sdm72dc)

set(CMAKE_BUILD_TYPE Debug)
set(tar sdm72dc)

find_package(PkgConfig REQUIRED)

pkg_check_modules(MODBUS REQUIRED libmodbus)
pkg_check_modules(OPENSSL openssl)
pkg_check_modules(MOSQUITTO libmosquitto)


# add the executable
add_executable(${tar}
	src/main.c
	src/conf.c
	src/util.c
	src/reg.c
	src/ieee754.c
)


target_link_directories(${tar}
	PUBLIC ${MODBUS_LIBRARY_DIRS}
)

target_include_directories(${tar} AFTER
	PUBLIC ${MODBUS_INCLUDE_DIRS}
)

target_link_libraries(${tar}
	${MODBUS_LIBRARIES}
)

IF (${MOSQUITTO_FOUND} AND ${OPENSSL_FOUND})

	target_sources(${tar} PRIVATE
		src/mqtt.c
	)

target_compile_definitions(${tar} PRIVATE
	USE_MQTT
	)

target_link_directories(${tar}
	PUBLIC ${MOSQUITTO_LIBRARY_DIRS}
	${OPENSSL_LIBRARY_DIRS}
)

target_include_directories(${tar} AFTER
	PUBLIC ${MOSQUITTO_INCLUDE_DIRS}
	${OPENSSL_INCLUDE_DIRS}

)

target_link_libraries(${tar}
	${MOSQUITTO_LIBRARIES}
	${OPENSSL_LIBRARIES}
)

ENDIF()
