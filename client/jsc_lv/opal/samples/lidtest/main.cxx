/*
 * main.cxx
 *
 * OPAL Line Interface Device test environment
 *
 * Copyright (C) 2008 Vox Lucida
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
 * The Original Code is Open H323
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 20397 $
 * $Author: rjongbloed $
 * $Date: 2008-06-05 03:32:28 +0000 (Thu, 05 Jun 2008) $
 */

#include <ptlib.h>

#include <lids/lidpluginmgr.h>


class LidTest : public PProcess
{
  PCLASSINFO(LidTest, PProcess)

  public:
    LidTest();

    void Main();
};

#define new PNEW

PCREATE_PROCESS(LidTest)


///////////////////////////////////////////////////////////////

LidTest::LidTest()
  : PProcess("Vox Lucida", "LidTest")
{
}


inline static PString DisplayableString(const char * str)
{
  return str != NULL ? str : "(none)";
}


void DisplayDefn(PluginLID_Definition & defn)
{
  cout << defn.name << endl
       << "  Timestamp: " << PTime().AsString(PTime::RFC1123) << endl
       << "  Device" << endl
       << "    Description:  " << DisplayableString(defn.description) << endl
       << "    Manufacturer: " << DisplayableString(defn.manufacturer) << endl
       << "    Model:        " << DisplayableString(defn.model) << endl
       << "    Revision:     " << DisplayableString(defn.revision) << endl
       << "    Email:        " << DisplayableString(defn.manufacturerEmail) << endl
       << "    URL:          " << DisplayableString(defn.manufacturerURL) << endl
       << "  Source" << endl
       << "    Author:       " << DisplayableString(defn.author) << endl
       << "    Email:        " << DisplayableString(defn.authorEmail) << endl
       << "    URL:          " << DisplayableString(defn.authorURL) << endl
       << "    Copyright:    " << DisplayableString(defn.copyright) << endl
       << "    License:      " << DisplayableString(defn.license) << endl
       << "    Version:      " << DisplayableString(defn.version) << endl
       ;
}


void DisplayPlugInInfo(const PString & name, const PPluginModuleManager::PluginListType & pluginList)
{
  for (int i = 0; i < pluginList.GetSize(); i++) {
    if (pluginList.GetKeyAt(i) == name) {
      PDynaLink & dll = pluginList.GetDataAt(i);
      PluginLID_GetDefinitionsFunction getDefinitions;
      if (!dll.GetFunction(PLUGIN_LID_GET_LIDS_FN_STR, (PDynaLink::Function &)getDefinitions)) {
        cout << "error: " << name << " is missing the function " << PLUGIN_LID_GET_LIDS_FN_STR << endl;
        return;
      }

      unsigned int count;
      PluginLID_Definition * lids = (*getDefinitions)(&count, PWLIB_PLUGIN_API_VERSION);
      if (lids == NULL || count == 0) {
        cout << "error: " << name << " does not define any LIDs for this version of the plugin API" << endl;
        return;
      } 
      cout << name << " contains " << count << " LIDs:" << endl;
      for (unsigned j = 0; j < count; j++) {
        cout << "---------------------------------------" << endl
            << "LID " << j+1 << endl;
        DisplayDefn(lids[j]);
      }
      return;
    }
  }

  cout << "error: plugin \"" << name << "\" not found, specify one of :\n"
       << setfill('\n') << pluginList << setfill(' ') << endl;
}


OpalLineInterfaceDevice * OpenLID(const PString & type, const PString & name)
{
  OpalLineInterfaceDevice * device = OpalLineInterfaceDevice::Create(type);
  if (device == NULL) {
    cerr << "Could not create a LID using type \"" << type << '"' << endl;
    return NULL;
  }

  PString adjustedName = name;
  if (name.IsEmpty()) {
    PStringArray names = device->GetAllNames();
    if (names.IsEmpty())
      return NULL;
    adjustedName = names[0];
  }
  if (device->Open(adjustedName) && device->IsLinePresent(0))
    return device;

  cerr << "Could not open a \"" << type << "\" LID using name \"" << name << '"' << endl;
  delete device;
  return NULL;
}


