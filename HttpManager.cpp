//HttpServer.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpManager.h"
#include "ppbox/httpd/HttpSession.h"

#include "ppbox/dispatcher/MuxDispatcher.h"
#include "ppbox/merge/MergeDispatcher.h"

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
            , addr_("0.0.0.0:9006")
        {
            dispatcher_[0] = new ppbox::merge::MergeDispatcher(daemon.io_svc());
            dispatcher_[1] = new ppbox::dispatcher::MuxDispatcher(daemon.io_svc());

            config().register_module("HttpManager")
                << CONFIG_PARAM_NAME_RDWR("addr", addr_);
        }

        HttpManager::~HttpManager()
        {
            delete dispatcher_[0];
            delete dispatcher_[1];
        }

        boost::system::error_code HttpManager::startup()
        {
            boost::system::error_code ec;
            dispatcher_[0]->start();
            dispatcher_[1]->start();
            start(addr_,ec);
            return ec;
        }

        void HttpManager::shutdown()
        {
            stop();
            dispatcher_[0]->stop();
            dispatcher_[1]->stop();
        }

    } // namespace httpd

} // namespace ppbox


