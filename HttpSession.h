// HttpSession.h

#ifndef _PPBOX_HTTPD_HTTP_SESSION_H_
#define _PPBOX_HTTPD_HTTP_SESSION_H_

#include <ppbox/dispatch/DispatchBase.h>

#include <util/tools/ClassFactory.h>

namespace ppbox
{
    namespace httpd
    {

        class HttpSession
            : public util::tools::ClassFactory<
                HttpSession, 
                std::string, 
                HttpSession * (void)
            >
        {
        public:
            HttpSession();

            virtual ~HttpSession();

        public:
            void close();

            bool empty() const;

        public:
            virtual void attach(
                framework::string::Url & url, 
                ppbox::dispatch::DispatcherBase *& dispatcher);

            virtual bool detach(
                framework::string::Url const & url, 
                ppbox::dispatch::DispatcherBase *& dispatcher);

        protected:
            void attach();

            void detach();

        private:
            size_t nref_;
        };

    } // namespace dispatch
} // namespace ppbox

#define PPBOX_REGISTER_HTTP_SESSION(k, c) UTIL_REGISTER_CLASS(k, c)

#endif // _PPBOX_HTTPD_M3U8_SESSION_H_
