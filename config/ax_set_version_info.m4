dnl @synopsis AX_SET_VERSION_INFO [(VERSION [,PREFIX])]
dnl @synopsis default $1 = $PACKAGE_VERSION
dnl @synopsis default $2 = <none>
dnl
dnl This macro is the successor of AC_SET_RELEASEINFO_VERSIONINFO but
dnl it can be used in parallel because it uses all different variables.
dnl
dnl check the $VERSION number and cut the two last digit-sequences off
dnl which will form a -version-info in a @VERSION_INFO@ ac_subst while
dnl the rest is going to the -release name in a @RELEASE_INFO@
dnl ac_subst.
dnl
dnl you should keep these two seperate - the release-name may contain
dnl alpha-characters and can be modified later with extra release-hints
dnl e.g. RELEASE_INFO="$RELEASE_INFO-debug" for a debug version of your
dnl lib. The $VERSION_INFO however should not be touched.
dnl
dnl example: a VERSION="2.4.18" will be transformed into
dnl
dnl    RELEASE_INFO = -release 2
dnl    VERSION_INFO = -versioninfo 4:18
dnl
dnl then use these two variables and push them to your libtool linker
dnl
dnl    libtest_la_LIBADD = @RELEASE_INFO@ @VERSION_INFO@
dnl
dnl and for a linux-target this will tell libtool to install the lib as
dnl
dnl           libmy.so libmy.la libmy.a libmy-2.so.4 libmy-2.so.4.0.18
dnl
dnl and executables will get link-resolve-infos for libmy-2.so.4 -
dnl therefore the patch-level is ignored during ldso linking, and ldso
dnl will use the one with the highest patchlevel. Using just "-release
dnl $(VERSION)" during libtool-linking would not do that - omitting the
dnl -version-info will libtool install libmy.so libmy.la libmy.a
dnl libmy-2.4.18.so and executables would get hardlinked with the
dnl 2.4.18 version of your lib.
dnl
dnl This background does also explain the default dll name for a win32
dnl target : libtool will choose to make up libmy-2-4.dll for this
dnl version spec.
dnl
dnl this macro does also set the usual three parts of a version spec
dnl $MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION but does not ac_subst
dnl for the plain AX_SET_VERSION_INFO macro. Use instead one of the
dnl numbered macros AX_SET_VERSION_INFO1 (use first number for release
dnl part) or that AX_SET_VERSION_INFO2 (use the first two numbers for
dnl release part).
dnl
dnl You may add sublevel parts like "1.4.2-ac5" where the sublevel is
dnl just killed from these version/release substvars. That allows to
dnl grab the version off a .spec file like with AX_SPEC_PACKAGE_VERSION
dnl where the $VERSION is used to name a tarball or distpack like
dnl mylib-2.2.9pre4
dnl
dnl Unlike earlier macros, you can use this one to break up different
dnl VERSIONs and put them into different variables, just hint with
dnl PREFIX-setting - i.e. _VERSION(2.4.5,TEST) will set variables named
dnl TEST_MAJOR_VERSION=2... and of course $TEST_RELEASE_INFO etc. (for
dnl the moment, it needs to be a literal prefix *sigh*)
dnl
dnl @category Misc
dnl @author Guido U. Draheim <guidod@gmx.de>
dnl @version 2006-10-13
dnl @license GPLWithACException

