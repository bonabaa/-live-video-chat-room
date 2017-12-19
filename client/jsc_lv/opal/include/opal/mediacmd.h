/*
 * mediacmd.h
 *
 * Abstractions for sending commands to media processors.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 */

#ifndef OPAL_OPAL_MEDIACMD_H
#define OPAL_OPAL_MEDIACMD_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

///////////////////////////////////////////////////////////////////////////////

/** This is the base class for a command to a media transcoder and/or media
    stream. The commands are highly context sensitive, for example
    VideoFastUpdate would only apply to a video transcoder.
  */
class OpalMediaCommand : public PObject
{
  PCLASSINFO(OpalMediaCommand, PObject);
  public:
  /**@name Overrides from PObject */
  //@{
    /**Standard stream print function.
       The PObject class has a << operator defined that calls this function
       polymorphically.
      */
    void PrintOn(
      ostream & strm    ///<  Stream to output text representation
    ) const { strm << GetName(); }

    /** Compare the two objects and return their relative rank. This function is
       usually overridden by descendent classes to yield the ranking according
       to the semantics of the object.
       
       The default function is to use the #CompareObjectMemoryDirect()#
       function to do a byte wise memory comparison of the two objects.

       @return
       #LessThan#, #EqualTo# or #GreaterThan#
       according to the relative rank of the objects.
     */
    virtual Comparison Compare(
      const PObject & obj   ///<  Object to compare against.
    ) const { return GetName().Compare(PDownCast(const OpalMediaCommand, &obj)->GetName()); }
  //@}

  /**@name Operations */
  //@{
    /**Get the name of the command.
      */
    virtual PString GetName() const = 0;

    /**Get data buffer pointer for transfer to/from codec plug-in.
      */
    virtual void * GetPlugInData() const { return NULL; }

    /**Get data buffer size for transfer to/from codec plug-in.
      */
    virtual unsigned * GetPlugInSize() const { return NULL; }
  //@}
};


#define OPAL_DEFINE_MEDIA_COMMAND(cls, name) \
  class cls : public OpalMediaCommand \
  { \
	PCLASSINFO(cls, OpalMediaCommand) \
    public: \
      virtual PString GetName() const { return name; } \
  }


#endif // OPAL_OPAL_MEDIACMD_H


// End of File ///////////////////////////////////////////////////////////////
