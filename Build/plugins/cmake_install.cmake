# Install script for directory: D:/captainT/project_13/ImageMultiView/trunk-master/plugins

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/CloudCompareProjects")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qAnimation/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qBlur/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qCork/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qCSF/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qCSVMatrixIO/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qDummyPlugin/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qEDL/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qFacets/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qGMMREG/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qHoughNormals/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qHPR/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qKinect/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qLAS_FWF/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qM3C2/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qPCL/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qPCV/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qPhotoscanIO/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qPoissonRecon/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qRANSAC_SD/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qSRA/cmake_install.cmake")
  include("D:/captainT/project_13/ImageMultiView/Build/plugins/qSSAO/cmake_install.cmake")

endif()

