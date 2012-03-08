// RtspSession.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/Mp4HttpDispatcher.h"
#include "ppbox/httpd/HttpError.h"

#ifndef PPBOX_DISABLE_CERTIFY
#include <ppbox/certify/Certifier.h>
#endif

#include <util/protocol/pptv/Url.h>

#include <framework/string/StringToken.h>

#include <framework/logger/LoggerStreamRecord.h>
using namespace framework::logger;

#include <util/protocol/http/HttpClient.h>
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
            if (ec)
            {
                LOG_S(Logger::kLevelAlarm, "[xsputn] ec:"<<ec.message());
                return 0;
            }
            return _Count;
        }

        Mp4HttpDispatcher::Mp4HttpDispatcher(
            util::daemon::Daemon & daemon)
            : HttpDispatcher(daemon)
            , io_svc_(daemon.io_svc())
            ,timer_(daemon.io_svc())
#ifndef PPBOX_DISABLE_VOD
            ,bigmp4_(NULL)
#endif
#ifndef PPBOX_DISABLE_PEER
            ,bigmp4_(NULL)
#endif
            ,stream_(NULL)
            ,streambuf_(NULL)
            ,seek_(0)
            ,session_id_(0)
            ,list_(NULL)
            ,playing_(false)
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
            LOG_S(Logger::kLevelAlarm, "[open_for_play] session_id:"<<session_id_);
            boost::system::error_code ec;

#if (defined(PPBOX_DISABLE_VOD) || defined(PPBOX_DISABLE_PEER))
            ec =ppbox::httpd::error::httpd_stream_end;
            resp(ec);
#else
            static boost::uint32_t g_session_id = 1;

            session_id = g_session_id++;

            if (playing_)
            {
                    //没有回调
                    if (NULL == list_)
                    {
                        list_ = new OpenList(session_id,play_link,format,resp);
                        bigmp4_->close();
                    }
                    else
                    {
                        boost::system::error_code ec1 = ppbox::httpd::error::httpd_stream_end;
                        list_->resp(ec1);
                        delete list_;
                        list_ = new OpenList(session_id,play_link,format,resp);
                    }
            } 
            else
            {//已回调
                seek_ = 0;
                assert(NULL == list_);
                timer_.cancel();
                session_id_ = session_id;

                if (playlink_ == play_link)
                {
                    resp(ec);
                    return ec;
                } 
                else
                {
                    std::string newurl(play_link);

                    if(NULL != bigmp4_) delete bigmp4_;

                    playlink_ = play_link;
                    bigmp4_ = new ppbox::vod::BigMp4(io_svc_);

                    newurl = newurl.substr(std::string("ppvod://").size());
                    newurl = parse_url(newurl,ec);

                    playing_ = true;

                    bigmp4_->async_open(
                        newurl
                        //,ppbox::vod::BigMp4::FetchMode::small_head
                        ,ppbox::vod::BigMp4::FetchMode::big_head
                        ,stream_tmp_
                        ,boost::bind(&Mp4HttpDispatcher::open_callback
                        ,this
                        ,resp,_1));
                }
            }
