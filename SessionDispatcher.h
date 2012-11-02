// SessionDispatcher.h

#ifndef _PPBOX_HTTPD_SESSION_DISPATCHER_H_
#define _PPBOX_HTTPD_SESSION_DISPATCHER_H_

#include <ppbox/dispatch/CustomDispatcher.h>

#include <framework/timer/ClockTime.h>

#include <boost/asio/deadline_timer.hpp>

namespace ppbox
{
    namespace httpd
    {

        class WrapSink;

        class SessionDispatcher
            : public ppbox::dispatch::CustomDispatcher
        {
        public:
            typedef boost::function<
                void (ppbox::dispatch::DispatcherBase &)
            > delete_t;

        public:
            SessionDispatcher(
                ppbox::dispatch::DispatcherBase & dispatcher, 
                delete_t deleter);

            virtual ~SessionDispatcher();

        public:
            void attach();

            void detach();

            void mark_close();

        public:
            virtual void async_open(
                framework::string::Url const & url, 
                ppbox::dispatch::response_t const & resp);

            virtual bool setup(
                boost::uint32_t index,      // Á÷±àºÅ
                util::stream::Sink & sink, 
                boost::system::error_code & ec); 

            virtual void async_play(
                ppbox::dispatch::SeekRange const & range, 
                ppbox::dispatch::response_t const & seek_resp,
                ppbox::dispatch::response_t const & resp);

            virtual bool close(
                boost::system::error_code & ec);

        private:
            void handle_open(
                boost::system::error_code const & ec);

            void handle_seek(
                ppbox::dispatch::response_t const & seek_resp,
                util::stream::Sink * sink, 
                boost::system::error_code const & ec);

            void handle_timer(
                boost::system::error_code const & ec);

        private:
            size_t nref_;
            std::vector<ppbox::dispatch::response_t> open_resps_;
            delete_t deleter_;
            bool open_start_;
            WrapSink * wrap_sink_;
            util::stream::Sink * sink_;
            boost::system::error_code last_ec_;
            typedef boost::asio::basic_deadline_timer<
                framework::timer::ClockTime> timer_t;
            timer_t timer_;
        };

    } // namespace httpd
} // namespace ppbox

#endif // _PPBOX_HTTPD_SESSION_DISPATCHER_H_
