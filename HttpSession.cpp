// HttpSession.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpSession.h"

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.httpd.HttpSession", framework::logger::Debug);

namespace ppbox
{
    namespace httpd
    {

        HttpSession::HttpSession()
        {
        }

        HttpSession::~HttpSession()
        {
        }

        void HttpSession::attach(
            framework::string::Url & url, 
            ppbox::dispatch::DispatcherBase *& dispatcher)
        {
            url.param("dispatch.fast", "true");
        }

        bool HttpSession::detach(
            framework::string::Url const & url, 
            ppbox::dispatch::DispatcherBase *& dispatcher)
        {
            return false;
        }

    } // namespace dispatch
} // namespace ppbox