AC_DEFUN([AX_SET_VERSION_INFO1],[dnl
AS_VAR_PUSHDEF([MAJOR],ifelse($2,,[MAJOR_VERSION],[$2_MAJOR_VERSION]))dnl
AS_VAR_PUSHDEF([MINOR],ifelse($2,,[MINOR_VERSION],[$2_MINOR_VERSION]))dnl
AS_VAR_PUSHDEF([MICRO],ifelse($2,,[MICRO_VERSION],[$2_MICRO_VERSION]))dnl
AS_VAR_PUSHDEF([PATCH],ifelse($2,,[PATCH_VERSION],[$2_PATCH_VERSION]))dnl
AS_VAR_PUSHDEF([LTREL],ifelse($2,,[RELEASE_INFO],[$2_RELEASE_INFO]))dnl
AS_VAR_PUSHDEF([LTVER],ifelse($2,,[VERSION_INFO],[$2_VERSION_INFO]))dnl
test ".$PACKAGE_VERSION" = "." && PACKAGE_VERSION="$VERSION"
AC_MSG_CHECKING(ifelse($2,,,[$2 ])out linker version info dnl
ifelse($1,,$PACKAGE_VERSION,$1) )
  MINOR=`echo ifelse( $1, , $PACKAGE_VERSION, $1 )`
  MAJOR=`echo "$MINOR" | sed -e 's/[[.]].*//'`
  MINOR=`echo "$MINOR" | sed -e "s/^$MAJOR//" -e 's/^.//'`
  MICRO="$MINOR"
  MINOR=`echo "$MICRO" | sed -e 's/[[.]].*//'`
  MICRO=`echo "$MICRO" | sed -e "s/^$MINOR//" -e 's/^.//'`
  PATCH="$MICRO"
  MICRO=`echo "$PATCH" | sed -e 's/[[^0-9]].*//'`
  PATCH=`echo "$PATCH" | sed -e "s/^$MICRO//" -e 's/^[[-.]]//'`
  if test "_$MICRO" = "_" ; then MICRO="0" ; fi
  if test "_$MINOR" = "_" ; then MINOR="$MAJOR" ; MAJOR="0" ; fi
  MINOR=`echo "$MINOR" | sed -e 's/[[^0-9]].*//'`
  LTREL="-release $MAJOR"
  LTVER="-version-info $MINOR:$MICRO"
AC_MSG_RESULT([/$MAJOR/$MINOR:$MICRO (-$MAJOR.so.$MINOR.0.$MICRO)])
AC_SUBST(MAJOR)
AC_SUBST(MINOR)
AC_SUBST(MICRO)
AC_SUBST(PATCH)
AC_SUBST(LTREL)
AC_SUBST(LTVER)
AS_VAR_POPDEF([LTVER])dnl
AS_VAR_POPDEF([LTREL])dnl
AS_VAR_POPDEF([PATCH])dnl
AS_VAR_POPDEF([MICRO])dnl
AS_VAR_POPDEF([MINOR])dnl
AS_VAR_POPDEF([MAJOR])dnl
])

AC_DEFUN([AX_SET_VERSION_INFO2],[dnl
AS_VAR_PUSHDEF([MAJOR],ifelse($2,,[MAJOR_VERSION],[$2_MAJOR_VERSION]))dnl
AS_VAR_PUSHDEF([MINOR],ifelse($2,,[MINOR_VERSION],[$2_MINOR_VERSION]))dnl
AS_VAR_PUSHDEF([MICRO],ifelse($2,,[MICRO_VERSION],[$2_MICRO_VERSION]))dnl
AS_VAR_PUSHDEF([PATCH],ifelse($2,,[PATCH_VERSION],[$2_PATCH_VERSION]))dnl
AS_VAR_PUSHDEF([LTREL],ifelse($2,,[RELEASE_INFO],[$2_RELEASE_INFO]))dnl
AS_VAR_PUSHDEF([LTVER],ifelse($2,,[VERSION_INFO],[$2_VERSION_INFO]))dnl
test ".$PACKAGE_VERSION" = "." && PACKAGE_VERSION="$VERSION"
AC_MSG_CHECKING(ifelse($2,,,[$2 ])out linker version info dnl
ifelse($1,,$PACKAGE_VERSION,$1) )
  MINOR=`echo ifelse( $1, , $PACKAGE_VERSION, $1 )`
  MAJOR=`echo "$MINOR" | sed -e 's/[[.]].*//'`
  MINOR=`echo "$MINOR" | sed -e "s/^$MAJOR//" -e 's/^.//'`
  MICRO="$MINOR"
  MINOR=`echo "$MICRO" | sed -e 's/[[.]].*//'`
  MICRO=`echo "$MICRO" | sed -e "s/^$MINOR//" -e 's/^.//'`
  PATCH="$MICRO"
  MICRO=`echo "$PATCH" | sed -e 's/[[^0-9]].*//'`
  PATCH=`echo "$PATCH" | sed -e "s/^$MICRO//" -e 's/^[[-.]]//'`
  test "_$MICRO" != "_" || MICRO="0"
  if test "_$MINOR" != "_" ; then MINOR="$MAJOR" ; MAJOR="0" ; fi
  MINOR=`echo "$MINOR" | sed -e 's/[[^0-9]].*//'`
  LTREL="-release $MAJOR.$MINOR"
  LTVER="-version-info 0:$MICRO"
AC_MSG_RESULT([/$MAJOR/$MINOR:$MICRO (-$MAJOR.so.$MINOR.0.$MICRO)])
AC_SUBST(MAJOR)
AC_SUBST(MINOR)
AC_SUBST(MICRO)
AC_SUBST(PATCH)
AC_SUBST(LTREL)
AC_SUBST(LTVER)
AS_VAR_POPDEF([LTVER])dnl
AS_VAR_POPDEF([LTREL])dnl
AS_VAR_POPDEF([PATCH])dnl
AS_VAR_POPDEF([MICRO])dnl
AS_VAR_POPDEF([MINOR])dnl
AS_VAR_POPDEF([MAJOR])dnl
])

