// HttpProto.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/M3u8Session.h"
#include "ppbox/httpd/SessionDispatcher.h"

#include <boost/asio/write.hpp>
#include <boost/bind.hpp>

namespace ppbox
{
    namespace httpd
    {

        class M3u8Dispatcher
            : public SessionDispatcher
        {
        public:
            M3u8Dispatcher(
                ppbox::dispatch::DispatcherBase & dispatcher)
                : SessionDispatcher(dispatcher)
                , sink_(NULL)
            {
            }

        public:
            virtual bool setup(
                boost::uint32_t index,      // 流编号
                util::stream::Sink & sink, 
                boost::system::error_code & ec)
            {
                sink_ = &sink;
                ec.clear();
                return true;
            }

            virtual void async_play(
                ppbox::dispatch::SeekRange const & range, 
                ppbox::dispatch::response_t const & seek_resp,
                ppbox::dispatch::response_t const & resp)
            {
                assert(sink_);
                ppbox::data::MediaInfo info;
                boost::system::error_code ec;
                if (get_media_info(info, ec)) {
                    // 这里不能异步写临时变量中的数据，所以交换到成员变量中
                    // 但是还是有可能M3u8Dispatcher提前析构
                    m3u8_content_.swap(info.format_data);
                    boost::asio::async_write(
                        *sink_, 
                        boost::asio::buffer(m3u8_content_), 
                        boost::bind(resp, _1));
                } else {
                    io_svc().post(boost::bind(resp, ec));
                }
            }

            virtual bool get_media_info(
                ppbox::data::MediaInfo & info, 
                boost::system::error_code & ec)
            {
                if (SessionDispatcher::get_media_info(info, ec)) {
                    info.file_size = info.format_data.size();
                    info.format = "m3u8";
                    return true;
                }
                return false;
            }

        private:
            util::stream::Sink * sink_;
            std::string m3u8_content_;
        };

        M3u8Session::M3u8Session(
            ppbox::dispatch::DispatcherBase & dispatcher, 
            delete_t deleter)
            : HttpSession(dispatcher, deleter)
        {
            dispatcher_ = new M3u8Dispatcher(*HttpSession::dispatcher_);
        }

        M3u8Session::~M3u8Session()
        {
        }

        static void nop_resp(boost::system::error_code const & ec) {}

        void M3u8Session::start(
            framework::string::Url const & url)
        {
            framework::string::Url url_format;
            url_format.protocol(url.protocol());
            url_format.host(url.host());
            url_format.svc(url.svc());
            url_format.path("/play.ts");
            url_format.param("start", "%n");
            url_format.param("session", url.param("session"));
            framework::string::Url url1 = url;
            url1.param("mux.M3u8Protocol.url_format", url_format.to_string());
            dispatcher_->async_open(url1, nop_resp);
        }

        void M3u8Session::close()
        {
            boost::system::error_code ec;
            dispatcher_->close(ec);
            HttpSession::close();
        }

        ppbox::dispatch::DispatcherBase * M3u8Session::attach(
            framework::string::Url & url)
        {
            if (url.param(ppbox::dispatch::param_format) == "ts") {
                // 分段请求
                return HttpSession::dispatcher_;
            } else {
                if (url.param("close") == "true") {
                    close();
                }
                return dispatcher_;
            }
        }

    } // namespace dispatch
} // namespace ppbox

