/*
 * pipechan.h
 *
 * Sub-process with communications using pipe I/O channel class.
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
 * $Revision: 23897 $
 * $Author: rjongbloed $
 * $Date: 2009-12-24 01:02:12 +0000 (Thu, 24 Dec 2009) $
 */

#ifndef PTLIB_PIPECHANNEL_H
#define PTLIB_PIPECHANNEL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/channel.h>


/**A channel that uses a operating system pipe between the current process and
   a sub-process. On platforms that support <i>multi-processing</i>, the
   sub-program is executed concurrently with the calling process.
   
   Where full multi-processing is not supported then the sub-program is run
   with its input supplied from, or output captured to, a disk file. The
   current process is then suspended during the execution of the sub-program.
   In the latter case the semantics of the #Execute()# and #Close()#
   functions change from the usual for channels.

   Note that for platforms that do not support multi-processing, the current
   process is suspended until the sub-program terminates. The input and output
   of the sub-program is transferred via a temporary file. The exact moment of
   execution of the sub-program depends on the mode. If mode is
   #ReadOnly# then it is executed immediately and its output
   captured. In #WriteOnly# mode the sub-program is run when the
   #Close()# function is called, or when the pipe channel is destroyed.
   In #ReadWrite# mode the sub-program is run when the
   #Execute()# function is called indicating that the output from the
   current process to the sub-program has completed and input is now desired.
   
   The #CanReadAndWrite()# function effectively determines whether full
   multi-processing is supported by the platform. Note that this is different
   to whether <i>multi-threading</i> is supported.
 */
 
class PChannel;


class PPipeChannel : public PChannel
{
  PCLASSINFO(PPipeChannel, PChannel);

  public:
  /**@name Construction */
  //@{
    /// Channel mode for the pipe to the sub-process.
    enum OpenMode {
      /// Pipe is only from the sub-process to the current process.
      ReadOnly,   
      /// Pipe is only from the current process to the sub-process.
      WriteOnly,  
      /// Pipe is bidirectional between current and sub-processes.
      ReadWrite,  
      /**Pipe is bidirectional between current and sub-processes but 
         sub-processes stdout and stderr goes to current processes
         stdout and stderr */
      ReadWriteStd
    };

    /**Create a new pipe channel.
     */
    PPipeChannel();
    /**Create a new pipe channel.
       This executes the subProgram and transfers data from its stdin/stdout/stderr.
       
       See the #Open()# function for details of various parameters.
     */
    PPipeChannel(
      const PString & subProgram,  ///< Sub program name or command line.
      OpenMode mode = ReadWrite,   ///< Mode for the pipe channel.
      PBoolean searchPath = PTrue,      ///< Flag for system PATH to be searched.
      PBoolean stderrSeparate = PFalse  ///< Standard error is on separate pipe
    );
    /**Create a new pipe channel.
       This executes the subProgram and transfers data from its stdin/stdout/stderr.
       
       See the #Open()# function for details of various parameters.
     */
    PPipeChannel(
      const PString & subProgram,  ///< Sub program name or command line.
      const PStringArray & argumentList, ///< Array of arguments to sub-program.
      OpenMode mode = ReadWrite,   ///< Mode for the pipe channel.
      PBoolean searchPath = PTrue,      ///< Flag for system PATH to be searched.
      PBoolean stderrSeparate = PFalse  ///< Standard error is on separate pipe
    );
    /**Create a new pipe channel.
       This executes the subProgram and transfers data from its stdin/stdout/stderr.
       
       See the #Open()# function for details of various parameters.
     */
    PPipeChannel(
      const PString & subProgram,  ///< Sub program name or command line.
      const PStringToString & environment, ///< Array of arguments to sub-program.
      OpenMode mode = ReadWrite,   ///< Mode for the pipe channel.
      PBoolean searchPath = PTrue,      ///< Flag for system PATH to be searched.
      PBoolean stderrSeparate = PFalse  ///< Standard error is on separate pipe
    );
    /**Create a new pipe channel.
       This executes the subProgram and transfers data from its stdin/stdout/stderr.
       
       See the #Open()# function for details of various parameters.
     */
    PPipeChannel(
      const PString & subProgram,  ///< Sub program name or command line.
      const PStringArray & argumentList, ///< Array of arguments to sub-program.
      const PStringToString & environment, ///< Array of arguments to sub-program.
      OpenMode mode = ReadWrite,   ///< Mode for the pipe channel.
      PBoolean searchPath = PTrue,      ///< Flag for system PATH to be searched.
      PBoolean stderrSeparate = PFalse  ///< Standard error is on separate pipe
    );

