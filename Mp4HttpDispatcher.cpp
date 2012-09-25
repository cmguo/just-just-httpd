// RtspSession.cpp
//

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/Mp4HttpDispatcher.h"
#include "ppbox/httpd/HttpError.h"

#ifndef PPBOX_DISABLE_CERTIFY
#include <ppbox/certify/Certifier.h>
#endif

#include <util/protocol/pptv/Url.h>

#include <framework/string/StringToken.h>
#include <framework/string/Format.h>
#include <framework/logger/StreamRecord.h>
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
                LOG_WARN("[xsputn] ec:"<<ec.message());
                return 0;
            }
            return _Count;
        }

        Mp4HttpDispatcher::Mp4HttpDispatcher(
            util::daemon::Daemon & daemon)
            : HttpDispatcher(daemon)
            , timer_(daemon.io_svc())
#ifndef PPBOX_DISABLE_VOD
            , bigmp4_(NULL)
#endif
#ifndef PPBOX_DISABLE_PEER
            , bigmp4_(NULL)
#endif
            , stream_(NULL)
            , streambuf_(NULL)
            , seek_(0)
            , session_id_(0)
            , list_(NULL)
            , playing_(false)
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
            framework::string::Url const & params,
            std::string const & format,
            ppbox::mux::session_callback_respone const &resp)
        {
            boost::system::error_code ec;
#if (defined(PPBOX_DISABLE_VOD) && defined(PPBOX_DISABLE_PEER))
            ec =ppbox::httpd::error::httpd_stream_end;
            resp(ec);
            return ec;
#endif
            static boost::uint32_t g_session_id = 1;
            session_id = g_session_id++;
            return open(session_id,play_link,params,format,resp);
        }

        boost::system::error_code Mp4HttpDispatcher::open(
            boost::uint32_t & session_id,
            std::string const & play_link,
            framework::string::Url const & params,
            std::string const & format,
            ppbox::mux::session_callback_respone const & resp)
        {
            boost::system::error_code ec;

            LOG_WARN("[open] session_id:"<<session_id);
#if (!defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER))
            if (playing_)
            {
                //没有回调
                if (NULL == list_)
                {
                    list_ = new OpenList(session_id,play_link,params,format,resp);
                    bigmp4_->close();
                }
                else
                {
                    boost::system::error_code ec1 = ppbox::httpd::error::httpd_stream_end;
                    list_->resp(ec1);
                    delete list_;
                    list_ = new OpenList(session_id,play_link,params,format,resp);
                }
            } 
            else
            {//已回调
                seek_ = 0;
                assert(NULL == list_);
                timer_.cancel();
                session_id_ = session_id;

                {
                    if (playlink_ != play_link)
                    {
                        stream_tmp_.str("");
                        if(NULL != bigmp4_) delete bigmp4_;
#ifndef PPBOX_DISABLE_VOD
                        bigmp4_ = new ppbox::vod::BigMp4(get_daemon().io_svc());
#endif
#ifndef PPBOX_DISABLE_PEER
                        bigmp4_ = new ppbox::peer::BigMp4(get_daemon().io_svc());
#endif
                    }
                    stream_tmp_.seekg(0, std::ios_base::beg);
                    playlink_ = play_link;
                    playing_ = true;
                    framework::string::Url url(play_link);
                    std::string mode = "false";
                    mode = url.param("bighead");
                    if (mode =="true")
                    {
                        bigmp4_->async_open(
                            play_link
#ifndef PPBOX_DISABLE_VOD
                            ,ppbox::vod::BigMp4::FetchMode::big_head
#endif
#ifndef PPBOX_DISABLE_PEER
                            ,ppbox::peer::BigMp4::FetchMode::big_head
#endif
                            ,stream_tmp_
                            ,boost::bind(&Mp4HttpDispatcher::open_callback
                            ,this
                            ,resp,_1));
                    }
                    else
                    {
                        bigmp4_->async_open(
                            play_link
#ifndef PPBOX_DISABLE_VOD
                            ,ppbox::vod::BigMp4::FetchMode::small_head
#endif
#ifndef PPBOX_DISABLE_PEER
                            ,ppbox::peer::BigMp4::FetchMode::small_head
#endif
                            ,stream_tmp_
                            ,boost::bind(&Mp4HttpDispatcher::open_callback
                            ,this
                            ,resp,_1));
                    }
                }
            }
#endif
            return ec;
        }

        void Mp4HttpDispatcher::open_callback( 
            ppbox::mux::session_callback_respone const &resp
            ,boost::system::error_code const & ec )
        {
#if (!defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER))
            playing_ = false;

            LOG_WARN("[open_callback] session_id:"<<session_id_
                <<" ec:"<<ec.message());
            assert(NULL != bigmp4_);

            resp(ec);

            if (NULL == list_)
            {
                if(ec)
                {
                    //启定时器
                    //延时关闭 启定时器
                    timer_.expires_from_now(boost::posix_time::seconds(10));
                    timer_.async_wait(boost::bind(&Mp4HttpDispatcher::handle_timer, this, _1));
                }
            }
            else
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
            LOG_WARN("[setup] session_id:"<<session_id);
            boost::system::error_code ec;
#if (!defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER))
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
#if (!defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER))
            playing_ = true;

            assert(NULL != bigmp4_);

            LOG_WARN("[play] session_id:"<<session_id);
            bigmp4_->async_tranfer(
                seek_
                ,*stream_
                ,boost::bind(&Mp4HttpDispatcher::tranfer_callback
                ,this
                ,resp,_1));
#endif
            return boost::system::error_code();
        }

        boost::system::error_code Mp4HttpDispatcher::record(
            boost::uint32_t session_id, 
            ppbox::mux::session_callback_respone const & resp)
        {
#if (!defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER))
            playing_ = true;

            assert(NULL != bigmp4_);

            LOG_WARN("[record] session_id:"<<session_id);
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
            std::string playlink = list_->play_link;
            framework::string::Url params = list_->params;
            std::string format = list_->format;
            //open_for_play session_id will ++
            boost::uint32_t session_id = list_->session_id;
            ppbox::mux::session_callback_respone resp;
            resp.swap(list_->resp);

            delete list_;
            list_ = NULL;

            //处理队列
            open(session_id,playlink,params,format,resp);
        }

        void Mp4HttpDispatcher::tranfer_callback( 
            ppbox::mux::session_callback_respone const &resp
            ,boost::system::error_code const & ec )
        {
            LOG_WARN("[tranfer_callback] session_id:"<<session_id_
                <<" ec:"<<ec.message());
            playing_ = false;
            resp(ec);

            if (NULL == list_)
            {
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
            LOG_WARN("[seek] session_id:"<<session_id);
            boost::system::error_code ec;
            seek_ = begin;
            resp(ec);
            return ec;
        }

        boost::system::error_code Mp4HttpDispatcher::close(
            const boost::uint32_t session_id)
        {
            LOG_WARN("[close] session_id:"<<session_id);
            boost::system::error_code ec;
#if (!defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER))
            if (session_id_ == session_id && bigmp4_)
            {
                bigmp4_->close();
            }
            else
            {
                if(list_ != NULL && session_id == list_->session_id)
                {
                    boost::system::error_code ec1 = ppbox::httpd::error::httpd_stream_end;
                    list_->resp(ec1);
                    delete list_;
                    list_ = NULL;
                }
            }
#endif
            return ec;
        }

        void Mp4HttpDispatcher::handle_timer( boost::system::error_code const & ec )
        {
#if (!defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER))

            LOG_WARN("[handle_timer] session_id:"<<session_id_
                <<" ec:"<<ec.message());

            if (ec)
            {
                return;
            }
            //定时器超时
            bigmp4_->close();
            delete bigmp4_;
            bigmp4_ = NULL;

            playlink_.clear();
            stream_tmp_.str("");
#endif
        }

        boost::system::error_code Mp4HttpDispatcher::get_file_length(boost::uint32_t& len)
        {
            boost::system::error_code ec;
#if (!defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER))
            ec = bigmp4_->get_total_size(len);
#endif
            return ec;
        }

        boost::system::error_code Mp4HttpDispatcher::open_mediainfo(
            boost::uint32_t& session_id,
            std::string const & play_link,
            framework::string::Url const & params,
            std::string const & format,
            std::string & body,
            ppbox::mux::session_callback_respone const & resp)
        {
            boost::system::error_code ec;
#if (defined(PPBOX_DISABLE_VOD) && defined(PPBOX_DISABLE_PEER))
            ec =ppbox::httpd::error::httpd_stream_end;
            resp(ec);
            return ec;
#endif
            body = "<root>hello world</root>";
            return open_for_play(session_id,play_link,params,format,resp);
        }


        boost::system::error_code Mp4HttpDispatcher::open_playinfo(
            boost::uint32_t& session_id,
            std::string const & play_link,
            framework::string::Url const & params,
            std::string const & format,
            std::string & body,
            ppbox::mux::session_callback_respone const &resp)
        {
            boost::system::error_code ec;
#if (!defined(PPBOX_DISABLE_VOD) || !defined(PPBOX_DISABLE_PEER))
#ifndef PPBOX_DISABLE_VOD
            ppbox::vod::BigMp4Info info;
#else
            ppbox::peer::BigMp4Info info;
#endif
            if (bigmp4_) 
            {
                ec = bigmp4_->get_info_statictis(info);
            }
            else
            {
                ec = util::protocol::http_error::not_found;
            }
            if(!ec)
            {
                TiXmlDocument doc;
                const char* strXMLContent =
                    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                    "<template module=\"playinfo\" version=\"1.0\">"
                    "</template>";

                doc.Parse( strXMLContent );
                if ( !doc.Error() )
                {
                    TiXmlNode* node = 0;
                    TiXmlElement* element = 0;
                    node = doc.FirstChild( "template");
                    element = node->ToElement();

                    TiXmlElement file("file");
                    TiXmlElement headsize("headsize");
                    TiXmlText file_headsize(framework::string::format(info.head_size).c_str());
                    headsize.InsertEndChild(file_headsize);
                    TiXmlElement bodysize("bodysize");
                    TiXmlText file_bodysize(framework::string::format(info.body_size).c_str());
                    bodysize.InsertEndChild(file_bodysize);
                    TiXmlElement tailsize("tailsize");
                    TiXmlText file_tailsize(framework::string::format(info.tail_size).c_str());
                    tailsize.InsertEndChild(file_tailsize);
                    file.InsertEndChild(headsize);
                    file.InsertEndChild(bodysize);
                    file.InsertEndChild(tailsize);

                    TiXmlElement download("download");
                    TiXmlElement offset("offset");
                    TiXmlText d_offset(framework::string::format(info.cur_offset).c_str());
                    offset.InsertEndChild(d_offset);
                    TiXmlElement speed("speed");
                    TiXmlText d_speed(framework::string::format(info.speed).c_str());
                    speed.InsertEndChild(d_speed);
                    TiXmlElement area("area");
                    TiXmlText d_area(framework::string::format(info.area).c_str());
                    area.InsertEndChild(d_area);
                    TiXmlElement percent("percent");
                    TiXmlText d_percent(framework::string::format(info.percent).c_str());
                    percent.InsertEndChild(d_percent);
                    TiXmlElement headpercent("headpercent");
                    TiXmlText head_percent(framework::string::format(info.headpercent).c_str());
                    headpercent.InsertEndChild(head_percent);

                    download.InsertEndChild(offset);
                    download.InsertEndChild(speed);
                    download.InsertEndChild(area);
                    download.InsertEndChild(percent);
                    download.InsertEndChild(headpercent);

                    element->InsertEndChild(file);
                    element->InsertEndChild(download);
                }

                TiXmlPrinter printer;
                printer.SetStreamPrinting();
                doc.Accept( &printer );
                body = printer.CStr();
            }
            
#else
            ec =ppbox::httpd::error::httpd_stream_end;
#endif
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
                LOG_ERROR("[parse_url] ec:"<<ec.message());
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
