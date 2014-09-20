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
        {
        public:
            HttpSession();

            virtual ~HttpSession();

        public:
            bool closed() const;

            bool empty() const;

        public:
            virtual void attach(
                framework::string::Url & url, 
                ppbox::dispatch::DispatcherBase *& dispatcher);

            virtual void detach(
                framework::string::Url const & url, 
                ppbox::dispatch::DispatcherBase *& dispatcher);

            virtual void close();

        private:
            bool closed_;
            size_t nref_;
            framework::string::Url url_;
            ppbox::dispatch::DispatcherBase * dispatcher_;
        };

        struct HttpSessionTraits
            : util::tools::ClassFactoryTraits
        {
            typedef std::string key_type;
            typedef HttpSession * (create_proto)();
        };

        typedef util::tools::ClassFactory<HttpSessionTraits> HttpSessionFactory;

    } // namespace dispatch
} // namespace ppbox

#define PPBOX_REGISTER_HTTP_SESSION(k, c) UTIL_REGISTER_CLASS(HttpSessionFactory, k, c)

#endif // _PPBOX_HTTPD_M3U8_SESSION_H_
