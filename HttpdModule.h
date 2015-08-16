// HttpdModule.h

#ifndef _JUST_HTTPD_HTTPD_MANAGER_H_
#define _JUST_HTTPD_HTTPD_MANAGER_H_

#include <just/dispatch/DispatchBase.h>

#include <just/common/CommonModuleBase.h>

#include <framework/network/ServerManager.h>

namespace just
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
            : public just::common::CommonModuleBase<HttpdModule>
            , public framework::network::ServerManager<HttpServer, HttpdModule>
        {
        public:
            HttpdModule(
                util::daemon::Daemon & daemon);

            ~HttpdModule();

        public:
            virtual bool startup(
                boost::system::error_code & ec);

            virtual bool shutdown(
                boost::system::error_code & ec);

        public:
            // avoid ambiguous
            using just::common::CommonModuleBase<HttpdModule>::io_svc;

        public:
            just::dispatch::DispatcherBase * attach(
                framework::string::Url & url, 
                boost::system::error_code & ec);

            void detach(
                framework::string::Url const & url, 
                just::dispatch::DispatcherBase * dispatcher);

        private:
            framework::network::NetName addr_;
            just::dispatch::DispatchModule & dispatch_module_;

        private:
            typedef std::map<std::string, HttpSession *> session_map_t;
            typedef std::map<just::dispatch::DispatcherBase *, HttpSession *> session_map2_t;
            session_map_t session_map_;
            session_map2_t session_map2_;
            std::vector<HttpSession *> closed_sessions_;
        };

    } // namespace httpd
} // namespace just

#endif // _JUST_HTTPD_HTTPD_MANAGER_H_
