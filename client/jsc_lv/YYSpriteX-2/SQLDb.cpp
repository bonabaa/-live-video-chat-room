#include "StdAfx.h"
#include ".\sqldb.h"
#include <ptlib.h>
#include "lpnmgr.h"
 extern PString  GetUserPath(unsigned int uid);
 extern PString PlainTextToUTF8(const char* lpszContent);
#define D_IM_KEY  "SSBNOiAo"  
 //+	PAbstractArray	{elementSize=0x00000001 theArray=0x0a0a2bd8 "<f conf_id="20072007" sentence_id="20071343363632" from_vfonid="2007" to_vfonid="2007" send_time="1343363632" send_text="KFNpbVN1bjsxNDswMDs0Mjc4MTkwMzM1OylJIE06ICBkZGRkFg==" ></f>" allocatedDynamically=true }	const PAbstractArray
 //  +	PAbstractArray	{elementSize=0x00000001 theArray=0x0a0ab8f8 "<f conf_id="20072007" sentence_id="20071343363732" from_vfonid="2007" to_vfonid="2007" send_time="1343363732" send_text="KEFyaWFsOzE0OzAwOzQyNzgxOTAzMzU7KUkgTTogIGRkZBY=" ></f>" allocatedDynamically=true }	const PAbstractArray


// +	data	0x06e0fb8e "KEFyaWFsOzE0OzAwOzQyNzgxOTAwODA7KUkgTTogIDIW"	char [1]
 //+	data	0x06d19b3e "KEFyaWFsOzE0OzAwOzQyNzgxOTAwODA7KUkgTTogIGRkZBY="	char [1]


 //send_text=      "KEFyaWFsOzE0OzAwOzQyNzgxOTAwODA7KUkgTTogIGdnZxY="
 //                 KEFyaWFsOzE0OzAwOzQyNzgxOTAwODA7KUkgTTogIDEW" ></f>" allocatedDynamically=true }	const PAbstractArray
 //                 KEFyaWFsOzE0OzAwOzQyNzgxOTAwODA7KUYgTDogICAg5pS25Yiw5paH5Lu26K+35rGCIDonMC0xMDEtc29hcHVpLXByb2plY3QueG1sJzsyOzhjMzFhZTlhLTYzMDctMTkxMC04MGZmLTAwMjZiOTFiZTVlNjtDOlxVc2Vyc1xibmJcRG9jdW1lbnRzXDAtMTAxLXNvYXB1aS1wcm9qZWN0LnhtbA=="	char [1]

 


CSQLDb::CSQLDb( )
{

  m_nOwerUID =-1;
}
bool CSQLDb::Open(unsigned int uid)
{
  try{
    PWaitAndSignal lockit(m_mxDB);
    m_uidfrom= m_uidto =m_send_time=0;
    m_nOwerUID =uid;
    PString strPathFile= GetUserPath(uid)+"\\im.db";
    m_db.close();
   
    m_db.open((const char*)PlainTextToUTF8(strPathFile));
    if (m_db.tableExists("im_msg") == false)
      m_db.execDML("create table im_msg(uidfrom int, uidto int, send_time,  sentence_id char(38), msg char(1000));");

  }catch(...){
    
  }
  return true;
}

CSQLDb::~CSQLDb(void)
{
}
bool CSQLDb::SaveIM(const BBQOfflineIMMessage* pIM, const PString& strIMValue, bool bSenderOrReceiver)
{
  try{
     
    if (strIMValue.Find(D_IM_KEY) ==P_MAX_INDEX) return true;//只保存 IM消息
    PWaitAndSignal lockit(m_mxDB);
    int uFrom =pIM->from_vfonid,nTo = pIM->to_vfonid;
    if ( bSenderOrReceiver ==false && m_uidfrom ==uFrom &&  m_uidto== nTo &&m_send_time == pIM->send_time ){
      return false;
    }else {
      m_uidfrom =uFrom ; m_uidto= nTo ;m_send_time = pIM->send_time;
      int uFrom = pIM->from_vfonid, nTo=pIM->to_vfonid;
      PString strBuf ;
      uint32 nsend_time = pIM->send_time;
      strBuf.sprintf(  "insert into im_msg (uidfrom, uidto,send_time,  sentence_id,msg ) values ( %d,%d,%u, '%s','%s' );",uFrom,nTo,nsend_time, pIM->sentence_id, (const char*)strIMValue   );
      return 1== m_db.execDML(strBuf);
        
    }
  }catch(...){
    return true;
  }

}
bool CSQLDb::GetIM(int nStart, int nEnd, int uIDChatWithMe  ,PString& strIMValue/* out*/)
{
  try{
    bool bResult =false;
    strIMValue="<?xml version=\"1.0\" encoding=\"utf-8\" ?><list>";
    //int uidFrom =m_nOwerUID  ;
    PWaitAndSignal lockit(m_mxDB);
    PString strSQL;
    if (uIDChatWithMe > D_FREEGROUP_ID_UPPERLIMIT && m_nOwerUID>  D_FREEGROUP_ID_UPPERLIMIT) 
      strSQL.sprintf("select * from im_msg where (uidfrom=%d and uidto =%d) or (uidfrom=%d and uidto =%d) order by send_time;",m_nOwerUID,  uIDChatWithMe, uIDChatWithMe,m_nOwerUID );
    else{
      //freegroup chat 
      int nFreeGroupID = -1;
      if (m_nOwerUID < D_FREEGROUP_ID_UPPERLIMIT)
        nFreeGroupID = m_nOwerUID;
      else if (uIDChatWithMe < D_FREEGROUP_ID_UPPERLIMIT)
        nFreeGroupID = uIDChatWithMe;

      strSQL.sprintf("select * from im_msg where (uidfrom=%d or uidto =%d)   order by send_time;",nFreeGroupID,  nFreeGroupID  );

    }
    CppSQLite3Query q = m_db.execQuery(strSQL);
    int i=0;
    
    while(q.eof() ==false   ){
      if (i<nStart)
        ;
      else{
        bResult=true;
  #if 0
        PString strNotIncludeXMLHeader;
	      strNotIncludeXMLHeader.sprintf("<f conf_id=\"%s\" sentence_id=\"%s\" from_vfonid=\"%d\" to_vfonid=\"%d\" send_time=\"%u\" send_text=\"%s\" ></f>",
		      "0",
		      q.getStringField("sentence_id") ,
          q.getIntField("uidfrom"),
		      q.getIntField("uidto"),
		      q.getIntField("send_time"),
		      q.getStringField("msg")
		      );
      strIMValue+=strNotIncludeXMLHeader;
  #else
        strIMValue+=  q.getStringField("msg");
  #endif
      //
       
      }
      q.nextRow();
      i++;
    }
    strIMValue+="</list>";
   
    return bResult;
  }catch(...){
  }
  return "";
}