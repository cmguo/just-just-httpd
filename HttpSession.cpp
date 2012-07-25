//HttpServer.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpSession.h"
#include "ppbox/httpd/Version.h"
#include "ppbox/httpd/HttpError.h"

#include "ppbox/httpd/HttpSink.h"
#include "ppbox/httpd/HttpDispatcher.h"
#include "ppbox/httpd/HttpManager.h"
#include "ppbox/httpd/Mp4HttpDispatcher.h"
using namespace ppbox::httpd::error;
using namespace ppbox::httpd;

#include <ppbox/mux/MuxerBase.h>

#include <util/protocol/http/HttpSocket.h>
using namespace util::protocol;

#include <framework/string/Url.h>
#include <framework/string/Base64.h>
#include <framework/logger/LoggerStreamRecord.h>
using namespace framework::network;
using namespace framework::string;
using namespace framework::logger;

#include <tinyxml/tinyxml.h>

#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
using namespace boost::system;
using namespace boost::asio;
using namespace boost::system;

#include <vector>

FRAMEWORK_LOGGER_DECLARE_MODULE("HttpSession");

namespace ppbox
{
    namespace httpd
    {


        std::string HttpSession::g_format_;
        bool HttpSession::g_record_ = false;

        HttpSession::HttpSession(
            HttpManager & mgr)
            : HttpProxy(mgr.io_svc())
            ,io_svc_(mgr.io_svc())
            ,len_(0)
			,session_id_(0)
            ,seek_(0)
            ,need_seek_(false)
        {
            dispatcher_ = mgr.dispatcher();
        }

        HttpSession::~HttpSession()
        {
            HttpSession::Close();
        }

        bool HttpSession::on_receive_request_head(HttpRequestHead & request_head)
        {
            return false;
        }

        void HttpSession::local_process(response_type const & resp)
        {
            //LOG_S(Logger::kLevelEvent, "[local_process]");

            get_request_head().get_content(std::cout);
            error_code ec;
            std::string tmphost = "http://host";
            framework::string::Url url(tmphost + get_request_head().path);
            std::string url_path(url.path_all());

            std::string option;
            std::string playlink;
            std::string type;
            std::string format;

            std::string url_profix = "base64";

            boost::optional<std::string> ht  = get_request_head().host;
            host_ = ht.get();

            //防止一些播放器不支持 playlink带参数的方式
            if (url_path.compare(1, url_profix.size(), url_profix) == 0) {
                url_path = url_path.substr(url_profix.size()+1, url_path.size() - url_profix.size()+1);
                url_path = Base64::decode(url_path);
                url_path = std::string("/") + url_path;
            }

            //tmphost += url_path;

            tmphost += url_path;
            framework::string::Url request_url(tmphost);
            //request_url.decode();

            playlink = request_url.param("playlink");
            type = request_url.param("type");
            head_ = request_url.param("head");
            if (!type.empty())
            {
                type = type +  ":///";
                playlink = type + playlink;
            }

            playlink = framework::string::Url::decode(playlink);

            if(head_.empty())
            {
                head_ = "1";
            }

            format = request_url.param("format");

            if (request_url.path().size() > 1) {
                option = request_url.path().substr(1);
                std::vector<std::string> parm;
                slice<std::string>(option, std::inserter(parm, parm.end()), ".");
                if (parm.size() == 2) {
                    option = parm[0];
                    if (format.empty())
                        format = parm[1];
                }
            }

            option_ = option;
            format_ = format;

            LOG_S(Logger::kLevelEvent, "[local_process] option :"<<option<<" format:"<<format_<<" this:"<<this);


            if ("mediainfo" == option)
            {//open
                get_response_head()["Content-Type"]="{application/xml}";
                dispatcher_->open_mediainfo(session_id_,playlink,request_url,format,body_,
                    boost::bind(&HttpSession::on_common,this,resp,_1));
            }
            else if ("playinfo" == option)
            {//open
                get_response_head()["Content-Type"]="{application/xml}";
                dispatcher_->open_playinfo(session_id_,playlink,request_url,format,body_,
                    boost::bind(&HttpSession::on_common,this,resp,_1));
            }
            else if ("seek" == option)
            {
                boost::uint32_t seek = 0;
                if (!request_url.param("time").empty()) {
                    seek = atoi(request_url.param("time").c_str());
                    seek *= 1000;
                }
                dispatcher_->seek(session_id_,seek
                    ,(head_ == "0")?boost::uint32_t(-1):0
                    ,boost::bind(&HttpSession::on_common,this,resp,_1));
            }
            else if ("pause" == option)
            {
                dispatcher_->pause(session_id_,
                    boost::bind(&HttpSession::on_common,this,resp,_1));
            }
            else if ("resume" == option)
            {
                dispatcher_->resume(session_id_,
                    boost::bind(&HttpSession::on_common,this,resp,_1));
            }
            else if ("info" == option)
            {

            }
            else if ("close" == option)
            {
                std::cout<<"Http Close"<<std::endl;
                dispatcher_->kill();
                resp(ec,Size());
            }
            else if("play" == option || "record" == option) 
            {

                if (!request_url.param("seek").empty()) 
                {
                    seek_ = atoi(request_url.param("seek").c_str());
                    seek_ *= 1000;
                    need_seek_ = true;
                }
                //Range Here
                g_format_ = format;
                g_record_ = (option == "record")?true:false;

                if(g_format_ == "mp4")
                {
                    static HttpDispatcher* g_mp4Dispather = NULL;
                    if(NULL == g_mp4Dispather) g_mp4Dispather =  new Mp4HttpDispatcher(global_daemon());

                    if(request().head().range.is_initialized())
                    {

                        seek_ = get_request_head().range.get()[0].begin();
                        need_seek_ = true;
                    }
                    dispatcher_ = g_mp4Dispather;
                }
                dispatcher_->open_for_play(session_id_,playlink,request_url,format,
                    boost::bind(&HttpSession::on_open,this,resp,_1));
            }
            else  
            {
                //open setup seek play
                if (g_format_ == "m3u8" && "ts" == format)
                {
                    option_ = g_record_?"record":"play";

                    seek_ = atoi(option.c_str());
                    need_seek_ = true;
                    dispatcher_->open_for_play(session_id_,"",request_url,"",
                        boost::bind(&HttpSession::on_open,this,resp,_1));

                }
                else
                {
                    ec_ = httpd_request_error;
                    make_error_response_body(body_,ec_);
                    resp(ec_,body_.size()); 
                }
            }
        }

