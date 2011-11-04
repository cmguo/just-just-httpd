//HttpServer.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpManager.h"

#include "ppbox/httpd/HttpDispatcher.h"
#include "ppbox/httpd/HttpSession.h"

FRAMEWORK_LOGGER_DECLARE_MODULE("HttpManager");

//#define LOG_S(a,b) std::cout<<b<<std::endl; 


namespace ppbox
{
    namespace httpd
    {

        HttpDispatcher* dispatch_ = NULL;

        HttpManager::HttpManager(
            util::daemon::Daemon & daemon)
            : ppbox::common::CommonModuleBase<HttpManager>(daemon, "HttpManager")
            , util::protocol::HttpProxyManager<HttpSession>(daemon.io_svc())
            , addr_(framework::network::NetName("0.0.0.0:9006"))
        {
            dispatch_ = new HttpDispatcher(daemon);
        }

        HttpManager::~HttpManager()
        {
        }

        boost::system::error_code HttpManager::startup()
        {
#ifndef _LIB
            PPBOX_StartP2PEngine("12","1","599253c13bb94a09b73a151cf3a803ce");
#endif
            boost::system::error_code ec;
            start(addr_,ec);
            return ec;
        }

        void HttpManager::shutdown()
        {
            stop();
        }

    } // namespace httpd

} // namespace ppbox


