/*
 * main.h
 *
 * PWLib application header file for PluginTest
 *
 * Copyright 2003 Equivalence
 *
 * $Revision: 20385 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:40:38 +0000 (Wed, 04 Jun 2008) $
 */

#ifndef _PluginTest_MAIN_H
#define _PluginTest_MAIN_H




class PluginTest : public PProcess
{
  PCLASSINFO(PluginTest, PProcess)

  public:
    PluginTest();
    void Main();
};


#endif  // _PluginTest_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
