#pragma once
#include "ptlib\udpsock.h"

class PUDPSocketEx :
  public PUDPSocket
{
public:
  CZ_channel m_channel;
  PUDPSocketEx(void);
  ~PUDPSocketEx(void);
    /**Write a datagram to a remote computer.

       @return PTrue if all the bytes were sucessfully written.
     */
    virtual PBoolean WriteTo(
      const void * buf,   ///< Data to be written as URGENT TCP data.
      PINDEX len,         ///< Number of bytes pointed to by #buf#.
      const Address & addr, ///< Address to which the datagram is sent.
      WORD port           ///< Port to which the datagram is sent.
    );
    /** Override of PChannel functions to allow connectionless reads
     */
    PBoolean Read(
      void * buf,   ///< Pointer to a block of memory to read.
      PINDEX len    ///< Number of bytes to read.
    );
    virtual PBoolean ReadFrom(
      void * buf,     ///< Data to be written as URGENT TCP data.
      PINDEX len,     ///< Number of bytes pointed to by #buf#.
      Address & addr, ///< Address from which the datagram was received.
      WORD & port     ///< Port from which the datagram was received.
    );

};
