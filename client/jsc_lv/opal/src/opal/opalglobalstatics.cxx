/*
 * opalglobalstatics.cxx
 *
 * Various global statics that need to be instantiated upon startup
 *
 * Portable Windows Library
 *
 * Copyright (C) 2004 Post Increment
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
 * $Revision: 21004 $
 * $Author: rjongbloed $
 * $Date: 2008-09-16 07:08:56 +0000 (Tue, 16 Sep 2008) $
 */

#include <ptlib.h>

#include <opal/buildopts.h>

#include <opal/mediafmt.h>
#include <opal/mediatype.h>
#include <codec/opalwavfile.h>

#include <codec/opalpluginmgr.h>
#ifdef _WIN32
#include <lids/lidpluginmgr.h>
#endif
#include <rtp/srtp.h>
#include <t38/t38proto.h>

#if OPAL_RFC4175
#include <codec/rfc4175.h>
#endif

//
// G.711 is *always* available
// Yes, it would make more sense for this to be in g711codec.cxx, but on 
// Linux it would not get loaded due to static initialisation optimisation
//
#include <codec/g711codec.h>
//OPAL_REGISTER_G711();

#if defined(P_PLUGINS)
class PluginLoader : public PProcessStartup
{
  PCLASSINFO(PluginLoader, PProcessStartup);
  public:
    void OnStartup()
    { 
#ifdef OPAL_PLUGIN_DIR
      PPluginManager::AddPluginDirs(OPAL_PLUGIN_DIR);
#endif
      OpalPluginCodecManager::Bootstrap(); 
      PWLibStupidLinkerHacks::mediaTypeLoader = 1;
      PWLibStupidLinkerHacks::opalwavfileLoader =1;
#if OPAL_SRTP
      PWLibStupidLinkerHacks::libSRTPLoader = 1;
#endif
#if OPAL_FAX
      PWLibStupidLinkerHacks::t38Loader = 1;
#endif
#if OPAL_RFC4175
      PWLibStupidLinkerHacks::rfc4175Loader = 1;
#endif
    }
};

static PFactory<PPluginModuleManager>::Worker<OpalPluginCodecManager> opalPluginCodecManagerFactory("OpalPluginCodecManager", true);
#if OPAL_LID
//static PFactory<PPluginModuleManager>::Worker<OpalPluginLIDManager> opalPluginLIDManagerFactory("OpalPluginLIDManager", true);
#endif
static PFactory<PProcessStartup>::Worker<PluginLoader> opalpluginStartupFactory("OpalPluginLoader", true);

#endif // P_PLUGINS

namespace PWLibStupidLinkerHacks {

int opalLoader;

#if OPAL_PTLIB_WAVFILE
extern int opalwavfileLoader;
#endif

}; // namespace PWLibStupidLinkerHacks

//////////////////////////////////

