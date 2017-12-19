/*
 * url.cxx
 *
 * URL parsing classes.
 *
 * Portable Tools Library
 *
 * Copyright (c) 1993-2008 Equivalence Pty. Ltd.
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
 * $Revision: 23915 $
 * $Author: rjongbloed $
 * $Date: 2010-01-11 03:12:43 +0000 (Mon, 11 Jan 2010) $
 */

#ifdef __GNUC__
#pragma implementation "url.h"
#endif

#include <ptlib.h>

#if P_URL

#define P_DISABLE_FACTORY_INSTANCES
#include <ptlib/pfactory.h>

#include <ptclib/url.h>

#include <ptlib/sockets.h>
#include <ctype.h>

#if defined(_WIN32) && !defined(_WIN32_WCE)
#include <shellapi.h>
#pragma comment(lib,"shell32.lib")
#endif


// RFC 1738
// http://host:port/path...
// https://host:port/path....
// gopher://host:port
// wais://host:port
// nntp://host:port
// prospero://host:port
// ftp://user:password@host:port/path...
// telnet://user:password@host:port
// file://hostname/path...

// mailto:user@hostname
// news:string

#define DEFAULT_FTP_PORT      21
#define DEFAULT_TELNET_PORT   23
#define DEFAULT_GOPHER_PORT   70
#define DEFAULT_HTTP_PORT     80
#define DEFAULT_NNTP_PORT     119
#define DEFAULT_WAIS_PORT     210
#define DEFAULT_HTTPS_PORT    443
#define DEFAULT_RTSP_PORT     554
#define DEFAULT_RTSPU_PORT    554
#define DEFAULT_PROSPERO_PORT 1525
#define DEFAULT_H323_PORT     1720
#define DEFAULT_H323S_PORT    1300
#define DEFAULT_H323RAS_PORT  1719
#define DEFAULT_MSRP_PORT     2855
#define DEFAULT_RTMP_PORT     1935
#define DEFAULT_SIP_PORT      5060
#define DEFAULT_SIPS_PORT     5061

