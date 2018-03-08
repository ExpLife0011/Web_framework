/* HTTPS���� */
#ifndef SERVER_HTTPS_H_INCLUDED
#define SERVER_HTTPS_H_INCLUDED

#include "server_http.h"
#include<boost/asio/ssl.h>

namespace Wenmingxing {
    //����HTTPS����
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> HTTPS;

    //����HTTPS����ģ������ΪHTTPS
    template<>
    class Server<HTTPS> : public ServerBase<HTTPS> {
    public:
        //һ��HTTPS��������http����������������������һ����֤���ļ�����һ����˽Կ�ļ�
        Server(unsigned short port, size_t num_threads,
               const std::string& cert_file, const std::string& private_key_file) :
               ServerBase<HTTPS>::ServerBase(port, num_threads), context(boost::asio::ssl::context::sslv23) {
                   //ʹ��֤���ļ�
                   context.use_certificate_chain_file(cert_file);
                   //ʹ��˽Կ�ļ������֮����Ҫ�ഫ��һ��������ָ���ļ��ĸ�ʽ
                   context.use_private_key_file(private_key_file, boost::asio::ssl::context::pem);
                   }
    private:
        //��http��������ȣ���Ҫ�ඨ��һ��ssl context����
        boost::asio::ssl::context context;

        /* HTTPS��������http���������
         * ���������ڶ�socket����Ĺ��췽ʽ������ͬ
         * HTTPS����socket��һ����IO�����м���
         * ���ʵ��accept������Ҫ��socket��ssl context��ʼ��
        */
        void accept() {
            //Ϊ��ǰ���Ӵ���һ���µ�socket
            //shared_ptr���ڴ�����ʱ�������������
            //socket���ͻᱻ�Ƶ�Ϊshared_ptr<HTTPS>
            auto socket = std::make_shared<HTTPS>(m_io_service, context);

            accetpor.async_accept (
                                   (*socket).lowest_layer(),
                                   [this, socket](const boost::system::error_code& ec) {
                                    //��������������һ��������
                                    accept();
                                    //�������
                                    if(!ec) {
                                        (*socket).async_handshake(boost::asio::ssl::stream_base::server,
                                                                  [this, socket](const boost::system::error_code& ec) {
                                                                  if (!ec) process_request_and_respond(socket);
                                                                  });
                                    }
                                   });
        }

    };
}

#endif // SERVER_HTTPS_H_INCLUDED
