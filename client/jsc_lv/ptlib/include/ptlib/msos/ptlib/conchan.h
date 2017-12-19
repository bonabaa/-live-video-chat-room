/*
 * conchan.h
 *
 * Console I/O channel class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 23005 $
 * $Author: rjongbloed $
 * $Date: 2009-06-29 04:07:13 +0000 (Mon, 29 Jun 2009) $
 */

///////////////////////////////////////////////////////////////////////////////
// PConsoleChannel

  public:
    ~PConsoleChannel();

    // Overrides from class PChannel
    virtual PString GetName() const;
      // Return the name of the channel.


    virtual PBoolean Read(void * buf, PINDEX len);
      // Low level read from the channel. This function will block until the
      // requested number of characters were read.

    virtual PBoolean Write(const void * buf, PINDEX len);
      // Low level write to the channel. This function will block until the
      // requested number of characters were written.

    virtual PBoolean Close();
      // Close the channel.

  protected:
    HANDLE m_hConsole;

// End Of File ///////////////////////////////////////////////////////////////