#define DEFINE_LEGACY_URL_SCHEME(schemeName, user, pass, host, def, defhost, query, params, frags, path, rel, port) \
  class PURLLegacyScheme_##schemeName : public PURLLegacyScheme \
  { \
    public: \
      PURLLegacyScheme_##schemeName() \
        : PURLLegacyScheme(#schemeName, user, pass, host, def, defhost, query, params, frags, path, rel, port) \
        { } \
  }; \
  static PFactory<PURLScheme>::Worker<PURLLegacyScheme_##schemeName> schemeName##Factory(#schemeName, true); \

//                       schemeName,user,   passwd, host,   defUser,defhost, query,  params, frags,  path,   rel,    port
DEFINE_LEGACY_URL_SCHEME(http,      PTrue,  PTrue,  PTrue,  PFalse, PTrue,   PTrue,  PTrue,  PTrue,  PTrue,  PTrue,  DEFAULT_HTTP_PORT )
DEFINE_LEGACY_URL_SCHEME(file,      PFalse, PFalse, PTrue,  PFalse, PTrue,   PFalse, PFalse, PFalse, PTrue,  PFalse, 0)
DEFINE_LEGACY_URL_SCHEME(https,     PFalse, PFalse, PTrue,  PFalse, PTrue,   PTrue,  PTrue,  PTrue,  PTrue,  PTrue,  DEFAULT_HTTPS_PORT)
DEFINE_LEGACY_URL_SCHEME(gopher,    PFalse, PFalse, PTrue,  PFalse, PTrue,   PFalse, PFalse, PFalse, PTrue,  PFalse, DEFAULT_GOPHER_PORT)
DEFINE_LEGACY_URL_SCHEME(wais,      PFalse, PFalse, PTrue,  PFalse, PFalse,  PFalse, PFalse, PFalse, PTrue,  PFalse, DEFAULT_WAIS_PORT)
DEFINE_LEGACY_URL_SCHEME(nntp,      PFalse, PFalse, PTrue,  PFalse, PTrue,   PFalse, PFalse, PFalse, PTrue,  PFalse, DEFAULT_NNTP_PORT)
DEFINE_LEGACY_URL_SCHEME(prospero,  PFalse, PFalse, PTrue,  PFalse, PTrue,   PFalse, PFalse, PFalse, PTrue,  PFalse, DEFAULT_PROSPERO_PORT)
DEFINE_LEGACY_URL_SCHEME(rtsp,      PFalse, PFalse, PTrue,  PFalse, PTrue,   PTrue,  PFalse, PFalse, PTrue,  PFalse, DEFAULT_RTSP_PORT)
DEFINE_LEGACY_URL_SCHEME(rtspu,     PFalse, PFalse, PTrue,  PFalse, PTrue,   PFalse, PFalse, PFalse, PTrue,  PFalse, DEFAULT_RTSPU_PORT)
DEFINE_LEGACY_URL_SCHEME(ftp,       PTrue,  PTrue,  PTrue,  PFalse, PTrue,   PFalse, PFalse, PFalse, PTrue,  PFalse, DEFAULT_FTP_PORT)
DEFINE_LEGACY_URL_SCHEME(telnet,    PTrue,  PTrue,  PTrue,  PFalse, PTrue,   PFalse, PFalse, PFalse, PFalse, PFalse, DEFAULT_TELNET_PORT)
DEFINE_LEGACY_URL_SCHEME(mailto,    PFalse, PFalse, PFalse, PFalse, PTrue,   PTrue,  PFalse, PFalse, PFalse, PFalse, 0)
DEFINE_LEGACY_URL_SCHEME(news,      PFalse, PFalse, PFalse, PFalse, PTrue,   PFalse, PFalse, PFalse, PFalse, PFalse, 0)
DEFINE_LEGACY_URL_SCHEME(h323,      PTrue,  PFalse, PTrue,  PTrue,  PFalse,  PFalse, PTrue,  PFalse, PFalse, PFalse, DEFAULT_H323_PORT)
DEFINE_LEGACY_URL_SCHEME(h323s,     PTrue,  PFalse, PTrue,  PTrue,  PFalse,  PFalse, PTrue,  PFalse, PFalse, PFalse, DEFAULT_H323S_PORT)
DEFINE_LEGACY_URL_SCHEME(rtmp,      PFalse, PFalse, PTrue,  PFalse, PFalse,  PFalse, PFalse, PFalse, PTrue,  PFalse, DEFAULT_RTMP_PORT)
DEFINE_LEGACY_URL_SCHEME(sip,       PTrue,  PTrue,  PTrue,  PFalse, PFalse,  PTrue,  PTrue,  PFalse, PFalse, PFalse, DEFAULT_SIP_PORT)
DEFINE_LEGACY_URL_SCHEME(sips,      PTrue,  PTrue,  PTrue,  PFalse, PFalse,  PTrue,  PTrue,  PFalse, PFalse, PFalse, DEFAULT_SIPS_PORT)
DEFINE_LEGACY_URL_SCHEME(tel,       PFalse, PFalse, PFalse, PTrue,  PFalse,  PFalse, PTrue,  PFalse, PFalse, PFalse, 0)
DEFINE_LEGACY_URL_SCHEME(fax,       PFalse, PFalse, PFalse, PTrue,  PFalse,  PFalse, PTrue,  PFalse, PFalse, PFalse, 0)
DEFINE_LEGACY_URL_SCHEME(callto,    PFalse, PFalse, PFalse, PTrue,  PFalse,  PFalse, PTrue,  PFalse, PFalse, PFalse, 0)
DEFINE_LEGACY_URL_SCHEME(msrp,      false,  false,  true,   false,  false,   true,   true,   false,  true,   false,  DEFAULT_MSRP_PORT)

#define DEFAULT_SCHEME "http"
#define FILE_SCHEME    "file"

//////////////////////////////////////////////////////////////////////////////
// PURL

PURL::PURL()
  : //scheme(SchemeTable[DEFAULT_SCHEME].name),
    scheme(DEFAULT_SCHEME),
    port(0),
    portSupplied (PFalse),
    relativePath(PFalse)
{
}


PURL::PURL(const char * str, const char * defaultScheme)
{
  Parse(str, defaultScheme);
}


PURL::PURL(const PString & str, const char * defaultScheme)
{
  Parse(str, defaultScheme);
}


PURL::PURL(const PFilePath & filePath)
  : scheme(FILE_SCHEME),
    port(0),
    portSupplied (PFalse),
    relativePath(PFalse)
{
  PStringArray pathArray = filePath.GetDirectory().GetPath();
  if (pathArray.IsEmpty())
    return;

  if (pathArray[0].GetLength() == 2 && pathArray[0][1] == ':')
    pathArray[0][1] = '|';

  pathArray.AppendString(filePath.GetFileName());

  SetPath(pathArray);
}


PObject::Comparison PURL::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PURL), PInvalidCast);
  return urlString.Compare(((const PURL &)obj).urlString);
}

