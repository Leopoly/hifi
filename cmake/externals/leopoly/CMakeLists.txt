include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME leopoly)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

if (WIN32)

  ExternalProject_Add(
        ${EXTERNAL_NAME}
        URL https://github.com/fecso8787/HifiPlugin/raw/master/LeoPolySDK-5.zip
        URL_MD5 fa1a1de950e45306a7c854ebf6c3871c
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD 1
  )
  
  # Hide this external target (for ide users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR} CACHE TYPE INTERNAL)
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/SculptAppDLL.lib CACHE TYPE INTERNAL)
  set(${EXTERNAL_NAME_UPPER}_LIBCURL ${SOURCE_DIR}/libcurl.lib CACHE TYPE INTERNAL)
  
  set(${EXTERNAL_NAME_UPPER}_DLL_PATH "${SOURCE_DIR}" CACHE TYPE INTERNAL)

elseif (APPLE)

  ExternalProject_Add(
        ${EXTERNAL_NAME}
        URL https://s3.amazonaws.com/hifi-public/dependencies/LeoPolySDK-Stub-5.zip
        URL_MD5 7e43058cc8c6e75b533d9dc6a281bc8d
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD 1
  )

  # Hide this external target (for ide users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR} CACHE TYPE INTERNAL)

elseif (ANDROID)
else () # Linux

  ExternalProject_Add(
        ${EXTERNAL_NAME}
        URL https://github.com/fecso8787/HifiPlugin/raw/master/LeoPolySDK-5-Linux.zip
        URL_MD5 57bd562f67c587161a7529040f5f1207
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD 1
  )

  # Hide this external target (for ide users)
  set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
  ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
  set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR} CACHE TYPE INTERNAL)
  set(${EXTERNAL_NAME_UPPER}_SHARED_OBJECT_DEBUG ${SOURCE_DIR}/lib/Debug/libSculptAppDLL.so CACHE FILEPATH "Leopoly core library")
  set(${EXTERNAL_NAME_UPPER}_SHARED_OBJECT_RELEASE ${SOURCE_DIR}/lib/Release/libSculptAppDLL.so CACHE FILEPATH "Leopoly core library")

endif()
 