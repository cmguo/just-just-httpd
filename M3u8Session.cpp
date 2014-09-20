// HttpProto.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/M3u8Session.h"

#include <ppbox/dispatch/CustomDispatcher.h>

#include <util/stream/Sink.h>

#include <boost/asio/write.hpp>
#include <boost/bind.hpp>

namespace ppbox
{
    namespace httpd
    {

        class M3u8Dispatcher
            : public ppbox::dispatch::CustomDispatcher
        {
        public:
            M3u8Dispatcher(
                ppbox::dispatch::DispatcherBase & dispatcher)
                : ppbox::dispatch::CustomDispatcher(dispatcher)
                , sink_(NULL)
            {
            }

        public:
            virtual bool setup(
                boost::uint32_t index,      // �����
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
                    // ���ﲻ���첽д��ʱ�����е����ݣ����Խ�������Ա������
                    // ���ǻ����п���M3u8Dispatcher��ǰ����
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
                if (CustomDispatcher::get_media_info(info, ec)) {
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

        M3u8Session::M3u8Session()
        {
        }

        M3u8Session::~M3u8Session()
        {
        }

        void M3u8Session::attach(
            framework::string::Url & url, 
            ppbox::dispatch::DispatcherBase *& dispatcher)
        {
            HttpSession::attach(url, dispatcher);
            if (url.param(ppbox::dispatch::param_format) == "m3u8") {
                if (!url_format_.is_valid()) {
                    url_format_.protocol(url.protocol());
                    url_format_.host(url.host());
                    url_format_.svc(url.svc());
                    url_format_.path("/play.ts");
                    url_format_.param("start", "%n");
                    url_format_.param("session", url.param("session"));
                }
                url.param("mux.M3u8Protocol.url_format", url_format_.to_string());
                dispatcher = new M3u8Dispatcher(*dispatcher);
            }
        }

        void M3u8Session::detach(
            framework::string::Url const & url, 
            ppbox::dispatch::DispatcherBase *& dispatcher)
        {
            if (url.param(ppbox::dispatch::param_format) == "m3u8") {
                M3u8Dispatcher * m3u8_dispatcher = (M3u8Dispatcher *)dispatcher;
                dispatcher = m3u8_dispatcher->detach();
                delete m3u8_dispatcher;
            }
            HttpSession::detach(url, dispatcher);
        }

    } // namespace dispatch
} // namespace ppbox

