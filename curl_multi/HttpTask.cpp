#include "stdafx.h"
#include "HttpTask.h"

int CHttpTask::m_nTask = 0;
MapType CHttpTask::m_mapTask;
CRITICAL_SECTION CHttpTask::m_csLock;

char* CHttpTask::m_szRegName = "task_robot";
int   CHttpTask::m_nConcurrent = START_TASK_NUM;
int   CHttpTask::m_nCount = ALL_TASK_NUM;

CHttpTask::CHttpTask()
  : m_multi(TaskDoneProc)
{
  ::InitializeCriticalSection(&m_csLock);
}


CHttpTask::~CHttpTask()
{
  ::DeleteCriticalSection(&m_csLock);

  auto it = m_mapTask.begin();
  for(; it != m_mapTask.end(); ++it)
  {
    if(it->second != NULL)
    {
      delete it->second;
    }
  }
}


BOOL CHttpTask::Init()
{
  if(!m_multi.Init())
  {
    return FALSE;
  }

  return TRUE;
}


// 执行完成后的回调
void CHttpTask::TaskDoneProc(CURL* handle)
{
  long response_code = 0;

  auto it = m_mapTask.find(handle);
  if(it == m_mapTask.end())
  {
    return;
  }

  HttpInfo* pInfo = (*it).second;

  curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE,
                     &response_code);

  printf("Task %d, Level %d, RET_CODE %d, DONE\n" ,
         pInfo->nTask,
         pInfo->nTaskLevel,
         response_code);

  switch(pInfo->nTaskLevel)
  {
    // 设置第二个任务
    case TASK_VIST_REG:
    {
      ::EnterCriticalSection(&m_csLock);
      // char* szBuf = new char[256];
      sprintf_s(pInfo->szBuff , MAXBYTE ,
                "name=%s%d&email=%s%d%%40test.com&password=%s&password1=%s&submit=%%E6%%B3%%A8%%E5%%86%%8C" ,
                m_szRegName,
                pInfo->nTask,
                m_szRegName,
                pInfo->nTask,
                m_szRegName,
                m_szRegName);

      // string strPostData = szBuf;
      pInfo->pHttp->sc_post("http://xxxxxxxx.com/user/reg/", pInfo->szBuff);
      curl_multi_add_handle(CSkyMultiHttp::m_curl_handle, pInfo->pHttp->get_handle());  
      pInfo->nTaskLevel = TASK_POST_REG;
      ::LeaveCriticalSection(&m_csLock);
    }
      break;

    // 设置第三个任务
    case TASK_POST_REG:
    {
      ::EnterCriticalSection(&m_csLock);
      // char szBuf[256] = { 0 };
      sprintf_s(pInfo->szBuff , MAXBYTE ,
                "name=%s%d&password=%s&submit=%%E7%%99%%BB%%E5%%BD%%95" ,
                m_szRegName,
                pInfo->nTask,
                m_szRegName);

      // string strPostData = szBuf;
      pInfo->pHttp->sc_post("http://xxxxxxxx.com/user/login/", pInfo->szBuff);
      curl_multi_add_handle(CSkyMultiHttp::m_curl_handle, pInfo->pHttp->get_handle());  
      pInfo->nTaskLevel = TASK_POST_LOGIN;
      ::LeaveCriticalSection(&m_csLock);
    }
      break;

    // 设置第四个任务
    case TASK_POST_LOGIN:
      ::EnterCriticalSection(&m_csLock);
      pInfo->pHttp->sc_get("http://xxxxxxxx.com/", *(pInfo->pstrRet));
      curl_multi_add_handle(CSkyMultiHttp::m_curl_handle, pInfo->pHttp->get_handle());  
      pInfo->nTaskLevel = TASK_VIST_ROOT;
      ::LeaveCriticalSection(&m_csLock);
      break;

    // 设置第五个任务
    case TASK_VIST_ROOT:
    {
      ::EnterCriticalSection(&m_csLock);
      // char szBuf[ 50 ] = { 0 };
      sprintf_s(pInfo->szBuff , MAXBYTE, "%s%d" , m_szRegName, pInfo->nTask);

      // 检查是否登录成功
      if(pInfo->pstrRet->find(pInfo->szBuff) != string::npos)
      {
        printf("%s 注册成功\n", pInfo->szBuff);
      }
      else
      {
        printf("%s 注册失败\n", pInfo->szBuff);
      }

      pInfo->pHttp->sc_get("http://xxxxxxxx.com/user/logout/");
      curl_multi_add_handle(CSkyMultiHttp::m_curl_handle, pInfo->pHttp->get_handle());  
      pInfo->nTaskLevel = TASK_GET_LOGOUT;
      ::LeaveCriticalSection(&m_csLock);
    }
      break;

    // 设置第一个任务
    case TASK_GET_LOGOUT:
      ::EnterCriticalSection(&m_csLock);
      if(m_nTask >= m_nCount)
      {
        ::LeaveCriticalSection(&m_csLock);
        break;
      }
      pInfo->nTask = ++m_nTask;
      ::LeaveCriticalSection(&m_csLock);
      pInfo->nTaskLevel = TASK_VIST_REG;
      pInfo->pHttp->sc_get("http://xxxxxxxx.com/user/reg/");
      curl_multi_add_handle(CSkyMultiHttp::m_curl_handle, pInfo->pHttp->get_handle());  
      break;

    default:
      break;
  }
  
  // fprintf(stderr, "Added task %d, Level %d\n", pInfo->nTask, pInfo->nTaskLevel);  
}

