// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef URL_CHARACTERS_H
#define URL_CHARACTERS_H

#define ALPHA "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define NUMERIC "0123456789"

#define UNRESERVED ALPHA NUMERIC "-._~" "%"
#define GEN_DELIMS ":/?#[]@"
#define SUB_DELIMS "!$&'()*+,;="
#define PCHAR UNRESERVED SUB_DELIMS ":@"
#define PCHAR_ENCODE UNRESERVED ":@"

#define ACSCHEME ALPHA NUMERIC ".-+"

//authority = [ userinfo "@" ] host [ ":" port ]
#define ACUSERINFO UNRESERVED SUB_DELIMS
#define ACHOST UNRESERVED SUB_DELIMS
#define ACPORT NUMERIC

#define ACPATHSEGMENT PCHAR
#define ACPATHSEGMENT_ENCODE PCHAR_ENCODE
#define ACQUERY PCHAR "/?"
#define ACQUERY_ENCODE PCHAR_ENCODE "/?"
#define ACFRAGMENT PCHAR "/?"
#define ACFRAGMENT_ENCODE PCHAR_ENCODE "/?"

#endif /* URL_CHARACTERS_H */
