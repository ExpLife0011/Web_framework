/* ��ܻ��� */
#ifndef SERVER_BASE_H_INCLUDED
#define SERVER_BASE_H_INCLUDED

#include<unordered_map>
#include<thread>
#include<regex>
#include<boost/asio.hpp>

namespace Wenmingxing {
    /*Request�ṹ�����ڽ������������󷽷�������·����HTTP�汾����Ϣ*/
    struct Request {
        //���󷽷���GET��POST������·����HTTP�汾
        std::string method, path, http_version;
        //��contentʹ������ָ��������ü��������ڱ����������а���������,��http��body����
        std::shared_ptr<std::istream> content;
        //��Ϊ������header����Ϣ��˳�����Բ���unordered_map������header
        std::unordered_map<std::string, std::string> headers;
        //��������ʽ����·���Ƿ�ƥ������
        std::smatch path_match;
    };

    //ʹ��typedef����Դ���͵ı�ʾ����
    /* resource_type��һ��map�����Ϊһ���ַ�����ֵ����һ����������unordered_map
     * ���unordered_map�ļ���Ȼ��һ���ַ�������ֵ����һ����������Ϊ�ա�����Ϊostream��Request�ĺ�����
     * ��ˣ�������ʹ�����׿�ܵ�ʱ�򣬵���������һ��server���󣬶�����Դ����������ʽ��
     * server.resource["^/info/?$"]["GET"] = [](ostream& response, Request& request) {//����������Դ}��
     * ����map���ڴ洢����·���ı��ʽ����unordered_map���ڴ洢���󷽷���lambda���ʽ���洦������
    */
    typedef std::map<std::string, std::unordered_map<std::string,
    std::function<void(std::ostream&, Request&)>>> resource_type;

