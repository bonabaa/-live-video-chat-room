/*
 * main.h
 *
 * OPAL application source file for playing RTP from a PCAP file
 *
 * Copyright (c) 2007 Equivalence Pty. Ltd.
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
 * $Revision: 20134 $
 * $Author: csoutheren $
 * $Date: 2008-04-28 03:45:00 +0000 (Mon, 28 Apr 2008) $
 */

#ifndef _PlayRTP_MAIN_H
#define _PlayRTP_MAIN_H


class PlayRTP : public PProcess
{
  PCLASSINFO(PlayRTP, PProcess)

  public:
    PlayRTP();
    ~PlayRTP();

    virtual void Main();
    void Play(const PFilePath & filename);

    PDECLARE_NOTIFIER(OpalMediaCommand, PlayRTP, OnTranscoderCommand);

    std::map<RTP_DataFrame::PayloadTypes, OpalMediaFormat> m_payloadType2mediaFormat;

    PIPSocket::Address m_srcIP;
    PIPSocket::Address m_dstIP;

    WORD m_srcPort;
    WORD m_dstPort;
    bool m_singleStep;
    bool m_info;

    OpalTranscoder     * m_transcoder;
    PSoundChannel      * m_player;
    PVideoOutputDevice * m_display;
};


#endif  // _PlayRTP_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
