// M3u8Session.h

#ifndef _PPBOX_HTTPD_M3U8_SESSION_H_
#define _PPBOX_HTTPD_M3U8_SESSION_H_

#include "ppbox/httpd/HttpSession.h"

namespace ppbox
{
    namespace httpd
    {

        class M3u8Dispatcher;

        class M3u8Session
            : public HttpSession
        {
        public:
            M3u8Session(
                ppbox::dispatch::DispatcherBase & dispatcher, 
                delete_t deleter);

            virtual ~M3u8Session();

        public:
            virtual void start(
                framework::string::Url const & url);

            virtual void close();

        public:
            virtual ppbox::dispatch::DispatcherBase * attach(
                framework::string::Url & url);

        protected:
            M3u8Dispatcher * dispatcher_;
        };

        PPBOX_REGISTER_HTTPD_PROTO(m3u8, M3u8Session);

    } // namespace httpd
} // namespace ppbox

#endif // _PPBOX_HTTPD_M3U8_SESSION_H_