PURL::PURL(const PURL & other)
{
  CopyContents(other);
}

PURL & PURL::operator=(const PURL & other)
{
  CopyContents(other);
  return *this;
}

void PURL::CopyContents(const PURL & other)
{
  urlString    = other.urlString;
  scheme       = other.scheme;
  username     = other.username;
  password     = other.password;
  hostname     = other.hostname;
  port         = other.port;
  portSupplied = other.portSupplied;
  relativePath = other.relativePath;
  path         = other.path;
  fragment     = other.fragment;

  paramVars    = other.paramVars;
  paramVars.MakeUnique();

  queryVars    = other.queryVars;
  queryVars.MakeUnique();

  m_contents   = other.m_contents;
}

PINDEX PURL::HashFunction() const
{
  return urlString.HashFunction();
}


void PURL::PrintOn(ostream & stream) const
{
  stream << urlString;
}


void PURL::ReadFrom(istream & stream)
{
  PString s;
  stream >> s;
  Parse(s);
}


PString PURL::TranslateString(const PString & str, TranslationType type)
{
  PString xlat = str;

  /* Characters sets are from RFC2396.
     The EBNF defines lowalpha, upalpha, digit and mark which are always
     allowed. The reserved list consisting of ";/?:@&=+$," may or may not be
     allowed depending on the syntatic element being encoded.
   */
  PString safeChars = "abcdefghijklmnopqrstuvwxyz"  // lowalpha
                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"  // upalpha
                      "0123456789"                  // digit
                      "-_.!~*'()";                  // mark
  switch (type) {
    case LoginTranslation :
      safeChars += ";&=+$,";  // Section 3.2.2
      break;

    case PathTranslation :
      safeChars += ":@&=+$,|";   // Section 3.3
      break;

    case QueryTranslation :
      break;    // Section 3.4, no reserved characters may be used
  }
  PINDEX pos = (PINDEX)-1;
  while ((pos = xlat.FindSpan(safeChars, pos+1)) != P_MAX_INDEX)
    xlat.Splice(psprintf("%%%02X", (BYTE)xlat[pos]), pos, 1);

  return xlat;
}


PString PURL::UntranslateString(const PString & str, TranslationType type)
{
  PString xlat = str;
  xlat.MakeUnique();

  PINDEX pos;
  if (type == PURL::QueryTranslation) {
    /* Even though RFC2396 never mentions this, and RFC1630 is quite vague
       about it, a lot of things do it so we have to do it too */
    pos = (PINDEX)-1;
    while ((pos = xlat.Find('+', pos+1)) != P_MAX_INDEX)
      xlat[pos] = ' ';
  }

  pos = (PINDEX)-1;
  while ((pos = xlat.Find('%', pos+1)) != P_MAX_INDEX) {
    int digit1 = xlat[pos+1];
    int digit2 = xlat[pos+2];
    if (isxdigit(digit1) && isxdigit(digit2)) {
      xlat[pos] = (char)(
            (isdigit(digit2) ? (digit2-'0') : (toupper(digit2)-'A'+10)) +
           ((isdigit(digit1) ? (digit1-'0') : (toupper(digit1)-'A'+10)) << 4));
      xlat.Delete(pos+1, 2);
    }
  }

  return xlat;
}


void PURL::SplitVars(const PString & str, PStringToString & vars, char sep1, char sep2)
{
  vars.RemoveAll();

  PINDEX sep1prev = 0;
  do {
    PINDEX sep1next = str.Find(sep1, sep1prev);
    if (sep1next == P_MAX_INDEX)
      sep1next--; // Implicit assumption string is not a couple of gigabytes long ...

    PINDEX sep2pos = str.Find(sep2, sep1prev);
    if (sep2pos > sep1next)
      sep2pos = sep1next;

    PCaselessString key = PURL::UntranslateString(str(sep1prev, sep2pos-1), PURL::QueryTranslation);
    if (!key) {
      PString data = PURL::UntranslateString(str(sep2pos+1, sep1next-1), PURL::QueryTranslation);

      if (vars.Contains(key))
        vars.SetAt(key, vars[key] + ',' + data);
      else
        vars.SetAt(key, data);
    }

    sep1prev = sep1next+1;
  } while (sep1prev != P_MAX_INDEX);
}


