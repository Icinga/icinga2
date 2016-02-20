/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#ifndef URL_CHARACTERS_H
#define URL_CHARACTERS_H

#define ALPHA "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define NUMERIC "0123456789"

#define UNRESERVED ALPHA NUMERIC "-._~" "%"
#define GEN_DELIMS ":/?#[]@"
#define SUB_DELIMS "!$&'()*+,;="
#define RESERVED GEN_DELIMS SUB-DELIMS
#define PCHAR UNRESERVED SUB_DELIMS ":@"

#define ACSCHEME ALPHA NUMERIC ".-+"

//authority = [ userinfo "@" ] host [ ":" port ]
#define ACUSERINFO UNRESERVED SUB_DELIMS 
#define ACHOST UNRESERVED SUB_DELIMS
#define ACPORT NUMERIC

#define ACPATHSEGMENT PCHAR
#define ACQUERY PCHAR "/?"
#define ACFRAGMENT PCHAR "/?"

#endif /* URL_CHARACTERS_H */