#endif
            return ec;
        }

        void Mp4HttpDispatcher::open_callback( 
            ppbox::mux::session_callback_respone const &resp
            ,boost::system::error_code const & ec )
        {
#if (!defined(PPBOX_DISABLE_VOD) && !defined(PPBOX_DISABLE_VOD))
            playing_ = false;

            LOG_S(Logger::kLevelAlarm, "[open_callback] session_id:"<<session_id_
                <<" ec:"<<ec.message());

            assert(NULL != bigmp4_);

            if (ec)
            {
                std::cout<<"async_open failed ec:"<<ec.message()<<std::endl;
                if(NULL != bigmp4_)
                {
                    delete bigmp4_;
                    bigmp4_ = NULL;
                }
            }
            resp(ec);

            if (NULL != list_)
            {
                open_list();
            }
            
#endif
        }

        boost::system::error_code Mp4HttpDispatcher::setup(
            boost::uint32_t session_id, 
            util::protocol::HttpSocket& sock,
            bool bChunked,
            ppbox::mux::session_callback_respone const & resp)
        {
            LOG_S(Logger::kLevelAlarm, "[setup] session_id:"<<session_id);
            boost::system::error_code ec;
#if (!defined(PPBOX_DISABLE_VOD) && !defined(PPBOX_DISABLE_VOD))
            assert(NULL != bigmp4_);

            if(NULL != stream_)
                delete stream_;
            if(NULL != streambuf_)
                delete streambuf_;

            streambuf_ = new MyStream(sock);
            stream_ = new std::ostream(streambuf_);
            resp(ec);
#endif
            return ec;
        }

        boost::system::error_code Mp4HttpDispatcher::play(
            boost::uint32_t session_id, 
            ppbox::mux::session_callback_respone const & resp)
        {
#if (!defined(PPBOX_DISABLE_VOD) && !defined(PPBOX_DISABLE_VOD))
            playing_ = true;

            assert(NULL != bigmp4_);

            LOG_S(Logger::kLevelAlarm, "[play] session_id:"<<session_id);
            bigmp4_->async_tranfer(
                seek_
                ,*stream_
                ,boost::bind(&Mp4HttpDispatcher::tranfer_callback
                ,this
                ,resp,_1));
#endif
            return boost::system::error_code();
        }

        void Mp4HttpDispatcher::open_list()
        {
            std::cout<<"open openlist"<<std::endl;
            std::string playlink = list_->play_link;
            std::string format = list_->format;
            boost::uint32_t session_id = list_->session_id;
            ppbox::mux::session_callback_respone resp;
            resp.swap(list_->resp);

            delete list_;
            list_ = NULL;

            //处理队列
            open_for_play(session_id,playlink,format,resp);
        }

        void Mp4HttpDispatcher::tranfer_callback( 
            ppbox::mux::session_callback_respone const &resp
            ,boost::system::error_code const & ec )
        {
            LOG_S(Logger::kLevelAlarm, "[tranfer_callback] session_id:"<<session_id_
                <<" ec:"<<ec.message());
            playing_ = false;
            resp(ec);

            if (NULL == list_)
            {
                std::cout<<"setimer"<<std::endl;
                //启定时器
                //延时关闭 启定时器
                timer_.expires_from_now(boost::posix_time::seconds(10));
                timer_.async_wait(boost::bind(&Mp4HttpDispatcher::handle_timer, this, _1));
            }
            else
            {
                open_list();
            }

        }

        boost::system::error_code Mp4HttpDispatcher::seek(
            const boost::uint32_t session_id
            ,const boost::uint32_t begin
            ,const boost::uint32_t end
            ,ppbox::mux::session_callback_respone const &resp)
        {
            LOG_S(Logger::kLevelAlarm, "[seek] session_id:"<<session_id);
            boost::system::error_code ec;
            seek_ = begin;
            resp(ec);
            return ec;
        }

        boost::system::error_code Mp4HttpDispatcher::close(
            const boost::uint32_t session_id)
        {
            LOG_S(Logger::kLevelAlarm, "[close] session_id:"<<session_id);
            boost::system::error_code ec;
#if (!defined(PPBOX_DISABLE_VOD) && !defined(PPBOX_DISABLE_VOD))
            if (session_id_ == session_id && bigmp4_)
            {
                bigmp4_->close();
            }
#endif
            return ec;
        }

        void Mp4HttpDispatcher::handle_timer( boost::system::error_code const & ec )
        {
#if (!defined(PPBOX_DISABLE_VOD) && !defined(PPBOX_DISABLE_VOD))

            LOG_S(Logger::kLevelAlarm, "[handle_timer] session_id:"<<session_id_
                <<" ec:"<<ec.message());

            std::cout<<"handle_timer failed" <<" ec:"<<ec.message()<<std::endl;
            if (ec)
            {
                std::cout<<"handle_timer failed,ec:"<<ec.message()<<std::endl;
                return;
            }
            //定时器超时
            delete bigmp4_;
            bigmp4_ = NULL;

            playlink_.clear();
#endif
        }

        boost::system::error_code Mp4HttpDispatcher::get_file_length(boost::uint32_t& len)
        {
            boost::system::error_code ec;
#if (!defined(PPBOX_DISABLE_VOD) && !defined(PPBOX_DISABLE_VOD))
            ec = bigmp4_->get_total_size(len);
#endif
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
