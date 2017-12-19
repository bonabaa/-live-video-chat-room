/*
 * cli.h
 *
 * Command line interpreter
 *
 * Copyright (C) 2006-2008 Post Increment
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
 * The Original Code is WOpenMCU
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): Craig Southeren (craigs@postincrement.com)
 *                 Robert Jongbloed (robertj@voxlucida.com.au)
 *
 * Portions of this code were written by Post Increment (http://www.postincrement.com)
 * with the assistance of funding from US Joint Forces Command Joint Concept Development &
 * Experimentation (J9) http://www.jfcom.mil/about/abt_j9.htm
 *
 * $Revision: 23007 $
 * $Author: rjongbloed $
 * $Date: 2009-06-29 04:13:04 +0000 (Mon, 29 Jun 2009) $
 */

#include <ptlib.h>
#include <ptlib/sockets.h>

#include <list>


/** Command Line Interpreter class.
    This class contains a set of commands, which are executed via a PNotifier,
    when entered via a PChannel.

    The system supports multiple simultaneous interpreter which may access the
    same command set. For example several telnet sessions.

    Note that the various command interpreters could be operating in different
    threads, so care should be taken for sybchronisation issues on the object
    being acted upon via the PNotifiers.
  */
class PCLI : public PObject
{
    PCLASSINFO(PCLI, PObject);
  public:
    class Context;

    /**Context for command line interpreter.
      */
    class Context : public PIndirectChannel
    {
      public:
      /**@name Construction */
      //@{
        /**Construct new command line interpreter context.
          */
        Context(
          PCLI & cli
        );

        /**Destroy command line interpreter context.
           This will close and wait for threads to exit.
          */
        virtual ~Context();
      //@}

      /**@name Overrides from PChannel */
      //@{
        /**Low level write to the channel. This function will block until the
           requested number of characters are written or the write timeout is
           reached. The GetLastWriteCount() function returns the actual number
           of bytes written.

           This translate new line character ('\n') to be the text string in
           PCLI::m_newLine.

           The GetErrorCode() function should be consulted after Write() returns
           PFalse to determine what caused the failure.

           @return
           PTrue if at least len bytes were written to the channel.
         */
        virtual PBoolean Write(
          const void * buf, ///< Pointer to a block of memory to write.
          PINDEX len        ///< Number of bytes to write.
        );
      //@}

      /**@name Operations */
      //@{
        /**Start a command line interpreter thread.
          */
        bool Start();

        /**Stop command line interpreter context.
           This will close the channel and wait for threads to exit.
          */
        void Stop();

        /**Call back frunction for when context is started.
           This is usually called from within a background thread.

           The default behaviour displays the prompt.
          */
        virtual void OnStart();

        /**Callback for when context is stopping.
           This is usually called from within a background thread.

           The default behaviour does nothing.
          */
        virtual void OnStop();

        /**Read a character from the attached channel an process.
           If the character was successfully read then ProcessInput() is called.
          */
        virtual bool ReadAndProcessInput();

        /**Process a character read from the channel.
          */
        virtual void ProcessInput(char ch);

        /**Call back for a command line was completed and ENTER pressed.
           The default behaviour processes the line into a PArgList and deals
           with the command history and help.

           Control is then passed on to PCLI::OnReceivedLine().
          */
        virtual void OnCompletedLine();
      //@}

      /**@name Member access */
      //@{
        /**Get the CLI.
          */
        PCLI & GetCLI() const { return m_cli; }

        /**Indicate is currently processing a command.
          */
        bool IsProcessingCommand() const { return m_processingCommand; }
      //@}

      protected:
        PDECLARE_NOTIFIER(PThread, Context, ThreadMain);

        PCLI      & m_cli;
        PString     m_commandLine;
        bool        m_ignoreNextLF;
        PStringList m_commandHistory;
        PThread   * m_thread;
        bool        m_processingCommand;
    };

    /**This class is an enhancement to PArgList to add context.
      */
    class Arguments : public PArgList
    {
      public:
      /**@name Construction */
      //@{
        Arguments(
          Context & context,
          const PString & rawLine
        );
      //@}

      /**@name Operations */
      //@{
        /**Write to the CLI output channel the usage for the current command.
          */
        Context & WriteUsage();

        /**Write an error to the CLI output channel.
          */
        Context & WriteError(
          const PString & error = PString::Empty()  /// Error message
        );
      //@}

      /**@name Member access */
      //@{
        /**Get the CLI context supplying the command line arguments.
          */
        Context & GetContext() const { return m_context; }
      //@}

      protected:
        Context & m_context;
        PString   m_command;
        PString   m_usage;

      friend class PCLI;
    };


  /**@name Construction */
  //@{
    /** Contracut a new command line interpreter.
      */
    PCLI(
      const char * prompt = NULL
    );