void TestRing(OpalLineInterfaceDevice * device)
{
  if (device == NULL)
    return;

  device->RingLine(0, 1);
  PThread::Sleep(2000);
  device->RingLine(0, 0);
  PThread::Sleep(2000);

  delete device;
}


void TestButtons(OpalLineInterfaceDevice * device)
{
  if (device == NULL)
    return;

  cout << "Press '#' to exit." << endl;
  bool oldHook = false;

  char digit;
  do {
    PThread::Sleep(100);

    bool newHook = device->IsLineOffHook(0);
    if (oldHook != newHook) {
      cout << "Line " << (newHook ? "OFF" : "ON") << " hook." << endl;
      oldHook = newHook;
    }

    digit = device->ReadDTMF(0);
    if (digit == '\0')
      continue;

    cout << "Read DTMF: ";
    if (isprint(digit))
      cout << digit;
    else
      cout << "0x" << hex << (unsigned)digit << dec;
    cout << endl;

  } while (digit != '#');

  delete device;
}


void LidTest::Main()
{
  cout << GetName()
       << " Version " << GetVersion(PTrue)
       << " by " << GetManufacturer()
       << " on " << GetOSClass() << ' ' << GetOSName()
       << " (" << GetOSVersion() << '-' << GetOSHardware() << ")\n\n";

  // Get and parse all of the command line arguments.
  PArgList & args = GetArguments();
  args.Parse(
             "p-pluginlist."
             "i-info:"
             "l-list."
             "T-type:"
             "N-name:"
             "r-ring."
             "b-buttons."
#if PTRACING
             "t-trace."
             "o-output:"
#endif
             , PFalse);

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  bool needHelp = true;

  OpalPluginLIDManager & lidMgr = *(OpalPluginLIDManager *)PFactory<PPluginModuleManager>::CreateInstance("OpalPluginLIDManager");
  PPluginModuleManager::PluginListType pluginList = lidMgr.GetPluginList();

  if (args.HasOption('p')) {
    cout << "Plugin LIDs:" << endl;
    for (int i = 0; i < pluginList.GetSize(); i++)
      cout << "   " << pluginList.GetKeyAt(i) << endl;
    cout << "\n\n";
    needHelp = false;
  }

  if (args.HasOption('i')) {
    PStringArray plugins = args.GetOptionString('i').Lines();
    for (PINDEX i = 0; i < plugins.GetSize(); i++)
      DisplayPlugInInfo(plugins[i], pluginList);
    needHelp = false;
  }

  if (args.HasOption('l')) {
    PStringList devices = OpalLineInterfaceDevice::GetAllDevices();
    cout << "LIDs:" << endl;
    for (int i = 0; i < devices.GetSize(); i++)
      cout << "   " << devices[i] << endl;
    cout << "\n\n";
    needHelp = false;
  }

  if (args.HasOption('r')) {
    TestRing(OpenLID(args.GetOptionString('T'), args.GetOptionString('N')));
    needHelp = false;
  }

  if (args.HasOption('b')) {
    TestButtons(OpenLID(args.GetOptionString('T'), args.GetOptionString('N')));
    needHelp = false;
  }

  if (needHelp) {
    cout << "available options:" << endl
        << "  -p --pluginlist     display codec plugins\n"
        << "  -i --info name      display info about a plugin\n"
        << "  -l --list           List all LID types and device names\n"
        << "  -T --type type      Set LID device type\n"
        << "  -N --name dev       Set LID device name\n"
        << "  -r --ring           Test ring on LID\n"
        << "  -b --buttons        Test buttons on LID\n"
        << "  -t --trace          Increment trace level\n"
        << "  -o --output         Trace output file\n"
        << "  -h --help           display this help message\n";
  }
}
