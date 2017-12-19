/*
 * sippres.h
 *
 * SIP Presence classes for Opal
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2009 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 22858 $
 * $Author: csoutheren $
 * $Date: 2009-06-12 22:50:19 +1000 (Fri, 12 Jun 2009) $
 */

#ifndef OPAL_SIP_SIPPRES_H
#define OPAL_SIP_SIPPRES_H

#include <ptlib.h>
#include <opal/buildopts.h>
#include <sip/sipep.h>

#if P_EXPAT

#include <opal/pres_ent.h>
#include <ptclib/pxml.h>


class XCAPClient : public PHTTPClient
{
  public:
    struct ElementSelector {
      ElementSelector(
        const PString & name = PString::Empty(),
        const PString & position = PString::Empty()
      ) : m_name(name)
        , m_position(position)
      { PAssert(!m_name.IsEmpty(), PInvalidParameter); }

      ElementSelector(
        const PString & name,
        const PString & attribute,
        const PString & value
      ) : m_name(name)
        , m_attribute(attribute)
        , m_value(value)
      { PAssert(!m_name.IsEmpty(), PInvalidParameter); }

      ElementSelector(
        const PString & name,
        const PString & position,
        const PString & attribute,
        const PString & value
      ) : m_name(name)
        , m_position(position)
        , m_attribute(attribute)
        , m_value(value)
      { PAssert(!m_name.IsEmpty(), PInvalidParameter); }

      PString AsString() const;

      PString m_name;
      PString m_position;
      PString m_attribute;
      PString m_value;
    };

    class NodeSelector : public std::list<ElementSelector>
    {
      public:
        NodeSelector()
          { }
        NodeSelector(
          const ElementSelector & selector
        ) { push_back(selector); }
        NodeSelector(
          const ElementSelector & selector1,
          const ElementSelector & selector2
        ) { push_back(selector1); push_back(selector2); }
        NodeSelector(
          const ElementSelector & selector1,
          const ElementSelector & selector2,
          const ElementSelector & selector3
        ) { push_back(selector1); push_back(selector2); push_back(selector3); }

        void AddElement(
          const PString & name,
          const PString & position = PString::Empty()
        ) { push_back(ElementSelector(name, position)); }

        void AddElement(
          const PString & name,
          const PString & attribute,
          const PString & value
        ) { push_back(ElementSelector(name, attribute, value)); }

        void AddElement(
          const PString & name,
          const PString & position,
          const PString & attribute,
          const PString & value
        ) { push_back(ElementSelector(name, position, attribute, value)); }

        void SetNamespace(
          const PString & space,
          const PString & alias = PString::Empty()
        ) { PAssert(!space.IsEmpty(), PInvalidParameter); m_namespaces[alias] = space; }

        void AddToURL(
          PURL & url
        ) const;

      protected:
        std::map<PString, PString> m_namespaces;
    };


    XCAPClient();

    bool GetXml(
      PXML & xml
    ) { return GetXml(BuildURL(), xml); }

    bool GetXml(
      const PURL & url,
      PXML & xml
    );

    bool PutXml(
      const PXML & xml
    ) { return PutXml(BuildURL(), xml); }

    bool PutXml(
      const PURL & url,
      const PXML & xml
    );

    bool DeleteXml() { return DeleteDocument(BuildURL()); }


    PURL BuildURL();


    void SetRoot(
      const PURL & server
    ) { m_root = server; }
    const PURL & GetRoot() const { return m_root; }

    void SetApplicationUniqueID(
      const PString & id
    ) { m_auid = id; }
    const PString & GetApplicationUniqueID() const { return m_auid; }

    void SetGlobal() { m_global = true; }
    bool IsGlobal() const { return m_global; }

    void SetUserIdentifier(
      const PString & id
    ) { m_global = false; m_xui = id; }
    const PString & GetUserIdentifier() const { return m_xui; }

    void SetFilename(
      const PString & fn
    ) { m_filename = fn; }
    const PString & GetFilename() const { return m_filename; }

    void SetNode(
      const NodeSelector & node
    ) { m_node = node; }
    const NodeSelector & GetNode() const { return m_node; }
    void ClearNode() { m_node.clear(); }

    void SetContentType(
      const PString & type
    ) { m_contentType = type; }
    const PString & GetContentType() const { return m_contentType; }

  protected:
    PURL         m_root;
    PString      m_auid;
    bool         m_global;
    PString      m_xui;
    PString      m_filename;
    NodeSelector m_node;
    PString      m_contentType;
};


class SIPWatcherInfoCommand : public OpalPresentityCommand {
  public:
    SIPWatcherInfoCommand(bool unsubscribe = false) : m_unsubscribe(unsubscribe) { }

