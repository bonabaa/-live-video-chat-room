/*
 * textfile.h
 *
 * A text file I/O channel class.
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
 * $Revision: 21788 $
 * $Author: rjongbloed $
 * $Date: 2008-12-12 05:42:13 +0000 (Fri, 12 Dec 2008) $
 */

#ifndef PTLIB_TEXTFILE_H
#define PTLIB_TEXTFILE_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


///////////////////////////////////////////////////////////////////////////////
// Text Files

/** A class representing a a structured file that is portable accross CPU
   architectures. Essentially this will normalise the end of line character
   which differs fromplatform to platform.
 */
class PTextFile : public PFile
{
  PCLASSINFO(PTextFile, PFile);

  public:
  /**@name Construction */
  //@{
    /** Create a text file object but do not open it. It does not initially
       have a valid file name. However, an attempt to open the file using the
       #PFile::Open()# function will generate a unique temporary file.
     */
    PTextFile();

    /** Create a unique temporary file name, and open the file in the specified
       mode and using the specified options. Note that opening a new, unique,
       temporary file name in ReadOnly mode will always fail. This would only
       be usefull in a mode and options that will create the file.

       The #PChannel::IsOpen()# function may be used after object
       construction to determine if the file was successfully opened.
     */
    PTextFile(
      OpenMode mode,          ///< Mode in which to open the file.
      int opts = ModeDefault  ///< #OpenOptions enum# for open operation.
    );
      
    /** Create a text file object with the specified name and open it in the
       specified mode and with the specified options.

       The #PChannel::IsOpen()# function may be used after object
       construction to determine if the file was successfully opened.
     */
    PTextFile(
      const PFilePath & name,    ///< Name of file to open.
      OpenMode mode = ReadWrite, ///< Mode in which to open the file.
      int opts = ModeDefault     ///< #OpenOptions enum# for open operation.
    );
  //@}

  /**@name Line I/O functions */
  //@{
    /** Read a line from the text file. What constitutes an end of line in the
       file is platform dependent.
       
       Use the #PChannel::GetLastError()# function to determine if there
       was some error other than end of file.
       
       @return
       PTrue if successful, PFalse if at end of file or a read error.
     */
    PBoolean ReadLine(
      PString & str  ///< String into which line of text is read.
    );

    /** Read a line from the text file. What constitutes an end of line in the
       file is platform dependent.
       
       Use the #PChannel::GetLastError()# function to determine the
       failure mode.

       @return
       PTrue if successful, PFalse if an error occurred.
     */
    PBoolean WriteLine(
      const PString & str  ///< String to write with end of line terminator.
    );
  //@}


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/textfile.h"
#else
#include "unix/ptlib/textfile.h"
#endif
};

#endif // PTLIB_TEXTFILE_H


// End Of File ///////////////////////////////////////////////////////////////
