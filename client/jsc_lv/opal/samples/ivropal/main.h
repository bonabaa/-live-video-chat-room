/*
 * main.h
 *
 * OPAL application source file for Interactive Voice Response using VXML
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
 * $Revision: 21163 $
 * $Author: rjongbloed $
 * $Date: 2008-09-25 06:36:06 +0000 (Thu, 25 Sep 2008) $
 */

#ifndef _IvrOPAL_MAIN_H
#define _IvrOPAL_MAIN_H


class MyManager : public OpalManager
{
    PCLASSINFO(MyManager, OpalManager)

  public:
    virtual void OnClearedCall(OpalCall & call); // Callback override

    PSyncPoint m_completed;
};


class IvrOPAL : public PProcess
{
    PCLASSINFO(IvrOPAL, PProcess)

  public:
    IvrOPAL();
    ~IvrOPAL();

    virtual void Main();

  private:
    MyManager * m_manager;
};


#endif  // _IvrOPAL_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
