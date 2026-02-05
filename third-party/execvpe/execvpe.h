// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef EXECVPE_H
#define EXECVPE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef _MSC_VER
int icinga2_execvpe(const char *file, char *const argv[], char *const envp[]);
#endif /* _MSC_VER */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* EXECVPE_H */
