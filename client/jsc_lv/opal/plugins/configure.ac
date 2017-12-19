AC_INIT(configure.ac)

if test \! -e ./aclocal.m4 ; then
  echo "ERROR: you must run 'aclocal' before 'autoconf'"
  exit
fi

PKG_PROG_PKG_CONFIG()
AC_CANONICAL_TARGET()

m4_include(../opal.m4)

OPAL_DETERMINE_DEBUG()
CFLAGS="$CFLAGS $DEFAULT_CFLAGS"
CXXFLAGS="$CXXFLAGS $DEFAULT_CFLAGS"

AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_CHECK_TOOL([NM], [nm], [:])
AC_CHECK_TOOL([LDD], [ldd], [:])
AC_CHECK_TOOL([OBJDUMP], [objdump], [:])

AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h limits.h float.h fcntl.h])

dnl Enable compiler warnings
dnl if test "x$ac_cv_c_compiler_gnu" = xyes; then
dnl for later...
dnl  CFLAGS="$CFLAGS -Wall -Wextra -Winit-self -Wswitch-default -Wswitch-enum -Wstrict-aliasing=2 -Wfloat-equal -Wshadow -Wno-comment -Wno-missing-field-initializers -Wno-unused"
dnl  CXXFLAGS="$CXXFLAGS -Wall -Wextra -Winit-self -Wswitch-default -Wswitch-enum -Wstrict-aliasing=2 -Wfloat-equal -Wshadow -Wno-comment -Wno-missing-field-initializers  -Wno-unused"
dnl fi

dnl ########################################################################
dnl Directories
dnl ########################################################################

OPAL_DETERMINE_VERSION([..])
OPAL_DETERMINE_PLUGIN_DIR()

AC_PLUGIN_DIR="${PLUGIN_DIR}/codecs/audio"
VC_PLUGIN_DIR="${PLUGIN_DIR}/codecs/video"
LID_PLUGIN_DIR="${PLUGIN_DIR}/lid"
FAX_PLUGIN_DIR="${PLUGIN_DIR}/fax"

AC_SUBST(prefix)
AC_SUBST(exec_prefix)
AC_SUBST(libdir)
AC_SUBST(target_os)
AC_SUBST(AC_PLUGIN_DIR)
AC_SUBST(VC_PLUGIN_DIR)
AC_SUBST(LID_PLUGIN_DIR)
AC_SUBST(FAX_PLUGIN_DIR)

dnl ########################################################################
dnl Platform specific section
dnl ########################################################################
WIN32=no
LDSO=
case "$target_os" in
	solaris*)
          STDCCFLAGS="$STDCCFLAGS -KPIC"
	  LDSO="-Bdynamic -G -h"
          PLUGINEXT=so
          AC_CHECK_HEADERS([sys/time.h sys/file.h netinet/in.h])
          AC_FUNC_FORK()
        ;;

        darwin*)  
	  CFLAGS="$CFLAGS -fno-common -dynamic"
          LDSO="-dynamiclib -multiply_defined suppress"
	  STDCCFLAGS="$STDCCFLAGS -fPIC -D__MACOSX__"
	  case "$target_cpu" in
	   x86 | i686 | i586 | i486 | i386 ) STDCCFLAGS="${STDCCFLAGS} -m486" ;;
          esac
	  PLUGINEXT=dylib 
          AC_CHECK_HEADERS([sys/time.h sys/file.h netinet/in.h])
          AC_FUNC_FORK()
        ;;

        cygwin*|mingw**)
	  LDSO="-module -avoid-version -shared"
	  PLUGINEXT=dll
	  WIN32=yes
          AC_CHECK_HEADERS([malloc.h])
          AC_DEFINE([PLUGIN_CODEC_DLL_EXPORTS], [1], [For win32 build])
        ;;

        *)
          STDCCFLAGS="$STDCCFLAGS -fPIC"
          LDSO='-shared -Wl,-soname,$(SONAME)'
          PLUGINEXT=so
          AC_CHECK_HEADERS([sys/time.h sys/file.h netinet/in.h])
          AC_FUNC_FORK()
        ;;
esac
AC_SUBST(LDSO)
AC_SUBST(PLUGINEXT)
AC_SUBST(CFLAGS)
AC_SUBST(STDCCFLAGS)
AC_SUBST(WIN32)


dnl ########################################################################
dnl Defaults
dnl ########################################################################

HAVE_VPB=yes
HAVE_CAPI=yes
HAVE_IXJ=no
HAVE_H263=yes
HAVE_H264=yes
HAVE_SPANDSP=yes
DEFAULT_TO_FULL_CAPABILITIES=yes
X264_LINK_STATIC=no
LIBAVCODEC_STACKALIGN_HACK=no

