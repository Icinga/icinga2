# Icinga 2
# Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)
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

#
# this file is used to build icinga-version.h.force
# for the Debian builds, it needs to be included in the tarball
#

cmake_minimum_required(VERSION 2.6)
project(icinga2)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../third-party/cmake")

include(GetGitRevisionDescription)
git_describe(GIT_VERSION --tags)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../icinga-version.h.cmake" icinga-version.h.force)

