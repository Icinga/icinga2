// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef EDITLINE_H
#define EDITLINE_H

extern "C" {

char *readline(const char *prompt);
int add_history(const char *line);
void rl_deprep_terminal();

typedef char *ELFunction(const char *, int);

extern char rl_completion_append_character;
extern ELFunction *rl_completion_entry_function;

}

#endif /* EDITLINE_H */
