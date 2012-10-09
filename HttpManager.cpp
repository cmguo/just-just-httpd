//HttpServer.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpManager.h"
#include "ppbox/httpd/HttpSession.h"

#include <ppbox/dispatcher/Dispatcher.h>
#include <ppbox/dispatcher/DispatcherManager.h>

FRAMEWORK_LOGGER_DECLARE_MODULE("HttpManager");

//#define LOG_S(a,b) std::cout<<b<<std::endl; 


namespace ppbox
{
    namespace httpd
    {


        HttpManager::HttpManager(
            util::daemon::Daemon & daemon)
            : ppbox::common::CommonModuleBase<HttpManager>(daemon, "HttpManager")
            , util::protocol::HttpProxyManager<HttpSession,HttpManager>(daemon.io_svc())
            , dispMgr_(util::daemon::use_module<ppbox::dispatcher::DispatcherManager>(daemon))
            , addr_("0.0.0.0:9006")
        {
            config().register_module("HttpManager")
                << CONFIG_PARAM_NAME_RDWR("addr", addr_);
        }

        HttpManager::~HttpManager()
        {
        }

        boost::system::error_code HttpManager::startup()
        {
            boost::system::error_code ec;
            start(addr_,ec);
            return ec;
        }

        void HttpManager::shutdown()
        {
            stop();
        }

        ppbox::dispatcher::Dispatcher * HttpManager::dispatcher()
        {
            return dispMgr_.dispather();
        }

    } // namespace httpd

} // namespace ppbox


