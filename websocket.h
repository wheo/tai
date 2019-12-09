#ifndef _WEBSOCKET_H_
#define _WEBSOCKET_H_

class CWebSocket : public PThread
{
public:
    CWebSocket();
    ~CWebSocket();

    void Open(char *filename);
    void Connect();

protected:

    void Run();
    void OnTerminate() {};
    //int destroy_flag;
    //int connection_flag;
    //int writeable_flag;
    struct lws_context *context;
    struct lws_context_creation_info info;
    struct lws *wsi;
    struct lws_protocols protocol;

    struct pthread_routine_tool
    {
        struct lws_context *context;
        struct lws *wsi;
    } tool;

    struct session_data {
        int fd;
    };

protected:

    char m_strFileName[256];
};

#endif //_WEBSOCKET_H_
