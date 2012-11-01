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

        class HttpServer;

        class HttpdModule
            : public ppbox::common::CommonModuleBase<HttpdModule>
            , public util::protocol::HttpServerManager<HttpServer, HttpdModule>
        {
        public:
            HttpdModule(
                util::daemon::Daemon & daemon);

            ~HttpdModule();

        public:
            virtual boost::system::error_code startup();

            virtual void shutdown();

        public:
            // avoid ambiguous
            using ppbox::common::CommonModuleBase<HttpdModule>::io_svc;

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