void CHttpTask::AddTask()
{
  HttpInfo* pInfo = NULL;
  CURL* handle = NULL;


  // 添加第一个任务
  for(int i = 0; i < m_nConcurrent && i < m_nCount; ++i)
  {
    pInfo = new HttpInfo;

    pInfo->nTask = ++m_nTask;
    pInfo->nTaskLevel = TASK_VIST_REG;

    handle = pInfo->pHttp->get_handle();
    SetHttpHeader(*(pInfo->pHttp) , m_nTask);
    pInfo->pHttp->sc_get("http://xxxxxxxx.com/user/reg/");

    m_mapTask.insert(MapType::value_type(handle , pInfo));

    //调用curl_multi_add_handle添加handle后，会回调start_timeout，然后调用  
    curl_multi_add_handle(CSkyMultiHttp::m_curl_handle, handle);  
    // fprintf(stderr, "Added task %d, Level %d\n", pInfo->nTask, pInfo->nTaskLevel);  
  }
}

void CHttpTask::Start()
{
  //事件循环由 uv_run 函数封装, 在使用libuv编程时, 该函数通常在最后才被调用.  
  AddTask();
  m_multi.loop();
}

void CHttpTask::InitFloder(char* szFloder)
{
  char szCmd[ MAXBYTE ] = { 0 };
  sprintf_s(szCmd ,  MAXBYTE,
            "if not exist \"%s\" (md \"%s\")" , szFloder, szFloder);
  system(szCmd);
}
  
void CHttpTask::SetHttpHeader(CSkyChaserHttp& http , int num)
{
  char* cookies_floder = "cookies";
  InitFloder(cookies_floder);
  char cookiefile[50];  
  sprintf(cookiefile, "%s\\%d.cookie", cookies_floder , num);  

  http.sc_setCookieFile(cookiefile);

  //设置统一的头部内容
  http.sc_cleanHeader();
  http.sc_appendHeader("Accept:text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
  http.sc_appendHeader("Accept-Encoding:gzip, deflate, sdch");
  http.sc_appendHeader("Accept-Language:zh-CN,zh;q=0.8,en;q=0.6");
  http.sc_appendHeader("Connection:keep-alive");
  http.sc_appendHeader("User-Agent:Mozilla/3.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko)");

  // http.sc_setProxy("10.0.2.2:8080");
}
  
void CHttpTask::SetEnv(char* szRegName , int nConcurrent , int nCount)
{
  m_szRegName = szRegName;
  m_nConcurrent = nConcurrent;
  m_nCount = nCount;
}
