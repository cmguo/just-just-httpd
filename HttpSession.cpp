//HttpServer.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpSession.h"
#include "ppbox/httpd/Version.h"
#include "ppbox/httpd/HttpError.h"
#include "ppbox/httpd/HttpSink.h"
#include "ppbox/httpd/HttpManager.h"
using namespace ppbox::httpd::error;

#include <ppbox/common/CommonUrl.h>
#include <ppbox/mux/MuxBase.h>


#include <ppbox/dispatcher/Dispatcher.h>
//#include <ppbox/merge/MergeDispatcher.h>


#include <util/protocol/http/HttpSocket.h>
using namespace util::protocol;

#include <framework/string/Base64.h>
#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>
using namespace framework::logger;

#include <tinyxml/tinyxml.h>

#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include <vector>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("HttpSession", 0);

namespace ppbox
{
    namespace httpd
    {
        HttpSession::HttpSession(
            HttpManager & mgr)
            : HttpProxy(mgr.io_svc())
            , mgr_(mgr)
            , ios_(mgr.io_svc())
            ,session_id_(0)
            ,seek_(-1)
            , dispatcher_(NULL)
        {
        }

        HttpSession::~HttpSession()
        {
            HttpSession::Close();

            for (std::vector<util::stream::Sink*>::iterator iter = sinks_.begin();
                iter != sinks_.end();
                ++iter)
            {
                delete *iter;
            }
            sinks_.clear();
        }

        bool HttpSession::on_receive_request_head(HttpRequestHead & request_head)
        {
            return false;
        }

        void HttpSession::local_process(response_type const & resp)
        {
            //LOG_INFO("[local_process]");

            get_request_head().get_content(std::cout);
            boost::system::error_code ec;
            std::string tmphost = "http://host";
            framework::string::Url url(tmphost + get_request_head().path);
            ppbox::common::CommonUrl comUrl(url);
            url_.from_url(comUrl);
            
            std::string option = url_.option();
            std::string playlink = url_.playlink();
            std::string format = url_.format();

            LOG_INFO( "[local_process] playlink:"<<playlink<<" option:"<<option<<" format:"<<format<<" this:"<<this);

            dispatcher_ = mgr_.dispatcher();

            //play mediainfo playinfo

            if (format == "m3u8" 
                || ( format == "ts" && playlink.empty()))
            {
                m3u8_protocol(ec);
            }
            else if (option == "close")
            {
                std::cout<<"Http Close"<<std::endl;
                m3u8_protocol(ec);
                dispatcher_->kill();
                resp(ec,Size());
                return;
            }

            if (ec)
            {
                make_error_response_body(body_,ec);
                resp(ec,body_.size());
                return;
            }
            

            dispatcher_->async_open(url_.url(),
                session_id_,
                boost::bind(&HttpSession::on_open,this,resp,_1));
        }

        void HttpSession::m3u8_protocol(boost::system::error_code& ec)
        {
            static std::string g_playlink;
            static boost::uint32_t g_pid = 1;

            //errors::httpd_not_open
            std::string playlink = url_.playlink();
            std::string format = url_.format();

            ec.clear();

            if (format  == "ts" && playlink.empty())
            {//http//127.0.0.1:9006/{g_pid}/1.ts  ---加入format与playlink参数
                if(url_.playid() == framework::string::format(g_pid))
                {
                    url_.url().param("playlink",framework::string::Url::encode(g_playlink));
                    url_.url().param("format","m3u8");
                    seek_ = atoi(url_.option().c_str());
                }
                else
                {
                    ec = error::httpd_not_open;
                }

            } 
            else if (format == "m3u8")
            {
                if (playlink != g_playlink)
                {
                    ++g_pid;
                    g_playlink = playlink;
                }
                boost::optional<std::string> ht  = get_request_head().host;
                host_ = ht.get();
                host_ += "/";
                host_ += framework::string::format(g_pid);
            }
            else if (url_.option() == "close")
            {
                g_playlink.clear();
                host_.clear();
                ++g_pid;
            }
            else
            {
                assert(0);
            }

        }


        void HttpSession::transfer_response_data(response_type const & resp)
        {
            get_response_head().get_content(std::cout);
            //mediainfo  m3u8  xml
            //play seek  Play  
            
            boost::system::error_code ec;
                
            //LOG_INFO( "[transfer_response_data] ");
            if (!body_.empty() )
            {
                std::cout<<body_<<std::endl;
                size_t tSize = body_.size();
                ec = write(body_);
                ////LOG_INFO( "[transfer_response_data] "<<option_);
                body_.clear();
                resp(ec,tSize);
            }
            else
            {
                //open most
                dispatcher_->async_play(session_id_
                    , seek_
                    , -1
                    ,boost::bind(&HttpSession::on_playend,this,resp,_1));
            }
        }

        void HttpSession::on_finish()
        {
            LOG_INFO( "[on_finish] session_id:"<<session_id_);
            HttpSession::Close();
        }
        void HttpSession::on_error(
            boost::system::error_code const & ec)
        {
            LOG_INFO( "[on_error] session_id:"<<session_id_<<" ec:" << ec.message());
            HttpSession::Close();
        }

