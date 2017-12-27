## 封装libcurl+libuv实现multi异步高效http请求


> 关于libcurl+libuv实现异步http请求的简单例子可以参考：http://blog.csdn.net/lijinqi1987/article/details/53996129<br><br>开发环境：win7, vs2013



## libuv使用流程

> 主要参考上面文章中的代码

1. 获取默认事件循环句柄`uv_loop_t loop = uv_default_loop();`

2. 初始化定时器`uv_timer_init(loop, &timeout);`

3. 设置回调函数

4. 开始循环`uv_run(loop, UV_RUN_DEFAULT);`

## libuv和libcurl的mutil接口结合

> 主要参考上面文章中的代码

1. 获得`curl multi`句柄`curl_handle = curl_multi_init();`

2. 设置回调

    ```cpp
    //调用handle_socket回调函数，传入新建的sockfd，根据传入的action状态添加到相应的事件管理器，如封装epoll的libev或libevent。 
    curl_multi_setopt(curl_handle, CURLMOPT_SOCKETFUNCTION, handle_socket); 
    /*当使用curl_multi_add_handle(g->multi, conn->easy)添加请求时会回调start_timeout，然后调用
    curl_multi_socket_action(curl_handle, CURL_SOCKET_TIMEOUT, 0, &running_handles)初始化请求并得到一个socket(fd)*/ 
    curl_multi_setopt(curl_handle, CURLMOPT_TIMERFUNCTION, start_timeout); 
    ```

3. 循环结束后清理环境`curl_multi_cleanup(curl_handle);`


## 封装

1. 对于`curl multi`接口的http请求而言，除了不是主动请求，其他使用方式和`easy`接口完全一致，因此根据需要修改了以前的`CSkyChaserHttp`类，仅为构造增加了一个默认参，在使用`multi`接口时不主动进行http请求

2. 对于`libuv`和`libcurl`配合使用的部分，由于除了完成请求后的回调需要自己控制，其他的都是固定的套路，因此封装了一个`CSkyMultiHttp`类，在构造时将回调函数传进去，然后调用`Init()`方法来初始化，然后通过`curl_multi_add_handle(CSkyMultiHttp::m_curl_handle, http_curl_handle);`来添加事件，最后通过`loop()`方法来开始事件循环

3. 最后再封装了一个`CHttpTask`类，用来管理所有的`http_curl_handle`和任务事件，从而实现连续的http请求动作，通过`AddTask()`方法来添加最初的事件任务，再通过`TaskDoneProc(...)`方法来处理一个请求完成后的动作，主要是添加下一个请求到事件循环中

## 效果

对一个小型网站进行注册测试：

- 开1000个并发连接，一共1万×5次请求（不包括30X重定向产生的请求），速度局限于对方服务器和网速，显得比较慢，而且这么多并发，直接导致对方服务器无响应了，失败几率很高，并且在测试期间，通过浏览器很难访问这个网站，而本机CPU占用率很低，在这种情况下无法测出性能的极限

## vs2013环境下编译libuv

[https://kevins.pro/bulid_libuv_with_vs2013.html](https://kevins.pro/bulid_libuv_with_vs2013.html)