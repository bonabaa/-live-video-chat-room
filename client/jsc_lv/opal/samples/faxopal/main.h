/*
 * main.h
 *
 * OPAL application source file for sending/receiving faxes via T.38
 *
 * Copyright (c) 2008 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 20534 $
 * $Author: rjongbloed $
 * $Date: 2008-07-04 03:47:05 +0000 (Fri, 04 Jul 2008) $
 */

#ifndef _FaxOPAL_MAIN_H
#define _FaxOPAL_MAIN_H


class MyManager : public OpalManager
{
    PCLASSINFO(MyManager, OpalManager)

  public:
    virtual void OnClearedCall(OpalCall & call); // Callback override

    PSyncPoint m_completed;
};


class FaxOPAL : public PProcess
{
    PCLASSINFO(FaxOPAL, PProcess)

  public:
    FaxOPAL();
    ~FaxOPAL();

    virtual void Main();

  private:
    MyManager * m_manager;
};


#endif  // _FaxOPAL_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
