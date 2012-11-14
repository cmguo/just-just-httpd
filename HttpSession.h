// HttpSession.h

#ifndef _PPBOX_HTTPD_HTTP_SESSION_H_
#define _PPBOX_HTTPD_HTTP_SESSION_H_

#include <ppbox/dispatch/DispatchBase.h>

#include <ppbox/common/ClassFactory.h>

namespace ppbox
{
    namespace httpd
    {

        class HttpSession
            : public ppbox::common::ClassFactory<
                HttpSession, 
                std::string, 
                HttpSession * (void)
            >
        {
        public:
            HttpSession();

            virtual ~HttpSession();

        public:
            virtual void attach(
                framework::string::Url & url, 
                ppbox::dispatch::DispatcherBase *& dispatcher);

            virtual bool detach(
                framework::string::Url const & url, 
                ppbox::dispatch::DispatcherBase *& dispatcher);
        };

    } // namespace dispatch
} // namespace ppbox

#define PPBOX_REGISTER_HTTP_SESSION(k, c) PPBOX_REGISTER_CLASS(k, c)

#endif // _PPBOX_HTTPD_M3U8_SESSION_H_
