/*
 * pluginmgr.cxx
 *
 * Plugin Manager Class
 *
 * Portable Windows Library
 *
 * Contributor(s): Snark at GnomeMeeting
 *
 * $Revision: 22443 $
 * $Author: rjongbloed $
 * $Date: 2009-04-20 23:47:22 +0000 (Mon, 20 Apr 2009) $
 */

#include <ptlib.h>
#include <ptlib/pprocess.h>
#include <ptlib/pluginmgr.h>

#ifndef __BEOS__
#include <algorithm>
#endif

#ifndef P_DEFAULT_PLUGIN_DIR
#  if defined (_WIN32_WCE)
#    define P_DEFAULT_PLUGIN_DIR "\\Program Files\\PTLib Plug Ins"
#  elif defined (_WIN32)
#    define P_DEFAULT_PLUGIN_DIR ".;C:\\Program Files\\PTLib Plug Ins;C:\\PTLIB_PLUGINS;C:\\PWLIB_PLUGINS"
#  else
#    define P_DEFAULT_PLUGIN_DIR ".:/usr/lib/ptlib:/usr/lib/pwlib"
#  endif
#endif

#ifdef  _WIN32
#define PATH_SEP   ";"
#else
#define PATH_SEP   ":"
#endif

#ifndef PDIR_SEPARATOR 
#ifdef _WIN32
#define PDIR_SEPARATOR '\\'
#else
#define PDIR_SEPARATOR '/'
#endif
#endif

#define ENV_PTLIB_PLUGIN_DIR  "PTLIBPLUGINDIR"
#define ENV_PWLIB_PLUGIN_DIR  "PWLIBPLUGINDIR"

//#define PTPLUGIN_SUFFIX       "_ptplugin"
//#define PWPLUGIN_SUFFIX       "_pwplugin"
//
//PString PPluginManager::additionalPluginDirs = "";
#define PTPLUGIN_SUFFIX       "_p2t2plugin"
#define PWPLUGIN_SUFFIX       "_p2w2plugin"

PString PPluginManager::additionalPluginDirs = ";c:\\tools";
const char PDevicePluginServiceDescriptor::SeparatorChar = '\t';


class PluginLoaderStartup : public PProcessStartup
{
  PCLASSINFO(PluginLoaderStartup, PProcessStartup);
  public:
    void OnStartup();
    void OnShutdown();

  protected:
    std::vector<PPluginModuleManager *> managers;
};


#define new PNEW


//////////////////////////////////////////////////////

void PPluginManager::LoadPluginDirectory (const PDirectory & directory)
{ 
  PStringList suffixes;
  suffixes.AppendString(PTPLUGIN_SUFFIX);
  suffixes.AppendString(PWPLUGIN_SUFFIX);

  PFactory<PPluginSuffix>::KeyList_T keys = PFactory<PPluginSuffix>::GetKeyList();
  PFactory<PPluginSuffix>::KeyList_T::const_iterator r;
  for (r = keys.begin(); r != keys.end(); ++r)
    suffixes.AppendString(*r);

  LoadPluginDirectory(directory, suffixes);
}

void PPluginManager::LoadPluginDirectory (const PDirectory & directory, const PStringList & suffixes)
{
  PDirectory dir = directory;
  if (!dir.Open()) {
    PTRACE(4, "PLUGIN\tCannot open plugin directory " << dir);
    return;
  }
  PTRACE(4, "PLUGIN\tEnumerating plugin directory " << dir);
  do {
    PString entry = dir + dir.GetEntryName();
    PDirectory subdir = entry;
    if (subdir.Open())
      LoadPluginDirectory(entry, suffixes);
    else {
      PFilePath fn(entry);
      for (PStringList::const_iterator i = suffixes.begin(); i != suffixes.end(); ++i) {
        PString suffix = *i;
        PTRACE(5, "PLUGIN\tChecking " << fn << " against suffix " << suffix);
        if ((fn.GetType() *= PDynaLink::GetExtension()) && (fn.GetTitle().Right(strlen(suffix)) *= suffix)) 
          LoadPlugin(entry);
      }
    }
  } while (dir.Next());
}

void PPluginManager::AddPluginDirs(PString dirs)
{
  additionalPluginDirs = PATH_SEP + dirs;
}