case "$target_os" in
        darwin*) HAVE_VPB=no 
                 HAVE_CAPI=no 
                 ;;
        cygwin*|mingw**)
	         HAVE_VPB=no 
                 HAVE_CAPI=no 
                 X264_LINK_STATIC=yes
                 ;;
esac

dnl ########################################################################
dnl Audio Codecs
dnl ########################################################################

dnl     ########################
dnl     libgsm (system/local)
dnl     ########################
AC_ARG_ENABLE([localgsm],
              [AC_HELP_STRING([--enable-localgsm],[Force use local version of GSM library rather than system version])],
              [localgsm=$enableval],
              [localgsm=])


if test "x${localgsm}" = "xyes" ; then
  AC_MSG_NOTICE(Forcing use of local GSM sources)
  GSM_SYSTEM=no

elif test "x${localgsm}" = "xno" ; then
  AC_MSG_NOTICE(Forcing use of system GSM library)
  OPAL_FIND_GSM([HAVE_GSM=yes], [HAVE_GSM=no])
  if test "x${HAVE_GSM}" != "xyes" ; then
    AC_MSG_ERROR([cannot find system GSM])
  else
    GSM_SYSTEM=yes
  fi

else
  OPAL_FIND_GSM([HAVE_GSM=yes], [HAVE_GSM=no])
  if test "x${HAVE_GSM}" = "xyes" ; then
    GSM_SYSTEM=yes
  else
    GSM_SYSTEM=no
  fi
fi

if test "x${GSM_SYSTEM}" = "xyes" ; then
  AC_SUBST(GSM_CFLAGS)
  AC_SUBST(GSM_LIBS)
  AC_MSG_RESULT(using system GSM)
fi
AC_SUBST(GSM_SYSTEM)

dnl     ########################
dnl     speex (system/local)
dnl     ########################
OPAL_SPEEX_TYPES()
AC_SUBST(SIZE16)
AC_SUBST(SIZE32)
OPAL_DETERMINE_SPEEX()
AC_SUBST(SPEEX_SYSTEM)
AC_SUBST(SPEEX_CFLAGS)
AC_SUBST(SPEEX_LIBS)

dnl ########################################################################
dnl Video Codecs
dnl ########################################################################

dnl     ########################
dnl     ffmpeg for H.263
dnl     #######################
OPAL_SIMPLE_OPTION([ffmpeg-h263],[HAVE_H263], [enable plugin supporting H.263 via patched FFMPEG])

dnl     ########################
dnl     Capabilities option
dnl     ########################
OPAL_SIMPLE_OPTION([default-to-full-capabilties],[DEFAULT_TO_FULL_CAPABILITIES], [default to full capabilities at empty fmtp lines])

dnl     ########################
dnl     libavcodec (ffmpeg)
dnl     ########################
PKG_CHECK_MODULES([LIBAVCODEC],
                  [libavcodec >= 51.11.0],
                  [
                   AC_SUBST(LIBAVCODEC_CFLAGS)
                   AC_SUBST(LIBAVCODEC_LIBS)
                   AC_DEFINE([HAVE_LIBAVCODEC], [1], [ffmpeg])
                   HAVE_LIBAVCODEC=yes
                   OPAL_LIBAVCODEC_HACK
                   case "$target_os" in
                      cygwin* | mingw* ) 
                                         LIBAVCODEC_LIB_NAME="avcodec.dll"
                                         HAVE_H263P=yes
                                         HAVE_MPEG4=yes
                                         HAVE_H264_DECODER=yes
                                         ;;
                                     * ) 
                                         OPAL_GET_LIBNAME([LIBAVCODEC], [libavcodec], [$LIBAVCODEC_LIBS])
                                         OPAL_CHECK_LIBAVCODEC([h263p_encoder], [HAVE_H263P=yes], [HAVE_H263P=no])
                                         OPAL_CHECK_LIBAVCODEC([mpeg4_encoder], [HAVE_MPEG4=yes], [HAVE_MPEG4=no])
                                         OPAL_CHECK_LIBAVCODEC([h264_decoder], [HAVE_H264_DECODER=yes], [HAVE_H264_DECODER=no])
                                         ;;
                   esac
                   AC_SUBST(HAVE_H263P)
                   AC_SUBST(HAVE_MPEG4)
                   AC_SUBST(LIBAVCODEC_LIB_NAME)
                   OPAL_LIBAVCODEC_HEADER
                  ],
                  [
                   HAVE_LIBAVCODEC=no
                  ]
                 )
AC_SUBST(HAVE_LIBAVCODEC)

OPAL_LIBAVCODEC_SOURCE()
AC_SUBST(LIBAVCODEC_SOURCE_DIR)

