// M3u8Session.h

#ifndef _PPBOX_HTTPD_M3U8_SESSION_H_
#define _PPBOX_HTTPD_M3U8_SESSION_H_

#include "ppbox/httpd/HttpSession.h"

namespace ppbox
{
    namespace httpd
    {

        class M3u8Session
            : public HttpSession
        {
        public:
            M3u8Session();

            virtual ~M3u8Session();

        public:
            virtual void attach(
                framework::string::Url & url, 
                ppbox::dispatch::DispatcherBase *& dispatcher);

            virtual void detach(
                framework::string::Url const & url, 
                ppbox::dispatch::DispatcherBase *& dispatcher);

        protected:
            framework::string::Url url_format_;
        };

        PPBOX_REGISTER_HTTP_SESSION("m3u8", M3u8Session);

    } // namespace httpd
} // namespace ppbox

#endif // _PPBOX_HTTPD_M3U8_SESSION_H_
