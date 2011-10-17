//HttpServer.h

#ifndef      _PPBOX_HTTP_SERVER_
#define      _PPBOX_HTTP_SERVER_

#include <ppbox/common/CommonModuleBase.h>

#include <util/protocol/http/HttpProxyManager.h>
#include <util/protocol/http/HttpProxy.h>
#include <util/protocol/http/HttpRequest.h>
#include <util/protocol/http/HttpResponse.h>

#include <boost/thread/thread.hpp>

#include <list>

namespace ppbox
{
    namespace httpd
    {
        struct MsgInfo;
        class PlayManager;

        typedef enum {
            closed,
            opened,
        } Status;

        class HttpServer
            : public util::protocol::HttpProxy
        {
        public:
            HttpServer(
                boost::asio::io_service & io_svc);

            ~HttpServer();

            virtual bool on_receive_request_head(
                util::protocol::HttpRequestHead & request_head);

            virtual void transfer_response_data(
                response_type const & resp);

            virtual void on_error(
                boost::system::error_code const & ec);

            void local_process(response_type const & resp);

        private:
            bool is_right_format(std::string const & format);

            bool is_right_type(std::string const & type);

            boost::system::error_code pretreat_msg(boost::system::error_code & ec);

        // static
        private:
            static MsgInfo * msg_info;
            static std::string last_select_format_;
        };

        class HttpMediaServer
            : public ppbox::common::CommonModuleBase<HttpMediaServer>
        {
        public:
            virtual boost::system::error_code startup();

            virtual void shutdown();

        public:
            HttpMediaServer(
                util::daemon::Daemon & daemon);

            ~HttpMediaServer();

            boost::system::error_code start();
            boost::system::error_code stop();

        private:
            boost::asio::io_service & io_srv_;
            PlayManager & play_mod_;
            framework::network::NetName addr_;
            util::protocol::HttpProxyManager<HttpServer> *mgr_;
        };

    } // namespace httpd

} // namespace ppbox

#endif
