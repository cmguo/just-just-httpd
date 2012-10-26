// HttpManager.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpManager.h"
#include "ppbox/httpd/HttpSession.h"
#include "ppbox/httpd/Version.h"

#include <ppbox/dispatch/DispatchModule.h>

FRAMEWORK_LOGGER_DECLARE_MODULE("ppbox.httpd.HttpManager");

namespace ppbox
{
    namespace httpd
    {

        HttpManager::HttpManager(
            util::daemon::Daemon & daemon)
            : ppbox::common::CommonModuleBase<HttpManager>(daemon, "HttpManager")
            , util::protocol::HttpServerManager<HttpSession,HttpManager>(daemon.io_svc())
            , addr_("0.0.0.0:9006")
            , dispatch_module_(util::daemon::use_module<ppbox::dispatch::DispatchModule>(get_daemon()))
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

    } // namespace httpd
} // namespace ppbox
