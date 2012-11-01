// HttpManager.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpdModule.h"
#include "ppbox/httpd/HttpServer.h"
#include "ppbox/httpd/Version.h"

#include <ppbox/dispatch/DispatchModule.h>

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.httpd.HttpdModule", framework::logger::Debug);

namespace ppbox
{
    namespace httpd
    {

        HttpdModule::HttpdModule(
            util::daemon::Daemon & daemon)
            : ppbox::common::CommonModuleBase<HttpdModule>(daemon, "HttpdModule")
            , util::protocol::HttpServerManager<HttpServer, HttpdModule>(daemon.io_svc())
            , addr_("0.0.0.0:9006")
            , dispatch_module_(util::daemon::use_module<ppbox::dispatch::DispatchModule>(get_daemon()))
        {
            config().register_module("HttpManager")
                << CONFIG_PARAM_NAME_RDWR("addr", addr_);
        }

        HttpdModule::~HttpdModule()
        {
        }

        boost::system::error_code HttpdModule::startup()
        {
            boost::system::error_code ec;
            start(addr_,ec);
            return ec;
        }

        void HttpdModule::shutdown()
        {
            stop();
        }

    } // namespace httpd
} // namespace ppbox
