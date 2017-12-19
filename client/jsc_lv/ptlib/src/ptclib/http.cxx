/*
 * http.cxx
 *
 * HTTP ancestor class and common classes.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2002 Equivalence Pty. Ltd.
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
 * $Revision: 23806 $
 * $Author: rjongbloed $
 * $Date: 2009-11-27 06:59:05 +0000 (Fri, 27 Nov 2009) $
 */

#ifdef __GNUC__
#pragma implementation "http.h"
#endif

#include <ptlib.h>

#if P_HTTP

#include <ptlib/sockets.h>
#include <ptclib/http.h>
#include <ptclib/url.h>


//////////////////////////////////////////////////////////////////////////////
// PHTTP

static char const * const HTTPCommands[PHTTP::NumCommands] = {
  // HTTP 1.0 commands
  "GET", "HEAD", "POST",

  // HTTP 1.1 commands
  "PUT",  "DELETE", "TRACE", "OPTIONS",

  // HTTPS command
  "CONNECT"
};


const PString & PHTTP::AllowTag           () { static PString s = "Allow"; return s; }
const PString & PHTTP::AuthorizationTag   () { static PString s = "Authorization"; return s; }
const PString & PHTTP::ContentEncodingTag () { static PString s = "Content-Encoding"; return s; }
const PString & PHTTP::ContentLengthTag   () { static PString s = "Content-Length"; return s; }
const PString & PHTTP::DateTag            () { static PString s = "Date"; return s; }
const PString & PHTTP::ExpiresTag         () { static PString s = "Expires"; return s; }
const PString & PHTTP::FromTag            () { static PString s = "From"; return s; }
const PString & PHTTP::IfModifiedSinceTag () { static PString s = "If-Modified-Since"; return s; }
const PString & PHTTP::LastModifiedTag    () { static PString s = "Last-Modified"; return s; }
const PString & PHTTP::LocationTag        () { static PString s = "Location"; return s; }
const PString & PHTTP::PragmaTag          () { static PString s = "Pragma"; return s; }
const PString & PHTTP::PragmaNoCacheTag   () { static PString s = "no-cache"; return s; }
const PString & PHTTP::RefererTag         () { static PString s = "Referer"; return s; }
const PString & PHTTP::ServerTag          () { static PString s = "Server"; return s; }
const PString & PHTTP::UserAgentTag       () { static PString s = "User-Agent"; return s; }
const PString & PHTTP::WWWAuthenticateTag () { static PString s = "WWW-Authenticate"; return s; }
const PString & PHTTP::MIMEVersionTag     () { static PString s = "MIME-Version"; return s; }
const PString & PHTTP::ConnectionTag      () { static PString s = "Connection"; return s; }
const PString & PHTTP::KeepAliveTag       () { static PString s = "Keep-Alive"; return s; }
const PString & PHTTP::TransferEncodingTag() { static PString s = "Transfer-Encoding"; return s; }
const PString & PHTTP::ChunkedTag         () { static PString s = "chunked"; return s; }
const PString & PHTTP::ProxyConnectionTag () { static PString s = "Proxy-Connection"; return s; }
const PString & PHTTP::ProxyAuthorizationTag(){ static PString s = "Proxy-Authorization"; return s; }
const PString & PHTTP::ProxyAuthenticateTag(){ static PString s = "Proxy-Authenticate"; return s; }
const PString & PHTTP::ForwardedTag       () { static PString s = "Forwarded"; return s; }
const PString & PHTTP::SetCookieTag       () { static PString s = "Set-Cookie"; return s; }
const PString & PHTTP::CookieTag          () { static PString s = "Cookie"; return s; }



PHTTP::PHTTP()
  : PInternetProtocol("www 80", NumCommands, HTTPCommands)
{
}


PINDEX PHTTP::ParseResponse(const PString & line)
{
  PINDEX endVer = line.Find(' ');
  if (endVer == P_MAX_INDEX) {
    lastResponseInfo = "Bad response";
    lastResponseCode = PHTTP::InternalServerError;
    return 0;
  }

  lastResponseInfo = line.Left(endVer);
  PINDEX endCode = line.Find(' ', endVer+1);
  lastResponseCode = line(endVer+1,endCode-1).AsInteger();
  if (lastResponseCode == 0)
    lastResponseCode = PHTTP::InternalServerError;
  lastResponseInfo &= line.Mid(endCode);
  return 0;
}

#endif // P_HTTP


// End Of File ///////////////////////////////////////////////////////////////