    /// Close the pipe channel, killing the sub-process.
    ~PPipeChannel();
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Determine if the two objects refer to the same pipe channel. This
       actually compares the sub-program names that are passed into the
       constructor.

       @return
       Comparison value of the sub-program strings.
     */
    Comparison Compare(
      const PObject & obj   ///< Another pipe channel to compare against.
    ) const;
  //@}


  /**@name Overrides from class PChannel */
  //@{
    /**Get the name of the channel.
    
       @return
       string for the sub-program that is run.
     */
    virtual PString GetName() const;

    /**Low level read from the channel. This function may block until the
       requested number of characters were read or the read timeout was
       reached. The GetLastReadCount() function returns the actual number
       of bytes read.

       If there are no more characters available as the sub-program has
       stopped then the number of characters available is returned. This is
       similar to end of file for the PFile channel.

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

    /**Low level write to the channel. This function will block until the
       requested number of characters are written or the write timeout is
       reached. The GetLastWriteCount() function returns the actual number
       of bytes written.

       If the sub-program has completed its run then this function will fail
       returning PFalse.

       The GetErrorCode() function should be consulted after Write() returns
       PFalse to determine what caused the failure.

       @return
       PTrue if at least len bytes were written to the channel.
     */
    virtual PBoolean Write(
      const void * buf, ///< Pointer to a block of memory to write.
      PINDEX len        ///< Number of bytes to write.
    );

    /**Close the channel. This will kill the sub-program's process (on
       platforms where that is relevent).
       
       For #WriteOnly# or #ReadWrite# mode pipe channels
       on platforms that do no support concurrent multi-processing and have
       not yet called the #Execute()# function this will run the
       sub-program.
     */
    virtual PBoolean Close();
  //@}

  /**@name New member functions */
  //@{
    /** Open a channel. */
    PBoolean Open(
      const PString & subProgram,  ///< Sub program name or command line.
      OpenMode mode = ReadWrite,   ///< Mode for the pipe channel.
      PBoolean searchPath = PTrue,      ///< Flag for system PATH to be searched.
      PBoolean stderrSeparate = PFalse  ///< Standard error is on separate pipe
    );
    /** Open a channel. */
    PBoolean Open(
      const PString & subProgram,  ///< Sub program name or command line.
      const PStringArray & argumentList, ///< Array of arguments to sub-program.
      OpenMode mode = ReadWrite,   ///< Mode for the pipe channel.
      PBoolean searchPath = PTrue,      ///< Flag for system PATH to be searched.
      PBoolean stderrSeparate = PFalse  ///< Standard error is on separate pipe
    );
    /** Open a channel. */
    PBoolean Open(
      const PString & subProgram,  ///< Sub program name or command line.
      const PStringToString & environment, ///< Array of arguments to sub-program.
      OpenMode mode = ReadWrite,   ///< Mode for the pipe channel.
      PBoolean searchPath = PTrue,      ///< Flag for system PATH to be searched.
      PBoolean stderrSeparate = PFalse  ///< Standard error is on separate pipe
    );
    /**Open a new pipe channel allowing the subProgram to be executed and
       data transferred from its stdin/stdout/stderr.
       
       If the mode is #ReadOnly# then the #stdout# of the
       sub-program is supplied via the #Read()# calls of the PPipeChannel.
       The sub-programs input is set to the platforms null device (eg
       /dev/nul).

       If mode is #WriteOnly# then #Write()# calls of the
       PPipeChannel are suppied to the sub-programs #stdin# and its
       #stdout# is sent to the null device.
       
       If mode is #ReadWrite# then both read and write actions can
       occur.

       The #subProgram# parameter may contain just the path of the
       program to be run or a program name and space separated arguments,
       similar to that provided to the platforms command processing shell.
       Which use of this parameter is determiend by whether arguments are
       passed via the #argumentPointers# or
       #argumentList# parameters.

       The #searchPath# parameter indicates that the system PATH
       for executables should be searched for the sub-program. If PFalse then
       only the explicit or implicit path contained in the
       #subProgram# parameter is searched for the executable.

       The #stderrSeparate# parameter indicates that the standard
       error stream is not included in line with the standard output stream.
       In this case, data in this stream must be read using the
       #ReadStandardError()# function.

       The #environment# parameter is a null terminated sequence
       of null terminated strings of the form name=value. If NULL is passed
       then the same invironment as calling process uses is passed to the
       child process.
     */
    PBoolean Open(
      const PString & subProgram,  ///< Sub program name or command line.
      const PStringArray & argumentList, ///< Array of arguments to sub-program.
      const PStringToString & environment, ///< Array of arguments to sub-program.
      OpenMode mode = ReadWrite,   ///< Mode for the pipe channel.
      PBoolean searchPath = PTrue,      ///< Flag for system PATH to be searched.
      PBoolean stderrSeparate = PFalse  ///< Standard error is on separate pipe
    );