    /**Destroy the command line interpreter. This will call Stop() to assure
       everything is cleaned up before exiting.
      */
    virtual ~PCLI();
  //@}

  /**@name Operations */
  //@{
    /**Start a command line interpreter.
       If runInBackground is true the all the command line interpreter contexts
       that have been added will have their background threads started.

       If runInBackground is false, then there must only be one context added
       and that context is continuously read until it's channel is closed or
       returns end of file.
      */
    virtual bool Start(
      bool runInBackground = true   ///< Spawn a thread to read and interpret commands
    );

    /**Stop and clean up command line interpreters.
       All the running contexts threads will be stopped, closing the channels
       and memory cleaned up.
      */
    virtual void Stop();

    /**Open a command line interpreter context.
      */
    bool StartContext(
      PChannel * channel,           ///< Channel to read/write
      bool autoDelete = true,       ///< Automatically delete channel on exit
      bool runInBackground = true   ///< Spawn a thread to read and interpret commands
    );
    bool StartContext(
      PChannel * readChannel,      ///< Channel to be used for both read operations.
      PChannel * writeChannel,     ///< Channel to be used for both write operations.
      bool autoDeleteRead = true,  ///< Automatically delete the read channel
      bool autoDeleteWrite = true, ///< Automatically delete the write channel
      bool runInBackground = true   ///< Spawn a thread to read and interpret commands
    );

    /**Create a new context.
       Users may use this to create derived classes for their own use.
      */
    virtual Context * CreateContext();

    /**Add a command line interpreter context to the system.
       If context is NULL then CreateContext() is called to create one.
      */
    virtual Context * AddContext(
      Context * context = NULL    ///< New context to add to the system.
    );

    /**Remove the command line interpreter context.
       The context thread is stopped, the channel closed and memory deleted.
      */
    virtual void RemoveContext(
      Context * context   ///< Context to remove
    );

    /**Remove any closed command line interpreter contexts.
      */
    virtual void GarbageCollection();

    /**Received a completed command line.
       The completed command line is parsed into arguments by the PArgList
       class, and passed to this function.

       The default behaviour searches the list of registered commands for a
       PNotifier to execute.
      */
    virtual void OnReceivedLine(
      Arguments & line
    );

    /**Set a string to all command line interpreter contexts.
      */
    void Broadcast(
      const PString & message   ///< Message to broadcast.
    ) const;

    /**Register a new command to be interpreted.
       Note the command may be a series of synonyms of the same command
       separated by the '\n' character.

       The command may also contain spaces which separates sub-commands, e.g.
       "list users".

       Returns false if one of the command synonyms was a dupicate of an
               existing command.
      */
    bool SetCommand(
      const char * command,       ///< Command(s) to register
      const PNotifier & notifier, ///< Callback to execute when command interpreted
      const char * help = NULL,   ///< Help text on command (what it does)
      const char * usage = NULL   ///< Usage text on command (syntax/options)
    );

    /**Show help for registered commands to the context.
      */
    void ShowHelp(
      Context & context   ///< Context to output help to.
    );
  //@}

  /**@name Member access */
  //@{
    /**Get prompt used for command line interpreter.
       Default is "CLI> ".
      */
    const PString & GetPrompt() const { return m_prompt; }

    /**Set prompt used for command line interpreter.
       Default is "CLI> ".
      */
    void SetPrompt(const PString & prompt) { m_prompt = prompt; }

    /**Get new line string output at the end of every line.
       Default is "\n".
      */
    const PString & GetNewLine() const { return m_newLine; }

    /**Set new line string output at the end of every line.
       Default is "\n".
      */
    void SetNewLine(const PString & newLine) { m_newLine = newLine; }

    /**Get command to be used to display help.
       Default is "?".
      */
    const PCaselessString & GetHelpCommand() const { return m_helpCommand; }

    /**Set command to be used to display help.
       Default is "?".
      */
    void SetHelpCommand(const PCaselessString & helpCommand) { m_helpCommand = helpCommand; }

    /**Get help on help.
      */
    const PString & GetHelpOnHelp() const { return m_helpOnHelp; }

    /**Set help on help.
      */
    void SetHelpOnHelp(const PCaselessString & helpOnHelp) { m_helpOnHelp = helpOnHelp; }

    /**Get the command to be used to repeat the last executed command.
       Default is "!!".
      */
    const PCaselessString & GetRepeatCommand() const { return m_repeatCommand; }

    /**Set the command to be used to repeat the last executed command.
       Default is "!!".
      */
    void SetRepeatCommand(const PCaselessString & repeatCommand) { m_repeatCommand = repeatCommand; }

    /**Get command that will list/execute command history.
       Default is "!".
      */
    const PCaselessString & GetHistoryCommand() const { return m_historyCommand; }

