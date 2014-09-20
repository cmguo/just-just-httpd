// HttpdModule.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpdModule.h"
#include "ppbox/httpd/HttpServer.h"
#include "ppbox/httpd/Version.h"
#include "ppbox/httpd/ClassRegister.h"

#include <ppbox/dispatch/DispatchModule.h>

#include <framework/network/TcpSocket.hpp>
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
            , framework::network::ServerManager<HttpServer, HttpdModule>(daemon.io_svc())
            , addr_("0.0.0.0:9006+")
            , dispatch_module_(util::daemon::use_module<ppbox::dispatch::DispatchModule>(get_daemon()))
        {
            config().register_module("HttpdModule")
                << CONFIG_PARAM_NAME_RDWR("addr", addr_);
        }

        HttpdModule::~HttpdModule()
        {
            assert(session_map2_.empty());
            assert(closed_sessions_.empty());
            session_map_t::iterator iter = session_map_.begin();
            for (; iter != session_map_.end(); ++iter) {
                delete iter->second;
                iter->second = NULL;
            }
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

        ppbox::dispatch::DispatcherBase * HttpdModule::attach(
            framework::string::Url & url, 
            boost::system::error_code & ec)
        {
            dispatch_module_.normalize_url(url, ec);
            url.param("dispatch.fast", "true");
            std::string::size_type pos = url.path().find('.');
            if (pos != std::string::npos) {
                url.path(url.path().substr(0, pos));
            }

            std::string session_id = url.param("session");
            bool close = false;

            HttpSession * session = NULL;

            if (!session_id.empty()) {
                if (session_id.compare(0, 5, "close") == 0) {
                    close = true;
                    session_id = session_id.substr(5);
                }
                session_map_t::const_iterator iter = session_map_.find(session_id);
                if (iter != session_map_.end()) {
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
                boost::system::error_code ec;
                session = HttpSessionFactory::create(proto, ec);
                if (session != NULL) {
                    if (session_id.empty()) {
                        session_id = proto + framework::string::format((long)session);
                        url.param("session", session_id);
                    }
                    session_map_.insert(std::make_pair(session_id, session));
                }
            }
            if (session != NULL) {
                session->attach(url, dispatcher); // 具体的协议可以指定自定义的dispatcher
                session_map2_.insert(std::make_pair(dispatcher, session));
                if (close) {
                    session->close();
                    session_map_.erase(session_id);
                    closed_sessions_.push_back(session);
                }
            }
            return dispatcher;
        }

        void HttpdModule::detach(
            framework::string::Url const & url, 
            ppbox::dispatch::DispatcherBase * dispatcher)
        {
            session_map2_t::iterator iter = session_map2_.find(dispatcher);
            if (iter == session_map2_.end())
                return;
            HttpSession * session = iter->second;
            session_map2_.erase(iter);
            session->detach(url, dispatcher);
            if (session->empty()) {
                closed_sessions_.erase(
                    std::find(closed_sessions_.begin(), closed_sessions_.end(), session));
                HttpSessionFactory::destroy(session);
            }
            dispatch_module_.free_dispatcher(dispatcher);
        }

    } // namespace httpd
} // namespace ppbox