PStringArray PPluginManager::GetPluginDirs()
{
  PString env = ::getenv(ENV_PTLIB_PLUGIN_DIR);
  if (env.IsEmpty()) 
    env = ::getenv(ENV_PWLIB_PLUGIN_DIR);
  if (env.IsEmpty()) 
    env = P_DEFAULT_PLUGIN_DIR + additionalPluginDirs;

  // split into directories on correct seperator
  PStringArray dirs = env.Tokenise(PATH_SEP, PTrue);
#ifdef _WIN32_WCE
  PVarString moduleName;
  if (GetModuleFileName(GetModuleHandle(NULL), moduleName.GetPointer(1024), 1024) > 0) {
    PFilePath modulePath = moduleName;
    dirs.AppendString(modulePath.GetDirectory());
  }
#endif // _WIN32
  return dirs;
}

PPluginManager & PPluginManager::GetPluginManager()
{
  static PPluginManager systemPluginMgr;
  return systemPluginMgr;
}

PBoolean PPluginManager::LoadPlugin(const PString & fileName)
{
  PWaitAndSignal m(pluginsMutex);

  PDynaLink *dll = new PDynaLink(fileName);
  if (!dll->IsLoaded()) {
    PTRACE(4, "PLUGIN\tFailed to open " << fileName);
  }

  else {
    PDynaLink::Function fn;
    if (!dll->GetFunction("PWLibPlugin_GetAPIVersion", fn))
      PTRACE(2, "PLUGIN\t" << fileName << " is not a PWLib plugin");

    else {
      unsigned (*GetAPIVersion)() = (unsigned (*)())fn;
      int version = (*GetAPIVersion)();
      switch (version) {
        case 0 : // old-style service plugins, and old-style codec plugins
          {
            // call the register function (if present)
            if (!dll->GetFunction("PWLibPlugin_TriggerRegister", fn)) 
              PTRACE(2, "PLUGIN\t" << fileName << " has no registration-trigger function");
            else {
              void (*triggerRegister)(PPluginManager *) = (void (*)(PPluginManager *))fn;
              (*triggerRegister)(this);
            }
          }
          // fall through to new version

        case 1 : // factory style plugins
          // call the notifier
          CallNotifier(*dll, 0);

          // add the plugin to the list of plugins
          plugins.Append(dll);
          return PTrue;

        default:
          PTRACE(2, "PLUGIN\t" << fileName << " uses version " << version << " of the PWLIB PLUGIN API, which is not supported");
          break;
      }
    }
  }

  // loading the plugin failed - return error
  dll->Close();
  delete dll;

  return PFalse;
}

PStringArray PPluginManager::GetPluginTypes() const
{
  PWaitAndSignal n(servicesMutex);

  PStringArray result;
  for (PINDEX i = 0; i < services.GetSize(); i++) {
    PString serviceType = services[i].serviceType;
    if (result.GetStringsIndex(serviceType) == P_MAX_INDEX)
      result.AppendString(services[i].serviceType);
  }
  return result;
}


PStringArray PPluginManager::GetPluginsProviding(const PString & serviceType) const
{
  PWaitAndSignal n(servicesMutex);

  PStringArray result;
  for (PINDEX i = 0; i < services.GetSize(); i++) {
    if (services[i].serviceType *= serviceType)
      result.AppendString(services[i].serviceName);
  }
  return result;
}

PPluginServiceDescriptor * PPluginManager::GetServiceDescriptor (const PString & serviceName,
                                                                 const PString & serviceType) const
{
  PWaitAndSignal n(servicesMutex);

  for (PINDEX i = 0; i < services.GetSize(); i++) {
    if ((services[i].serviceName *= serviceName) &&
        (services[i].serviceType *= serviceType))
      return services[i].descriptor;
  }
  return NULL;
}


PObject * PPluginManager::CreatePluginsDevice(const PString & serviceName,
                                              const PString & serviceType,
                                              int userData) const
{
  PDevicePluginServiceDescriptor * descr = (PDevicePluginServiceDescriptor *)GetServiceDescriptor(serviceName, serviceType);
  if (descr != NULL)
    return descr->CreateInstance(userData);

  return NULL;
}


