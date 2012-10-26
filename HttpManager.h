// HttpServer.h

#ifndef _PPBOX_HTTPD_HTTP_MANAGER_H_
#define _PPBOX_HTTPD_HTTP_MANAGER_H_

#include <ppbox/common/CommonModuleBase.h>
#include <util/protocol/http/HttpServerManager.h>

namespace ppbox
{
    namespace dispatch
    {
        class DispatchModule;
    }

    namespace httpd
    {
        class HttpSession;

        class HttpManager
            : public ppbox::common::CommonModuleBase<HttpManager>
            , public util::protocol::HttpServerManager<HttpSession, HttpManager>
        {
        public:
            HttpManager(
                util::daemon::Daemon & daemon);

            ~HttpManager();

        public:
            virtual boost::system::error_code startup();

            virtual void shutdown();

        public:
            // avoid ambiguous
            using ppbox::common::CommonModuleBase<HttpManager>::io_svc;

            ppbox::dispatch::DispatchModule & dispatch_module()
            {
                return dispatch_module_;
            }

        private:
            framework::network::NetName addr_;
            ppbox::dispatch::DispatchModule & dispatch_module_;
        };

    } // namespace httpd
} // namespace ppbox

#endif // _PPBOX_HTTPD_HTTP_MANAGER_H_