    /**Get the full file path for the sub-programs executable file.

       @return
       file path name for sub-program.
     */
    const PFilePath & GetSubProgram() const;

    /**Start execution of sub-program for platforms that do not support
       multi-processing, this will actually run the sub-program passing all
       data written to the PPipeChannel.
       
       For platforms that do support concurrent multi-processing this will
       close the pipe from the current process to the sub-process.
      
       As the sub-program is run immediately and concurrently, this will just
       give an end of file to the stdin of the remote process. This is often
       necessary.

       @return PTrue if execute was successful.
     */
    PBoolean Execute();

    /**Determine if the program associated with the PPipeChannel is still
       executing. This is useful for determining the status of PPipeChannels
       which take a long time to execute on operating systems which support
       concurrent multi-processing.
       
       @return
       PTrue if the program associated with the PPipeChannel is still running
     */
    PBoolean IsRunning() const;

    /**Get the return code from the most recent Close;

       @return
       Return code from the closing process, if the process is still running
              then -2 is returned. If the process never started due to some
              error then -1 is returned.
     */
    int GetReturnCode() const;

    /**This function will block and wait for the sub-program to terminate.
    
       @return
       Return code from the closing process
     */
    int WaitForTermination();
    
    /**This function will block and wait for the sub-program to terminate.
       It will wait only for the specified amount of time.
    
       @return
       Return code from the closing process, -1 if timed out.
     */
    int WaitForTermination(
      const PTimeInterval & timeout  ///< Amount of time to wait for process.
    );

    /**This function will terminate the sub-program using the signal code
       specified.
     
       @return
       PTrue if the process received the signal. Note that this does not mean
       that the process has actually terminated.
     */
    PBoolean Kill(
      int signal = 9  ///< Signal code to be sent to process.
    );

    /**Read all available data on the standard error stream of the
       sub-process. If the #wait# parameter is PFalse then only
       the text currently available is returned. If PTrue then the function
       blocks as long as necessary to get some number of bytes.

       @return
       PTrue indicates that at least one character was read from stderr.
       PFalse means no bytes were read due to timeout or some other I/O error.
     */
    PBoolean ReadStandardError(
      PString & errors,   ///< String to receive standard error text.
      PBoolean wait = PFalse   ///< Flag to indicate if function should block
    );

    /**Determine if the platform can support simultaneous read and writes from
       the PPipeChannel (eg MSDOS returns PFalse, Unix returns PTrue).
       
       @return
       PTrue if platform supports concurrent multi-processing.
     */
    static PBoolean CanReadAndWrite();
  //@}


  protected:
    // Member variables
    /// The fully qualified path name for the sub-program executable.
    PFilePath subProgName;


  private:
    PBoolean PlatformOpen(const PString & subProgram,
                      const PStringArray & arguments,
                      OpenMode mode,
                      PBoolean searchPath,
                      PBoolean stderrSeparate,
                      const PStringToString * environment);


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/pipechan.h"
#else
#include "unix/ptlib/pipechan.h"
#endif
};


#endif // PTLIB_PIPECHANNEL_H


// End Of File ///////////////////////////////////////////////////////////////
