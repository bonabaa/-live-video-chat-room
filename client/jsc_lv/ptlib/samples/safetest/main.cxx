/*
 * main.cxx
 *
 * PWLib application source file for safetest
 *
 * Main program entry point.
 *
 * Copyright (c) 2006 Indranet Technologies Ltd
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
 * The Initial Developer of the Original Code is Indranet Technologies Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 20385 $
 * $Author: rjongbloed $
 * $Date: 2008-06-04 10:40:38 +0000 (Wed, 04 Jun 2008) $
 */


#ifdef P_USE_PRAGMA
#pragma implementation "main.h"
#endif


#include "precompile.h"
#include "main.h"
#include "version.h"

PCREATE_PROCESS(SafeTest);

#include  <ptclib/dtmf.h>
#include  <ptclib/random.h>



SafeTest::SafeTest()
  : PProcess("Derek Smithies code factory", "safetest", MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
}

void SafeTest::Main()
{
  PArgList & args = GetArguments();

  args.Parse(
             "h-help."               "-no-help."
             "d-delay:"              "-no-delay."
	     "c-count:"              "-no-count."
	     "r-reporting."
	     "b-banpthreadcreate."
	     "a-alternate."
#if PTRACING
             "o-output:"             "-no-output."
             "t-trace."              "-no-trace."
#endif
             "v-version."
  );

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  if (args.HasOption('v')) {
    cout << "Product Name: " << GetName() << endl
         << "Manufacturer: " << GetManufacturer() << endl
         << "Version     : " << GetVersion(PTrue) << endl
         << "System      : " << GetOSName() << '-'
         << GetOSHardware() << ' '
         << GetOSVersion() << endl;
    return;
  }

  if (args.HasOption('h')) {
    PError << "Available options are: " << endl         
	   << "-a                    Use a non opal end DelayThread mechanism" << endl
	   << "-b                    Avoid the usage of PThread::Create" << endl
           << "-h  or --help         print this help" << endl
	   << "-r                    print reporting (every minute) on current statistics" << endl
           << "-v  or --version      print version info" << endl
           << "-d  or --delay ##     where ## specifies how many milliseconds the created thread waits for" << endl
	   << "-c  or --count ##     where ## specifies the number of active threads allowed " << endl
#if PTRACING
           << "o-output              output file name for trace" << endl
           << "t-trace.              trace level to use." << endl
#endif
           << endl
           << endl << endl;
    return;
  }

  delay = 2000;
  if (args.HasOption('d'))
    delay = args.GetOptionString('d').AsInteger();

  delay = PMIN(1000000, PMAX(1, delay));
  cout << "Created thread will wait for " << delay << " milliseconds before ending" << endl;

  useOnThreadEnd = args.HasOption('a');
  if (useOnThreadEnd)
    cout << "Will use classes from the author to handle the end of a DelayThread instance" << endl;
  else
    cout << "Use methods similar to those in opal to handle the end of a DelayThread instance" << endl;

  avoidPThreadCreate = args.HasOption('b');
  if (avoidPThreadCreate)
    cout << "Will never ever call PThread::Create" << endl;
  else
    cout << "Will use PThread::Create in preference is classes written by the author"  << endl;

  regularReporting = args.HasOption('r');
  if (regularReporting)
    cout << "Will generate reports every minute on current status " << endl;
  else
    cout << "will keep silent about progress" << endl;

  activeCount = 10;
  if (args.HasOption('c'))
    activeCount = args.GetOptionString('c').AsInteger();
  activeCount = PMIN(100, PMAX(1, activeCount));
  cout << "There will be " << activeCount << " threads in operation" << endl;

  delayThreadsActive.SetAutoDeleteObjects();

  UserInterfaceThread ui(*this);
  ui.Resume();
  ui.WaitForTermination();

  exitNow = PTrue;

  cerr << "in preexit delay, let all threads die" << endl;
  PThread::Sleep(delay * 2);
}

void SafeTest::OnReleased(DelayThread & delayThread)
{
  PString id = delayThread.GetId();
  PTRACE(3, "DelayThread " << id << " OnRelease");
  delayThreadsActive.RemoveAt(id);
  PTRACE(3, "DelayThread " << id << " OnRelease all done");
  --currentSize;
}