PBoolean PURL::InternalParse(const char * cstr, const char * defaultScheme)
{
  scheme.MakeEmpty();
  username.MakeEmpty();
  password.MakeEmpty();
  hostname.MakeEmpty();
  port = 0;
  portSupplied = PFalse;
  relativePath = PFalse;
  path.SetSize(0);
  paramVars.RemoveAll();
  fragment.MakeEmpty();
  queryVars.RemoveAll();
  m_contents.MakeEmpty();

  // copy the string so we can take bits off it
  while (((*cstr & 0x80) == 0x00) && isspace(*cstr))
    cstr++;
  PString url = cstr;

  // Character set as per RFC2396
  PINDEX pos = 0;
  while ( ((*cstr & 0x80) != 0x00) || isalnum(url[pos]) || url[pos] == '+' || url[pos] == '-' || url[pos] == '.')
    pos++;

  PString schemeName;

  // get information which tells us how to parse URL for this
  // particular scheme
  PURLScheme * schemeInfo = NULL;

  // Determine if the URL has an explicit scheme
  if (url[pos] == ':') {
    // get the scheme information
    schemeInfo = PFactory<PURLScheme>::CreateInstance(url.Left(pos));
    if (schemeInfo != NULL)
      url.Delete(0, pos+1);
  }

  // if we could not match a scheme, then use the specified default scheme
  if (schemeInfo == NULL && defaultScheme != NULL) {
    schemeInfo = PFactory<PURLScheme>::CreateInstance(defaultScheme);
    PAssert(schemeInfo != NULL, "Default scheme " + PString(defaultScheme) + " not available");
  }

  // if that still fails, then there is nowehere to go
  if (schemeInfo == NULL)
    return false;

  scheme = schemeInfo->GetName();
  return schemeInfo->Parse(url, *this) && !IsEmpty();
}

