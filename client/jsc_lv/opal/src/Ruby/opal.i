%module opal

  %{
    /* Includes the header in the wrapper code */
    #include "opal.h"
  %}

  %include "typemaps.i"

  %feature("autodoc", "3");

  /* Parse the header file to generate wrappers */
  %include "opal.h"
