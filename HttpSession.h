//HttpServer.h

#ifndef      _PPBOX_HTTP_SESSION_
#define      _PPBOX_HTTP_SESSION_

#include <ppbox/common/ParseUrlTool.h>

#include <util/protocol/http/HttpProxyManager.h>
#include <util/protocol/http/HttpProxy.h>
#include <util/protocol/http/HttpRequest.h>
#include <util/protocol/http/HttpResponse.h>

namespace util
{
    namespace stream
    {
        class Sink;
    }
}

namespace ppbox
{
    namespace dispatcher
    {
        class Dispatcher;
    }

    namespace httpd
    {
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
            void on_open(response_type const &resp,
                boost::system::error_code const & ec);

            void on_playend(response_type const &resp,
                boost::system::error_code const & ec);

 
            boost::system::error_code write(std::string const& msg);

            void make_error_response_body(
                std::string& respone_str,
                boost::system::error_code const & last_error);

            void Close();

            void m3u8_protocol(boost::system::error_code& ec);

            void make_mediainfo(boost::system::error_code& ec);
            void make_playinfo(boost::system::error_code& ec);
            void make_m3u8(boost::system::error_code& ec);

        private:
            boost::asio::io_service& ios_;
            HttpManager & mgr_;

            ppbox::common::ParseUrlTool url_;

            std::string body_;
            std::string host_;
            boost::uint32_t session_id_;
            boost::uint32_t seek_;

            ppbox::dispatcher::Dispatcher * dispatcher_;
            std::vector<util::stream::Sink*> sinks_;
        };

    } // namespace httpd

} // namespace ppbox

#endif
