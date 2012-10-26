// HttpSession.h

#ifndef _PPBOX_HTTPD_HTTP_SESSION_H_
#define _PPBOX_HTTPD_HTTP_SESSION_H_

#include <util/protocol/http/HttpServer.h>
#include <util/protocol/http/HttpRequest.h>
#include <util/protocol/http/HttpResponse.h>

#include <framework/string/Url.h>

namespace ppbox
{
    namespace dispatch
    {
        class DispatcherBase;
    }

    namespace httpd
    {

        class HttpManager;

        class HttpSession
            : public util::protocol::HttpServer
        {
        public:
            HttpSession(
                HttpManager & mgr);

            ~HttpSession();

        public:
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

            void make_error_response_body(
                std::string& respone_str,
                boost::system::error_code const & last_error);

            void m3u8_protocol(boost::system::error_code& ec);

            void make_mediainfo(boost::system::error_code& ec);
            void make_playinfo(boost::system::error_code& ec);
            void make_m3u8(boost::system::error_code& ec);

        private:
            HttpManager & mgr_;

            framework::string::Url url_;

            std::string body_;
            std::string host_;
            boost::uint32_t seek_;

            ppbox::dispatch::DispatcherBase * dispatcher_;
            std::vector<util::stream::Sink*> sinks_;
        };

    } // namespace httpd

} // namespace ppbox

#endif // _PPBOX_HTTPD_HTTP_SESSION_H_
