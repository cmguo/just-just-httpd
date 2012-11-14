// HttpManager.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpdModule.h"
#include "ppbox/httpd/HttpServer.h"
#include "ppbox/httpd/Version.h"
#define PPBOX_ENABLE_REGISTER_CLASS
#include "ppbox/httpd/M3u8Session.h"

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

        struct HttpdModule::find_call_detach
        {
            find_call_detach(
                framework::string::Url const & url, 
                ppbox::dispatch::DispatcherBase *& dispatcher) 
                : url_(url)
                , dispatcher_(dispatcher)
            {
            }

            bool operator()(
                session_map_t::value_type const & v) const
            {
                return v.second->detach(url_, dispatcher_);
            }

        private:
            framework::string::Url const & url_;
            ppbox::dispatch::DispatcherBase *& dispatcher_;
        };

        HttpdModule::session_map_t & HttpdModule::session_map()
        {
            static session_map_t g_map;
            return g_map;
        }

        ppbox::dispatch::DispatcherBase * HttpdModule::attach(
            framework::string::Url & url)
        {
            boost::system::error_code ec;
            dispatch_module_.normalize_url(url, ec);

            std::string session_id = url.param("session");

            //if (session_id.empty()) {
            //    return module.dispatch_module().alloc_dispatcher(true);
            //}

            HttpSession * session = NULL;

            if (!session_id.empty()) {
                session_map_t::const_iterator iter = session_map().find(session_id);
                if (iter != session_map().end()) {
                    session = iter->second;
                }
            }

            ppbox::dispatch::DispatcherBase * dispatcher = 
                dispatch_module_.alloc_dispatcher(true);

            if (session == NULL) {
                std::string proto = url.param("proto");
                if (proto.empty()) {
                    proto = url.param(ppbox::dispatch::param_format);
                }
                session = HttpSession::create(proto);
                if (session != NULL) {
                    if (session_id.empty()) {
                        session_id = proto + framework::string::format((long)session);
                        url.param("session", session_id);
                    }
                    session_map().insert(std::make_pair(session_id, session));
                }
            }
            if (session != NULL) {
                session->attach(url, dispatcher); // 具体的协议可以指定自定义的dispatcher
            }
            return dispatcher;
        }

        void HttpdModule::detach(
            framework::string::Url const & url, 
            ppbox::dispatch::DispatcherBase * dispatcher)
        {
            std::find_if(session_map().begin(), session_map().end(), find_call_detach(url, dispatcher));
            dispatch_module_.free_dispatcher(dispatcher);
        }

    } // namespace httpd
} // namespace ppbox