PBoolean PURL::LegacyParse(const PString & _url, const PURLLegacyScheme * schemeInfo)
{
  PString url = _url;
  PINDEX pos;

  // Super special case!
  if (scheme *= "callto") {

    // Actually not part of MS spec, but a lot of people put in the // into
    // the URL, so we take it out of it is there.
    if (url.GetLength() > 2 && url[0] == '/' && url[1] == '/')
      url.Delete(0, 2);

    // For some bizarre reason callto uses + instead of ; for paramters
    // We do a loop so that phone numbers of the form +61243654666 still work
    do {
      pos = url.Find('+');
    } while (pos != P_MAX_INDEX && isdigit(url[pos+1]));

    if (pos != P_MAX_INDEX) {
      SplitVars(url(pos+1, P_MAX_INDEX), paramVars, '+', '=');
      url.Delete(pos, P_MAX_INDEX);
    }

    hostname = paramVars("gateway");
    if (!hostname)
      username = UntranslateString(url, LoginTranslation);
    else {
      PCaselessString type = paramVars("type");
      if (type == "directory") {
        pos = url.Find('/');
        if (pos == P_MAX_INDEX)
          username = UntranslateString(url, LoginTranslation);
        else {
          hostname = UntranslateString(url.Left(pos), LoginTranslation);
          username = UntranslateString(url.Mid(pos+1), LoginTranslation);
        }
      }
      else {
        // Now look for an @ and split user and host
        pos = url.Find('@');
        if (pos != P_MAX_INDEX) {
          username = UntranslateString(url.Left(pos), LoginTranslation);
          hostname = UntranslateString(url.Mid(pos+1), LoginTranslation);
        }
        else {
          if (type == "ip" || type == "host")
            hostname = UntranslateString(url, LoginTranslation);
          else
            username = UntranslateString(url, LoginTranslation);
        }
      }
    }

    // Allow for [ipv6] form
    pos = hostname.Find(']');
    if (pos == P_MAX_INDEX)
      pos = 0;
    pos = hostname.Find(':', pos);
    if (pos != P_MAX_INDEX) {
      port = (WORD)hostname.Mid(pos+1).AsUnsigned();
      portSupplied = PTrue;
      hostname.Delete(pos, P_MAX_INDEX);
    }

    password = paramVars("password");
    return PTrue;
  }

  // if the URL should have leading slash, then remove it if it has one
  if (schemeInfo != NULL && schemeInfo->hasHostPort && schemeInfo->hasPath) {
    if (url.GetLength() > 2 && url[0] == '/' && url[1] == '/')
      url.Delete(0, 2);
    else
      relativePath = PTrue;
  }

  // parse user/password/host/port
  if (!relativePath && schemeInfo->hasHostPort) {
    PString endHostChars;
    if (schemeInfo->hasPath)
      endHostChars += '/';
    if (schemeInfo->hasQuery)
      endHostChars += '?';
    if (schemeInfo->hasParameters)
      endHostChars += ';';
    if (schemeInfo->hasFragments)
      endHostChars += '#';
    if (endHostChars.IsEmpty())
      pos = P_MAX_INDEX;
    else if (schemeInfo->hasUsername) {
      //';' showing in the username field should be valid.
      // Looking for ';' after the '@' for the parameters.
      PINDEX posAt = url.Find('@');
      if (posAt != P_MAX_INDEX)
        pos = url.FindOneOf(endHostChars, posAt);
      else 
        pos = url.FindOneOf(endHostChars);
    }
    else
      pos = url.FindOneOf(endHostChars);

    PString uphp = url.Left(pos);
    if (pos != P_MAX_INDEX)
      url.Delete(0, pos);
    else
      url.MakeEmpty();

    // if the URL is of type UserPasswordHostPort, then parse it
    if (schemeInfo->hasUsername) {
      // extract username and password
      PINDEX pos2 = uphp.Find('@');
      PINDEX pos3 = P_MAX_INDEX;
      if (schemeInfo->hasPassword)
        pos3 = uphp.Find(':');
      switch (pos2) {
        case 0 :
          uphp.Delete(0, 1);
          break;

        case P_MAX_INDEX :
          if (schemeInfo->defaultToUserIfNoAt) {
            if (pos3 == P_MAX_INDEX)
              username = UntranslateString(uphp, LoginTranslation);
            else {
              username = UntranslateString(uphp.Left(pos3), LoginTranslation);
              password = UntranslateString(uphp.Mid(pos3+1), LoginTranslation);
            }
            uphp.MakeEmpty();
          }
          break;

        default :
          if (pos3 > pos2)
            username = UntranslateString(uphp.Left(pos2), LoginTranslation);
          else {
            username = UntranslateString(uphp.Left(pos3), LoginTranslation);
            password = UntranslateString(uphp(pos3+1, pos2-1), LoginTranslation);
          }
          uphp.Delete(0, pos2+1);
      }
    }

    // if the URL does not have a port, then this is the hostname
    if (schemeInfo->defaultPort == 0)
      hostname = UntranslateString(uphp, LoginTranslation);
    else {
      // determine if the URL has a port number
      // Allow for [ipv6] form
      pos = uphp.Find(']');
      if (pos == P_MAX_INDEX)
        pos = 0;
      pos = uphp.Find(':', pos);
      if (pos == P_MAX_INDEX)
        hostname = UntranslateString(uphp, LoginTranslation);
      else {
        hostname = UntranslateString(uphp.Left(pos), LoginTranslation);
        port = (WORD)uphp.Mid(pos+1).AsUnsigned();
        portSupplied = PTrue;
      }

      if (hostname.IsEmpty() && schemeInfo->defaultHostToLocal)
        hostname = PIPSocket::GetHostName();
    }
  }

  if (schemeInfo->hasQuery) {
    // chop off any trailing query
    pos = url.Find('?');
    if (pos != P_MAX_INDEX) {
      SplitQueryVars(url(pos+1, P_MAX_INDEX), queryVars);
      url.Delete(pos, P_MAX_INDEX);
    }
  }

  if (schemeInfo->hasParameters) {
    // chop off any trailing parameters
    pos = url.Find(';');
    if (pos != P_MAX_INDEX) {
      SplitVars(url(pos+1, P_MAX_INDEX), paramVars, ';', '=');
      url.Delete(pos, P_MAX_INDEX);
    }
  }

  if (schemeInfo->hasFragments) {
    // chop off any trailing fragment
    pos = url.Find('#');
    if (pos != P_MAX_INDEX) {
      fragment = UntranslateString(url(pos+1, P_MAX_INDEX), PathTranslation);
      url.Delete(pos, P_MAX_INDEX);
    }
  }

  if (schemeInfo->hasPath)
    SetPathStr(url);   // the hierarchy is what is left
  else {
    // if the rest of the URL isn't a path, then we are finished!
    m_contents = UntranslateString(url, PathTranslation);
    Recalculate();
  }

  if (port == 0 && schemeInfo->defaultPort != 0 && !relativePath) {
    // Yes another horrible, horrible special case!
    if (scheme == "h323" && paramVars("type") == "gk")
      port = DEFAULT_H323RAS_PORT;
    else
      port = schemeInfo->defaultPort;
    Recalculate();
  }

  return PTrue;
}