dnl     ########################
dnl     x264
dnl     ########################
PKG_CHECK_MODULES([X264],
                  [x264 >= 0.48],
                  [
                   AC_SUBST(X264_CFLAGS)
                   AC_SUBST(X264_LIBS)
                   HAVE_X264=yes
                   OPAL_GET_LIBNAME([X264], [libx264], [$X264_LIBS])
                   AC_SUBST(X264_LIB_NAME)
                   OPAL_X264_LINKAGE
                  ],
                  [
                   HAVE_X264=no
                  ]
                 )
AC_SUBST(X264_LINK_STATIC)
AC_SUBST(HAVE_X264)

dnl     ########################
dnl     theora
dnl     ########################
PKG_CHECK_MODULES([THEORA],
                  [theora],
                  [
                   AC_SUBST(THEORA_CFLAGS)
                   AC_SUBST(THEORA_LIBS)
                   AC_DEFINE([HAVE_THEORA], [1], [theora])
                   HAVE_THEORA=yes
                  ],
                  [
                   HAVE_THEORA=no
                  ])
AC_SUBST(HAVE_THEORA)

dnl     ########################
dnl     LIBDL
dnl     ########################
OPAL_FIND_LIBDL([HAVE_LIBDL=yes], [HAVE_LIBDL=no])
AC_SUBST(HAVE_LIBDL)
AC_SUBST(DL_LIBS)

dnl     ########################
dnl     H.264
dnl     ########################
if test "x$X264_LINK_STATIC" != "xyes"&& test "x$HAVE_LIBDL" != "xyes" ; then
  AC_MSG_NOTICE(H.264 support disabled since static linking was disabled and libdl was not found)
  HAVE_H264=no
else
  OPAL_SIMPLE_OPTION([h264],[HAVE_H264], [enable H.264 support], [HAVE_X264], [HAVE_H264_DECODER])
fi

dnl ########################################################################
dnl LID Devices
dnl ########################################################################

dnl     ########################
dnl     Quicknet xJACK cards
dnl     #######################
AC_ARG_ENABLE([ixj],
              [AC_HELP_STRING([--enable-ixj],[enable xJack cards])],
              [HAVE_IXJ=$enableval])

