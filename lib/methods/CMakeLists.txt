# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

mkembedconfig_target(methods-itl.conf methods-itl.cpp)

add_library(methods OBJECT
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

target_link_libraries(methods PUBLIC
  base
  config
  icinga
)

set_target_properties (
  methods PROPERTIES
  FOLDER Lib
)
