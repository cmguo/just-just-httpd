//HttpServer.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpSession.h"
#include "ppbox/httpd/HttpError.h"
#include "ppbox/httpd/HttpSink.h"
#include "ppbox/httpd/HttpManager.h"

#include <ppbox/dispatch/DispatchModule.h>
#include <ppbox/dispatch/DispatcherBase.h>

#include <ppbox/data/MediaInfo.h>

#include <ppbox/common/CommonUrl.h>

#include <util/protocol/http/HttpSocket.h>
#include <util/archive/XmlOArchive.h>
#include <util/serialization/ErrorCode.h>
using namespace util::protocol;

#include <framework/string/Base64.h>
#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>
using namespace framework::logger;

#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.httpd.HttpSession", Debug);

namespace util
{
    namespace serialization
    {

        template <
            typename Archive
        >
        void serialize(
            Archive & ar, 
            ppbox::data::MediaInfo & info)
        {
            ar & SERIALIZATION_NVP_1(info, file_size);
            ar & SERIALIZATION_NVP_1(info, duration);
            ar & SERIALIZATION_NVP_1(info, bitrate);
            ar & SERIALIZATION_NVP_1(info, is_live);
        }

    }
}

namespace ppbox
{
    namespace httpd
    {

        HttpSession::HttpSession(
            HttpManager & mgr)
            : HttpServer(mgr.io_svc())
            , mgr_(mgr)
            , dispatcher_(NULL)
        {
        }

        HttpSession::~HttpSession()
        {
            for (std::vector<util::stream::Sink *>::iterator iter = sinks_.begin();
                iter != sinks_.end();
                ++iter) {
                delete *iter;
            }
            sinks_.clear();

            if (dispatcher_) {
                mgr_.dispatch_module().free_dispatcher(dispatcher_);
            }
        }

        void HttpSession::local_process(response_type const & resp)
        {
            //LOG_INFO("[local_process]");

            request_head().get_content(std::cout);

            url_.from_string("http://host" + request_head().path);

            boost::system::error_code ec;
            ppbox::common::decode_url(url_, ec)
                && mgr_.dispatch_module().normalize_url(url_, ec);

            if (!ec) {
                std::string option = url_.path();
                if (option != "/mediainfo"
                    && option != "/playinfo"
                    && option != "/play") {
                        ec = framework::system::logic_error::invalid_argument;
                }

                if (option == "/play") {
                    if(request().head().range.is_initialized()) {
                        util::protocol::http_field::RangeUnit unit = 
                            request().head().range.get()[0];
                        seek_range_.type = ppbox::dispatch::SeekRange::byte;
                        seek_range_.beg = unit.begin();
                        if (unit.has_end()) {
                            seek_range_.end = unit.end();
                        }
                    } else if (!url_.param("start").empty()) { // 伪HTTP协议
                        seek_range_.type = ppbox::dispatch::SeekRange::time;
                        seek_range_.beg = framework::string::parse<boost::uint64_t>(url_.param("start"));
                    }
                }
            }

            if (ec) {
                make_error(ec);
                resp(ec, response_data().size());
                return;
            }

            dispatcher_ = mgr_.dispatch_module().alloc_dispatcher();

            dispatcher_->async_open(url_, 
                boost::bind(&HttpSession::handle_open, this,resp, _1));
        }

        void HttpSession::transfer_response_data(
            response_type const & resp)
        {
            response_head().get_content(std::cout);
            
            boost::system::error_code ec;
                
            if (response_data().size()) {
                HttpServer::transfer_response_data(resp);
            } else {
                assert(url_.path() == "/play");
                dispatcher_->async_play(seek_range_, ppbox::dispatch::response_t(), 
                    boost::bind(&HttpSession::handle_play,this, resp, _1));
            }
        }

        void HttpSession::on_finish()
        {
            LOG_INFO( "[on_finish] dispatcher:"<< dispatcher_);
        }

        void HttpSession::on_error(
            boost::system::error_code const & ec)
        {
            LOG_INFO("[on_error] dispatcher:" << dispatcher_ << " ec:" << ec.message());
            boost::system::error_code ec1;
            if (dispatcher_) {
                dispatcher_->close(ec1);
            }
        }

        char const * format_mine[][2] = {
            {"flv", "video/x-flv"}, 
            {"ts", "video/MP2T"}, 
            {"m3u8", "application/x-mpegURL"},
        };

        char const * content_type(
            std::string const & format)
        {
            for (size_t i = 0; i < sizeof(format_mine) / sizeof(format_mine[0]); ++i) {
                if (format == format_mine[i][0]) {
                    return format_mine[i][1];
                }
            }
            return "video";
        }

        //Dispatch 线程
        void HttpSession::handle_open(
            response_type const & resp,
            boost::system::error_code const & ec)
        {
            LOG_INFO( "[handle_open] dispatcher:" << dispatcher_ << " ec:" << ec.message());

            boost::system::error_code ec1 = ec;
            std::string option = url_.path();

            ppbox::data::MediaInfo info;

            if (!ec1) {
                if (option == "/play") {
                    if (url_.param("chunked") == "true") {
                        response_head()["Transfer-Encoding"]="{chunked}";
                    }
                    dispatcher_->setup(-1, response_stream(), ec1)
                        && dispatcher_->get_media_info(info, ec1);
                }
            }

            if (ec1) {
                make_error(ec1);
                resp(ec1, response_data().size());
                return;
            }

            if (option == "/play") {
                response_head()["Content-Type"] = std::string("{") + content_type(info.format) + "}";
                if (info.file_size == ppbox::data::invalid_size) {
                    resp(ec1, Size());
                } else {
                    if (seek_range_.type == ppbox::dispatch::SeekRange::byte) {
                        util::protocol::http_field::ContentRange content_range(info.file_size, seek_range_.beg, seek_range_.end);
                        response_head().content_range = content_range;
                        resp(ec1, Size((size_t)(info.file_size - seek_range_.beg)));
                    } else {
                        resp(ec1, Size((size_t)info.file_size));
                    }
                }
            } else {
                response_head()["Content-Type"]="{application/xml}";
                if (option == "/mediainfo") {
                    make_mediainfo(ec1);
                } else if (option == "/playinfo") {
                    make_playinfo(ec1);
                }
                if (ec1) {
                    make_error(ec1);
                }
                resp(ec1, response_data().size());
            }
        }

        void HttpSession::handle_play(
            response_type const & resp,
            boost::system::error_code const & ec)
        {
            LOG_INFO( "[handle_play] ec:"<<ec.message());
            resp(ec, 0);
        }

        void HttpSession::make_error(
            boost::system::error_code const & ec)
        {

            response_head().err_code = 500;
            response_head().err_msg = "Internal Server Error";
            response_head()["Content-Type"]="{application/xml}";

            util::archive::XmlOArchive<> oa(response_data());
            oa << ec;
            oa << SERIALIZATION_NVP_NAME("message", ec.message());
        }

        void HttpSession::make_mediainfo(
            boost::system::error_code & ec)
        {
            ppbox::data::MediaInfo info;
            dispatcher_->get_media_info(info, ec);
            if (!ec) {
                util::archive::XmlOArchive<> oa(response_data());
                oa << info;
            }
        }

        void HttpSession::make_playinfo(
            boost::system::error_code& ec)
        {
            ppbox::data::MediaInfo info;
            dispatcher_->get_play_info(info, ec);
            if (!ec) {
                util::archive::XmlOArchive<> oa(response_data());
                oa << info;
            }
        }

    } // namespace httpd
} // namespace ppbox
