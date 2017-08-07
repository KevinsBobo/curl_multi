#include "HttpTask.h"

int main()
{
  time_t start_time = 0;
  time_t stop_time = 0;

  char szRegName[ MAXBYTE ] = { 0 };
  int  nConcurrent = 0;
  int  nCount = 0;

  printf("请输入要注册的用户名，注册的用户名将为xxx1,xxx2...，密码为用户名\n：");
  scanf_s("%s", szRegName , MAXBYTE );
  fflush(stdin);
  printf("请输入并发数量：");
  scanf_s("%d" , &nConcurrent);
  fflush(stdin);
  printf("请输入要注册的数量：");
  scanf_s("%d" , &nCount);
  fflush(stdin);

  CHttpTask::SetEnv(szRegName , nConcurrent , nCount);

  time(&start_time);

  CHttpTask myTask;

  if(!myTask.Init())
  {
    return 0;
  }

  myTask.Start();


  time(&stop_time);

  printf("共用时 %lld 秒，按任意键继续..." , stop_time - start_time);
  getchar();
  return 0;
}