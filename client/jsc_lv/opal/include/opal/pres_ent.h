/*
 * prese_ent.h
 *
 * Presence Entity classes for Opal
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

#ifndef OPAL_IM_PRES_ENT_H
#define OPAL_IM_PRES_ENT_H

#include <ptlib.h>
#include <opal/buildopts.h>

#include <ptlib/pfactory.h>
#include <ptlib/safecoll.h>
#include <ptclib/url.h>
#include <ptclib/guid.h>

#include <list>
#include <queue>

class OpalManager;
class OpalPresentityCommand;


////////////////////////////////////////////////////////////////////////////////////////////////////

/**Presencu state information
  */
class OpalPresenceInfo
{
  public:
    /// Presence states.
    enum State {
      InternalError = -3,    // something bad happened
      Forbidden     = -2,    // access to presence information was specifically forbidden
      NoPresence    = -1,    // remove presence status - not the same as Unavailable or Away

      // basic states (from RFC 3863)
      Unchanged,
      Available,
      Unavailable,

      // extended states (from RFC 4480)
      // if this is changed, also change the tables in sippres.cxx and handlers.cxx - look for RFC 4480
      ExtendedBase    = 100,
      UnknownExtended = ExtendedBase,
      Appointment,
      Away,
      Breakfast,
      Busy,
      Dinner,
      Holiday,
      InTransit,
      LookingForWork,
      Lunch,
      Meal,
      Meeting,
      OnThePhone,
      Other,
      Performance,
      PermanentAbsence,
      Playing,
      Presentation,
      Shopping,
      Sleeping,
      Spectator,
      Steering,
      Travel,
      TV,
      Vacation,
      Working,
      Worship
    };

    State   m_state;    ///< New state for presentity
    PString m_note;     ///< Additional information about state change
    PURL    m_entity;   ///< The presentity whose state had changed
    PURL    m_target;   ///< The presentity that is being informed about the state change

    OpalPresenceInfo(State state = Unchanged) : m_state(state) { }

    static PString AsString(State state);
    static State FromString(const PString & str);
    PString AsString() const;
};

ostream & operator<<(ostream & strm, OpalPresenceInfo::State state);


////////////////////////////////////////////////////////////////////////////////////////////////////

/**Representation of a presence identity.
   This class contains an abstraction of the functionality for "presence"
   using a URL string as the "identity". The concrete class depends on the
   scheme of the identity URL.

   The general architecture is that commands will be sent to the preentity
   concrete class via concrete versions of OpalPresentityCommand class. These
   may be protocol specific or one of the abstracted versions.
  */
class OpalPresentity : public PSafeObject
{
    PCLASSINFO(OpalPresentity, PSafeObject);

  /**@name Construction */
  //@{
  protected:
    /// Construct the presentity class
    OpalPresentity();

  public:
    /**Create a concrete class based on the scheme of the URL provided.
      */
    static OpalPresentity * Create(
      OpalManager & manager, ///< Manager for presentity
      const PURL & url,      ///< URL for presence identity
      const PString & scheme = PString::Empty() ///< Overide the url scheme
    );
  //@}

  /**@name Initialisation */
  //@{
    /** Open the presentity handler.
        This will perform whatever is required to allow this presentity
        to access servers etc for the underlying protocol. It may start open
        network channels, start threads etc.

        Note that a return value of true does not necessarily mean that the
        presentity has successfully been indicated as "present" on the server
        only that the underlying system can do so at some time.
      */
    virtual bool Open() = 0;

    /** Indicate if the presentity has been successfully opened.
      */
    virtual bool IsOpen() const = 0;

    /**Close the presentity.
      */
    virtual bool Close() = 0;
  //@}

  /**@name Attributes */
  //@{
    /**Dictionary of attributes associated with this presentity.
      */
    class Attributes :  public PStringToString
    {
      public:
        /// Determine of the attribute exists.
        virtual bool Has(const PString &   key   ) const { return Contains(key  ); }
        virtual bool Has(const PString & (*key)()) const { return Contains(key()); }

        /// Get the attribute value.
        virtual PString Get(const PString &   key   , const PString & deflt = PString::Empty()) const { return (*this)(key  , deflt); }
                PString Get(const PString & (*key)(), const PString & deflt = PString::Empty()) const { return (*this)(key(), deflt); }

