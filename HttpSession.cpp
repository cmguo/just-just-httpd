// HttpSession.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpSession.h"

#include <ppbox/common/UrlHelper.h>

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.httpd.HttpSession", framework::logger::Debug);

namespace ppbox
{
    namespace httpd
    {

        HttpSession::HttpSession()
            : closed_(false)
            , nref_(0)
        {
        }

        HttpSession::~HttpSession()
        {
        }

        bool HttpSession::closed() const
        {
            return closed_;
        }

        bool HttpSession::empty() const
        {
            return closed_ && nref_ == 0;
        }

        void HttpSession::attach(
            framework::string::Url & url, 
            ppbox::dispatch::DispatcherBase *& dispatcher)
        {
            if (!url_.is_valid()) {
                url_ = url;
            } else {
                ppbox::common::apply_params(url, url_);
            }
            ++nref_;
        }

        void HttpSession::detach(
            framework::string::Url const & url, 
            ppbox::dispatch::DispatcherBase *& dispatcher)
        {
            --nref_;
        }

        void HttpSession::close()
        {
            closed_ = true;
        }

    } // namespace dispatch
} // namespace ppbox
