#
#  Copyright 2015 Yihenew Beyene
#  
#  This file is part of SoftRadar.
#  
#      SoftRadar is free software: you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation, either version 3 of the License, or
#      (at your option) any later version.
#  
#      SoftRadar is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#  
#      You should have received a copy of the GNU General Public License
#      along with SoftRadar.  If not, see <http://www.gnu.org/licenses/>.


########################################################################
# Append to the list of sources for comlabusrpio
########################################################################
set(libsgoyihenewcpp_sources 
	${CMAKE_CURRENT_SOURCE_DIR}/usrpDevice.cpp
	)

########################################################################
# Setup libcomlabusrpio library
########################################################################
include(cBoost)
find_package(UHD  REQUIRED)
 

########################################################################
# Make sure the compiler can find include files
########################################################################

########################################################################
# Make sure the compiler can find linked libraries
########################################################################
LINK_DIRECTORIES (${UHD_LIBRARY_DIRS} 
                  ${Boost_LIBRARY_DIRS})


ADD_LIBRARY(sgoyihenewcpp  ${libsgoyihenewcpp_sources})
TARGET_LINK_LIBRARIES(sgoyihenewcpp
                      ${Boost_LIBRARIES}
                      ${UHD_LIBRARIES})

########################################################################
# 	Install libraries
########################################################################
INSTALL(TARGETS sgoyihenewcpp
    LIBRARY DESTINATION ${LIBRARY_DIR} COMPONENT libraries # .so file
    ARCHIVE DESTINATION ${LIBRARY_DIR} COMPONENT libraries # .lib file
    RUNTIME DESTINATION ${RUNTIME_DIR} COMPONENT libraries # .dll file
)


