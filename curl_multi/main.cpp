#include "HttpTask.h"

int main()
{
  time_t start_time = 0;
  time_t stop_time = 0;

  char szRegName[ MAXBYTE ] = { 0 };
  int  nConcurrent = 0;
  int  nCount = 0;

  printf("������Ҫע����û�����ע����û�����Ϊxxx1,xxx2...������Ϊ�û���\n��");
  scanf_s("%s", szRegName , MAXBYTE );
  fflush(stdin);
  printf("�����벢��������");
  scanf_s("%d" , &nConcurrent);
  fflush(stdin);
  printf("������Ҫע���������");
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

  printf("����ʱ %lld �룬�����������..." , stop_time - start_time);
  getchar();
  return 0;
}