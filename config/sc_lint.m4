dnl
dnl Copyright 2005-2006 Sun Microsystems, Inc.  All rights reserved.
dnl
dnl Permission is hereby granted, free of charge, to any person obtaining a
dnl copy of this software and associated documentation files (the
dnl "Software"), to deal in the Software without restriction, including
dnl without limitation the rights to use, copy, modify, merge, publish,
dnl distribute, and/or sell copies of the Software, and to permit persons
dnl to whom the Software is furnished to do so, provided that the above
dnl copyright notice(s) and this permission notice appear in all copies of
dnl the Software and that both the above copyright notice(s) and this
dnl permission notice appear in supporting documentation.
dnl
dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
dnl OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
dnl MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
dnl OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
dnl HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
dnl INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
dnl FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
dnl NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
dnl WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
dnl
dnl Except as contained in this notice, the name of a copyright holder
dnl shall not be used in advertising or otherwise to promote the sale, use
dnl or other dealings in this Software without prior written authorization
dnl of the copyright holder.

# SC_WITH_LINT()
# ----------------
# Minimum version: 1.1.0
#
# Sets up flags for source checkers such as lint and sparse if --with-lint
# is specified.   (Use --with-lint=sparse for sparse.)
# Sets $LINT to name of source checker passed with --with-lint (default: splint)
# Sets $LINT_FLAGS to flags to pass to source checker
# Sets LINT automake conditional if enabled (default: disabled)
#
AC_DEFUN([SC_WITH_LINT],[

# Allow checking code with lint, sparse, etc.
AC_ARG_WITH(lint, [AC_HELP_STRING([--with-lint],
    [Use a lint-style source code checker (default: disabled)])],
     [use_lint=$withval], [use_lint=no])
if test "x$use_lint" = "xyes" ; then
  LINT="splint"
else
  LINT="$use_lint"
fi
if test "x$LINT_FLAGS" = "x" -a "x$LINT" != "xno" ; then
    case $LINT in
      lint|*/lint)
        case $host_os in
          solaris*)
            LINT_FLAGS="-u -b -h -erroff=E_INDISTING_FROM_TRUNC2"
            ;;
        esac
        ;;
      splint|*/splint)
        LINT_FLAGS="-weak -badflag -preproc -posix-lib"
        ;;
    esac
fi

AC_SUBST(LINT)
AC_SUBST(LINT_FLAGS)
AM_CONDITIONAL(LINT, [test x$LINT != xno])

]) # SC_WITH_LINT
