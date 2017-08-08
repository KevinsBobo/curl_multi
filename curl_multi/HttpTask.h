#pragma once
#include "SkyMultiHttp.h"
#include <string>
#include <map>
using std::string;
using std::map;

#define TASK_VIST_REG   0
#define TASK_POST_REG   1
#define TASK_POST_LOGIN 2
#define TASK_VIST_ROOT  3
#define TASK_GET_LOGOUT 4

#define START_TASK_NUM 10
#define ALL_TASK_NUM   10

typedef struct _HttpInfo
{
  CSkyChaserHttp* pHttp;
  string* pstrRet;
  int nTask;
  int nTaskLevel;
  char* szBuff;
  inline _HttpInfo()
    : pHttp(NULL)
    , pstrRet(NULL)
    , nTask(0)
    , nTaskLevel(0)
    , szBuff(NULL)
  {
    pHttp = new CSkyChaserHttp(true);
    pstrRet = new string;
    szBuff = new char[ MAXBYTE ];
  }

  inline ~_HttpInfo()
  {
    if(pHttp != NULL)
    {
      // curl_easy_cleanup(pHttp->get_handle());  
      delete pHttp;
    }
    if(pstrRet != NULL)
    {
      delete pstrRet;
    }
    if(szBuff != NULL)
    {
      delete szBuff;
    }
  }
} HttpInfo;


typedef map<CURL* , HttpInfo*> MapType;

class CHttpTask
{
public:
  CHttpTask();
  ~CHttpTask();
  BOOL Init();
  static void TaskDoneProc(CURL* handle);
  void Start();
  static void SetEnv(char* szRegName , int nConcurrent , int nCount);

protected:
  static MapType m_mapTask;
  CSkyMultiHttp m_multi;
  void InitFloder(char* szFloder);
  void SetHttpHeader(CSkyChaserHttp& http , int num);
  void AddTask();
  static CRITICAL_SECTION m_csLock;
  static int m_nTask;
  static char* m_szRegName;
  static int   m_nConcurrent;
  static int   m_nCount;
};

