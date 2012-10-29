// HttpSession.h

#ifndef _PPBOX_HTTPD_HTTP_SESSION_H_
#define _PPBOX_HTTPD_HTTP_SESSION_H_

#include <ppbox/dispatch/DispatchBase.h>

#include <util/protocol/http/HttpServer.h>
#include <util/protocol/http/HttpRequest.h>
#include <util/protocol/http/HttpResponse.h>

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
            void handle_open(
                response_type const & resp,
                boost::system::error_code const & ec);

            void handle_play(
                response_type const & resp,
                boost::system::error_code const & ec);

            void make_error(
                boost::system::error_code const & ec);

            void make_mediainfo(
                boost::system::error_code& ec);

            void make_playinfo(
                boost::system::error_code& ec);

            void m3u8_protocol(
                boost::system::error_code& ec);

            void make_m3u8(boost::system::error_code& ec);

        private:
            HttpManager & mgr_;
            framework::string::Url url_;
            ppbox::dispatch::DispatcherBase * dispatcher_;
            std::vector<util::stream::Sink*> sinks_;
            ppbox::dispatch::SeekRange seek_range_;
        };

    } // namespace httpd
} // namespace ppbox

#endif // _PPBOX_HTTPD_HTTP_SESSION_H_
