// M3u8Session.h

#ifndef _JUST_HTTPD_M3U8_SESSION_H_
#define _JUST_HTTPD_M3U8_SESSION_H_

#include "just/httpd/HttpSession.h"

namespace just
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
                just::dispatch::DispatcherBase *& dispatcher);

            virtual void detach(
                framework::string::Url const & url, 
                just::dispatch::DispatcherBase *& dispatcher);

        protected:
            framework::string::Url url_format_;
        };

        JUST_REGISTER_HTTP_SESSION("m3u8", M3u8Session);

    } // namespace httpd
} // namespace just

#endif // _JUST_HTTPD_M3U8_SESSION_H_
