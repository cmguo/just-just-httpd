// HttpdModule.h

#ifndef _PPBOX_HTTPD_HTTPD_MANAGER_H_
#define _PPBOX_HTTPD_HTTPD_MANAGER_H_

#include <ppbox/dispatch/DispatchBase.h>

#include <ppbox/common/CommonModuleBase.h>

#include <framework/network/ServerManager.h>

namespace ppbox
{
    namespace dispatch
    {
        class DispatchModule;
    }

    namespace httpd
    {

        class HttpServer;
        class HttpSession;

        class HttpdModule
            : public ppbox::common::CommonModuleBase<HttpdModule>
            , public framework::network::ServerManager<HttpServer, HttpdModule>
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

        public:
            ppbox::dispatch::DispatcherBase * attach(
                framework::string::Url & url, 
                boost::system::error_code & ec);

            void detach(
                framework::string::Url const & url, 
                ppbox::dispatch::DispatcherBase * dispatcher);

        private:
            framework::network::NetName addr_;
            ppbox::dispatch::DispatchModule & dispatch_module_;

        private:
            struct find_by_session;
            struct find_call_detach;

            typedef std::map<std::string, HttpSession *> session_map_t;
            static session_map_t & session_map();
        };

    } // namespace httpd
} // namespace ppbox

#endif // _PPBOX_HTTPD_HTTPD_MANAGER_H_
