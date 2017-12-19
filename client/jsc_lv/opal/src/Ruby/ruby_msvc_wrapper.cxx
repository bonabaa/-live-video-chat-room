/*
 * ruby_msvc_wrapper.c
 *
 * Ruby interface for OPAL
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2009 Vox Lucida Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <opal/buildopts.h>

#if OPAL_RUBY

#if _MSC_VER != RUBY_MSC_VER

  #define MAKESTRING_(s) #s
  #define MAKESTRING(s) MAKESTRING_(s)
  #pragma message("Using MSVC " MAKESTRING(_MSC_VER) " while the Ruby interface is expecting MSVC " MAKESTRING(RUBY_MSC_VER) ", faking it!")
  #undef MAKESTRING_
  #undef MAKESTRING

  #undef _MSC_VER
  #define _MSC_VER RUBY_MSC_VER
#endif

#define NT 1
#define IMPORT 1

#pragma comment(lib, RUBY_LIBRARY)

#pragma warning(disable:4005 4054 4100 4115 4127 4201 4214 4244 4273 4305 4702)

#include "ruby_swig_wrapper.inc"

#endif

