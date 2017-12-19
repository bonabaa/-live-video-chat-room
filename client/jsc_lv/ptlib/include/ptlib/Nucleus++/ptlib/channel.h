/*
 * channel.h
 *
 * I/O channel ancestor class.
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
 * $Revision: 20385 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:40:38 +0000 (Wed, 04 Jun 2008) $
 */

#ifndef _PCHANNEL

#ifndef __NUCLEUS_MNT__
#pragma interface
#endif

#include <ptlib/mutex.h>

#include "../../channel.h"

  public:
    enum {
      PXReadBlock,
      PXWriteBlock,
      PXAcceptBlock,
      PXConnectBlock
    };

  protected:
    PBoolean PXSetIOBlock(int type, const PTimeInterval & timeout);
    PBoolean PXSetIOBlock(int type, int blockHandle, const PTimeInterval & timeout);

    int  PXClose();

    PString channelName;

#ifdef P_PTHREADS
#define P_IO_BREAK_SIGNAL SIGPROF
  protected:
    PMutex    PX_readMutex;
    pthread_t PX_readThreadId;
#endif
};

#endif