    //socket_typeΪHTTP��HTTPS
    template <typename socket_type> class ServerBase {
    public:
        //���ڷ�����������Դ����ʽ
        resource_type resource;
        //���ڱ���Ĭ����Դ�Ĵ���ʽ
        resource_type default_resource;

        //���캯��,��ʼ���˿ڣ�Ĭ���̸߳���Ϊ1
        ServerBase(unsigned short port, size_t num_threads = 1):
            endpoint(boost::asio::ip::tcp::v4(), port),
            acceptor(m_io_service, endpoint),
            num_threads(num_threads) {}

        //����������
        void start() {
            //Ĭ����Դ����vector��ĩβ������Ĭ��Ӧ��
            //Ĭ�ϵ���������Ҳ���ƥ������·��ʱ�����з��ʣ�����������
            for (auto it = resource.begin(); it != resource.end(); ++it) {
                all_resources.push_back(it);    //all_resources�洢�����������е���Դ
            }
            for (auto it = default_resource.begin(); it != default_resource.end(); ++it) {
                all_resources.push_back(it);
            }

            //����socket�����ӷ�ʽ������Ҫ������ʵ��accept()�߼�
            //������ͨ������accept�������ȴ����Կͻ��˵��������󣬽���������������һ�������ӵ���������
            accept();   //����protected�е��麯��

            //���num_threads>1����ôm_io_service.run()
            //�����У�num_threads-1���̳߳�Ϊ�̳߳�
            for (size_t c = 1; c < num_threads; ++c) {
                threads.emplace_back([this](){m_io_service.run();});
            }

            //���߳�
            m_io_service.run();

            //�ȴ������̣߳�����еĻ����͵ȴ���Щ�̵߳Ľ���
            for (auto& t:threads)
                t.join();
        }


    protected:
        /* boost.asio��ʹ�� */
        //asioҪ��ÿ��Ӧ�ö�����һ��io_service����ĵ�����,�����첽IOʱ�䶼Ҫͨ�������ַ�����
        //���仰˵����ҪIO�Ķ���Ĺ��캯��������Ҫ����һ��io_service����
        boost::asio::io_serviece m_io_service;
        //ʵ��TCP socket���ӣ���Ҫһ��acceptor���󣬶���ʼ��һ��acceptor�������Ҫһ��endpoint����
        boost::asio::ip::tcp::endpoint endpoint;    //endpoint��Ϊһ��socketλ�ڷ���˵Ķ˵㣨IP���˿ںţ�Э��汾��
        boost::asio::ip::tcp::acceptor acceptor;    //acceptor�������ڽ������ӣ�ͨ��io_service��endpoint���죬����ָ���˿��ϵȴ�����

        /* vectorʵ���̳߳� */
        size_t num_threads;
        std::vector<std::thread> threads;

        //�����ڲ�ʵ�ֶ�������Դ�Ĵ���������Դ������vectorβ����ӣ�����start()�д���
        std::vector<resource_type::iterator> all_resource;

        //��Ҫ��ͬ���͵ķ�����ʵ������������Էֱ���HTTP��HTTPS�������Զ���Ϊ�麯��
        //HTTP��HTTPS���ַ�����֮��Ĵ������󡢷�������Ψһ���������������δ�����ͻ��˽������ӵķ�ʽ�ϣ�Ҳ����accept����
        virtual void accept() {}

        //���������Ӧ��
        void process_request_and_respond(std::shared_ptr<socket_type> socket) const {
            //Ϊasync_read_untile()�����µĶ�����
            //shared_ptr���ڴ�����ʱ�������������
            //�ᱻ�Ƶ�Ϊstd::shared_ptr<boost::asio::streambuf>
            /*boost::asio::streambuf������һ������������������ж�ȡsocket�е�����*/
            auto read_buffer = std::make_shared<boost::asio::streambuf>();

            /* ��ΪЭ���ǻ�����ʵ�ֵģ�����ʹ��boost::asio::async_read_until��������\r\n\r\n���н綨 */
            /* �����lambda���ʽΪread_handler������һ���޷������͵ĺ�������
             * ��������������һ��boost::system::error_code,һ��size_t(bytes_transferred)
             * error_code�������������Ƿ�ɹ���bytes_transferred����ȷ�����յ��ֽ���
             * �����read_handler��lambda���ʽ���У�ʵ�������ڲ��϶�ȡsocket�е�����
            */
            boost::asio::async_read_until(*socket, *read_buffer, "\r\n\r\n",
                                [this,socket, read_buffer](const boost::system::error_code& ec,size_t bytes_transferred)
                                {
                                    if(!ec) {
                                            //read_buffer->size()�Ĵ�С��һ����bytes_transferred���
                                            //��async_read_until�����ɹ���streambuf�ڽ綨��֮����ܰ���һЩ���������
                                            //���ԽϺõ�������ֱ�Ӵ�������ȡ��������ǰread_buffer��ߵı�ͷ����ƴ��async_read���������
                                            size_t total = read_buffer->size();

                                            //ת����istream
                                            std::istream stream(read_buffer.get());

                                            //���Ƶ�Ϊstd::shared_ptr<Request>����
                                            auto request = std::make_shared<Request>();

                                            //������ʹ��parse_request������stream�е�������Ϣ���н�����Ȼ�󱣴浽request������
                                            *request = parse_request(stream);   //�˷���ʵ��������

                                            size_t num_additional_bytes = total - bytes_transferred;

                                            //������㣬ͬ����ȡ
                                            if (request->header.count("Content-Length") > 0) {
                                                /* boost::asio::async_read�����������async_read_until����һ��,Ψһ����������Ҫָ����ȡ����
                                                 * ��ȡ����ͨ��boost::asio::transfer::exactlyָ��
                                                 * stoull�����������������пհ׷�
                                                 */
                                                boost::asio::async_read(*socket, *read_buffer,
                                                boost::asio::transfer_exactly(stoull(request->header["Content-Length"]) - num_additional_bytes),
                                                [this, socket, read_buffer, request](const boost::system::error_code& ec, size_t bytes_transferred) {
                                                    if (!ec) {
                                                        //��ָ����Ϊistream����洢��read_buffer��
                                                        request->content = std::shared_ptr<std::istream>(new std::istream(read_buffer.get()));
                                                        //��Ӧ������ʵ��������
                                                        respond(socket, request);
                                                    }
                                                });
                                            } else {
                                            respond(socket, request);
                                            }
                                          }
                                });
        }

        //���ڽ�������
        Request parse_request(std::istream& stream) const {
            Request request;

            std::regex e("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");

            std::smatch sub_match;

            //�ӵ�һ���н�������������·����HTTP�汾
            std::string line;
            getline(stream, line);
            line.pop_back();    //Ӧ����ȥ���ո���Ϣ
            if (std::regex_match(line, sub_match, e)) {
                //��������ɽ���
                request.method = sub_match[1];
                request.path = sub_match[2];
                request.http_version = sub_match[3];

                bool matched;
                e="^([^:]*): ?(.*)$";
                //����ͷ��������Ϣ
                do {
                    getline(stream, line);
                    line.pop_back();
                    matched = std::regex_match(line, sub_match, e);
                    if (matched) {
                        request.header[sub_match[1]] = sub_match[2];
                    }
                } while (matched = true);
            }
            return request;
        }

        //Ӧ�𷽷�ʵ��
        void respond(std::shared_ptr<socket_type> socket, std::shared_ptr<Request> request) const {
            //������·���ͷ�������ƥ����ң���������Ӧ
            for (auto res_it:all_resources) {
                std::regex e(res_it->first);
                std::smatch sm_res;
                if (std::regex_match(request->path, sm_res, e)) {
                    if (res_it->second.count(request->method) > 0) {
                        request->path_match = move(sm_res);

                        //�ᱻ�Ƶ�Ϊstd::shared_ptr<boost::asio::streambuf>
                        auto write_buffer = std::make_shared<boost::asio::streambuf>();
                        std::ostream response(write_buffer.get());
                        res_it->second[request->method](response, *request);

                        //��lambda�в���write_bufferʹ�䲻����async_write���ǰ������
                        boost::asio::async_write(*socket, *write_buffer,
                            [this, socket, request, write_buffer](const boost::system::error_code& ec, size_t bytes_transferred){
                                                //�����HTTP�־����ӣ�HTTP1.1��
                                                if(!ec && stof(request->http_version) > 1.05)
                                                    process_request_and_respond(socket);
                                                });
                                                return;
                    }
                }
            }
        }

    };

    template<typename socket_type> class Server : public ServerBase<socket_type> {};
}

#endif // SERVER_BASE_H_INCLUDED