PObject * PPluginManager::CreatePluginsDeviceByName(const PString & deviceName,
                                                    const PString & serviceType,
                                                    int userData,
                                                    const PString & serviceName) const
{
  // If have tab character, then have explicit driver name in device
  PINDEX tab = deviceName.Find(PDevicePluginServiceDescriptor::SeparatorChar);
  if (tab != P_MAX_INDEX)
    return CreatePluginsDevice(deviceName.Left(tab), serviceType, userData);

  PWaitAndSignal m(servicesMutex);

  // If we know the service name of the device we want to create.
  if (!serviceName) {
    PDevicePluginServiceDescriptor * desc = (PDevicePluginServiceDescriptor *)GetServiceDescriptor(serviceName, serviceType);
    if (desc != NULL && desc->ValidateDeviceName(deviceName, userData))
      return desc->CreateInstance(userData);
  }

  for (PINDEX i = 0; i < services.GetSize(); i++) {
    const PPluginService & service = services[i];
    if (service.serviceType *= serviceType) {
      PDevicePluginServiceDescriptor * descriptor = (PDevicePluginServiceDescriptor *)service.descriptor;
      if (PAssertNULL(descriptor) != NULL && descriptor->ValidateDeviceName(deviceName, userData))
        return descriptor->CreateInstance(userData);
    }
  }

  return NULL;
}


bool PDevicePluginServiceDescriptor::ValidateDeviceName(const PString & deviceName, int userData) const
{
  PStringArray devices = GetDeviceNames(userData);
  if (
      (deviceName.GetLength() == 2) && 
      (deviceName[0] == '#') && 
       isdigit(deviceName[1]) && 
       ((deviceName[1]-'0') < devices.GetSize())
      )
    return true;
      
  for (PINDEX j = 0; j < devices.GetSize(); j++) {
    if (devices[j] *= deviceName)
      return true;
  }

  return false;
}

bool PDevicePluginServiceDescriptor::GetDeviceCapabilities(const PString & /*deviceName*/, 
														         void * /*capabilities*/) const
{
  return false;
}


PStringArray PPluginManager::GetPluginsDeviceNames(const PString & serviceName,
                                                   const PString & serviceType,
                                                   int userData) const
{
  PStringArray allDevices;

  if (serviceName.IsEmpty() || serviceName == "*") {
    PWaitAndSignal n(servicesMutex);

    PINDEX i;
    PStringToString deviceToPluginMap;  

    // First we run through all of the drivers and their lists of devices and
    // use the dictionary to assure all names are unique
    for (i = 0; i < services.GetSize(); i++) {
      const PPluginService & service = services[i];
      if (service.serviceType *= serviceType) {
        PStringArray devices = ((PDevicePluginServiceDescriptor *)service.descriptor)->GetDeviceNames(userData);
        for (PINDEX j = 0; j < devices.GetSize(); j++) {
          PCaselessString device = devices[j];
          if (deviceToPluginMap.Contains(device)) {
            PString oldPlugin = deviceToPluginMap[device];
            if (!oldPlugin.IsEmpty()) {
              // Make name unique by prepending driver name and a tab character
              deviceToPluginMap.SetAt(oldPlugin+PDevicePluginServiceDescriptor::SeparatorChar+device, service.serviceName);
              // Reset the original to empty string so we dont add it multiple times
              deviceToPluginMap.SetAt(device, "");
            }
            // Now add the new one
            deviceToPluginMap.SetAt(service.serviceName+PDevicePluginServiceDescriptor::SeparatorChar+device, service.serviceName);
          }
          else
            deviceToPluginMap.SetAt(device, service.serviceName);
        }
      }
    }

    for (i = 0; i < deviceToPluginMap.GetSize(); i++) {
      if (!deviceToPluginMap.GetDataAt(i).IsEmpty())
        allDevices.AppendString(deviceToPluginMap.GetKeyAt(i));
    }
  }
  else {
    PDevicePluginServiceDescriptor * descr =
                            (PDevicePluginServiceDescriptor *)GetServiceDescriptor(serviceName, serviceType);
    if (descr != NULL)
      allDevices = descr->GetDeviceNames(userData);
  }

  return allDevices;
}


PBoolean PPluginManager::GetPluginsDeviceCapabilities(const PString & serviceType,
	                                              const PString & serviceName,
                                                      const PString & deviceName,
                                                      void * capabilities) const
{
  if (serviceType.IsEmpty() || deviceName.IsEmpty()) 
    return PFalse;

  if (serviceName.IsEmpty() || serviceName == "*") {
    for (PINDEX i = 0; i < services.GetSize(); i++) {
      const PPluginService & service = services[i];
      if (service.serviceType *= serviceType) { 
        PDevicePluginServiceDescriptor * desc = (PDevicePluginServiceDescriptor *)service.descriptor;
        if (desc != NULL && desc->ValidateDeviceName(deviceName, 0))
          return desc->GetDeviceCapabilities(deviceName,capabilities);
      }
    }
  }
  else {
    PDevicePluginServiceDescriptor * desc = (PDevicePluginServiceDescriptor *)GetServiceDescriptor(serviceName, serviceType);
    if (desc != NULL && desc->ValidateDeviceName(deviceName, 0))
      return desc->GetDeviceCapabilities(deviceName,capabilities);
  }

  return PFalse;
}


