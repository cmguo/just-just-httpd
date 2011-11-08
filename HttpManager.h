//HttpServer.h

#ifndef      _PPBOX_HTTP_MANAGER_
#define      _PPBOX_HTTP_MANAGER_

#include <ppbox/common/CommonModuleBase.h>
#include <util/protocol/http/HttpProxyManager.h>

namespace ppbox
{
    namespace httpd
    {
        class HttpSession;
        class HttpDispatcher;

        class HttpManager
            : public ppbox::common::CommonModuleBase<HttpManager>
            , public util::protocol::HttpProxyManager<HttpSession,HttpManager>
        {
        public:
            virtual boost::system::error_code startup();

            virtual void shutdown();

        public:
            HttpManager(
                util::daemon::Daemon & daemon);

            ~HttpManager();

            using ppbox::common::CommonModuleBase<HttpManager>::io_svc;

            HttpDispatcher * dispatcher()
            {
                return dispatcher_;
            }

        private:
            framework::network::NetName addr_;
            HttpDispatcher* dispatcher_;
        };

    } // namespace httpd

} // namespace ppbox

#endif
