// SessionDispatcher.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/SessionDispatcher.h"

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>

#include <boost/bind.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.httpd.SessionDispatcher", framework::logger::Debug);

namespace ppbox
{
    namespace httpd
    {

        class WrapSink
            : public util::stream::Sink
        {
        public:
            WrapSink(
                boost::asio::io_service & io_svc)
                : Sink(io_svc)
            {
            }

        public:
            void set_sink(
                util::stream::Sink * sink)
            {
                sink_ = sink;
            }

        private:
            virtual std::size_t private_write_some(
                buffers_t const & buffers,
                boost::system::error_code & ec)
            {
                return sink_->write_some(buffers, ec);
            }

            virtual void private_async_write_some(
                buffers_t const & buffers, 
                handler_t const & handler)
            {
                sink_->async_write_some(buffers, handler);
            }

        private:
            util::stream::Sink * sink_;
        };

        void nop_deleter(ppbox::dispatch::DispatcherBase & dispatcher) {}

        SessionDispatcher::SessionDispatcher(
            ppbox::dispatch::DispatcherBase & dispatcher, 
            delete_t deleter)
            : CustomDispatcher(dispatcher)
            , nref_(0)
            , deleter_(deleter)
            , open_start_(false)
            , timer_(io_svc())
        {
            wrap_sink_= new WrapSink(dispatcher.io_svc());
        }

        SessionDispatcher::~SessionDispatcher()
        {
            delete wrap_sink_;
        }

        void SessionDispatcher::attach()
        {
            ++nref_;
        }

        void SessionDispatcher::detach()
        {
            if (--nref_ == 0) {
                CustomDispatcher::close(last_ec_);
                deleter_(dispatcher_);
                delete this;
            }
        }

        void SessionDispatcher::mark_close()
        {
            last_ec_ = boost::asio::error::operation_aborted;
            boost::system::error_code ec;
            timer_.cancel(ec);
        }

        void SessionDispatcher::async_open(
            framework::string::Url const & url, 
            ppbox::dispatch::response_t const & resp)
        {
            if (!open_start_) {
                attach();
                CustomDispatcher::async_open(url, 
                    boost::bind(&SessionDispatcher::handle_open, this, _1));
                open_start_ = true;
                open_resps_.push_back(resp);
            } else if (open_resps_.empty()) {
                io_svc().post(boost::bind(resp, last_ec_));
                return;
            } else {
                open_resps_.push_back(resp);
            }
        }

        bool SessionDispatcher::setup(
            boost::uint32_t index, 
            util::stream::Sink & sink, 
            boost::system::error_code & ec)
        {
            LOG_DEBUG("[setup] sink:" << &sink);
            sink_ = &sink;
            ec = last_ec_;
            return !ec;
        }

        void SessionDispatcher::async_play(
            ppbox::dispatch::SeekRange const & range, 
            ppbox::dispatch::response_t const & seek_resp,
            ppbox::dispatch::response_t const & resp)
        {
            LOG_DEBUG("[async_play] sink:" << sink_);
            // HttpServer 会在调用setup后立即调用async_play，所以 sink_ 应该是当前请求的
            assert(sink_ != NULL);
            if (last_ec_) {
                io_svc().post(boost::bind(resp, last_ec_));
                return;
            }
            CustomDispatcher::async_play(
                range, 
                boost::bind(&SessionDispatcher::handle_seek, this, seek_resp, sink_, _1), 
                resp);
            sink_ = NULL;
        }

        bool SessionDispatcher::close(
            boost::system::error_code & ec)
        {
            if (nref_ == 1 && !open_resps_.empty()) {
                CustomDispatcher::cancel(ec);
                return !ec;
            } else {
                ec.clear();
                return true;
            }
        }

        void SessionDispatcher::handle_open(
            boost::system::error_code const & ec)
        {
            last_ec_ = ec;
            if (!last_ec_) {
                CustomDispatcher::setup(-1, *wrap_sink_, last_ec_);
            }
            std::vector<ppbox::dispatch::response_t> open_resps;
            open_resps.swap(open_resps_);
            for (size_t i = 0; i < open_resps.size(); ++i) {
                open_resps[i](last_ec_);
            }
            handle_timer(ec);
        }

        void SessionDispatcher::handle_seek(
            ppbox::dispatch::response_t const & seek_resp,
            util::stream::Sink * sink, 
            boost::system::error_code const & ec)
        {
            LOG_DEBUG("[handle_seek] sink:" << sink);
            wrap_sink_->set_sink(sink);
            if (!seek_resp.empty()) {
                seek_resp(ec);
            } else if (!ec) {
                boost::system::error_code ec1;
                CustomDispatcher::resume(ec1);
            }
        }

        void SessionDispatcher::handle_timer(
            boost::system::error_code const & ec)
        {
            ppbox::data::PlayInfo info;
            if (!last_ec_ && CustomDispatcher::get_play_info(info, last_ec_)) {
                attach();
                timer_.expires_from_now(framework::timer::Duration::seconds(5));
                timer_.async_wait(
                    boost::bind(&SessionDispatcher::handle_timer, this, _1));
            }
            detach();
        }


    } // namespace dispatch
} // namespace ppbox

