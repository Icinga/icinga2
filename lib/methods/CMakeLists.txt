# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

mkembedconfig_target(methods-itl.conf methods-itl.cpp)

set(methods_SOURCES
  i2-methods.hpp methods-itl.cpp
  clusterchecktask.cpp clusterchecktask.hpp
  clusterzonechecktask.cpp clusterzonechecktask.hpp
  dummychecktask.cpp dummychecktask.hpp
  exceptionchecktask.cpp exceptionchecktask.hpp
  icingachecktask.cpp icingachecktask.hpp
  ifwapichecktask.cpp ifwapichecktask.hpp
  nullchecktask.cpp nullchecktask.hpp
  nulleventtask.cpp nulleventtask.hpp
  pluginchecktask.cpp pluginchecktask.hpp
  plugineventtask.cpp plugineventtask.hpp
  pluginnotificationtask.cpp pluginnotificationtask.hpp
  randomchecktask.cpp randomchecktask.hpp
  timeperiodtask.cpp timeperiodtask.hpp
  sleepchecktask.cpp sleepchecktask.hpp
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
