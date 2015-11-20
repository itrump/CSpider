#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <curl/curl.h>

uv_loop_t *loop;
CURLM *curl_handle;
uv_timer_t timeout;

typedef struct curl_context_s {
  uv_poll_t poll_handle;
  curl_socket_t  sockfd;
} curl_context_t;

curl_context_t *create_curl_context(curl_socket_t sockfd) {
  curl_context_t *context = (curl_context_t*)malloc(sizeof(curl_context_t));
  context->sockfd = sockfd;
  int r = uv_poll_init_socket(loop, &context->poll_handle, sockfd);
  assert(r == 0);
  /*
    使用data域来保存指向自身context的指针
  */
  context->poll_handle.data = context;

  return context;
}

void curl_close_cb(uv_handle_t *handle) {
  curl_context_t *context = (uv_poll_t *)handle->data;
  free(context);
}

void destroy_curl_context(curl_context_t *context) {
  uv_close((uv_handle_t*)&context->poll_handle, curl_close_cb);
}

/*
  添加第num个url
*/
void add_download(const char* url, int num) {
  char filename[50];
  sprintf(filename, "%d.download", num);

  File *file;

  file = fopen(filename, "w");
  if (file == NULL) {
    fprintf(stderr, "error opening %s", filename);
    return;
  }

  CURL *handle = curl_easy_init();
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, file);
  curl_easy_setopt(handle, CURLOPT_URL, url);

  curl_multi_add_handle(curl_handle, handle);
  fprintf(stderr, "添加下载：%s -> %s\n", url, filename);
}

int main() {
  loop = uv_default_loop();

  if (argc <= 1) {
    return 0;
  }

  if (curl_global_init(CURL_GLOBAL_ALL)) {
    fprintf(stderr, "could not init curl\n");
    return 1;
  }

  uv_timer_init(loop, &timeout);

  curl_handle = curl_multi_init();
  curl_multi_setopt(curl_handle, CURLMOPT_SOCKETFUNCTION, handle_socket);
  curl_multi_setopt(curl_handle, CURLMOPT_TIMERFUNCION, start_timeout);

  while(argc-- > 1) {
    add_download(argv[argc], argc);
  }

  uv_run(loop, UV_RUN_DEFAULT);
  curl_multi_cleanup(curl_handle);
  return 0;
}