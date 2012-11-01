// HttpSession.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpSession.h"
#include "ppbox/httpd/SessionDispatcher.h"
#include "ppbox/httpd/HttpdModule.h"
#include "ppbox/httpd/M3u8Session.h"

#include <ppbox/dispatch/DispatchModule.h>
#include <ppbox/dispatch/DispatcherBase.h>

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>
using namespace framework::logger;

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.httpd.HttpSession", framework::logger::Debug);

namespace ppbox
{
    namespace httpd
    {

        HttpSession::HttpSession(
            ppbox::dispatch::DispatcherBase & dispatcher, 
            delete_t deleter)
        {
            dispatcher_ = new SessionDispatcher(dispatcher, 
                boost::bind(&HttpSession::delete_dispatcher, this, deleter, _1));
        }

        HttpSession::~HttpSession()
        {
        }

        static void nop_resp(boost::system::error_code const & ec) {}

        void HttpSession::start(
            framework::string::Url const & url)
        {
            dispatcher_->async_open(url, nop_resp);
        }

        void HttpSession::close()
        {
            boost::system::error_code ec;
            dispatcher_->close(ec);
        }

        ppbox::dispatch::DispatcherBase * HttpSession::attach(
            framework::string::Url & url)
        {
            if (url.param("close") == "true") {
                close();
            }
            return dispatcher_;
        }

        void HttpSession::delete_dispatcher(
            delete_t deleter, 
            ppbox::dispatch::DispatcherBase & dispatcher)
        {
            struct find_by_session
            {
                find_by_session(
                    HttpSession * session) 
                    : session_(session)
                {
                }

                bool operator()(
                    session_map_t::value_type const & v)
                {
                    return v.second == session_;
                }

            private:
                HttpSession * session_;
            } finder(this);

            deleter(&dispatcher);
            session_map().erase(std::find_if(session_map().begin(), session_map().end(), finder));
            delete this;
        }

        HttpSession::proto_map_t & HttpSession::proto_map()
        {
            static proto_map_t g_map;
            return g_map;
        }

        void HttpSession::register_proto(
            std::string const & proto, 
            register_type func)
        {
            proto_map().insert(std::make_pair(proto, func));
        }

        HttpSession::session_map_t & HttpSession::session_map()
        {
            static session_map_t g_map;
            return g_map;
        }

        ppbox::dispatch::DispatcherBase * HttpSession::attach(
            HttpdModule & module, 
            framework::string::Url & url)
        {
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

            if (session == NULL) {
                ppbox::dispatch::DispatcherBase * dispatcher = 
                    module.dispatch_module().alloc_dispatcher(true);
                {
                    std::string proto = url.param("proto");
                    if (proto.empty()) {
                        proto = url.param(ppbox::dispatch::param_format);
                    }
                    proto_map_t::const_iterator iter = proto_map().find(proto);
                    if (iter == proto_map().end()) {
                        if (session_id.empty()) {
                            return module.dispatch_module().alloc_dispatcher(true);
                        } else {
                            session = new HttpSession(*dispatcher, 
                                boost::bind(&ppbox::dispatch::DispatchModule::free_dispatcher, &module.dispatch_module(), _1));
                        }
                    } else {
                        session = iter->second(*dispatcher, 
                            boost::bind(&ppbox::dispatch::DispatchModule::free_dispatcher, &module.dispatch_module(), _1));
                        if (session_id.empty()) {
                            session_id = proto + framework::string::format((long)session);
                            url.param("session", session_id);
                        }
                    }
                }
                session->start(url);
                session_map().insert(std::make_pair(session_id, session));
            }

            return session->attach(url);
        }

        void HttpSession::detach(
            HttpdModule & module, 
            ppbox::dispatch::DispatcherBase * dispatcher)
        {
            module.dispatch_module().free_dispatcher(dispatcher);
        }

    } // namespace dispatch
} // namespace ppbox
