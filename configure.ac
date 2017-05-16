dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(clonk, m4_format([[[%s.%s.%s.%s]]], m4_include(version)))
AC_COPYRIGHT([©2005, 2006, 2007 Günther Brammer])
AC_CONFIG_SRCDIR([engine/src/C4Map.cpp])
AC_CONFIG_AUX_DIR([autotools])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])
AC_CANONICAL_HOST
dnl foreign to tell automake to shut up,
dnl no-dist because we do not need dist
dnl and subdir-objects because it makes the Makefile smaller.
dnl no-define because PACKAGE and VERSION are not used
AM_INIT_AUTOMAKE([foreign subdir-objects nostdinc no-dist no-define 1.10])

dnl The new every random file in the repos policy
AM_MAINTAINER_MODE

AC_PROG_CXX
AC_LANG([C++])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_RANLIB

# The wonders of code stolen from docs.
case $host in
  *-*-mingw32* | *-*-cygwin* | *-*-windows*)
    win32=true; osx=false;;
  *-*-darwin*)
    win32=false; osx=true;;
  *)
    win32=false; osx=false;;
esac

# Detect Wine. Unfortunately, at the moment winegcc does not meet the demands
AC_CHECK_HEADER([windows.h], [win32=true], [], [[ ]])

AM_CONDITIONAL(WIN32, [test $win32 = true])

