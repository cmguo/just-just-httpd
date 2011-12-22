// RtspSession.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/Mp4HttpDispatcher.h"

#ifndef PPBOX_DISABLE_CERTIFY
#include <ppbox/certify/Certifier.h>
#endif

#include <util/protocol/pptv/Url.h>

#include <framework/string/StringToken.h>

#include <framework/system/LogicError.h>
using namespace framework::system::logic_error;
using namespace framework::logger;

#include <boost/bind.hpp>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("Mp4HttpDispatcher", 0)

namespace ppbox
{
    namespace httpd
    {

        MyStream::MyStream(util::protocol::HttpSocket& sock)
            :sock_(sock)
        {

        }
        int MyStream::xsputn(
            const char *_Ptr,
            int _Count)
        {
            boost::system::error_code ec;
            boost::asio::const_buffer bufV(_Ptr,_Count);
            std::deque<boost::asio::const_buffer> vec;
            vec.push_back(bufV);
            boost::asio::write(
                sock_, 
                vec, 
                boost::asio::transfer_all(), 
                ec);
            return _Count;
        }

        Mp4HttpDispatcher::Mp4HttpDispatcher(
            util::daemon::Daemon & daemon)
            : HttpDispatcher(daemon)
            ,bigmp4_(daemon.io_svc())
            ,stream_(NULL)
            ,streambuf_(NULL)
        {

        }

        Mp4HttpDispatcher::~Mp4HttpDispatcher()
        {
            if(NULL != stream_)
                delete stream_;
            if(NULL != streambuf_)
                delete streambuf_;
        }

        boost::system::error_code Mp4HttpDispatcher::open_for_play(
            boost::uint32_t& session_id,
            std::string const & play_link,
            std::string const & format,
            ppbox::mux::session_callback_respone const & resp)
        {
            boost::system::error_code ec;
            std::string newurl(play_link);

            newurl = newurl.substr(std::string("ppvod://").size());

            newurl = parse_url(newurl,ec);
            bigmp4_.async_open(
                newurl
                //,ppbox::vod::BigMp4::FetchMode::small_head
                ,ppbox::vod::BigMp4::FetchMode::big_head
                ,stream_tmp_
                ,resp);
            return boost::system::error_code();
        }

        boost::system::error_code Mp4HttpDispatcher::setup(
            boost::uint32_t session_id, 
            util::protocol::HttpSocket& sock,
            bool bChunked,
            ppbox::mux::session_callback_respone const & resp)
        {
            boost::system::error_code ec;

            streambuf_ = new MyStream(sock);
            stream_ = new std::ostream(streambuf_);
            resp(ec);
            return ec;
        }

        boost::system::error_code Mp4HttpDispatcher::play(
            boost::uint32_t size_beg, 
            ppbox::mux::session_callback_respone const & resp)
        {
            bigmp4_.async_tranfer(
                size_beg
                ,*stream_
                ,resp);
            return boost::system::error_code();
        }

        boost::system::error_code Mp4HttpDispatcher::get_file_length(boost::uint32_t& len)
        {
            boost::system::error_code ec;
            ec = bigmp4_.get_total_size(len);
            return ec;
        }

        boost::system::error_code Mp4HttpDispatcher::open_mediainfo(
            boost::uint32_t& session_id,
            std::string const & play_link,
            std::string const & format,
            std::string & body,
            ppbox::mux::session_callback_respone const & resp)
        {
            boost::system::error_code ec;
            assert(0);
            resp(ec);
            return boost::system::error_code();
        }

        boost::system::error_code Mp4HttpDispatcher::open_playinfo(
            boost::uint32_t& session_id,
            std::string const & play_link,
            std::string const & format,
            std::string & body,
            ppbox::mux::session_callback_respone const & resp)
        {
            boost::system::error_code ec;
            assert(0);
            resp(ec);
            return boost::system::error_code();
        }

        std::string Mp4HttpDispatcher::parse_url(std::string const &url,boost::system::error_code& ec)
        {
            std::string  newUrl;
            std::string key;
#ifdef PPBOX_DISABLE_CERTIFY
            key = "kioe257ds";
#else
            //ppbox::certify::Certifier& cert = util::daemon::use_module<ppbox::certify::Certifier>(global_daemon());
            //cert.certify_url(ppbox::certify::CertifyType::vod,"",key,ec);
            if (ec)
            {
                LOG_S(framework::logger::Logger::kLevelError,"[parse_url] ec:"<<ec.message());
                return newUrl;
            }
#endif
            newUrl = util::protocol::pptv::url_decode(url, key);

            framework::string::StringToken st(newUrl, "||");
            if (!st.next_token(ec)) {
                newUrl = st.remain();
            }

            return newUrl;
        }
    } // namespace rtspd
} // namespace ppbox