AC_DEFUN([AX_SET_VERSION_INFO],[dnl
AS_VAR_PUSHDEF([MAJOR],ifelse($2,,[MAJOR_VERSION],[$2_MAJOR_VERSION]))dnl
AS_VAR_PUSHDEF([MINOR],ifelse($2,,[MINOR_VERSION],[$2_MINOR_VERSION]))dnl
AS_VAR_PUSHDEF([MICRO],ifelse($2,,[MICRO_VERSION],[$2_MICRO_VERSION]))dnl
AS_VAR_PUSHDEF([PATCH],ifelse($2,,[PATCH_VERSION],[$2_PATCH_VERSION]))dnl
AS_VAR_PUSHDEF([LTREL],ifelse($2,,[RELEASE_INFO],[$2_RELEASE_INFO]))dnl
AS_VAR_PUSHDEF([LTVER],ifelse($2,,[VERSION_INFO],[$2_VERSION_INFO]))dnl
test ".$PACKAGE_VERSION" = "." && PACKAGE_VERSION="$VERSION"
AC_MSG_CHECKING(ifelse($2,,,[$2 ])out linker version info dnl
ifelse($1,,$PACKAGE_VERSION,$1) )
  MINOR=`echo ifelse( $1, , $PACKAGE_VERSION, $1 )`
  MAJOR=`echo "$MINOR" | sed -e 's/[[.]].*//'`
  MINOR=`echo "$MINOR" | sed -e "s/^$MAJOR//" -e 's/^.//'`
  MICRO="$MINOR"
  MINOR=`echo "$MICRO" | sed -e 's/[[.]].*//'`
  MICRO=`echo "$MICRO" | sed -e "s/^$MINOR//" -e 's/^.//'`
  PATCH="$MICRO"
  MICRO=`echo "$PATCH" | sed -e 's/[[^0-9]].*//'`
  PATCH=`echo "$PATCH" | sed -e "s/^$MICRO//" -e 's/[[-.]]//'`
  if test "_$MICRO" = "_" ; then MICRO="0" ; fi
  if test "_$MINOR" = "_" ; then MINOR="$MAJOR" ; MAJOR="0" ; fi
  MINOR=`echo "$MINOR" | sed -e 's/[[^0-9]].*//'`
  LTREL="-release $MAJOR"
  LTVER="-version-info $MINOR:$MICRO"
AC_MSG_RESULT([/$MAJOR/$MINOR:$MICRO (-$MAJOR.so.$MINOR.0.$MICRO)])
AC_SUBST(LTREL)
AC_SUBST(LTVER)
AS_VAR_POPDEF([LTVER])dnl
AS_VAR_POPDEF([LTREL])dnl
AS_VAR_POPDEF([PATCH])dnl
AS_VAR_POPDEF([MICRO])dnl
AS_VAR_POPDEF([MINOR])dnl
AS_VAR_POPDEF([MAJOR])dnl
])