PFilePath PURL::AsFilePath() const
{
  /* While it is never explicitly stated anywhere in RFC1798, there is an
     implication RFC 1808 that the path is absolute unless the relative
     path rules of that RFC apply. We follow that logic. */

  if (path.IsEmpty() || scheme != FILE_SCHEME || (!hostname.IsEmpty() && hostname != "localhost"))
    return PString::Empty();

  PStringStream str;

  if (path[0].GetLength() == 2 && path[0][1] == '|')
    str << path[0][0] << ':' << PDIR_SEPARATOR; // Special case for Windows paths with drive letter
  else {
    if (!relativePath)
      str << PDIR_SEPARATOR;
    str << path[0];
  }

  for (PINDEX i = 1; i < path.GetSize(); i++)
    str << PDIR_SEPARATOR << path[i];

  return str;
}


PString PURL::AsString(UrlFormat fmt) const
{
  if (fmt == FullURL)
    return urlString;

  if (scheme.IsEmpty())
    return PString::Empty();

  const PURLScheme * schemeInfo = PFactory<PURLScheme>::CreateInstance(scheme);
  if (schemeInfo == NULL)
    schemeInfo = PFactory<PURLScheme>::CreateInstance(DEFAULT_SCHEME);

  return schemeInfo->AsString(fmt, *this);
}

PString PURL::LegacyAsString(PURL::UrlFormat fmt, const PURLLegacyScheme * schemeInfo) const
{
  PStringStream str;
  PINDEX i;

  if (fmt == HostPortOnly) {
    str << scheme << ':';

    if (relativePath) {
      if (schemeInfo->relativeImpliesScheme)
        return PString::Empty();
      return str;
    }

    if (schemeInfo->hasPath && schemeInfo->hasHostPort)
      str << "//";

    if (schemeInfo->hasUsername) {
      if (!username) {
        str << TranslateString(username, LoginTranslation);
        if (schemeInfo->hasPassword && !password)
          str << ':' << TranslateString(password, LoginTranslation);
        str << '@';
      }
    }

    if (schemeInfo->hasHostPort) {
      if (hostname.Find(':') != P_MAX_INDEX && hostname[0] != '[')
        str << '[' << hostname << ']';
      else
        str << hostname;
    }

    if (schemeInfo->defaultPort != 0) {
      if (port != schemeInfo->defaultPort || portSupplied)
        str << ':' << port;
    }

    // Problem was fixed for handling legacy schema like tel URI.
    // HostPortOnly format: if there is no default user and host fields, only the schema itself is being returned.
    // URIOnly only format: the pathStr will be retruned.
    // The Recalculate() will merge both HostPortOnly and URIOnly formats for the completed uri string creation.
    if (schemeInfo->defaultToUserIfNoAt)
      return str;

    if (str.GetLength() > scheme.GetLength()+1)
      return str;

    // Cannot JUST have the scheme: ....
    return PString::Empty();
  }

  // URIOnly and PathOnly
  if (schemeInfo->hasPath)
    str << GetPathStr();
  else
    str << TranslateString(m_contents, PathTranslation);

  if (fmt == URIOnly) {
    if (!fragment)
      str << "#" << TranslateString(fragment, PathTranslation);

    for (i = 0; i < paramVars.GetSize(); i++) {
      str << ';' << TranslateString(paramVars.GetKeyAt(i), QueryTranslation);
      PString data = paramVars.GetDataAt(i);
      if (!data)
        str << '=' << TranslateString(data, QueryTranslation);
    }

    if (!queryVars.IsEmpty())
      str << '?' << GetQuery();
  }

  return str;
}


void PURL::SetScheme(const PString & s)
{
  scheme = s;
  Recalculate();
}