void SafeTest::OnReleased(const PString & delayThreadId)
{
  PTRACE(3, "DelayThread " << delayThreadId << " OnRelease");
  delayThreadsActive.RemoveAt(delayThreadId);
  PTRACE(3, "DelayThread " << delayThreadId << " OnRelease all done");
  --currentSize;
}
    
void SafeTest::AppendRunning(PSafePtr<DelayThread> delayThread, PString id)
{
  ++currentSize;
  PTRACE(3, "Add a delay thread of " << id);
  if (delayThreadsActive.FindWithLock(id) != NULL) {
    PAssertAlways("Appending multiple instances at the same id");
  }

  delayThreadsActive.SetAt(id, delayThread);
}

void SafeTest::DelayThreadsDict::DeleteObject(PObject * object) const
{
  DelayThread * delayThread = PDownCast(DelayThread, object);
  if (delayThread != NULL) {
    PTRACE(3, "Delete DelayThread " << *delayThread);
    delete delayThread;
  }
}

PBoolean SafeTest::UseOnThreadEnd()
{
  return useOnThreadEnd;
}

////////////////////////////////////////////////////////////////////////////////

OnDelayThreadEnd::OnDelayThreadEnd(SafeTest &_safeTest, const PString & _delayThreadId)
  : PThread(10000, AutoDeleteThread), 
    safeTest(_safeTest), 
    delayThreadId(_delayThreadId)
{
  Resume();
}

void OnDelayThreadEnd::Main()
{
  PThread::Sleep(1000);  //Let the DelayThread end good and proper

  safeTest.OnReleased(delayThreadId);
}

/////////////////////////////////////////////////////////////////////////////

DelayWorkerThread::DelayWorkerThread(DelayThread & _delayThread, PInt64 _iteration)
  : PThread(10000, AutoDeleteThread), 
    delayThread(_delayThread),
    iteration(_iteration)
{
  thisThreadName << iteration << " Delay Thread";
  SetThreadName(thisThreadName);
  Resume();
}

void DelayWorkerThread::Main()
{
  delayThread.DelayThreadMain(*this, 0000);
}

/////////////////////////////////////////////////////////////////////////////

DelayThreadTermination::DelayThreadTermination(DelayThread & _delayThread)
  : PThread(10000, AutoDeleteThread), 
    delayThread(_delayThread)
{
  thisThreadName <<"%X DT term";
  SetThreadName(thisThreadName);
  Resume();
}

void DelayThreadTermination::Main()
{
  delayThread.OnReleaseThreadMain(*this, 0000);
}

/////////////////////////////////////////////////////////////////////////////

DelayThread::DelayThread(SafeTest &_safeTest, PINDEX _delay, PInt64 iteration)
  : safeTest(_safeTest),
    delay(_delay)
{
  threadRunning = PTrue;

  PTRACE(5, "Constructor for a non auto deleted delay thread");

  if (safeTest.AvoidPThreadCreate()) {
    new DelayWorkerThread(*this, iteration);
  } else {
    name << PString(iteration) << " Delay Thread";
    PThread::Create(PCREATE_NOTIFIER(DelayThreadMain), 30000,
		    PThread::AutoDeleteThread,
		    PThread::NormalPriority,
		    name);
  }    
}

DelayThread::~DelayThread()
{
  if (threadRunning) {
    PAssertAlways("Destroy this thread while it is still running. Bad karma");
  }
    
  PTRACE(5, "Destructor for a delay thread");
}

void DelayThread::DelayThreadMain(PThread &thisThread, INT)  
{
  id = thisThread.GetThreadName();
  PTRACE(3, "DelayThread starting " << id);
  safeTest.AppendRunning(this, id);
  PThread::Sleep(delay);
  PTRACE(3, "DelayThread finished " << id);


  if (safeTest.UseOnThreadEnd()) {
    threadRunning = PFalse;
    new OnDelayThreadEnd(safeTest, id);
  } else {
    SafeReference();    
    Release();
  }
}

void DelayThread::Release()
{
    if (safeTest.AvoidPThreadCreate()) {
      new DelayThreadTermination(*this);
    } else {
      // Add a reference for the thread we are about to start
      PThread::Create(PCREATE_NOTIFIER(OnReleaseThreadMain), 10000,
		      PThread::AutoDeleteThread,
		      PThread::NormalPriority,
		      "%X: Release");
    }
}