if test "x$HAVE_IXJ" = "xyes" ; then
OPAL_MSG_CHECK([IXJ], [$HAVE_IXJ])
  if test "x$HAVE_IXJ" = "xyes" ; then
    AC_CHECK_HEADERS([linux/telephony.h], 
		     [HAVE_IXJ=yes],
		     [AC_CHECK_HEADERS([ sys/telephony.h], [HAVE_IXJ=yes], [HAVE_IXJ=no])])
    if test "x$HAVE_IXJ" = "xyes" ; then
      AC_TRY_COMPILE([#include <linux/ixjuser.h>],
                     [int main(int ac,char **av) { return 0; }], [HAVE_IXJ=yes], [HAVE_IXJ=no])
    fi
  fi
fi
OPAL_MSG_CHECK([IXJ], [$HAVE_IXJ])

AC_SUBST(HAVE_IXJ)


dnl     ########################
dnl     VoiceTronics VPB card
dnl     ########################
OPAL_SIMPLE_OPTION([vpb],[HAVE_VPB], [enable voicetronix vpb])

dnl     ########################
dnl     CAPI
dnl     ########################
AC_ARG_ENABLE([capi],
              [AC_HELP_STRING([--enable-capi],[enable CAPI])],
              [HAVE_CAPI=$enableval])

if test "x$HAVE_CAPI" = "xyes" ; then
  AC_CHECK_HEADERS([capi20.h], [HAVE_CAPI=yes], [HAVE_CAPI=no])
fi

OPAL_MSG_CHECK([CAPI], [$HAVE_CAPI])
AC_SUBST(HAVE_CAPI)

dnl ########################################################################
dnl FAX Devices
dnl ########################################################################

dnl     ########################
dnl     spanDSP
dnl     ########################
AC_ARG_ENABLE([spandsp],
              [AC_HELP_STRING([--enable-spandsp],[enable spanDSP-based fax support])],
              [HAVE_SPANDSP=$enableval])

if test "x${HAVE_SPANDSP}" = "xyes" ; then
    OPAL_FIND_SPANDSP([HAVE_SPANDSP=yes], [HAVE_SPANDSP=no])
fi

AC_SUBST(HAVE_SPANDSP)
AC_SUBST(SPANDSP_LIBS)
dnl ########################################################################

if test -d audio/GSM0610 ; then
  AC_CONFIG_FILES(audio/GSM0610/Makefile)
fi
if test -d audio/gsm-amr ; then
  AC_CONFIG_FILES(audio/gsm-amr/Makefile)
fi
if test -d audio/iLBC ; then
  AC_CONFIG_FILES(audio/iLBC/Makefile)
fi
if test -d audio/Speex ; then
  AC_CONFIG_FILES(audio/Speex/Makefile)
fi
if test -d audio/LPC_10 ; then
  AC_CONFIG_FILES(audio/LPC_10/Makefile)
fi
if test -d audio/G726 ; then
  AC_CONFIG_FILES(audio/G726/Makefile)
fi
if test -d audio/IMA_ADPCM ; then
  AC_CONFIG_FILES(audio/IMA_ADPCM/Makefile)
fi
if test -d video/H.261-vic ; then
  AC_CONFIG_FILES(video/H.261-vic/Makefile)
fi
if test -d video/H.263-ffmpeg ; then
  AC_CONFIG_FILES(video/H.263-ffmpeg/Makefile)
fi
if test -d video/H.263-1998 ; then
  AC_CONFIG_FILES(video/H.263-1998/Makefile)
fi
if test -d video/MPEG4-ffmpeg ; then
  AC_CONFIG_FILES(video/MPEG4-ffmpeg/Makefile)
fi
if test -d video/H.264 ; then
  AC_CONFIG_FILES(video/H.264/Makefile)
fi
if test -d video/H.264/gpl; then
  AC_CONFIG_FILES(video/H.264/gpl/Makefile)
fi
if test -d video/THEORA ; then
  AC_CONFIG_FILES(video/THEORA/Makefile)
fi
if test -d LID/IxJ ; then
  AC_CONFIG_FILES(LID/IxJ/Makefile)
fi
if test -d LID/VPB ; then
  AC_CONFIG_FILES(LID/VPB/Makefile)
fi
if test -d LID/CAPI ; then
  AC_CONFIG_FILES(LID/CAPI/Makefile)
fi
if test -d fax/fax_spandsp -a -d fax/fax_spandsp/spandsp_util ; then
  AC_CONFIG_FILES(fax/fax_spandsp/Makefile)
fi

AC_CONFIG_FILES(Makefile)

dnl ########################################################################
dnl Header file

dnl ########################################################################
dnl Output configured files

AC_OUTPUT(audio/Speex/libspeex/speex_config_types.h)
AC_CONFIG_HEADERS(plugin-config.h)
AC_OUTPUT()

dnl ########################################################################
dnl Summary
dnl ########################################################################
echo ""
echo "=================== Plugin configuration ======================"
echo " prefix                           : $prefix"
echo " exec_prefix                      : $exec_prefix"
echo " libdir                           : $libdir"
echo " Audio Codec plugin install dir   : "'${libdir}'"/$AC_PLUGIN_DIR"
echo " Video Codec plugin install prefix: "'${libdir}'"/$VC_PLUGIN_DIR"
echo " LID plugin install prefix        : "'${libdir}'"/$LID_PLUGIN_DIR"
echo " FAX plugin install prefix        : "'${libdir}'"/$FAX_PLUGIN_DIR"

echo ""
if test "x${GSM_SYSTEM}" = "xyes" ; then
echo "                              GSM :  yes (system)"
else
echo "                              GSM :  yes (internal)"
fi

echo "                          gsm-amr :  yes"
echo "                             iLBC :  yes"
if test "x${SPEEX_SYSTEM}" = "xyes" ; then
echo "                            Speex :  yes (system, version $SPEEX_SYSTEM_VERSION)"
else
echo "                            Speex :  yes (internal, version $SPEEX_INTERNAL_VERSION)"
fi
echo "                            LPC10 :  yes"
echo "                            G.726 :  yes"
echo "                        IMA_ADPCM :  yes"

echo ""

echo "                           H.261  :  yes"
echo "                           H.263  :  ${HAVE_H263}"
echo "                           H.263+ :  ${HAVE_H263P}"
echo "                           H.264  :  ${HAVE_H264}"
echo "                          THEORA  :  ${HAVE_THEORA}"

if test "x${HAVE_MPEG4}" = "xyes" ; then
if test "x${LIBAVCODEC_SOURCE_DIR}" = "x" ; then
echo "                    MPEG4 Part 2  :  yes (no rate control correction)"
else
echo "                    MPEG4 Part 2  :  yes"
fi
else
echo "                    MPEG4 Part 2  :  no"
fi

echo ""
echo "                     SpanDSP FAX  :  $HAVE_SPANDSP"

echo ""

echo "                            CAPI  :  $HAVE_CAPI"
echo "           Quicknet xJACK support :  $HAVE_IXJ"
echo "         Voicetronics VPB support :  $HAVE_VPB"

echo ""

echo "                         OS Type  :  $target_os"
echo "                    Machine Type  :  $target_cpu"
echo ""
echo " If all settings are OK, type make and make install "
echo "========================================================"
