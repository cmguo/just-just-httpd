// HttpSession.h

#ifndef _JUST_HTTPD_HTTP_SESSION_H_
#define _JUST_HTTPD_HTTP_SESSION_H_

#include <just/dispatch/DispatchBase.h>

#include <util/tools/ClassFactory.h>

namespace just
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
                just::dispatch::DispatcherBase *& dispatcher);

            virtual void detach(
                framework::string::Url const & url, 
                just::dispatch::DispatcherBase *& dispatcher);

            virtual void close();

        private:
            bool closed_;
            size_t nref_;
            framework::string::Url url_;
            just::dispatch::DispatcherBase * dispatcher_;
        };

        struct HttpSessionTraits
            : util::tools::ClassFactoryTraits
        {
            typedef std::string key_type;
            typedef HttpSession * (create_proto)();
        };

        typedef util::tools::ClassFactory<HttpSessionTraits> HttpSessionFactory;

    } // namespace dispatch
} // namespace just

#define JUST_REGISTER_HTTP_SESSION(k, c) UTIL_REGISTER_CLASS(HttpSessionFactory, k, c)

#endif // _JUST_HTTPD_M3U8_SESSION_H_