void PURL::SetUserName(const PString & u)
{
  username = u;
  Recalculate();
}


void PURL::SetPassword(const PString & p)
{
  password = p;
  Recalculate();
}


void PURL::SetHostName(const PString & h)
{
  hostname = h;
  Recalculate();
}


void PURL::SetPort(WORD newPort)
{
  port = newPort;
  portSupplied = true;
  Recalculate();
}


void PURL::SetPathStr(const PString & pathStr)
{
  path = pathStr.Tokenise("/", PTrue);

  if (path.GetSize() > 0 && path[0].IsEmpty()) 
    path.RemoveAt(0);

  for (PINDEX i = 0; i < path.GetSize(); i++) {
    path[i] = UntranslateString(path[i], PathTranslation);
    if (i > 0 && path[i] == ".." && path[i-1] != "..") {
      path.RemoveAt(i--);
      path.RemoveAt(i--);
    }
  }

  Recalculate();
}


PString PURL::GetPathStr() const
{
  PStringStream strm;
  for (PINDEX i = 0; i < path.GetSize(); i++) {
    if (i > 0 || !relativePath)
      strm << '/';
    strm << TranslateString(path[i], PathTranslation);
  }
  return strm;
}


void PURL::SetPath(const PStringArray & p)
{
  path = p;
  Recalculate();
}


void PURL::AppendPath(const PString & segment)
{
  path.AppendString(segment);
  Recalculate();
}


PString PURL::GetParameters() const
{
  PStringStream str;

  for (PINDEX i = 0; i < paramVars.GetSize(); i++) {
    if (i > 0)
      str << ';';
    str << TranslateString(paramVars.GetKeyAt(i), QueryTranslation);
    PString data = paramVars.GetDataAt(i);
    if (!data)
      str << '=' << TranslateString(data, QueryTranslation);
  }

  return str;
}


void PURL::SetParameters(const PString & parameters)
{
  SplitVars(parameters, paramVars, ';', '=');
  Recalculate();
}


void PURL::SetParamVars(const PStringToString & p)
{
  paramVars = p;
  Recalculate();
}


void PURL::SetParamVar(const PString & key, const PString & data, bool emptyDataDeletes)
{
  if (emptyDataDeletes && data.IsEmpty())
    paramVars.RemoveAt(key);
  else
    paramVars.SetAt(key, data);
  Recalculate();
}


PString PURL::GetQuery() const
{
  PStringStream str;

  for (PINDEX i = 0; i < queryVars.GetSize(); i++) {
    if (i > 0)
      str << '&';
    PString key  = TranslateString(queryVars.GetKeyAt (i), QueryTranslation);
    PString data = TranslateString(queryVars.GetDataAt(i), QueryTranslation);
    if (key.IsEmpty())
      str << data;
    else if (data.IsEmpty())
      str << key;
    else
      str << key << '=' << data;
  }

  return str;
}


void PURL::SetQuery(const PString & queryStr)
{
  SplitQueryVars(queryStr, queryVars);
  Recalculate();
}


void PURL::SetQueryVars(const PStringToString & q)
{
  queryVars = q;
  Recalculate();
}


void PURL::SetQueryVar(const PString & key, const PString & data)
{
  if (data.IsEmpty())
    queryVars.RemoveAt(key);
  else
    queryVars.SetAt(key, data);
  Recalculate();
}


void PURL::SetContents(const PString & str)
{
  m_contents = str;
  Recalculate();
}


PBoolean PURL::OpenBrowser(const PString & url)
{
#ifdef _WIN32
  SHELLEXECUTEINFO sei;
  ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
  sei.cbSize = sizeof(SHELLEXECUTEINFO);
  sei.lpVerb = TEXT("open");
  PVarString file = url;
  sei.lpFile = file;

  if (ShellExecuteEx(&sei) != 0)
    return PTrue;

  PVarString msg = "Unable to open page" & url;
  PVarString name = PProcess::Current().GetName();
  MessageBox(NULL, msg, name, MB_TASKMODAL);

#endif // WIN32
  return PFalse;
}


void PURL::Recalculate()
{
  if (scheme.IsEmpty())
    scheme = DEFAULT_SCHEME;

  urlString = AsString(HostPortOnly) + AsString(URIOnly);
}

#endif // P_URL


// End Of File ///////////////////////////////////////////////////////////////
