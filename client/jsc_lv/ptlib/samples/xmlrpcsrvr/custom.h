/*
 * custom.h
 *
 * PWLib application header file for xmlrpcsrvr
 *
 * Customisable application configurationfor OEMs.
 *
 * Copyright 2002 Equivalence
 *
 * $Revision: 20385 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:40:38 +0000 (Wed, 04 Jun 2008) $
 */

#include <ptclib/httpsvc.h>

enum {
  SkName, SkCompany, SkEMail,
  NumSecuredKeys
};


extern PHTTPServiceProcess::Info ProductInfo;


// End of File ///////////////////////////////////////////////////////////////
