/*
 * memfile.h
 *
 * WAV file I/O channel class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2002 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is
 * Equivalence Pty Ltd
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21788 $
 * $Author: rjongbloed $
 * $Date: 2008-12-12 05:42:13 +0000 (Fri, 12 Dec 2008) $
 */

#ifndef PTLIB_PMEMFILE_H
#define PTLIB_PMEMFILE_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


/**This class is used to allow a block of memory to substitute for a disk file.
 */
class PMemoryFile : public PFile
{
  PCLASSINFO(PMemoryFile, PFile);
  public:
  /**@name Construction */
  //@{
    /**Create a new, empty, memory file.
      */
    PMemoryFile();

    /**Create a new memory file initialising to the specified content.
      */
    PMemoryFile(
      const PBYTEArray & data  ///< New content filr memory file.
    );
  //@}


  /**@name Overrides from class PObject */
  //@{
    /**Determine the relative rank of the two objects. This is essentially the
       string comparison of the #PFilePath# names of the files.

       @return
       relative rank of the file paths.
     */
    Comparison Compare(
      const PObject & obj   ///< Other file to compare against.
    ) const;
  //@}


  /**@name Overrides from class PChannel */
  //@{
    /**Low level read from the memory file channel. The read timeout is
       ignored.  The GetLastReadCount() function returns the actual number
       of bytes read.

       The GetErrorCode() function should be consulted after Read() returns
       PFalse to determine what caused the failure.

       @return
       PTrue indicates that at least one character was read from the channel.
       PFalse means no bytes were read due to timeout or some other I/O error.
     */
    virtual PBoolean Read(
      void * buf,   ///< Pointer to a block of memory to receive the read bytes.
      PINDEX len    ///< Maximum number of bytes to read into the buffer.
    );

    /**Low level write to the memory file channel. The write timeout is
       ignored. The GetLastWriteCount() function returns the actual number
       of bytes written.

       The GetErrorCode() function should be consulted after Write() returns
       PFalse to determine what caused the failure.

       @return PTrue if at least len bytes were written to the channel.
     */
    virtual PBoolean Write(
      const void * buf, ///< Pointer to a block of memory to write.
      PINDEX len        ///< Number of bytes to write.
    );
  //@}


  /**@name Overrides from class PFile */
  //@{
    /**Get the current size of the file.
       The size of the file corresponds to the size of the data array.

       @return
       length of file in bytes.
     */
    off_t GetLength() const;
      
    /**Set the size of the file, padding with 0 bytes if it would require
       expanding the file, or truncating it if being made shorter.

       @return
       PTrue if the file size was changed to the length specified.
     */
    PBoolean SetLength(
      off_t len   ///< New length of file.
    );

    /**Set the current active position in the file for the next read or write
       operation. The #pos# variable is a signed number which is
       added to the specified origin. For #origin == PFile::Start#
       only positive values for #pos# are meaningful. For
       #origin == PFile::End# only negative values for
       #pos# are meaningful.

       @return
       PTrue if the new file position was set.
     */
    PBoolean SetPosition(
      off_t pos,                         ///< New position to set.
      FilePositionOrigin origin = Start  ///< Origin for position change.
    );

    /**Get the current active position in the file for the next read or write
       operation.

       @return
       current file position relative to start of file.
     */
    off_t GetPosition() const;
  //@}


  /**@name Overrides from class PFile */
  //@{
    /**Get the memory data the file has operated with.
      */
    const PBYTEArray & GetData() const { return data; }
  //@}


  protected:
    PBYTEArray data;
    off_t position;
};


#endif // PTLIB_PMEMFILE_H


// End of File ///////////////////////////////////////////////////////////////
