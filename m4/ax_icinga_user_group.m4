#/******************************************************************************
# * Icinga 2                                                                   *
# * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
# *                                                                            *
# * This program is free software; you can redistribute it and/or              *
# * modify it under the terms of the GNU General Public License                *
# * as published by the Free Software Foundation; either version 2             *
# * of the License, or (at your option) any later version.                     *
# *                                                                            *
# * This program is distributed in the hope that it will be useful,            *
# * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
# * GNU General Public License for more details.                               *
# *                                                                            *
# * You should have received a copy of the GNU General Public License          *
# * along with this program; if not, write to the Free Software Foundation     *
# * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
# ******************************************************************************/

AC_DEFUN([ACICINGA_CHECK_USER],[
  x=$1
  y=$2
  AC_MSG_CHECKING([if $y user $x exists])
  AS_IF([ getent passwd $x >/dev/null 2>&1 || dscl . -read /Users/$x >/dev/null 2>&1 ],
    [ AC_MSG_RESULT([found]) ],
    [ AC_MSG_WARN([not found]) ])
])

AC_DEFUN([ACICINGA_CHECK_GROUP],[
  x=$1
  y=$2
  AC_MSG_CHECKING([if $y group $x exists])
  AS_IF([ getent group $x >/dev/null 2>&1 || dscl . -read /Groups/$x >/dev/null 2>&1 ],
    [ AC_MSG_RESULT([found]) ],
    [ AC_MSG_WARN([not found]) ])
])