    /**Set command that will list/execute command history.
       Default is "!".
      */
    void SetHistoryCommand(const PCaselessString & historyCommand) { m_historyCommand = historyCommand; }

    /**Get error message for if there is no history.
      */
    const PString & GetNoHistoryError() const { return m_noHistoryError; }

    /**Set error message for if there is no history.
      */
    void SetNoHistoryError(const PString & noHistoryError) { m_noHistoryError = noHistoryError; }

    /**Get usage prefix for if Arguments::WriteError() called.
      */
    const PString & GetCommandUsagePrefix() const { return m_commandUsagePrefix; }

    /**Set usage prefix for if Arguments::WriteError() called.
      */
    void SetCommandUsagePrefix(const PString & commandUsagePrefix) { m_commandUsagePrefix = commandUsagePrefix; }

    /**Get error prefix for if Arguments::WriteError() called.
      */
    const PString & GetCommandErrorPrefix() const { return m_commandErrorPrefix; }

    /**Set error prefix for if Arguments::WriteError() called.
      */
    void SetCommandErrorPrefix(const PString & commandErrorPrefix) { m_commandErrorPrefix = commandErrorPrefix; }

    /**Get error message for if unknown command is entered.
      */
    const PString & GetUnknownCommandError() const { return m_unknownCommandError; }

    /**Set error message for if unknown command is entered.
      */
    void SetUnknownCommandError(const PString & unknownCommandError) { m_unknownCommandError = unknownCommandError; }
  //@}

  protected:
    PString         m_prompt;
    PString         m_newLine;
    PCaselessString m_helpCommand;
    PString         m_helpOnHelp;
    PCaselessString m_repeatCommand;
    PCaselessString m_historyCommand;
    PString         m_noHistoryError;
    PString         m_commandUsagePrefix;
    PString         m_commandErrorPrefix;
    PString         m_unknownCommandError;

    struct InternalCommand {
      PNotifier m_notifier;
      PString   m_help;
      PString   m_usage;
    };
    typedef std::map<PString, InternalCommand> CommandMap_t;
    CommandMap_t m_commands;

    typedef std::list<Context *> ContextList_t;
    ContextList_t m_contextList;
    PMutex        m_contextMutex;
};


/**Command Line Interpreter over standard input/output.
  */
class PCLIStandard : public PCLI
{
  public:
  /**@name Construction */
  //@{
    /**Create new command line interpreter for standard I/O.
      */
    PCLIStandard(
      const char * prompt = NULL
    );
  //@}

  /**@name Overrides from PCLI */
  //@{
    /**Start a command line interpreter.
       As for ancestor function, however if no contexts have been added, then
       on that takes a PConsoleChannel is automatically added.
      */
    virtual bool Start(
      bool runInBackground = true   ///< Spawn a thread to read and interpret commands
    );
  //@}
};


/**Command Line Interpreter over TCP sockets.
   This class allows for access and automatic creation of command line
   interpreter contexts from incoming TCP connections on a listening port.
  */
class PCLISocket : public PCLI
{
  public:
  /**@name Construction */
  //@{
    PCLISocket(
      WORD port = 0,
      const char * prompt = NULL,
      bool singleThreadForAll = false
    );
    ~PCLISocket();
  //@}

  /**@name Overrides from PCLI */
  //@{
    /**Start a command line interpreter.
       This will start listening for incoming TCP connections and
       create/dispatch contexts to handle them.
      */
    virtual bool Start(
      bool runInBackground = true   ///< Spawn a thread to read and interpret commands
    );

    /**Stop and clean up command line interpreters.
       All the running contexts threads will be stopped, closing the channels
       and memory cleaned up.

       The listening socket is also closed and the listener dispatch thread
       shut down.
      */
    virtual void Stop();

    /**Add a command line interpreter context to the system.
       If context is NULL then CreateContext() is called to create one.
      */
    virtual Context * AddContext(
      Context * context = NULL
    );

    /**Remove the command line interpreter context.
       The context thread is stopped, the channel closed and memory deleted.
      */
    virtual void RemoveContext(
      Context * context
    );
  //@}

  /**@name Operations */
  //@{
    /**Start listening socket.
      */
    bool Listen(
      WORD port = 0
    );

    /**Get the port we are listing on.
      */
    WORD GetPort() const { return m_listenSocket.GetPort(); }

  protected:
    PDECLARE_NOTIFIER(PThread, PCLISocket, ThreadMain);
    bool HandleSingleThreadForAll();
    bool HandleIncoming();

    bool m_singleThreadForAll;

    PTCPSocket m_listenSocket;
    PThread  * m_thread;

    typedef std::map<PSocket *, Context *> ContextMap_t;
    ContextMap_t m_contextBySocket;
};
