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
            : nref_(1)
        {
        }

        HttpSession::~HttpSession()
        {
        }

        void HttpSession::close()
        {
            --nref_;
        }

        bool HttpSession::empty() const
        {
            return nref_ == 0;
        }

        void HttpSession::attach(
                framework::string::Url & url, 
            ppbox::dispatch::DispatcherBase *& dispatcher)
        {
        }

        bool HttpSession::detach(
            framework::string::Url const & url, 
            ppbox::dispatch::DispatcherBase *& dispatcher)
        {
            return false;
        }

        void HttpSession::attach()
        {
            ++nref_;
        }

        void HttpSession::detach()
        {
            --nref_;
        }

    } // namespace dispatch
} // namespace ppbox