void DelayThread::OnReleaseThreadMain(PThread &, INT)
{
  safeTest.OnReleased(*this);
  threadRunning = PFalse;
  SafeDereference();
}

void DelayThread::PrintOn(ostream & strm) const
{
  strm << id << " ";
}
///////////////////////////////////////////////////////////////////////////

  
ReporterThread::ReporterThread(LauncherThread & _launcher)
  : PThread(10000, NoAutoDeleteThread),
    launcher(_launcher)
{
  terminateNow = PFalse;
  Resume();
}

void ReporterThread::Terminate()
{
  terminateNow = PTrue;
  exitFlag.Signal();
}

void ReporterThread::Main()
{
  while (!terminateNow) {
    exitFlag.Wait(1000 * 60);
  
    launcher.ReportAverageTime();
    launcher.ReportIterations();
    launcher.ReportElapsedTime();
    cout << " " << endl << flush;
  }
}

LauncherThread::LauncherThread(SafeTest &_safeTest)
  : PThread(10000, NoAutoDeleteThread),
    safeTest(_safeTest)
{ 
  iteration = 0; 
  keepGoing = PTrue; 

  if (safeTest.RegularReporting())
    reporter = new ReporterThread(*this);
  else
    reporter = NULL;
}

LauncherThread::~LauncherThread()
{
  if (reporter != NULL) {
    reporter->Terminate();
    reporter->WaitForTermination();
    delete reporter;
    reporter = NULL;
  }
}
    

void LauncherThread::Main()
{

  PINDEX count = safeTest.ActiveCount();
  while(keepGoing) {
    PINDEX delay = safeTest.Delay() + safeTest.GetRandom();
    while(safeTest.CurrentSize() < count) {
      iteration++;
      void * location = new DelayThread(safeTest, delay, iteration);
      if (((PUInt64)location) < 0xffff) {
	int a, b;
	a = 1;
	b = 0;
	a = a/b;   //Your handy dandy divide by zero error. Immediate end.
      }
    }    
    PThread::Yield();
  }
}

void LauncherThread::ReportAverageTime()
{
  PInt64 i = GetIteration();
  if (i == 0) {
    cout << "Have not completed an iteration yet, so time per "
	 << "iteration is unavailable" << endl;
  } else {
    cout << "Average time per iteration is " 
	 << (GetElapsedTime().GetMilliSeconds()/((double) i)) 
	 << " milliseconds" << endl;
  }
}

void LauncherThread::ReportIterations()
{
  cout << "\nHave completed " 
       << GetIteration() << " iterations" << endl;
}

void LauncherThread::ReportElapsedTime()
{
  cout << "\nElapsed time is " 
       << GetElapsedTime() 
       << " (Hours:mins:seconds.millseconds)" << endl;
}
////////////////////////////////////////////////////////////////////////////////

void UserInterfaceThread::Main()
{
  PConsoleChannel console(PConsoleChannel::StandardInput);
  cout << "This program will repeatedly create and destroy a thread until "
       << "terminated from the console" << endl;

  PStringStream help;
  help << "Press : " << endl
       << "         D      average Delay time of each thread" << endl
       << "         H or ? help"                              << endl
       << "         R      report count of threads done"      << endl
       << "         T      time elapsed"                      << endl
       << "         X or Q exit "                             << endl;
 
  cout << endl << help;

  LauncherThread launch(safeTest);
  launch.Resume();

  console.SetReadTimeout(P_MAX_INDEX);
  for (;;) {
    int ch = console.ReadChar();

    switch (tolower(ch)) {
    case 'd' :
      {
        launch.ReportAverageTime();
        cout << "Command ? " << flush;
        break;
      }
    case 'r' :
      launch.ReportIterations();
      cout << "Command ? " << flush;
      break;
    case 't' :
      launch.ReportElapsedTime();
      cout << "Command ? " << flush;
      break;

    case 'x' :
    case 'q' :
      cout << "Exiting." << endl;
      launch.Terminate();
      launch.WaitForTermination();
      return;
      break;
    case '?' :
    case 'h' :
      cout << help << endl;
      cout << "Command ? " << flush;
    default:
      break;
    } // end switch

  } // end for

}


// End of File ///////////////////////////////////////////////////////////////