        /// Set the attribute value.
        virtual void Set(const PString &   key   , const PString & value) { SetAt(key  , value); }
                void Set(const PString & (*key)(), const PString & value) { SetAt(key(), value); }
    };

    ///< Get the attributes for this presentity.
    Attributes & GetAttributes() { return m_attributes; }

    static const PString & AuthNameKey();         ///< Key for authentication name attribute
    static const PString & AuthPasswordKey();     ///< Key for authentication password attribute
    static const PString & FullNameKey();         ///< Key for full name attribute
    static const PString & SchemeKey();           ///< Key for scheme used attribute
    static const PString & TimeToLiveKey();       ///< Key for Time-To-Live attribute, in seconds for underlying protocol

    /** Get the address-of-record for the presentity.
        This is typically a URL which represents our local identity in the
        presense system.
      */
    const PURL & GetAOR() const { return m_aor; }
  //@}

  /**@name Commands */
  //@{
    /** Subscribe to presence state of another presentity.
        This function is a wrapper and the OpalSubscribeToPresenceCommand
        command.

        Returns true if the command was queued. Note that this does not
        mean the command succeeded, only that the underlying protocol is
        capabable of the action.
      */
    virtual bool SubscribeToPresence(
      const PURL & presentity,      ///< Other presentity to monitor
      bool subscribe = true         ///< true if to subscribe, else unsubscribe
    );

    /** Unsubscribe to presence state of another presentity.
        This function is a wrapper and the OpalSubscribeToPresenceCommand
        command.

        Returns true if the command was queued. Note that this does not
        mean the command succeeded, only that the underlying protocol is
        capabable of the action.
      */
    virtual bool UnsubscribeFromPresence(
      const PURL & presentity    ///< Other presentity to monitor
    );

    /// Authorisation modes for SetPresenceAuthorisation()
    enum Authorisation {
      AuthorisationPermitted,
      AuthorisationDenied,
      AuthorisationDeniedPolitely,
      AuthorisationConfirming,
      AuthorisationRemove,
      NumAuthorisations
    };

    /** Called to allow/deny another presentity access to our presence
        information.

        This function is a wrapper and the OpalAuthorisationRequestCommand
        command.

        Returns true if the command was queued. Note that this does not
        mean the command succeeded, only that the underlying protocol is
        capabable of the action.
      */
    virtual bool SetPresenceAuthorisation(
      const PURL & presentity,        ///< Remote presentity to be authorised
      Authorisation authorisation     ///< Authorisation mode
    );

    /** Set our presence state.
        This function is a wrapper and the OpalSetLocalPresenceCommand command.

        Returns true if the command was queued. Note that this does not
        mean the command succeeded, only that the underlying protocol is
        capabable of the action.
      */
    virtual bool SetLocalPresence(
      OpalPresenceInfo::State state,            ///< New state for our presentity
      const PString & note = PString::Empty()   ///< Additional note attached to the state change
    );

    /** Low level function to create a command.
        As commands have protocol specific implementations, we use a factory
        to create them.
      */
    template <class cls>
    __inline cls * CreateCommand()
    {
      return dynamic_cast<cls *>(InternalCreateCommand(typeid(cls).name()));
    }

    /** Lowlevel function to send a command to the presentity handler.
        All commands are asynchronous. They will usually initiate an action
        for which an indication (callback) will give a result.

        Note that the command is typically created by the CreateCommand()
        function and is subsequently deleted by this function.

        Returns true if the command was queued. Note that this does not
        mean the command succeeded, only that the underlying protocol is
        processing commands.
      */
    virtual bool SendCommand(
      OpalPresentityCommand * cmd   ///< Command to be processed
    );
  //@}

  /**@name Indications (callbacks) */
  //@{
    /** Callback when another presentity requests access to our presence.
        It is expected that the handler will call SetPresenceAuthorisation
        with whatever policy is appropriate.

        Default implementation calls m_onRequestPresenceNotifier if non-NULL
        otherwise will authorise the request.
      */
    virtual void OnAuthorisationRequest(
      const PURL & presentity    ///< Other presentity requesting our presence
    );

    typedef PNotifierTemplate<const PURL &> AuthorisationRequestNotifier;
    #define PDECLARE_AuthorisationRequestNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalPresentity, cls, fn, const PURL &)
    #define PCREATE_AuthorisationRequestNotifier(fn) PCREATE_NOTIFIER2(fn, const PURL &)

