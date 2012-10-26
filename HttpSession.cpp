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
using namespace util::protocol;

#include <framework/string/Base64.h>
#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>
using namespace framework::logger;

#include <tinyxml/tinyxml.h>

#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.httpd.HttpSession", Debug);

namespace ppbox
{
    namespace httpd
    {

        HttpSession::HttpSession(
            HttpManager & mgr)
            : HttpServer(mgr.io_svc())
            , mgr_(mgr)
            , seek_(-1)
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
            }

            if (ec) {
                make_error_response_body(body_,ec);
                resp(ec,body_.size());
                return;
            }

            dispatcher_ = mgr_.dispatch_module().alloc_dispatcher();

            dispatcher_->async_open(url_, 
                boost::bind(&HttpSession::on_open, this,resp, _1));
        }

        static void nop_resp(boost::system::error_code const & ec) {}

        void HttpSession::transfer_response_data(
            response_type const & resp)
        {
            response_head().get_content(std::cout);
            
            boost::system::error_code ec;
                
            if (!body_.empty()) {
                std::cout << body_ << std::endl;
                size_t tSize = body_.size();
                write_some(boost::asio::buffer(body_), ec);
                body_.clear();
                resp(ec, tSize);
            } else {
                assert(url_.path() == "/play");
                ppbox::dispatch::SeekRange range;
                if(request().head().range.is_initialized()) {
                    util::protocol::http_field::RangeUnit unit = 
                        request().head().range.get()[0];
                    range.type = ppbox::dispatch::SeekRange::byte;
                    range.beg = unit.begin();
                    if (unit.has_end()) {
                        range.end = unit.end();
                    }
                } else if (!url_.param("start").empty()) { // 伪HTTP协议
                    range.type = ppbox::dispatch::SeekRange::time;
                    range.beg = framework::string::parse<boost::uint64_t>(url_.param("start"));
                }
                dispatcher_->async_play(range, nop_resp, 
                    boost::bind(&HttpSession::on_playend,this, resp, _1));
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
        }

        //Dispatch 线程
        void HttpSession::on_open(
            response_type const & resp,
            boost::system::error_code const & ec)
        {
            LOG_INFO( "[on_open] dispatcher:" << dispatcher_ << " ec:" << ec.message());

            boost::system::error_code ec1 = ec;
            std::string option = url_.path();

            if (!ec1) {
                if (option == "/play") {
                    if (url_.param("chunked") == "true") {
                        response_head()["Transfer-Encoding"]="{chunked}";
                    }
                    dispatcher_->setup(-1, response_stream(), ec1);
                }
            }

            if (ec1) {
                make_error_response_body(body_,ec1);
                resp(ec1, body_.size());
                return;
            }

            if (option == "mediainfo") {
                make_mediainfo(ec1);
                resp(ec1, body_.size());
            } else if (option == "playinfo") {
                make_playinfo(ec1);
                resp(ec1, body_.size());
            } else {
                
            }
        }

        void HttpSession::on_playend(
            response_type const & resp,
            boost::system::error_code const & ec)
        {
            LOG_INFO( "[on_playend] ec:"<<ec.message());
            resp(ec, 0);
        }

        void HttpSession::make_error_response_body(
            std::string& respone_str, 
            boost::system::error_code const & last_error)
        {

            response_head().err_code = 500;
            response_head().err_msg = "Internal Server Error";
            response_head()["Content-Type"]="{application/xml}";

            respone_str = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<root>\r\n<category>";
            // set error code value
            respone_str += last_error.category().name();
            respone_str += "</category>\r\n<vaule>";
            respone_str += framework::string::format(last_error.value());
            respone_str += "</vaule>\r\n<message>";
            // set error msg
            if (last_error) {
                respone_str += last_error.message();
            } else {
                respone_str += "";
            }
            respone_str += "</message>\r\n</root>";
        }

        void HttpSession::make_mediainfo(
            boost::system::error_code& ec)
        {
            response_head()["Content-Type"]="{application/xml}";
        }

        void HttpSession::make_playinfo(
            boost::system::error_code& ec)
        {
            response_head()["Content-Type"]="{application/xml}";
        }

    } // namespace httpd
} // namespace ppbox
