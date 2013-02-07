# ===========================================================================
# ax_prog_asciidoc
# ===========================================================================
#
# SYNOPSIS
#
#   AD_INIT_ASCIIDOC(PROJECT-NAME, DOXYFILE-PATH, [OUTPUT-DIR])
#   AD_ASCIIDOC_FEATURE(ON|OFF)
#
# DESCRIPTION
#
# Based on the Doxygen Macro, modified for Asciidoc detection
#
# LICENSE
#
#   Copyright (c) 2013 Icinga Development Team
#   Copyright (c) 2009 Oren Ben-Kiki <oren@ben-kiki.org>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 12

## ----------##
## Defaults. ##
## ----------##

AD_ENV=""
AC_DEFUN([AD_FEATURE_doc],  ON)

## --------------- ##
## Private macros. ##
## --------------- ##

# AD_ENV_APPEND(VARIABLE, VALUE)
# ------------------------------
# Append VARIABLE="VALUE" to AD_ENV for invoking asciidoc.
AC_DEFUN([AD_ENV_APPEND], [AC_SUBST([AD_ENV], ["$AD_ENV $1='$2'"])])

# AD_DIRNAME_EXPR
# ---------------
# Expand into a shell expression prints the directory part of a path.
AC_DEFUN([AD_DIRNAME_EXPR],
         [[expr ".$1" : '\(\.\)[^/]*$' \| "x$1" : 'x\(.*\)/[^/]*$']])

# AD_IF_FEATURE(FEATURE, IF-ON, IF-OFF)
# -------------------------------------
# Expands according to the M4 (static) status of the feature.
AC_DEFUN([AD_IF_FEATURE], [ifelse(AD_FEATURE_$1, ON, [$2], [$3])])

# AD_REQUIRE_PROG(VARIABLE, PROGRAM)
# ----------------------------------
# Require the specified program to be found for the AD_CURRENT_FEATURE to work.
AC_DEFUN([AD_REQUIRE_PROG], [
AC_PATH_TOOL([$1], [$2])
if test "$AD_FLAG_[]AD_CURRENT_FEATURE$$1" = 1; then
    AC_MSG_WARN([$2 not found - will not AD_CURRENT_DESCRIPTION])
    AC_SUBST(AD_FLAG_[]AD_CURRENT_FEATURE, 0)
fi
])

# AD_TEST_FEATURE(FEATURE)
# ------------------------
# Expand to a shell expression testing whether the feature is active.
AC_DEFUN([AD_TEST_FEATURE], [test "$AD_FLAG_$1" = 1])
# AD_FEATURE_ARG(FEATURE, DESCRIPTION,
#                CHECK_DEPEND, CLEAR_DEPEND,
#                REQUIRE, DO-IF-ON, DO-IF-OFF)
# --------------------------------------------
# Parse the command-line option controlling a feature. CHECK_DEPEND is called
# if the user explicitly turns the feature on (and invokes AD_CHECK_DEPEND),
# otherwise CLEAR_DEPEND is called to turn off the default state if a required
# feature is disabled (using AD_CLEAR_DEPEND). REQUIRE performs additional
# requirement tests (AD_REQUIRE_PROG). Finally, an automake flag is set and
# DO-IF-ON or DO-IF-OFF are called according to the final state of the feature.
AC_DEFUN([AD_ARG_ABLE], [
    AC_DEFUN([AD_CURRENT_FEATURE], [$1])
    AC_DEFUN([AD_CURRENT_DESCRIPTION], [$2])
    AC_ARG_ENABLE(asciidoc-$1,
                  [AS_HELP_STRING(AD_IF_FEATURE([$1], [--disable-asciidoc-$1],
                                                      [--enable-asciidoc-$1]),
                                  AD_IF_FEATURE([$1], [don't $2], [$2]))],
                  [
case "$enableval" in
#(
y|Y|yes|Yes|YES)
    AC_SUBST([AD_FLAG_$1], 1)
    $3
;; #(
n|N|no|No|NO)
    AC_SUBST([AD_FLAG_$1], 0)
;; #(
*)
    AC_MSG_ERROR([invalid value '$enableval' given to asciidoc-$1])
;;
esac
], [
AC_SUBST([AD_FLAG_$1], [AD_IF_FEATURE([$1], 1, 0)])
$4
])
if AD_TEST_FEATURE([$1]); then
    $5
    :
fi
AM_CONDITIONAL(AD_COND_$1, AD_TEST_FEATURE([$1]))
if AD_TEST_FEATURE([$1]); then
    $6
    :
else
    $7
    :
fi
])

## -------------- ##
## Public macros. ##
## -------------- ##

# AD_XXX_FEATURE(DEFAULT_STATE)
# -----------------------------
AC_DEFUN([AD_ASCIIDOC_FEATURE], [AC_DEFUN([AD_FEATURE_asciidoc],  [$1])])

# AD_INIT_ASCIIDOC(PROJECT, [CONFIG-FILE], [OUTPUT-DOC-DIR])
# ---------------------------------------------------------
# PROJECT also serves as the base name for the documentation files.
AC_DEFUN([AD_INIT_ASCIIDOC], [

# Files:
AC_SUBST([AD_PROJECT], [$1])
AC_SUBST([AD_CONFIG], [ifelse([$2], [], [], [$2])])
AC_SUBST([AD_DOCDIR], [ifelse([$3], [], docs, [$3])])

# Asciidoc itself:
AD_ARG_ABLE(doc, [generate any asciidoc documentation],
            [],
            [],
            [AD_REQUIRE_PROG([AD_ASCIIDOC], asciidoc)]
            )
])
