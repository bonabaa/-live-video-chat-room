/*
 * main.h
 *
 * PWLib application header file for XMLRPCApp
 *
 * Copyright 2002 Equivalence
 *
 * $Revision: 20385 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:40:38 +0000 (Wed, 04 Jun 2008) $
 */

#ifndef _XMLRPCApp_MAIN_H
#define _XMLRPCApp_MAIN_H

#include <ptlib/pprocess.h>

class XMLRPCApp : public PProcess
{
  PCLASSINFO(XMLRPCApp, PProcess)

  public:
    XMLRPCApp();
    void Main();
};


#endif  // _XMLRPCApp_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
