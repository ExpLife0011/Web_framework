#ifndef SERVER_HTTP_H_INCLUDED
#define SERVER_HTTP_H_INCLUDED

#include "server_base.h"

namespace ShiyanlouWeb {
    typedef boost::asio::ip::tcp::socket HTTP;
    template<>
    class Server<HTTP> : public ServerBase<HTTP> {
    public:
        //���캯����ͨ���˿ںš��߳���������Web��������HTTP�������ϼ򵥣�����Ҫ��������ù���
        Server(unsigned short port, size_t num_threads = 1) : ServerBase<HTTP>::ServerBase(port, num_threads) {};
    private:
        //ʵ��accept����
        void accept() {
            //Ϊ��ǰ���Ӵ���һ���µ�socket
            //shared_ptr���ڴ�����ʱ�������������
            //socket�ᱻ�Ƶ�Ϊshared_ptr<HTTP>����
            /* Ҫ��֤async_accept�������첽�����ڼ�socket������Ч���������첽�����ڼ�socketû�ˣ����ҿͻ�������֮�����socket�������ã���
             * ����ķ�����ʹ��һ������ָ��shared_prt<socket>���������ָ����Ϊ�����󶨵��ص������ϡ� ��Ҳ��ǰ��ʹ������ָ���ԭ��
            */
            auto socket = std::make_shared<HTTP>(m_io_service)��

            /* ��Asio�У��첽��ʽ�ĺ�������ǰ�涼��async_ǰ׺�������������Ҫ���һ���ص�������
             * �첽����ִ�к󣬲�����û����ɶ����������أ����ǿ�����һЩ�������飬֪���ص����������ã�˵���첽������ɡ�
             * �ص�����һ������һ��error_code������һ��ʹ��bind����������������ʹ��lambda����bind
            */
            acceptor.async_accept(*socket, [this,socket](const boost::system::error_code& ec)
                                  {
                                      //��������������һ������
                                      accept();
                                      //������ִ����������Ӧ��request��respond
                                      if (!ec) process_request_and_respond(socket);
                                  });
        }
    };
}

#endif // SERVER_HTTP_H_INCLUDED
