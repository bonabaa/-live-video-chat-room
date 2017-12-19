/*
 * xmpp_roster.h
 *
 * Extensible Messaging and Presence Protocol (XMPP) IM
 * Roster management classes
 *
 * Portable Windows Library
 *
 * Copyright (c) 2004 Reitek S.p.A.
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
 * $Revision: 21788 $
 * $Author: rjongbloed $
 * $Date: 2008-12-12 05:42:13 +0000 (Fri, 12 Dec 2008) $
 */

#ifndef PTLIB_XMPP_ROSTER_H
#define PTLIB_XMPP_ROSTER_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptclib/xmpp_c2s.h>

#if P_EXPAT

///////////////////////////////////////////////////////

namespace XMPP
{
  class Roster : public PObject
  {
    PCLASSINFO(Roster, PObject);
  public:

    enum ItemType { // Subscription type
      None,
      To,
      From,
      Both,
      Unknown = 999
    };

    class Item : public PObject
    {
      PCLASSINFO(Item, PObject);
      PDICTIONARY(PresenceInfo, PString, Presence);

    public:
      Item(PXMLElement * item = 0);
      Item(PXMLElement& item);
      Item(const JID& jid, ItemType type, const PString& group, const PString& name = PString::Empty());

      const JID&          GetJID() const        { return m_JID; }
      ItemType            GetType() const       { return m_Type; }
      const PString&      GetName() const       { return m_Name; }
      const PStringSet&   GetGroups() const     { return m_Groups; }
      const PresenceInfo& GetPresence() const   { return m_Presence; }

      virtual void  SetJID(const JID& jid, PBoolean dirty = PTrue)
                                                { m_JID = jid; if (dirty) SetDirty(); }
      virtual void  SetType(ItemType type, PBoolean dirty = PTrue)
                                                { m_Type = type; if (dirty) SetDirty(); }
      virtual void  SetName(const PString& name, PBoolean dirty = PTrue) 
                                                { m_Name = name; if (dirty) SetDirty(); }

      virtual void  AddGroup(const PString& group, PBoolean dirty = PTrue);
      virtual void  RemoveGroup(const PString& group, PBoolean dirty = PTrue);

      virtual void  SetPresence(const Presence& p);

      void SetDirty(PBoolean b = PTrue) { m_IsDirty = b; }

      /** This operator will set the dirty flag
       */
      Item & operator=(
        const PXMLElement& item
      );

      virtual PXMLElement * AsXML(PXMLElement * parent) const;

    protected:
      BareJID     m_JID;
      ItemType    m_Type;
      PString     m_Name;
      PStringSet  m_Groups;

      // The item's presence state: for each resource (the key to the dictionary) a
      // a presence stanza if kept.
      PDictionary<PString, Presence> m_Presence;

      PBoolean        m_IsDirty; // item modified locally, server needs to be updated
    };
    PLIST(ItemList, Item);

  public:
    Roster(XMPP::C2S::StreamHandler * handler = 0);
    ~Roster();

    const ItemList& GetItems() const    { return m_Items; }

    virtual Item * FindItem(const PString& jid);

    virtual PBoolean SetItem(Item * item, PBoolean localOnly = PFalse);
    virtual PBoolean RemoveItem(const PString& jid, PBoolean localOnly = PFalse);
    virtual PBoolean RemoveItem(Item * item, PBoolean localOnly = PFalse);

    virtual void  Attach(XMPP::C2S::StreamHandler * handler);
    virtual void  Detach();
    virtual void  Refresh(PBoolean sendPresence = PTrue);

    virtual PNotifierList& ItemChangedHandlers()    { return m_ItemChangedHandlers; }
    virtual PNotifierList& RosterChangedHandlers()  { return m_RosterChangedHandlers; }

  protected:
    PDECLARE_NOTIFIER(XMPP::C2S::StreamHandler, Roster, OnSessionEstablished);
    PDECLARE_NOTIFIER(XMPP::C2S::StreamHandler, Roster, OnSessionReleased);
    PDECLARE_NOTIFIER(XMPP::Presence, Roster, OnPresence);
    PDECLARE_NOTIFIER(XMPP::IQ, Roster, OnIQ);

    ItemList m_Items;
    XMPP::C2S::StreamHandler * m_Handler;
    PNotifierList m_ItemChangedHandlers;
    PNotifierList m_RosterChangedHandlers;
  };

} // namespace XMPP


#endif  // P_EXPAT

#endif  // PTLIB_XMPP_ROSTER_H

// End of File ///////////////////////////////////////////////////////////////
