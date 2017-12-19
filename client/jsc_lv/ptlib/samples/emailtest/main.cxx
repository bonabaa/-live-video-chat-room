/*
 * main.cxx
 *
 * PWLib application source file for emailtest
 *
 * Main program entry point.
 *
 * Copyright (c) 2004 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 20385 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:40:38 +0000 (Wed, 04 Jun 2008) $
 */

#include "precompile.h"
#include "main.h"
#include "version.h"

#include <ptlib/sockets.h>
#include <ptclib/inetmail.h>

PCREATE_PROCESS(Emailtest);

Emailtest::Emailtest()
  : PProcess("Post Increment", "emailtest", MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
}

void Emailtest::Main()
{
  PArgList & args = GetArguments();

  args.Parse(
             "h-help.   "
             "v-version.    "
             "s-server:"
             "t-to:"
             "f-from:"
             "r-re:"
             "a-attachment:"

#if PTRACING
             "o-output:"             "-no-output."
             "t-trace."              "-no-trace."
#endif
  );
 
  if (args.HasOption('h')) {
    cout << "usage: " <<  (const char *)GetName()
         << endl
         << "     -s or --server srv     : set mail server" << endl
         << "     -t or --to     dst     : set to address" << endl
         << "     -f or --from   frm     : set from address " << endl
         << "     -r or --re     sbj     : set subject " << endl
         << "     -a or --attachment atc : list of attachments " << endl
         << "     -h or --help           : get help on usage " << endl
         << "     -v or --version        : report program version " << endl
#if PTRACING
         << "  -t --trace   : Enable trace, use multiple times for more detail" << endl
         << "  -o --output  : File for trace output, default is stderr" << endl
#endif
         << endl;
    return;
  }

 if (args.HasOption('v')) {
    cout << endl
         << "Product Name: " <<  (const char *)GetName() << endl
         << "Manufacturer: " <<  (const char *)GetManufacturer() << endl
         << "Version     : " <<  (const char *)GetVersion(PTrue) << endl
         << "System      : " <<  (const char *)GetOSName() << '-'
         <<  (const char *)GetOSHardware() << ' '
         <<  (const char *)GetOSVersion() << endl
         << endl;
    return;
  }
  

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  PRFC822Channel email(PRFC822Channel::Sending);

  PString to = args.GetOptionString("to");
  PString from = args.GetOptionString("from");

  email.SetToAddress(to);
  email.SetFromAddress(from);
  email.SetSubject(args.GetOptionString("re"));

  PStringArray attachments = args.GetOptionString("attachment").Lines();

  PString server = args.GetOptionString("server");
  if (server.IsEmpty())
    server = "127.0.0.1";

  PTCPSocket socket("smtp 25");
  if (!socket.Connect(server)) {
    PError << "error: could not connect to SMTP server " << server << endl;
    return;
  }

  PSMTPClient smtpClient;
  if (!smtpClient.Open(socket)) {
    PError << "error: could not open SMTP server " << server << endl;
    return;
  }

  if (!email.Open(smtpClient)) {
    PError << "error: cannot open email message " << server << endl;
    return;
  }

  if (!smtpClient.BeginMessage(from, to)) {
    PError << "error: could not begin SMTP message " << smtpClient.GetErrorText() << endl;
    return;
  }

  PString boundary;
  if (attachments.GetSize() > 0) {
    boundary = email.MultipartMessage();
  }

  for (PINDEX i = 0; i < args.GetCount(); ++i) {
    email.Write((const char *)args[i], args[i].GetLength());
    email << "\n";
  }

  if (attachments.GetSize() > 0) {
    for (PINDEX i = 0; i < attachments.GetSize(); ++i) {
      PFilePath filename = attachments[i];
      PFile file(filename, PFile::ReadOnly);
      if (file.IsOpen()) {
        email.NextPart(boundary);
        email.SetContentAttachment(filename.GetFileName());
        PString fileType = filename.GetType();
        PString contentType = PMIMEInfo::GetContentType(fileType);
        if ((fileType *= "txt") || (fileType == "html"))
          email.SetTransferEncoding("7bit", PFalse);
        else
          email.SetTransferEncoding("base64", PTrue);
        BYTE buffer[1024];
        for (;;) {
          if (!file.Read(buffer, sizeof(buffer)))
            break;
          email.Write(buffer, file.GetLastReadCount());
        }
      }
    }
  }

  smtpClient.EndMessage();

  email.Close();
}


// End of File ///////////////////////////////////////////////////////////////
