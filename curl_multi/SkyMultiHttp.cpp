#include <assert.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include "SkyMultiHttp.h"

uv_loop_t *  CSkyMultiHttp::m_loop = NULL;
CURLM *      CSkyMultiHttp::m_curl_handle = NULL;
uv_timer_t   CSkyMultiHttp::m_timeout = { 0 };
CSkyMultiHttp* CSkyMultiHttp::m_obj = NULL;
done_proc CSkyMultiHttp::m_done_proc = NULL;

CSkyMultiHttp::CSkyMultiHttp(done_proc proc)
{
  CSkyMultiHttp::m_obj = this;
  CSkyMultiHttp::m_done_proc = proc;
}


CSkyMultiHttp::~CSkyMultiHttp()
{
}

BOOL CSkyMultiHttp::Init()
{
  /*libuv �ṩ��һ��Ĭ�ϵ��¼�ѭ��, �����ͨ�� uv_default_loop ����ø��¼�ѭ��,  
  �����ĳ�����ֻ��һ���¼�ѭ��, ���Ӧ��ʹ�� libuv Ϊ�����ṩ��Ĭ���¼�ѭ��*/  
  m_loop = uv_default_loop();  
  
  
  if (curl_global_init(CURL_GLOBAL_ALL)) {  
      fprintf(stderr, "Could not init cURL\n");  
      return FALSE;  
  }  
  
  //��ʼ����ʱ��  
  uv_timer_init(m_loop, &m_timeout);  
  
  m_curl_handle = curl_multi_init();  
  //����handle_socket�ص������������½���sockfd�����ݴ����action״̬��ӵ���Ӧ���¼������������װepoll��libev��libevent��  
  curl_multi_setopt(m_curl_handle, CURLMOPT_SOCKETFUNCTION, handle_socket);  
  /*��ʹ��curl_multi_add_handle(g->multi, conn->easy)�������ʱ��ص�start_timeout��Ȼ����� 
  curl_multi_socket_action(curl_handle, CURL_SOCKET_TIMEOUT, 0, &running_handles)��ʼ�����󲢵õ�һ��socket(fd)*/  
  curl_multi_setopt(m_curl_handle, CURLMOPT_TIMERFUNCTION, start_timeout);  

  return TRUE;
}

void CSkyMultiHttp::loop()
{
    //�¼�ѭ���� uv_run ������װ, ��ʹ��libuv���ʱ, �ú���ͨ�������ű�����.  
    uv_run(m_loop, UV_RUN_DEFAULT);  
    curl_multi_cleanup(m_curl_handle);  
    curl_global_cleanup();
}

curl_context_t *CSkyMultiHttp::create_curl_context(curl_socket_t sockfd) {  
    curl_context_t *context;  
  
    context = (curl_context_t*) malloc(sizeof *context);  
  
    context->sockfd = sockfd;  
  
    //ʹ��socket��������ʼ��handle  
    int r = uv_poll_init_socket(m_loop, &context->poll_handle, sockfd);  
    assert(r == 0);  
    context->poll_handle.data = context;  
  
    return context;  
}  
  
void CSkyMultiHttp::curl_close_cb(uv_handle_t *handle) {  
    curl_context_t *context = (curl_context_t*) handle->data;  
    free(context);  
}  
  
void CSkyMultiHttp::destroy_curl_context(curl_context_t *context) {  
    //uv_close�ر����е���  
    uv_close((uv_handle_t*) &context->poll_handle, curl_close_cb);  
}  
  

//���easy handle�Ƿ��ͳɹ����ɹ����easy handle�Ƴ���multi stack  
void CSkyMultiHttp::check_multi_info(void) {  
    // char *done_url;  
    CURLMsg *message;  
    int pending;  
    CURL* handle = NULL;
    // long response_code;
  
    while ((message = curl_multi_info_read(m_curl_handle, &pending))) {  
        switch (message->msg) {  
        case CURLMSG_DONE:  
            // curl_easy_getinfo(message->easy_handle, CURLINFO_EFFECTIVE_URL,  
            //                 &done_url);  
            // curl_easy_getinfo(message->easy_handle, CURLINFO_RESPONSE_CODE,
            //                    &response_code);
            // printf("%s DONE\n", done_url);  
  
            handle = message->easy_handle;

            curl_multi_remove_handle(m_curl_handle, message->easy_handle);  
            // curl_easy_cleanup(message->easy_handle);  

            m_done_proc(handle);

            break;  
  
        default:  
            fprintf(stderr, "CURLMSG default\n");  
            abort();  
        }  
    }  
}  
  
void CSkyMultiHttp::curl_perform(uv_poll_t *req, int status, int events) {  
    //ֹͣtimer���ص������������ٱ�����  
    uv_timer_stop(&m_timeout);  
    int running_handles;  
    int flags = 0;  
    if (status < 0)                      flags = CURL_CSELECT_ERR;  
    if (!status && events & UV_READABLE) flags |= CURL_CSELECT_IN;  
    if (!status && events & UV_WRITABLE) flags |= CURL_CSELECT_OUT;  
  
    curl_context_t *context;  
  
    context = (curl_context_t*)req;  
  
    /*���¼�����������socket״̬�ı�ʱͨ��curl_multi_socket_action(g->multi, fd, action, &g->still_running) 
    ֪ͨlibcurl��д���ݣ�Ȼ���ٵ���sock_cb֪ͨ�¼�����������˷�����*/  
    curl_multi_socket_action(m_curl_handle, context->sockfd, flags, &running_handles);  
    m_obj->check_multi_info();     
}  
  
void CSkyMultiHttp::on_timeout(uv_timer_t *req) {  
    int running_handles;  
    //��ʼ�����󲢵õ�һ��socketfd  
    curl_multi_socket_action(m_curl_handle, CURL_SOCKET_TIMEOUT, 0, &running_handles);  
    m_obj->check_multi_info();  
}  
  
void CSkyMultiHttp::start_timeout(CURLM *multi, long timeout_ms, void *userp) {  
    if (timeout_ms <= 0)  
        timeout_ms = 1; /* 0 means directly call socket_action, but we'll do it in a bit */  
    //ע���Լ��Ķ�ʱ�ص�����uv_timer_start  
    uv_timer_start(&m_timeout, on_timeout, timeout_ms, 0);  
}  
  
int CSkyMultiHttp::handle_socket(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp) {  
    curl_context_t *curl_context = NULL;  
    if (action == CURL_POLL_IN || action == CURL_POLL_OUT) {  
        if (socketp) {  
            curl_context = (curl_context_t*) socketp;  
        }  
        else {  
            curl_context = m_obj->create_curl_context(s);  
            curl_multi_assign(m_curl_handle, s, (void *) curl_context);  
        }  
    }  
  
    switch (action) {  
        case CURL_POLL_IN:  
            //��ʼpolling�ļ���������һ����⵽���¼��������curl_perform����������status����Ϊ0  
            uv_poll_start(&curl_context->poll_handle, UV_READABLE, curl_perform);  
            break;  
        case CURL_POLL_OUT:  
            uv_poll_start(&curl_context->poll_handle, UV_WRITABLE, curl_perform);  
            break;  
        case CURL_POLL_REMOVE:  
            if (socketp) {  
                //ֹͣpolling�ļ�������  
                uv_poll_stop(&((curl_context_t*)socketp)->poll_handle);  
                m_obj->destroy_curl_context((curl_context_t*) socketp);                  
                curl_multi_assign(m_curl_handle, s, NULL);  
            }  
            break;  
        default:  
            abort();  
    }  
  
    return 0;  
}  
