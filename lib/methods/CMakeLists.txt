# Icinga 2
# Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

mkembedconfig_target(methods-itl.conf methods-itl.cpp)

set(methods_SOURCES
  i2-methods.hpp methods-itl.cpp
  clusterchecktask.cpp clusterchecktask.hpp
  clusterzonechecktask.cpp clusterzonechecktask.hpp
  dummychecktask.cpp dummychecktask.hpp
  exceptionchecktask.cpp exceptionchecktask.hpp
  icingachecktask.cpp icingachecktask.hpp
  nullchecktask.cpp nullchecktask.hpp
  nulleventtask.cpp nulleventtask.hpp
  pluginchecktask.cpp pluginchecktask.hpp
  plugineventtask.cpp plugineventtask.hpp
  pluginnotificationtask.cpp pluginnotificationtask.hpp
  randomchecktask.cpp randomchecktask.hpp
  timeperiodtask.cpp timeperiodtask.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(methods methods methods_SOURCES)
endif()

add_library(methods OBJECT ${methods_SOURCES})

add_dependencies(methods base config icinga)

set_target_properties (
  methods PROPERTIES
  FOLDER Lib
)