# various used headers
dnl the whitespace is there to prevent AC_INCLUDES_DEFAULT
AC_CHECK_HEADERS([stdint.h unistd.h sys/stat.h sys/types.h locale.h sys/socket.h signal.h langinfo.h execinfo.h sys/inotify.h sys/syscall.h], , , [[ ]])
# Mingw does not ship with multimon.h
AC_CHECK_HEADERS([multimon.h io.h direct.h share.h], [], [], [[#include <windows.h>]])
# iconv
AX_ICONV
# vasprintf is a GNU extension
AC_CHECK_FUNCS(vasprintf)

AC_ARG_ENABLE([debug],
  [AC_HELP_STRING([--enable-debug], [debugging options [default=no]])],
  , [enable_debug=no])
if test $enable_debug = yes; then
  AC_DEFINE([_DEBUG], 1, [compile with debug options])
else
  AC_DEFINE([NDEBUG], 1, [compile without debug options])
fi

AC_ARG_ENABLE([debugrec],
  [AC_HELP_STRING([--enable-debugrec], [write additional debug control to records [default=no]])], 
    , [enable_debugrec=no])
if test $enable_debugrec = yes; then
  AC_DEFINE([DEBUGREC], 1, [Activate DebugRecs])
fi

AC_ARG_ENABLE([network],
  [AC_HELP_STRING([--disable-network], [network play support [default=yes]])], 
    , [if test -r "$srcdir/engine/src/C4NetIO.cpp"; then
         enable_network=yes
       else
         enable_network=no
       fi])
if test $enable_network = yes; then
  AC_DEFINE([NETWORK], 1, [Network play support])
else
  AC_DEFINE([NONETWORK], 1, [Network play support disabled])
fi
AM_CONDITIONAL(NETWORK, [test $enable_network = yes])

AC_ARG_WITH([gtk],
  [AC_HELP_STRING([--with-gtk], [Use gtk for the developer mode [default=no]])],
  , [with_gtk=no])
if test $with_gtk = yes; then
  PKG_CHECK_MODULES(GTK, glib-2.0 >= 2.4 gtk+-2.0 >= 2.4)
  AC_DEFINE([WITH_GLIB], 1, [Glib])
  AC_DEFINE([WITH_DEVELOPER_MODE], 1, [Developer mode])
  DEVELOPER_MODE=true
fi
AM_CONDITIONAL(DEVELOPER_MODE, [test $with_gtk = yes])

AC_ARG_ENABLE([console],
  [AC_HELP_STRING([--enable-console],[compile as pure console application [default=no]])],
  , [enable_console=no])
if test $enable_console = no; then
  AC_ARG_WITH([gl],
    [AC_HELP_STRING([--with-gl],[compile with opengl support [default=yes]])],
    , [with_gl=yes])

  AC_ARG_ENABLE([sound],
    [AC_HELP_STRING([--enable-sound],[compile with sound support [default=yes]])],
    , [enable_sound=yes])

  if test $enable_sound = yes; then
    AC_ARG_ENABLE([mp3],
      [AC_HELP_STRING([--with-mp3],[enable mp3 support [default=no]])],
      , [enable_mp3=no])
    if test $enable_mp3 = yes; then
      AC_DEFINE([USE_MP3], 1, [MP3 music])
    fi
  fi
  
  if test $win32 = true; then
    AC_SUBST(WINDOWS_CFLAGS, "-mwindows")
    AC_ARG_WITH([directx],
      [AC_HELP_STRING([--with-directx],[compile with directx support [default=no]])],
      , [with_directx="no"])
    enable_sdlmainloop=no
    have_sdl=no
  else
    with_directx=no
    
    # Check for SDL
    SDL_VERSION=1.2.0
    AM_PATH_SDL($SDL_VERSION,
      [AC_DEFINE(HAVE_SDL, 1, [Define to 1 if you have SDL.])
       have_sdl=yes;],
      [AC_MSG_WARN([libSDL not found.])
       have_sdl=no;])
    AC_SUBST(SDL_CFLAGS)
    AC_SUBST(SDL_LIBS)
    if test $enable_sound = yes; then
      # Check for SDL_mixer library
      AC_CHECK_LIB(SDL_mixer, Mix_OpenAudio,
        [AC_DEFINE(HAVE_LIBSDL_MIXER,1,[Define to 1 if you have SDL_mixer.]) 
        SDL_LIBS="-lSDL_mixer $SDL_LIBS"],
        [AC_MSG_WARN([SDL_mixer is used for sound and music.])])
    fi
    AC_ARG_ENABLE(sdlmainloop,
      [AC_HELP_STRING([--enable-sdlmainloop],[use SDL instead of X11 or Win32 [default=no]])],
      , [if test $osx = true; then enable_sdlmainloop=yes; else enable_sdlmainloop=no; fi ])
  fi
else
  # console mode
  AC_DEFINE([USE_CONSOLE], 1, [dedicated server mode])
  with_directx=no
  with_gl=no
  with_x=no
  enable_sdlmainloop=no
  have_sdl=no
  enable_sound=no
  VL_LIB_READLINE
fi

# Check for libjpeg
AC_ARG_WITH([internal-libjpeg],
  [AC_HELP_STRING([--with-internal-libjpeg], [Use the included copy of libjpeg [default=no]])])
if test "$with_internal_libjpeg" != yes; then
  AC_CHECK_LIB(jpeg, jpeg_read_header, [
    CLONK_LIBS="-ljpeg $CLONK_LIBS"
    with_internal_libjpeg=no
  ], [
    if test "$with_internal_libjpeg" = no; then
      AC_MSG_ERROR([libjpeg not found.])
    else
      with_internal_libjpeg=yes
    fi
  ])
fi
if test "$with_internal_libjpeg" = yes; then
  INCLUDES="$INCLUDES -I\$(top_srcdir)/standard/jpeglib"
fi
AM_CONDITIONAL(INTERNAL_JPEG, [test $with_internal_libjpeg = yes])

# Check for libpng
AC_ARG_WITH([internal-libpng],
  [AC_HELP_STRING([--with-internal-libpng], [Use the included copy of libpng [default=no]])])
if test "$with_internal_libpng" != yes; then
  AC_CHECK_LIB(png, png_read_image, [
    CLONK_LIBS="-lpng $CLONK_LIBS"
    with_internal_libpng=no
  ], [
    if test "$with_internal_libpng" = no; then
      AC_MSG_ERROR([libpng not found.])
    else
      with_internal_libpng=yes
    fi
  ], [-lz])
fi
if test "$with_internal_libpng" = yes; then
  INCLUDES="$INCLUDES -I\$(top_srcdir)/standard/lpng121"
fi
AM_CONDITIONAL(INTERNAL_PNG, [test $with_internal_libpng = yes])

# Check for libfreetype
AC_ARG_WITH([internal-libfreetype],
  [AC_HELP_STRING([--with-internal-libfreetype], [Use the included copy of libfreetype [default=no]])])
if test "$with_internal_libfreetype" != yes && test $enable_console = no; then
  if test "$cross_compiling" = no; then
    AC_CHECK_PROG(FREETYPE_CONFIG, freetype-config, freetype-config)
  fi
  if test $FREETYPE_CONFIG; then
    AC_SUBST(FREETYPE_CFLAGS, [`$FREETYPE_CONFIG --cflags`])
    AC_SUBST(FREETYPE_LIBS, [`$FREETYPE_CONFIG --libs`])
    AC_DEFINE(HAVE_FREETYPE, 1, [Define to 1 if you have FreeType2.])
    with_internal_libfreetype=no
  else
    if test "$with_internal_libfreetype" = no; then
      AC_MSG_WARN([libfreetype not found.])
    else
      with_internal_libfreetype=yes
    fi
  fi
fi
if test "$with_internal_libfreetype" = yes; then
  INCLUDES="$INCLUDES -I\$(top_srcdir)/standard/freetype2"
fi
AM_CONDITIONAL(INTERNAL_FREETYPE, [test $with_internal_libfreetype = yes])

AC_SUBST(INCLUDES)

if test $enable_sdlmainloop = yes; then
  with_x=no
  AC_DEFINE(USE_SDL_MAINLOOP, 1, [Define to 1 if SDL is used for the main loop])
fi
AM_CONDITIONAL(SDL_MAIN_LOOP, [test $enable_sdlmainloop = yes])

# X11, Xpm
AC_PATH_XTRA
if test $have_x = yes; then
  AC_DEFINE(USE_X11, 1, [Define to 1 if the X Window System is used])
  CLONK_LIBS="$X_LIBS $CLONK_LIBS"
  AC_CHECK_HEADERS([X11/keysym.h X11/extensions/xf86vmode.h X11/xpm.h], ,
    [AC_MSG_ERROR([A required X11 header was not found.])], [[#include <X11/Xlib.h>]])
  AC_CHECK_LIB(X11, XOpenDisplay, [CLONK_LIBS="-lX11 $CLONK_LIBS"],
    [AC_MSG_ERROR([libX11 not found.])], [$X_LIBS])
  AC_CHECK_LIB(Xpm, XpmCreatePixmapFromData, [CLONK_LIBS="-lXpm $CLONK_LIBS"],
    [AC_MSG_ERROR([libXpm not found.])], [$X_LIBS])
  AC_CHECK_LIB(Xxf86vm, XF86VidModeQueryVersion, [CLONK_LIBS="-lXxf86vm $CLONK_LIBS"],
    [AC_MSG_ERROR([XF86VidMode not found.])], [$X_LIBS])
fi

if test $with_gl = yes; then  
  # OpenGL
  if test $osx = true; then
    CLONK_LIBS="-framework OpenGL $CLONK_LIBS"
  elif test $win32 = true; then
    AC_CHECK_LIB(opengl32, main, [CLONK_LIBS="-lopengl32 $CLONK_LIBS"],
      [AC_MSG_ERROR([opengl32 not found.])])
    AC_CHECK_LIB(glu32, main, [CLONK_LIBS="-lglu32 $CLONK_LIBS"],
      [AC_MSG_ERROR([glu32 not found.])])
  else
    AC_CHECK_LIB(GL, glBegin, [CLONK_LIBS="-lGL $CLONK_LIBS"],
      [AC_MSG_ERROR([libGL not found.])])
    AC_CHECK_LIB(GLU, gluOrtho2D, [CLONK_LIBS="-lGLU $CLONK_LIBS"],
      [AC_MSG_ERROR([libGLU not found.])])
  fi
  AC_DEFINE([USE_GL], 1, [OpenGL graphics])
fi
  
# DirectX
if test "$with_directx" = yes; then
  AC_CHECK_LIB(d3d8, main, [CLONK_LIBS="-ld3d8 $CLONK_LIBS"],
    [AC_MSG_ERROR([d3d8 not found.])])
  AC_DEFINE([USE_DIRECTX], 1, [DirectX graphics])
fi

if test $win32 = false; then
  # OpenSSL
  if test -z "${OPENSSL_LIBS}"; then
    AC_CHECK_LIB(crypto, BIO_new, [OPENSSL_LIBS="-lcrypto"],
      [AC_MSG_ERROR([libcrypto (OpenSSL) not found.])])
  fi
  
  # pthread
  ACX_PTHREAD( , [AC_MSG_ERROR([No pthread support.])])
  AC_SUBST(PTHREAD_LIBS)
  AC_SUBST(PTHREAD_CFLAGS)
  
  # inotify syscall workaround
  AC_CHECK_DECL(SYS_inotify_init,
  	AC_DEFINE([HAVE_SYSCALL_INOTIFY], 1, [inotify reachable using syscall]),,
  	[#include <sys/syscall.h>])
  
  # FMod is used for windows only
  AM_CONDITIONAL(USE_FMOD, false)
else
  # Link openssl statically, always
  OPENSSL_LIBS='$(srcdir)/standard/openssl/libcrypto.a'
  # Windows
  for WIN32LIB in gdi32 comdlg32 winmm ws2_32 vfw32
  do
    AC_CHECK_LIB([$WIN32LIB], main, [CLONK_LIBS="-l$WIN32LIB $CLONK_LIBS"], [AC_MSG_ERROR([$WIN32LIB not found.])])
  done
  
  # FMod (Library comes with standard)
  if test "$enable_sound" = yes; then
    AC_DEFINE([USE_FMOD], 1, [Wether FMOD shall be used])
  fi
  AM_CONDITIONAL(USE_FMOD, test "$enable_sound" = yes)
  
  # windres (check needed for crosscompilation)
  AC_CHECK_TOOL(WINDRES, windres, no)
  if test $WINDRES = no; then
    AC_MSG_ERROR([windres not found.])
  fi
fi

AC_SUBST(CLONK_LIBS)
AC_ARG_VAR(OPENSSL_LIBS, [linker flags for libcrypto])

# precompiled headers and -Wextra
AC_MSG_CHECKING([whether we are using a GNU C++ compiler version 4.1 or newer])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [[#if defined(__GNUC__) && (((__GNUC__ >= 4) && (__GNUC_MINOR__ >= 1)) || (__GNUC__ > 4))
// all ok
#else
       choke me
#endif
]])], RECENT_GCC="yes", RECENT_GCC="no")
AC_MSG_RESULT([$RECENT_GCC])
AM_CONDITIONAL(RECENT_GCC, [test $RECENT_GCC = yes])

AC_OUTPUT

echo "Configuration:
  Debug:	$enable_debug
  Debugrec:	$enable_debugrec
  Pure console:	$enable_console
  DirectX:	$with_directx
  OpenGL:	$with_gl
  Sound:	$enable_sound
  Network:	$enable_network
  GTK+: 	$with_gtk
  X11:  	$have_x"

if test "$enable_sdlmainloop" = yes; then echo "  SDL:  	mainloop";
else echo "  SDL:  	$have_sdl"; fi
if test "$with_internal_libjpeg" = yes; then echo "  libjpeg:	internal";
else echo "  libjpeg:	external"; fi
if test "$with_internal_libpng" = yes; then echo "  libpng:	internal";
else echo "  libpng:	external"; fi
if test "$with_internal_libfreetype" = yes; then echo "  freetype:	internal";
else if test $FREETYPE_CONFIG; then echo "  freetype:	external";
else echo "  freetype:	no"; fi fi
