#include "uv.h"  
#include "SkyChaserHttp.h"
#include "curl.h" 
#include "curl_multi.h"

typedef void(*done_proc)(CURL* handle);

typedef struct curl_context_s {  
    uv_poll_t poll_handle;  
    curl_socket_t sockfd;  
} curl_context_t;  
  
class CSkyMultiHttp
{
public:
  CSkyMultiHttp(done_proc proc);
  ~CSkyMultiHttp();
  BOOL Init();

  curl_context_t *create_curl_context(curl_socket_t sockfd);  
  void destroy_curl_context(curl_context_t *context);  
  void check_multi_info(void);  
  static void curl_close_cb(uv_handle_t *handle);  
  static void curl_perform(uv_poll_t *req, int status, int events);  
  static void on_timeout(uv_timer_t *req);  
  static void start_timeout(CURLM *multi, long timeout_ms, void *userp);  
  static int handle_socket(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp);  
  void loop();

  static uv_loop_t * m_loop;  
  static CURLM *     m_curl_handle;  
  static uv_timer_t  m_timeout;  

protected:
  static CSkyMultiHttp* m_obj;
  static done_proc m_done_proc;
};