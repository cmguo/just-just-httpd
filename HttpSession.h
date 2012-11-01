// HttpSession.h

#ifndef _PPBOX_HTTPD_HTTP_SESSION_H_
#define _PPBOX_HTTPD_HTTP_SESSION_H_

#include <ppbox/common/Call.h>
#include <ppbox/common/Create.h>

#include <framework/string/Url.h>

#define PPBOX_REGISTER_HTTPD_PROTO(n, c) \
    static ppbox::common::Call reg_ ## c(ppbox::httpd::HttpSession::register_proto, #n, ppbox::common::Creator<c>())


namespace ppbox
{
    namespace dispatch
    {
        class DispatcherBase;
    }

    namespace httpd
    {

        class HttpdModule;
        class SessionDispatcher;

        /* 
            利用dispatch模块SharedDispatcher提供的会话管理功能。
            HttpSession在SessionManager中保持一个会话，使SessionManager不会关闭相应的SessionGroup
            HttpSession可以补充省略的playlink、format等参数
        */

        class HttpSession
        {
        public:
            typedef boost::function<
                void (ppbox::dispatch::DispatcherBase *)
            > delete_t;

            typedef boost::function<HttpSession * (
                ppbox::dispatch::DispatcherBase &, 
                delete_t)
            > register_type;

        public:
            static void register_proto(
                std::string const & proto, 
                register_type func);

        public:
            HttpSession(
                ppbox::dispatch::DispatcherBase & dispatcher, 
                delete_t deleter);

            virtual ~HttpSession();

        public:
            virtual void start(
                framework::string::Url const & url);

            virtual void close();

        public:
            virtual ppbox::dispatch::DispatcherBase * attach(
                framework::string::Url & url);

        private:
            void delete_dispatcher(
                delete_t deleter, 
                ppbox::dispatch::DispatcherBase & dispatcher);

        public:
            static ppbox::dispatch::DispatcherBase * attach(
                HttpdModule & module, 
                framework::string::Url & url);

            static void detach(
                HttpdModule & module, 
                ppbox::dispatch::DispatcherBase * dispatcher);

        protected:
            SessionDispatcher * dispatcher_;

        private:
            typedef std::map<std::string, register_type> proto_map_t;

            typedef std::map<std::string, HttpSession *> session_map_t;

            static proto_map_t & proto_map();

            static session_map_t & session_map();
        };

    } // namespace dispatch
} // namespace ppbox

#endif // _PPBOX_HTTPD_M3U8_SESSION_H_
