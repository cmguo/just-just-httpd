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
        class HttpManager
            : public ppbox::common::CommonModuleBase<HttpManager>
            , public util::protocol::HttpProxyManager<HttpSession>
        {
        public:
            virtual boost::system::error_code startup();

            virtual void shutdown();

        public:
            HttpManager(
                util::daemon::Daemon & daemon);

            ~HttpManager();

        private:
            framework::network::NetName addr_;
        };

    } // namespace httpd

} // namespace ppbox

#endif
