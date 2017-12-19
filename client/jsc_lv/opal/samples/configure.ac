AC_INIT
AC_CONFIG_SRCDIR([include/opal/manager.h])

m4_include(../opal.m4)

AC_ARG_ENABLE([samples-static-linking],
              [AC_HELP_STRING([--enable-samples-static-linking],[Link the samples to static opal and ptlib libraries])],
              [OPAL_SAMPLES_STATIC=$enableval],
              [OPAL_SAMPLES_STATIC=no])

AC_ARG_ENABLE([samples-debug],
              [AC_HELP_STRING([--enable-debug],[Enable samples debug flags and link to debug opal and ptlib libraries.])],
              [OPAL_SAMPLES_DEBUG=$enableval],
              [OPAL_SAMPLES_DEBUG=no])

if test "x$OPAL_SAMPLES_DEBUG" = xyes; then
  if test "x$OPAL_SAMPLES_STATIC" = xyes; then
    SAMPLES_LIBS="-lopal_d_s -lpt_d_s"
  else
    SAMPLES_LIBS="-lopal_d -lpt_d"
  fi
  SAMPLES_DEBUG=yes
else
  if test "x$OPAL_SAMPLES_STATIC" = xyes; then
    SAMPLES_LIBS="-lopal_s -lpt_s"
  else
    SAMPLES_LIBS="-lopal -lpt"
  fi
  SAMPLES_DEBUG=no
fi


AC_SUBST(SAMPLES_LIBS)
AC_SUBST(SAMPLES_DEBUG)

AC_CONFIG_FILES(opal_samples.mak)
AC_OUTPUT