    /// Set the notifier for the OnAuthorisationRequest() function.
    void SetAuthorisationRequestNotifier(
      const AuthorisationRequestNotifier & notifier   ///< Notifier to be called by OnAuthorisationRequest()
    );

    /** Callback when presentity has changed its state.
        Note if the m_entity and m_target fields of the OpalPresenceInfo
        structure are the same, then this indicates that the local presentity
        itself has successfully "registered" itself with the presence agent
        server.

        Default implementation calls m_onPresenceChangeNotifier.
      */
    virtual void OnPresenceChange(
      const OpalPresenceInfo & info ///< Info on other presentity that changed state
    );

    typedef PNotifierTemplate<const OpalPresenceInfo &> PresenceChangeNotifier;
    #define PDECLARE_PresenceChangeNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalPresentity, cls, fn, const OpalPresenceInfo &)
    #define PCREATE_PresenceChangeNotifier(fn) PCREATE_NOTIFIER2(fn, const OpalPresenceInfo &)

    /// Set the notifier for the OnPresenceChange() function.
    void SetPresenceChangeNotifier(
      const PresenceChangeNotifier & notifier   ///< Notifier to be called by OnPresenceChange()
    );
  //@}

  /**@name Buddy Lists */
  //@{
    /**Buddy list entry.
       The buddy list is a list of presentities that the application is
       expecting to get presence status for.
      */
    struct BuddyInfo {
      BuddyInfo(
        const PURL & presentity = PString::Empty(),
        const PString & displayName = PString::Empty()
      ) : m_presentity(presentity)
        , m_displayName(displayName)
      { }

      PURL    m_presentity;   ///< Typicall URI address-of-record
      PString m_displayName;  ///< Human readable name

      PString m_contentType;  ///< MIME type code for XML
      PString m_rawXML;       ///< Raw XML of buddy list entry
    };

    typedef std::list<BuddyInfo> BuddyList;

    /**Get complete buddy list.
      */
    virtual bool GetBuddyList(
      BuddyList & buddies   ///< List of buddies
    );

    /**Set complete buddy list.
      */
    virtual bool SetBuddyList(
      const BuddyList & buddies   ///< List of buddies
    );

    /**Delete the buddy list.
      */
    virtual bool DeleteBuddyList();

    /**Get a specific buddy from the buddy list.
       Note the buddy.m_presentity field must be preset to the URI to search
       the buddy list for.
      */
    virtual bool GetBuddy(
      BuddyInfo & buddy
    );

    /**Set/Add a buddy to the buddy list.
      */
    virtual bool SetBuddy(
      const BuddyInfo & buddy
    );

    /**Delete a buddy to the buddy list.
      */
    virtual bool DeleteBuddy(
      const PURL & presentity
    );

    /**Subscribe to buddy list.
       Send a subscription for the presence of every presentity in the current
       buddy list. This might cause multiple calls to SubscribeToPresence() or
       if the underlying protocol allows a single call for all.
      */
    virtual bool SubscribeBuddyList(
      bool subscribe = true
    );

    /**Unsubscribe to buddy list.
       Send an unsubscription for the presence of every presentity in the current
       buddy list. This might cause multiple calls to UnsubscribeFromPresence() or
       if the underlying protocol allows a single call for all.
      */
    virtual bool UnsubscribeBuddyList();

    virtual PString GetID() const;
  //@}

  protected:
    OpalPresentityCommand * InternalCreateCommand(const char * cmdName);

    OpalManager        * m_manager;
    PGloballyUniqueID    m_guid;
    PURL                 m_aor;
    Attributes           m_attributes;

    AuthorisationRequestNotifier m_onAuthorisationRequestNotifier;
    PresenceChangeNotifier       m_onPresenceChangeNotifier;

    PMutex m_notificationMutex;
    PAtomicInteger::IntegerType m_idNumber;
};


////////////////////////////////////////////////////////////////////////////////////////////////////

/**Representation of a presence identity that uses a background thread to
   process commands.
  */
class OpalPresentityWithCommandThread : public OpalPresentity
{
    PCLASSINFO(OpalPresentityWithCommandThread, OpalPresentity);

  /**@name Construction */
  //@{
  protected:
    /// Construct the presentity class that uses a command thread.
    OpalPresentityWithCommandThread();

