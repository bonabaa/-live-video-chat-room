
#include <map>
#include <string>
using namespace std;

const char*  gStrings[]= {
  "local", "local",
  "callincoming", "Is calling you: %s( %s )",
  "desktopincoming", "正在请求桌面共享: %s( %s )",
  "videosize", "视频大小",
    
};
std::map<std::string, std::string> g_Strings;
bool g_InitMultLanguageString( )
{
  g_Strings.clear();
  int nCount =   sizeof(gStrings)/ sizeof(char*);
  for (int i=0;i< nCount;i+=2)
  {
    g_Strings[gStrings[i] ]= gStrings[i+1];
  }
  return true;
}
const char* g_GetStrings(const char* key)
{
   
  std::map<std::string, std::string>::iterator iter = g_Strings.find( key);
  if (iter != g_Strings.end()) return iter->second.c_str();
  else 
    return "";
}