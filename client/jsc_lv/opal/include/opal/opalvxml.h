/*
 * opalvxml.h
 *
 * Header file for IVR code
 *
 * A H.323 IVR application.
 *
 * Copyright (C) 2002 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 22971 $
 * $Author: rjongbloed $
 * $Date: 2009-06-25 06:03:42 +0000 (Thu, 25 Jun 2009) $
 */

#ifndef OPAL_OPAL_OPALVXML_H
#define OPAL_OPAL_OPALVXML_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <ptclib/vxml.h>


class OpalConnection;


//////////////////////////////////////////////////////////////////


#if OPAL_PTLIB_VXML

class PTextToSpeech;

class OpalVXMLSession : public PVXMLSession 
{
  PCLASSINFO(OpalVXMLSession, PVXMLSession);
  public:
    OpalVXMLSession(
      OpalConnection * _conn,
      PTextToSpeech * tts = NULL,
      PBoolean autoDelete = PFalse
    );

    virtual PBoolean Close();
    virtual void OnEndSession();
    virtual void OnTransfer(const PString & destination, bool bridged);

    virtual PWAVFile * CreateWAVFile(
      const PFilePath & fn,
      PFile::OpenMode mode,
      int opts,
      unsigned fmt
    );

  protected:
    OpalConnection * conn;
};

#endif

#endif // OPAL_OPAL_OPALVXML_H


// End of File ///////////////////////////////////////////////////////////////
