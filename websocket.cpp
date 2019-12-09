#include "global.h"
#include "websocket.h"

int destroy_flag;
int connection_flag;
int writeable_flag;

char recvBuff[1024*1024];
char xmlBuff[1024*1024];
int initmsg = 0;
size_t rd = 0;


int websocket_write_back(struct lws *wsi_in, char *str, int str_size_in); 

int ws_service_callback(struct lws *wsi,enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char cTime[16];
    char runDate[16];
    char uuid[37];

    char heartbeat[4096];
    char sche[4096];

    switch (reason) {

        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf(KYEL"[Main Service] Connect with server success.\n"RESET);
            connection_flag = 1;
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf(KRED"[Main Service] Connect with server error.\n"RESET);
            destroy_flag = 1;
            // reconnect 요청 해야 함
            connection_flag = 0;
            break;

        case LWS_CALLBACK_CLOSED:
            printf(KYEL"[Main Service] LWS_CALLBACK_CLOSED\n"RESET);
            // reconnect 요청 해야 함
            destroy_flag = 1;
            connection_flag = 0;
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            //printf(KCYN_L"[Main Service] Client recvived:%s\n"RESET, (char *)in);
            memcpy(&recvBuff[rd], in, len);
            rd = rd + len;
            _d("len %zu\n", len);
            if (len < 4096)
            {
                memcpy(xmlBuff, recvBuff, rd);
                _d("%s\n", xmlBuff);

                memset(xmlBuff, 0x00, sizeof(xmlBuff));
                memset(recvBuff, 0x00, sizeof(recvBuff));
                rd = 0;
            }
#if 0
            if (writeable_flag)
                destroy_flag = 1;
#endif
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE :
            printf(KYEL"[Main Service] On writeable is called.\n"RESET);
            if (!initmsg)
            {
                sprintf(cTime, "%4d-%02d-%02dT%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                sprintf(runDate, "%4d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

                getUUID(uuid);
                sprintf(sche, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><APC_Message id=\"urn:uuid:%s\" dateTime=\"%s\" messageType=\"Schedule\" origin=\"myksystem\" destination=\"apc\"><request channel_code=\"12\" channel_name=\"2TV\" programming_local_station_code=\"00\" programing_local_station_name=\"본사\" running_date=\"%s\"/></APC_Message>", cTime, uuid, runDate);

                websocket_write_back(wsi, sche, -1);
                initmsg++; //최초 한번만 실행
                break;
            }
            else
            {
                t = time(NULL);
                tm = *localtime(&t);
                sprintf(cTime, "%4d-%02d-%02dT%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                sprintf(runDate, "%4d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

                getUUID(uuid);

                sprintf(sche, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><APC_Message id=\"urn:uuid:%s\" dateTime=\"%s\" messageType=\"Schedule\" origin=\"myksystem\" destination=\"apc\"><request channel_code=\"12\" channel_name=\"2TV\" programming_local_station_code=\"00\" programing_local_station_name=\"본사\" running_date=\"%s\"/></APC_Message>", cTime, uuid, runDate);

                sprintf(heartbeat, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><APC_Message id=\"urn:uuid:%s\" dateTime=\"%s\" messageType=\"Heartbeat\" origin=\"myksystem\" destination=\"apc\"/>", uuid, cTime);

                websocket_write_back(wsi, heartbeat, -1);
                break;
            }

        default:
            break;
    }

    return 0;
}

int websocket_write_back(struct lws *wsi_in, char *str, int str_size_in) 
{
    if (str == NULL || wsi_in == NULL)
        return -1;

    int n;
    int len;
    //char *out = NULL;
    unsigned char *out = NULL;

    if (str_size_in < 1) 
        len = strlen(str);
    else
        len = str_size_in;

    out = (unsigned char *)malloc(sizeof(unsigned char)*(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));
    //* setup the buffer*/
    memcpy (out + LWS_SEND_BUFFER_PRE_PADDING, str, len );
    //* write out*/
    n = lws_write(wsi_in, out + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);

    //printf(KBLU"[websocket_write_back] %s\n"RESET, str);
    //* free the buffer*/
    free(out);

    return n;
}


CWebSocket::CWebSocket()
{
    m_bExit = false;
}

CWebSocket::~CWebSocket()
{
    destroy_flag = 1;
    Terminate();
}

void CWebSocket::Connect()
{
    connection_flag = 0;
    writeable_flag = 0;
    context = NULL;
    wsi = NULL;

    memset(&protocol, 0, sizeof protocol);
    memset(&info, 0, sizeof info);

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.iface = NULL;
    info.protocols = &protocol;
    info.ssl_cert_filepath = NULL;
    info.ssl_private_key_filepath = NULL;
    info.extensions = lws_get_internal_extensions();
    info.gid = -1;
    info.uid = -1;
    info.options = 0;

    protocol.name = "KBS2_ADMIXER";
    protocol.callback = &ws_service_callback;
    protocol.per_session_data_size = sizeof(struct session_data);
    protocol.rx_buffer_size = 0;
    protocol.id = 0;
    protocol.user = NULL;

    context = lws_create_context(&info);
    printf(KRED"[Main] context created.\n"RESET);

    if (context == NULL)
    {
        printf(KRED"[Main] context is NULL.\n"RESET);
        //return -1;
        //재시도 처리 해야함 안전한 프로그램을 만들기 위하여
    }
    wsi = lws_client_connect(context, "210.115.200.34", 4011, 0, "/cistech/auth/url/pooq01/pooq01!", "210.115.200.34:4011", NULL, NULL, -1);
    if (wsi == NULL) {
        printf(KRED"[Main] wsi create error.\n"RESET);
        //return -1;
        //Connection Error Retry
    }

    printf(KGRN"[Main] wsi create success.\n"RESET);
    tool.wsi = wsi;
    tool.context = context;

    Start();

    while(!destroy_flag)
    {
        lws_service(context, 50);
        //_d("[WEBSOCKET] lws_service is running...\n");
    }
    _d("[WEBSOCKET] lws_service exit loop\n");
    _d("[WEBSOCKET] context pointer : %p\n", context);
    lws_context_destroy(context);
    //Connect();
}

void CWebSocket::Run()
{
    //* waiting for connection with server done.*/
    while(!connection_flag)
        usleep(1000*20);

    //*Send greeting to server*/
    printf(KBRN"[WEBSOCKET] Server is ready. send a greeting message to server.\n"RESET); 
    //websocket_write_back(tool->wsi, "Good day", -1);
    while (!destroy_flag)
    {
        printf(KBRN"[pthread_routine] call on writable.\n"RESET);   
        sleep(10);
        lws_callback_on_writable(wsi);
    }
    _d("[WEBSOCKET] Server is disconnected\n");
}