        void HttpSession::transfer_response_data(response_type const & resp)
        {
            get_response_head().get_content(std::cout);
            //mediainfo  m3u8  xml
            //play seek  Play  
            std::string response_str;
            LOG_S(Logger::kLevelEvent, "[transfer_response_data] ");

            
            if (!body_.empty() || (option_ != "play" && option_ != "record"))
            {
                // std::cout<<body_<<std::endl;
                size_t tSize = body_.size();
                ec_ = write(body_);
                LOG_S(Logger::kLevelEvent, "[transfer_response_data] "<<option_);
                body_.clear();
                resp(ec_,tSize);
            }
            else
            {
                //open most
                if (!need_seek_ )
                { //直接Play
                    io_svc_.post(boost::bind(&HttpSession::on_seekend,this,resp,ec_));
                }
                else
                {//Seek
                    dispatcher_->seek(session_id_,seek_
                        ,(head_ == "0")?boost::uint32_t(-1):0
                        ,io_svc_.wrap(boost::bind(&HttpSession::on_seekend,this,resp,_1)));
                    seek_ = 0;
                    need_seek_ = false;
                }
                return;
            }
           

        }

        void HttpSession::on_finish()
        {
            LOG_S(Logger::kLevelEvent, "[on_finish] sessiin_id:"<<session_id_);
            HttpSession::Close();
        }
        void HttpSession::on_error(
            boost::system::error_code const & ec)
        {
            LOG_S(Logger::kLevelDebug, "[on_error] sessiin_id:"<<session_id_<<" ec:" << ec.message());
            HttpSession::Close();
        }