        //Dispatch 线程
        void HttpSession::on_open(response_type const &resp,
            boost::system::error_code const & ec)
        {
            LOG_INFO( "[on_open] session_id:"<<session_id_<<" ec:" << ec.message());
            boost::system::error_code ec1 = ec;
            boost::uint32_t len = 0;

            if(!ec1)
            {
                if (url_.option() == "mediainfo")
                {
                    make_mediainfo(ec1);

                } 
                else if (url_.option() == "playinfo")
                {
                    make_playinfo(ec1);

                }
                else if (url_.format() == "m3u8") 
                {
                    make_m3u8(ec1);
                } 
                else
                {

                    if(request().head().range.is_initialized())
                    {
                        seek_ = get_request_head().range.get()[0].begin();
                    }

                    if (!url_.url().param("seek").empty())
                    {
                        seek_ = atoi(url_.url().param("seek").c_str());
                        seek_ *= 1000;
                    }
                    

//setup 
                    //确定用什么sink Connection: keep-alive
                    if (url_.url().param("chunked") == "true")
                    {
                        get_response_head()["Transfer-Encoding"]="{chunked}";
                        get_response_head()["Connection"] = "Keep-Alive";
                        /*dispatcher_->setup(session_id_,get_client_data_stream(),true,
                            io_svc_.wrap(boost::bind(&HttpSession::open_setupup,this,resp,_1)));*/
                    }
                    else
                    {
                        get_response_head()["Accept-Ranges"]="{bytes}";
                        ppbox::dispatcher::MediaInfo info;
                        dispatcher_->get_media_info(info);
                        len = info.filesize;
                        //dispatcher_->get_file_length(len);
                        //LOG_INFO( "[on_open] Len:"<<len);
                        if (seek_ != boost::uint32_t(-1))
                        {
                            if(seek_ > 0 && len > seek_)
                            {
                                std::string strformat("bytes ");
                                strformat += framework::string::format(seek_);
                                strformat += "-";
                                strformat += framework::string::format(len-1);
                                strformat += "/";
                                strformat += framework::string::format(len);
                                get_response_head()["Content-Range"]="{"+strformat+"}";

                                len -= seek_;
                                get_response().head().err_code = 206;
                                get_response().head().err_msg = "Partial Content";
                            }
                        }
                        HttpSink * sink = new HttpSink(ios_,get_client_data_stream());
                        sinks_.push_back(sink);
                        dispatcher_->setup(session_id_,-1,sink,ec1);
                    }
                    
                    if(url_.format()  == "ts")
                    {
                        get_response_head()["Content-Type"]="{video/MP2T}";
                    }
                    else if(url_.format() == "flv")
                    {
                        get_response_head()["Content-Type"]="{video/x-flv}";
                    }
                    else if(url_.format() == "mp4")
                    {
                        get_response_head()["Content-Type"]="{video/mp4}";
                    }
                    else
                    {
                        LOG_ERROR( "[open_setupup] format_:"<<url_.format());
                    }


                    if(len > 0)
                        resp(ec, (size_t)len);
                    else
                        resp(ec,Size());
                    return;
                }
            }
            if (ec1)
            {
                make_error_response_body(body_,ec1);
            }
            resp(ec1,body_.size());
        }

        void HttpSession::on_playend(response_type const &resp,
            boost::system::error_code const & ec)
        {
            LOG_INFO( "[on_playend] ec:"<<ec.message());
            resp(ec,std::pair<std::size_t, std::size_t>(0,0));
        }


        boost::system::error_code HttpSession::write(std::string const& msg)
        {
            boost::system::error_code ec;
            boost::asio::write(
                get_client_data_stream(), 
                boost::asio::buffer(msg.c_str(), msg.size()), 
                boost::asio::transfer_all(), 
                ec);

            LOG_DEBUG( "[write] msg size:"<<msg.size()<<" ec:"<<ec.message());
            return ec;
        }

        void HttpSession::make_error_response_body(
            std::string& respone_str, 
            boost::system::error_code const & last_error)
        {

            get_response().head().err_code = 500;
            get_response().head().err_msg = "Internal Server Error";
            get_response_head()["Content-Type"]="{application/xml}";

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

        void HttpSession::Close()
        {
            LOG_INFO( "[Close] session_id:"<<session_id_);
            if(session_id_)
            {
                if (url_.format() == "m3u8" && ("ts" == url_.format() || "m3u8" == url_.format()))
                {
                    //LOG_INFO( "[Close] m3u8/ts not close ");
                }
                else
                {
                    boost::system::error_code ec;
                    dispatcher_->close(session_id_,ec);
                }
                session_id_ = 0;
            }
        }

        void HttpSession::make_mediainfo(boost::system::error_code& ec)
        {
            get_response_head()["Content-Type"]="{application/xml}";
        }

        void HttpSession::make_playinfo(boost::system::error_code& ec)
        {
            get_response_head()["Content-Type"]="{application/xml}";
        }

        void HttpSession::make_m3u8(boost::system::error_code& ec)
        {
            get_response_head()["Content-Type"]="{application/x-mpegURL}";
            get_response_head()["Connection"] = "Close";
            
            //((ppbox::dispatcher::MuxDispatcher*)dispatcher_)->set_host(host_);

            ppbox::dispatcher::MediaInfo infoTemp;
            ec = dispatcher_->get_media_info(infoTemp);

            if(!ec && NULL != infoTemp.attachment)
                body_ = *(std::string*)infoTemp.attachment;
        }

    } // namespace httpd

} // namespace ppbox


