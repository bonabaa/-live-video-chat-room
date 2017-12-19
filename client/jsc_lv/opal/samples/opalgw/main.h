/*
 * main.h
 *
 * PWLib application header file for OPAL Gateway
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * $Revision: 21283 $
 * $Author: rjongbloed $
 * $Date: 2008-10-11 07:10:58 +0000 (Sat, 11 Oct 2008) $
 */

#ifndef _OpalGw_MAIN_H
#define _OpalGw_MAIN_H


#if OPAL_H323

class MyGatekeeperServer;

class MyGatekeeperCall : public H323GatekeeperCall
{
  PCLASSINFO(MyGatekeeperCall, H323GatekeeperCall);
  public:
    MyGatekeeperCall(
      MyGatekeeperServer & server,
      const OpalGloballyUniqueID & callIdentifier, /// Unique call identifier
      Direction direction
    );
    ~MyGatekeeperCall();

    virtual H323GatekeeperRequest::Response OnAdmission(
      H323GatekeeperARQ & request
    );

#ifdef H323_TRANSNEXUS_OSP
    PBoolean AuthoriseOSPCall(H323GatekeeperARQ & info);
    OpalOSP::Transaction * ospTransaction;
#endif
};



class MyGatekeeperServer : public H323GatekeeperServer
{
    PCLASSINFO(MyGatekeeperServer, H323GatekeeperServer);
  public:
    MyGatekeeperServer(H323EndPoint & ep);

    // Overrides
    virtual H323GatekeeperCall * CreateCall(
      const OpalGloballyUniqueID & callIdentifier,
      H323GatekeeperCall::Direction direction
    );
    virtual PBoolean TranslateAliasAddress(
      const H225_AliasAddress & alias,
      H225_ArrayOf_AliasAddress & aliases,
      H323TransportAddress & address,
      PBoolean & isGkRouted,
      H323GatekeeperCall * call
    );

    // new functions
    PBoolean Initialise(PConfig & cfg, PConfigPage * rsrc);

#ifdef H323_TRANSNEXUS_OSP
    OpalOSP::Provider * GetOSPProvider() const
    { return ospProvider; }
#endif

  private:
    H323EndPoint & endpoint;

    class RouteMap : public PObject {
        PCLASSINFO(RouteMap, PObject);
      public:
        RouteMap(
          const PString & alias,
          const PString & host
        );
        RouteMap(
          const RouteMap & map
        ) : alias(map.alias), regex(map.alias), host(map.host) { }

        void PrintOn(
          ostream & strm
        ) const;

        PBoolean IsValid() const;

        PBoolean IsMatch(
          const PString & alias
        ) const;

        const H323TransportAddress & GetHost() const { return host; }

      private:
        PString              alias;
        PRegularExpression   regex;
        H323TransportAddress host;
    };
    PList<RouteMap> routes;

    PMutex reconfigurationMutex;

#ifdef H323_TRANSNEXUS_OSP
    OpalOSP::Provider * ospProvider;
    PString ospRoutingURL;
    PString ospPrivateKeyFileName;
    PString ospPublicKeyFileName;
    PString ospServerKeyFileName;
#endif
};


#endif


class MyManager : public OpalManager
{
  PCLASSINFO(MyManager, OpalManager);

  public:
    MyManager();
    ~MyManager();

    PBoolean Initialise(PConfig & cfg, PConfigPage * rsrc);
#if OPAL_H323
    PBoolean OnPostControl(const PStringToString & data, PHTML & msg);
    PString OnLoadEndPointStatus(const PString & htmlBlock);
    PString OnLoadCallStatus(const PString & htmlBlock);
#endif

  protected:
#if OPAL_H323
    H323EndPoint       * h323EP;
    MyGatekeeperServer * gkServer;
#endif
#if OPAL_SIP
    SIPEndPoint      * sipEP;
#endif
#if OPAL_LID
    OpalLineEndPoint * potsEP;
#endif
#if OPAL_PTLIB_EXPAT
    OpalIVREndPoint  * ivrEP;
#endif

  friend class OpalGw;
};


class OpalGw : public OpalGwProcessAncestor
{
  PCLASSINFO(OpalGw, OpalGwProcessAncestor)

  public:
    OpalGw();
    virtual void Main();
    virtual PBoolean OnStart();
    virtual void OnStop();
    virtual void OnControl();
    virtual void OnConfigChanged();
    virtual PBoolean Initialise(const char * initMsg);

    static OpalGw & Current() { return (OpalGw &)PProcess::Current(); }

    MyManager manager;
};


#if OPAL_H323

class MainStatusPage : public PServiceHTTPString
{
  PCLASSINFO(MainStatusPage, PServiceHTTPString);

  public:
    MainStatusPage(OpalGw & app, PHTTPAuthority & auth);
    
    virtual PBoolean Post(
      PHTTPRequest & request,
      const PStringToString &,
      PHTML & msg
    );
  
  private:
    OpalGw & app;
};

#endif // OPAL_H323


#endif  // _OpalGw_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