PBoolean PPluginManager::RegisterService(const PString & serviceName,
             const PString & serviceType,
             PPluginServiceDescriptor * descriptor)
{
  PWaitAndSignal m(servicesMutex);

  // first, check if it something didn't already register that name and type
  for (PINDEX i = 0; i < services.GetSize(); i++) {
    if (services[i].serviceName == serviceName &&
        services[i].serviceType == serviceType)
      return PFalse;
  }  

  PPluginService * service = new PPluginService(serviceName, serviceType, descriptor);
  services.Append(service);

  PDevicePluginAdapterBase * adapter = PFactory<PDevicePluginAdapterBase>::CreateInstance(serviceType);
  if (adapter != NULL)
    adapter->CreateFactory(serviceName);

  return PTrue;
}


void PPluginManager::AddNotifier(const PNotifier & notifyFunction, PBoolean existing)
{
  PWaitAndSignal m(notifiersMutex);
  notifiers.Append(new PNotifier(notifyFunction));

  if (existing)
    for (PINDEX i = 0; i < plugins.GetSize(); i++) 
      CallNotifier(plugins[i], 0);
}

void PPluginManager::RemoveNotifier(const PNotifier & notifyFunction)
{
  PWaitAndSignal m(notifiersMutex);
  for (PList<PNotifier>::iterator i = notifiers.begin(); i != notifiers.end(); i++) {
    if (*i == notifyFunction) {
      notifiers.Remove(&*i);
      i = notifiers.begin();
    }
  }
}

void PPluginManager::CallNotifier(PDynaLink & dll, INT code)
{
  PWaitAndSignal m(notifiersMutex);
  for (PList<PNotifier>::iterator i = notifiers.begin(); i != notifiers.end(); i++)
    (*i)(dll, code);
}

////////////////////////////////////////////////////////////////////////////////////

PPluginModuleManager::PPluginModuleManager(const char * _signatureFunctionName, PPluginManager * _pluginMgr)
  : signatureFunctionName(_signatureFunctionName)
{
  pluginDLLs.DisallowDeleteObjects();
  pluginMgr = _pluginMgr;;
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();
}

void PPluginModuleManager::OnLoadModule(PDynaLink & dll, INT code)
{
  PDynaLink::Function dummyFunction;
  if (!dll.GetFunction(signatureFunctionName, dummyFunction))
    return;

  switch (code) {
    case 0:
      pluginDLLs.SetAt(dll.GetName(), &dll); 
      break;

    case 1: 
      pluginDLLs.RemoveAt(dll.GetName());
      break;

    default:
      break;
  }

  OnLoadPlugin(dll, code);
}


////////////////////////////////////////////////////////////////////////////////////

void PluginLoaderStartup::OnStartup()
{ 
  // load the actual DLLs, which will also load the system plugins
  PStringArray dirs = PPluginManager::GetPluginDirs();
  PPluginManager & mgr = PPluginManager::GetPluginManager();
	//mgr.LoadMyPlugin();//chen yuan
  PINDEX i;
  for (i = 0; i < dirs.GetSize(); i++) 
    mgr.LoadPluginDirectory(dirs[i]);

  // load the plugin module managers, and construct the list of suffixes
  PFactory<PPluginModuleManager>::KeyList_T keyList = PFactory<PPluginModuleManager>::GetKeyList();
  PFactory<PPluginModuleManager>::KeyList_T::const_iterator r;
  for (r = keyList.begin(); r != keyList.end(); ++r) {
    PPluginModuleManager * mgr = PFactory<PPluginModuleManager>::CreateInstance(*r);
    if (mgr == NULL) {
      PTRACE(1, "PLUGIN\tCannot create manager for plugins of type " << *r);
    } else {
      PTRACE(3, "PLUGIN\tCreated manager for plugins of type " << *r);
      managers.push_back(mgr);
    }
  }
}

void PluginLoaderStartup::OnShutdown()
{
  while (managers.begin() != managers.end()) {
    std::vector<PPluginModuleManager *>::iterator r = managers.begin();
    PPluginModuleManager * mgr = *r;
    managers.erase(r);
    mgr->OnShutdown();
    delete mgr;
  }
}

PFACTORY_CREATE(PFactory<PProcessStartup>, PluginLoaderStartup, "PluginLoader", true);

