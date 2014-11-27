// HttpServer.h

#ifndef _JUST_HTTPD_HTTP_SERVER_H_
#define _JUST_HTTPD_HTTP_SERVER_H_

#include <just/dispatch/DispatchBase.h>

#include <util/protocol/http/HttpServer.h>
#include <util/protocol/http/HttpRequest.h>
#include <util/protocol/http/HttpResponse.h>

namespace just
{
    namespace dispatch
    {
        class DispatcherBase;
        class WrapSink;
    }

    namespace httpd
    {

        class HttpdModule;

        class HttpServer
            : public util::protocol::HttpServer
        {
        public:
            HttpServer(
                HttpdModule & mgr);

            ~HttpServer();

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

        private:
            HttpdModule & mgr_;
            framework::string::Url url_;
            just::dispatch::DispatcherBase * dispatcher_;
            just::dispatch::SeekRange seek_range_;
        };

    } // namespace httpd
} // namespace just

#endif // _JUST_HTTPD_HTTP_SERVER_H_