        //Dispatch 线程
        void HttpSession::on_open(response_type const &resp,
            boost::system::error_code const & ec)
        {
            boost::system::error_code ec1 = ec;
            if(!ec1)
            {
                if (format_ == "m3u8") 
                {
                    get_response_head()["Content-Type"]="{application/x-mpegURL}";
                    get_response_head()["Connection"] = "Close";
                    dispatcher_->set_host(host_);
                    ppbox::mux::MediaFileInfo infoTemp; //= cur_mov_->muxer->mediainfo();
                    ec1 = dispatcher_->get_info(infoTemp);
                    if(!ec1 && NULL != infoTemp.attachment)
                    {
                        body_ = *(std::string*)infoTemp.attachment;
                    }
                } else 
                {
                    //确定用什么sink Connection: keep-alive
#ifdef PPBOX_HTTP_CHUNKED
                    get_response_head()["Transfer-Encoding"]="{chunked}";
                    get_response_head()["Connection"] = "Keep-Alive";
                    dispatcher_->setup(session_id_,get_client_data_stream(),true,
                        io_svc_.wrap(boost::bind(&HttpSession::open_setupup,this,resp,_1)));
#else
                    dispatcher_->get_file_length(len_);
                    LOG_S(Logger::kLevelEvent, "[on_open] Len:"<<len_);
                    if (need_seek_)
                    {
                        if(seek_ > 0 && g_format_ == "mp4")
                        {
                            len_ -= seek_;
                            get_response().head().err_code = 206;
                            get_response().head().err_msg = "Partial Content";
                        }
                    }

                    dispatcher_->setup(session_id_,get_client_data_stream(),false,
                        io_svc_.wrap(boost::bind(&HttpSession::open_setupup,this,resp,_1)));
#endif
                    return;
                }
            }
            ec_ = ec1;
            if (ec_)
            {
                make_error_response_body(body_,ec_);
            }
            io_svc_.post(boost::bind(resp, ec_,body_.size()));
        }

        // 主线程 下面
        void HttpSession::open_setupup(response_type const &resp,
            boost::system::error_code const & ec)
        {
            LOG_S(Logger::kLevelEvent, "[open_setupup] ec:"<<ec.message());
            ec_ = ec;
            if (ec)
            {
                make_error_response_body(body_,ec_);
                resp(ec_,body_.size());
            } 
            else 
            {//与播放相关
                //type
                if(format_  == "ts")
                {
                    get_response_head()["Content-Type"]="{video/MP2T}";
                }
                else if(format_ == "flv")
                {
                    get_response_head()["Content-Type"]="{video/x-flv}";
                }
                else if(format_ == "mp4")
                {
                    get_response_head()["Content-Type"]="{video/mp4}";
                }
                else
                {
                    LOG_S(Logger::kLevelError, "[open_setupup] format_:"<<format_);
                }

				if(len_ > 0)
                    resp(ec, (size_t)len_);
				else
					resp(ec,Size());
            }
        }

        void HttpSession::on_common(response_type const &resp,
            boost::system::error_code const & ec)
        {
            LOG_S(Logger::kLevelEvent, "[on_common] ec:"<<ec.message());
            ec_ = ec;
            if (ec)
            {
                make_error_response_body(body_,ec_);
            }
            io_svc_.post(boost::bind(resp, ec_,body_.size()));
        }

        void HttpSession::on_playend(response_type const &resp,
            boost::system::error_code const & ec)
        {
            LOG_S(Logger::kLevelEvent, "[on_playend] ec:"<<ec.message());
            resp(ec,std::pair<std::size_t, std::size_t>(0,0));
        }

        void HttpSession::on_seekend(response_type const &resp,
            boost::system::error_code const & ec)
        {
            LOG_S(Logger::kLevelEvent,"[on_seekend] ec:"<<ec.message());
            if (ec)
            {
                resp(ec,std::pair<std::size_t, std::size_t>(0,0));
            }
            else
            {//play
                if (!g_record_)
                {
                    dispatcher_->play(session_id_,
                        io_svc_.wrap(boost::bind(&HttpSession::on_playend,this,resp,_1)));
                }
                else 
                {
                    dispatcher_->record(session_id_,
                        io_svc_.wrap(boost::bind(&HttpSession::on_playend,this,resp,_1)));
                }
            }
        }

        boost::system::error_code HttpSession::write(std::string const& msg)
        {
            boost::system::error_code ec;
            boost::asio::write(
                get_client_data_stream(), 
                boost::asio::buffer(msg.c_str(), msg.size()), 
                boost::asio::transfer_all(), 
                ec);

            LOG_S(Logger::kLevelDebug, "[write] msg size:"<<msg.size()<<" ec:"<<ec.message());
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
            respone_str += format(last_error.value());
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
            if(session_id_)
            {
                if (g_format_ == "m3u8" && "ts" == format_)
                {
                    LOG_S(Logger::kLevelEvent, "[Close] m3u8/ts not close session_id:"<<session_id_);
                }
                else
                {
                    dispatcher_->close(session_id_);
                }
                session_id_ = 0;
            }
        }

    } // namespace httpd

} // namespace ppbox


