//HttpServer.h

#ifndef      _PPBOX_HTTP_SESSION_
#define      _PPBOX_HTTP_SESSION_


#include <util/protocol/http/HttpProxyManager.h>
#include <util/protocol/http/HttpProxy.h>
#include <util/protocol/http/HttpRequest.h>
#include <util/protocol/http/HttpResponse.h>

namespace ppbox
{

    namespace httpd
    {
        class HttpDispatcher;
        class HttpManager;

        class HttpSession
            : public util::protocol::HttpProxy
        {
        public:
            HttpSession(
                HttpManager & mgr);

            ~HttpSession();

            virtual bool on_receive_request_head(
                util::protocol::HttpRequestHead & request_head);

            virtual void transfer_response_data(
                response_type const & resp);

            virtual void on_error(
                boost::system::error_code const & ec);

            virtual void on_finish();
            

            virtual void local_process(response_type const & resp);

        private:
            void open_callback(response_type const &resp,
                boost::system::error_code const & ec);

            void on_open(response_type const &resp,
                boost::system::error_code const & ec);

            void open_setupup(response_type const &resp,
                boost::system::error_code const & ec);

            void on_playend(response_type const &resp,
                 boost::system::error_code const & ec);

            void on_seekend(response_type const &resp,
                boost::system::error_code const & ec);

            void on_common(response_type const &resp,
                boost::system::error_code const & ec);

            //���ͻ���д����
            boost::system::error_code write(std::string const& msg);

            void make_error_response_body(
                std::string& respone_str,
                boost::system::error_code const & last_error);

        private:
            static std::string g_format_;  //������һ�ε�format

        private:
            boost::asio::io_service & io_svc_;
            std::string option_;  //transfer_response_data�ж��ϴδ����ʲô��Ϣ
            std::string format_;  //transfer_response_data�ж��ϴδ����ʲô��Ϣ
            std::string body_;    //����ظ�����Ϣ��
            std::string host_;   //����host��ַ
			std::string head_;
            boost::uint32_t len_;
            boost::uint32_t session_id_;
            boost::uint32_t seek_; //����seek��λ��
            bool need_seek_;
            boost::system::error_code ec_; //����open�ĳɹ����
             
            HttpDispatcher * dispatcher_;
			
        };

    } // namespace httpd

} // namespace ppbox

#endif