  public:
    /** Destory the presentity class that uses a command thread.
        Note this will block until the background thread has stopped.
      */
    ~OpalPresentityWithCommandThread();
  //@}

  /**@name Overrides from OpalPresentity */
  //@{
    /** Lowlevel function to send a command to the presentity handler.
        All commands are asynchronous. They will usually initiate an action
        for which an indication (callback) will give a result.

        The default implementation queues the command for the background
        thread to handle.

        Returns true if the command was queued. Note that this does not
        mean the command succeeded, only that the underlying protocol is
        processing commands, that is the backgrund thread is running
      */
    virtual bool SendCommand(
      OpalPresentityCommand * cmd   ///< Command to execute
    );
  //@}

  /**@name Overrides from OpalPresentity */
  //@{
    /**Start the background thread to handle commands.
       This is typically called from the concrete classes Open() function.
      */
    void StartThread();

    /**Stop the background thread to handle commands.
       This is typically called from the concrete classes Close() function.
       It is also called fro the destructor to be sure the thread has stopped
       before the object is destroyed.
      */
    void StopThread();
  //@}

  protected:
    void ThreadMain();

    typedef std::queue<OpalPresentityCommand *> CommandQueue;
    CommandQueue   m_commandQueue;
    PMutex         m_commandQueueMutex;
    PAtomicInteger m_commandSequence;
    PSyncPoint     m_commandQueueSync;

    bool      m_threadRunning;
    PThread * m_thread;
};


////////////////////////////////////////////////////////////////////////////////////////////////////

/**Abstract class for all OpelPresentity commands.
  */
class OpalPresentityCommand {
  public:
    OpalPresentityCommand(bool responseNeeded = false) 
      : m_responseNeeded(responseNeeded)
    { }
    virtual ~OpalPresentityCommand() { }

    /**Function to process the command.
       This typically calls functions on the concrete OpalPresentity class.
      */
    virtual void Process(
      OpalPresentity & presentity
    ) = 0;

    typedef PAtomicInteger::IntegerType CmdSeqType;
    CmdSeqType m_sequence;
    bool       m_responseNeeded;
    PURL       m_presentity;
};

/** Macro to define the factory that creates a concrete command class.
  */
#define OPAL_DEFINE_COMMAND(command, entity, func) \
  class entity##_##command : public command \
  { \
    public: virtual void Process(OpalPresentity & presentity) { dynamic_cast<entity &>(presentity).func(*this); } \
  }; \
  static PFactory<OpalPresentityCommand>::Worker<entity##_##command> \
  s_entity##_##command(PDefaultPFactoryKey(entity::Class())+typeid(command).name())


/** Command for subscribing to the status of another presentity.
  */
class OpalSubscribeToPresenceCommand : public OpalPresentityCommand {
  public:
    OpalSubscribeToPresenceCommand(bool subscribe = true) : m_subscribe(subscribe) { }

    bool m_subscribe;   ///< Flag to indicate subscribing/unsubscribing
};


/** Command for authorising a request by some other presentity to see our status.
    This action is typically due to some other presentity doing the equivalent
    of a OpalSubscribeToPresenceCommand. The mechanism by which this happens
    is dependent on the concrete OpalPresentity class and it's underlying
    protocol.
  */
class OpalAuthorisationRequestCommand : public OpalPresentityCommand {
  public:
    OpalAuthorisationRequestCommand() : m_authorisation(OpalPresentity::AuthorisationPermitted) { }

    OpalPresentity::Authorisation m_authorisation;  ///< Authorisation mode to indicate to remote
};


/** Command for adusting our current status.
    This action typically causes all presentities to be notified of the
    change. The mechanism by which this happens is dependent on the concrete
    OpalPresentity class and it's underlying protocol.
  */
class OpalSetLocalPresenceCommand : public OpalPresentityCommand, public OpalPresenceInfo {
  public:
    OpalSetLocalPresenceCommand(State state = NoPresence) : OpalPresenceInfo(state) { }
};


///////////////////////////////////////////////////////////////////////////////

// Include concrete classes here so the factories are initialised
#if OPAL_SIP && OPAL_PTLIB_EXPAT
PFACTORY_LOAD(SIPLocal_Presentity);
PFACTORY_LOAD(SIPXCAP_Presentity);
PFACTORY_LOAD(SIPOMA_Presentity);
#endif


#endif  // OPAL_IM_PRES_ENT_H