    bool m_unsubscribe;
};


class SIP_Presentity : public OpalPresentityWithCommandThread
{
    PCLASSINFO(SIP_Presentity, OpalPresentityWithCommandThread);

  public:
    static const PString & DefaultPresenceServerKey();
    static const PString & PresenceServerKey();

    ~SIP_Presentity();

    virtual bool Open();
    virtual bool IsOpen() const;
    virtual bool Close();

    SIPEndPoint & GetEndpoint() { return *m_endpoint; }

    /** Set the default presentity class for "sip" scheme.
        When OpalPresentity::Create() is called with a URI containing the
        "sip" scheme several different preentity protocols could be used.
        This determines which: e.g. "sip-local", "sip-xcap", "sip-oma".
        */
    static bool SetDefaultPresentity(
      const PString & prefix
    );

  protected:
    SIP_Presentity();

    SIPEndPoint * m_endpoint;
    int           m_watcherInfoVersion;
    PString       m_publishedTupleId;
};


class SIPLocal_Presentity : public SIP_Presentity
{
    PCLASSINFO(SIPLocal_Presentity, SIP_Presentity);

  public:
    ~SIPLocal_Presentity();
};


class SIPXCAP_Presentity : public SIP_Presentity
{
    PCLASSINFO(SIPXCAP_Presentity, SIP_Presentity);

  public:
    static const PString & XcapRootKey();
    static const PString & XcapAuthIdKey();
    static const PString & XcapPasswordKey();
    static const PString & XcapAuthAuidKey();
    static const PString & XcapAuthFileKey();
    static const PString & XcapBuddyListKey();

    SIPXCAP_Presentity();
    ~SIPXCAP_Presentity();

    /**@name Overrides from OpalPresentity */
    virtual bool Open();
    virtual bool Close();

    virtual bool GetBuddyList(BuddyList & buddies);
    virtual bool SetBuddyList(const BuddyList & buddies);
    virtual bool DeleteBuddyList();
    virtual bool GetBuddy(BuddyInfo & buddy);
    virtual bool SetBuddy(const BuddyInfo & buddy);
    virtual bool DeleteBuddy(const PURL & presentity);
    virtual bool SubscribeBuddyList(bool subscribe = true);
  //@}

    void Internal_SendLocalPresence(const OpalSetLocalPresenceCommand & cmd);
    void Internal_SubscribeToPresence(const OpalSubscribeToPresenceCommand & cmd);
    void Internal_AuthorisationRequest(const OpalAuthorisationRequestCommand & cmd);
    void Internal_SubscribeToWatcherInfo(const SIPWatcherInfoCommand & cmd);

  protected:
    PDECLARE_NOTIFIER2(SIPSubscribeHandler, SIPXCAP_Presentity, OnWatcherInfoSubscriptionStatus, const SIPSubscribe::SubscriptionStatus &);
    PDECLARE_NOTIFIER2(SIPSubscribeHandler, SIPXCAP_Presentity, OnWatcherInfoNotify, SIPSubscribe::NotifyCallbackInfo &);
    PDECLARE_NOTIFIER2(SIPSubscribeHandler, SIPXCAP_Presentity, OnPresenceSubscriptionStatus, const SIPSubscribe::SubscriptionStatus &);
    PDECLARE_NOTIFIER2(SIPSubscribeHandler, SIPXCAP_Presentity, OnPresenceNotify, SIPSubscribe::NotifyCallbackInfo &);

    unsigned GetExpiryTime() const;
    virtual void OnReceivedWatcherStatus(PXMLElement * watcher);
    virtual void InitRuleSet(PXML & xml);
    bool ChangeAuthNode(XCAPClient & xcap, const OpalAuthorisationRequestCommand & cmd);
    void InitBuddyXcap(
      XCAPClient & xcap,
      const PString & entryName = PString::Empty(),
      const PString & listName = PString::Empty()
    );

    PIPSocketAddressAndPort m_presenceServer;
    PString                 m_watcherSubscriptionAOR;

    typedef std::map<PString, PString> StringMap;
    StringMap m_watcherAorById;
    StringMap m_presenceIdByAor;
    StringMap m_presenceAorById;
    StringMap m_authorisationIdByAor;
};


class SIPOMA_Presentity : public SIPXCAP_Presentity
{
    PCLASSINFO(SIPOMA_Presentity, SIPXCAP_Presentity);

  public:
    SIPOMA_Presentity();

  protected:
    virtual void InitRuleSet(PXML & xml);
};


#endif // P_EXPAT

#endif // OPAL_SIP_SIPPRES_H
