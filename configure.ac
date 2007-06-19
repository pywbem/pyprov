#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(openwbem-python-providerifc, 1.0.0)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADER([config.h])
AC_PROG_LIBTOOL

AC_PREFIX_DEFAULT(/usr)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LEX
AC_PROG_YACC

AC_PATH_PROG(RM, rm, rm)
AC_PATH_PROG(MV, mv, mv)
AC_PATH_PROG(CP, cp, cp)

DEBUG_FLAGS="-DDEBUG -g" # Additional debugging flags.
FULL_DEBUG_FLAGS="-D_GLIBCXX_DEBUG"	# Additional debugging flags.
OPT_FLAGS="  -DNDEBUG -O2"	# Additional optimization flags.

CXXFLAGS="$CXXFLAGS -fno-strict-aliasing"
CPPFLAGS="$CPPFLAGS -fno-strict-aliasing"
CFLAGS="$CFLAGS -fno-strict-aliasing"

##NOTE:  whereever CXXFLAGS is used, we need to also use CPPFLAGS, for the AC_CHECK_HEADERS - 
##       It now uses compile (primarily) but still uses pre-processor for interim transition / backward compatibility
##       pre-processor needs the CPPFLAGS

if test "x$prefix" != "xNONE"; then
	prefixDir=${prefix}
else
	prefixDir=${ac_default_prefix}
fi
AC_DEFINE_UNQUOTED(VMWH_PREFIX_DIR, "${prefixDir}", "Default prefix dir")

#######################################################################
# set the location of the sysconf dir
if test "x$sysconfdir" != "x\${prefix}/etc"; then
	tempDir=$sysconfdir
else
	tempDir=${prefixDir}/etc
fi

AC_DEFINE_UNQUOTED(VMWH_SYSCONF_DIR, "${tempDir}", "Default sysconfig dir")

#######################################################################
# Process additional search dirs if specified
AC_ARG_WITH(search-dir,
	[ --with-search-dir=PATH   Specify an additional directory to look for include/ and lib/ sub dirs ],
	[
      if test "x$withval" != "xno" ; then
        	CXXFLAGS="$CXXFLAGS -I$withval/include" 
        	CPPFLAGS="$CPPFLAGS -I$withval/include" 
        	CFLAGS="$CFLAGS -I$withval/include" 
        	LDFLAGS="$LDFLAGS -L$withval/lib" 
      fi
	]
)

debuglevel="0"
#######################################################################
# Set the debug flag if specified
AC_ARG_ENABLE(debug-mode,
	[--enable-debug-mode Enable debugging mode],
	[
		debuglevel="1"
	]
)

AC_ARG_WITH(debug-level,
	[ --with-debug-level=LEVEL   Specify a debug level valid values: 0=none 1=partial debug 2=full debug ],
	[
		case "x$withval" in
		x0|xno)
			debuglevel="0";;
		x1)
			debuglevel="1";;
		x2)
			debuglevel="2";;
		*)
			AC_MSG_WARN("Invalid debug level specified. Valid values are 0, 1 or 2")
			exit 1;;
		esac
	]
)

case $debuglevel in
1)
	AC_DEFINE(SMASH_DEBUG, 1, "Debugging flag.")
	CXXFLAGS="$CXXFLAGS $DEBUG_FLAGS -Wall"
	CPPFLAGS="$CPPFLAGS $DEBUG_FLAGS -Wall"
	CFLAGS="  $CFLAGS   $DEBUG_FLAGS -Wall"
	LDFLAGS=" $LDFLAGS  $DEBUG_FLAGS";;
2)
	AC_DEFINE(SMASH_DEBUG, 2, "Debugging flag.")
	CXXFLAGS="$CXXFLAGS $DEBUG_FLAGS $FULL_DEBUG_FLAGS -Wall"
	CPPFLAGS="$CPPFLAGS $DEBUG_FLAGS $FULL_DEBUG_FLAGS -Wall"
	CFLAGS="  $CFLAGS   $DEBUG_FLAGS $FULL_DEBUG_FLAGS -Wall"
	LDFLAGS=" $LDFLAGS  $DEBUG_FLAGS $FULL_DEBUG_FLAGS";;
*)
	CXXFLAGS="$CXXFLAGS $OPT_FLAGS -Wall"
	CPPFLAGS="$CPPFLAGS $OPT_FLAGS -Wall"
	CFLAGS="  $CFLAGS   $OPT_FLAGS -Wall"
	LDFLAGS=" $LDFLAGS  $OPT_FLAGS";;
esac


#
# If enable-dist-only specified, don't check for dependencies
#

# Checks for two basic and common openwbem libraries
AC_CHECK_LIB([owcimomcommon], [main],[LIBS="$LIBS"],[AC_MSG_ERROR(Must have openwbem-devel or build will fail)])
AC_CHECK_LIB([owclient], [main], [LIBS="$LIBS"], [AC_MSG_ERROR(Must have openwbem-devel or build will fail)])
#AC_CHECK_LIB([sqlite3], [main], [LIBS="$LIBS"], [AC_MSG_ERROR(Must have sqlite3-devel or build will fail)])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([openwbem/OW_config.h fcntl.h stdlib.h string.h unistd.h sys/time.h ],,[AC_MSG_ERROR(Missing headers: likely won't compile)])

AC_CHECK_HEADERS([omc/Base.hpp],[AC_MSG_ERROR(Missing omc base headers: Will not compile)])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_UID_T
AC_HEADER_TIME
AC_TYPE_MODE_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_REPLACE_FNMATCH
AC_FUNC_FORK
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_CHOWN
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_STAT
AC_FUNC_STRERROR_R

AC_CHECK_FUNCS([gettimeofday memset regcomp strchr strdup strerror strstr strtol strtoul uname])


AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/pycxx/Makefile
	src/ifc/pyprovider/Makefile
	src/ifc/Makefile
	src/pyproxy/Makefile
	mof/Makefile
	doc/Makefile
	doc/tutorial/Makefile
	test/Makefile
	test/LogicalFile/Makefile
	])

AC_OUTPUT
