/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef EXECVPE_H
#define EXECVPE_H

#include "base/visibility.hpp"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef I2_EXECVPE_BUILD
#	define I2_EXECVPE_API I2_EXPORT
#else
#	define I2_EXECVPE_API I2_IMPORT
#endif /* I2_EXECVPE_BUILD */

#ifndef _MSC_VER
I2_EXECVPE_API int icinga2_execvpe(const char *file, char *const argv[], char *const envp[]);
#endif /* _MSC_VER */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* EXECVPE_H */
