#include "server_https.h"
#include "handler.h"

using namespace Wenmingxing;

int main()
{
    //HTTPS����������12345�˿ڣ��������ĸ��߳�,��������֤���˽Կ��ʼ��
    Server<HTTPS> server(12345, 4, "server.crt", "server.key");
    start_server<Server<HTTPS>>(server);

    return 0;
}
