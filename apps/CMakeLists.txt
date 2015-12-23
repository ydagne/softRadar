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



#######################################################################
# Make sure the compiler can find linked libraries
########################################################################
LINK_DIRECTORIES (${CMAKE_BINARY_DIR}/cpp)

########################################################################
# Make sure the compiler can find include files
########################################################################
INCLUDE_DIRECTORIES (${CMAKE_SOURCE_DIR}/cpp )


########################################################################
# Executables                                                          #
########################################################################

ADD_EXECUTABLE(deviceManager ${CMAKE_CURRENT_SOURCE_DIR}/deviceManager.cpp )
TARGET_LINK_LIBRARIES(deviceManager  sgoyihenewcpp )


