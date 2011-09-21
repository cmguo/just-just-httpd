//HttpServer.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpServer.h"
#include "ppbox/httpd/PlayManager.h"
#include "ppbox/httpd/Version.h"

using namespace ppbox::httpd::error;
using namespace ppbox::httpd;

#include <ppbox/ppbox/IAdapter.h>
#include <ppbox/ppbox/IDemuxer.h>

#include <framework/string/Url.h>
#include <boost/lexical_cast.hpp>

#include <util/protocol/http/HttpSocket.h>
#include <util/protocol/http/HttpHead.h>
using namespace util::protocol;
using namespace boost::system;
using namespace framework::network;

#include <framework/configure/Config.h>
#include <framework/logger/LoggerStreamRecord.h>
#include <framework/string/Format.h>
#include <framework/thread/MessageQueue.h>
#include <framework/string/Base64.h>
using namespace framework::string;
using namespace framework::string;
using namespace framework::logger;

#include <boost/bind.hpp>
using namespace boost::system;
using namespace boost::asio;

#include <tinyxml/tinyxml.h>

FRAMEWORK_LOGGER_DECLARE_MODULE("Httpd");

namespace ppbox
{
    namespace httpd
    {

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

        MsgInfo * HttpServer::msg_info = new MsgInfo;

        HttpServer::HttpServer(boost::asio::io_service & io_svc)
                             : HttpProxy(io_svc)
        {
        }

        HttpServer::~HttpServer()
        {
        }

        bool HttpServer::on_receive_request_head(HttpRequestHead & request_head)
        {
            return false;
        }

        void HttpServer::local_process(local_process_response_type const & resp)
        {
            error_code ec;
            std::string tmphost = "http://host";
            std::string url_profix = "base64";
            std::string url_path = get_request_head().path;
            if (url_path.compare(1, url_profix.size(), url_profix) == 0) {
                url_path = url_path.substr(url_profix.size()+1, url_path.size() - url_profix.size()+1);
                url_path = Base64::decode(url_path);
                url_path = std::string("/") + url_path;
            }
            tmphost += url_path;
            framework::string::Url request_url(tmphost);

            std::string option;
            std::string format;
            if (request_url.path().size() > 1) {
                option = request_url.path().substr(1);
                std::vector<std::string> parm;
                slice<std::string>(option, std::inserter(parm, parm.end()), ".");
                if (parm.size() == 2) {
                    option = parm[0];
                    format = parm[1];
                }
            }

            if ("start" == option) {
                msg_info->option = START;
                msg_info->gid = request_url.param("gid");
                msg_info->pid  = request_url.param("pid");
                msg_info->auth = request_url.param("auth");
                msg_info->write_socket = &get_client_data_stream();
            } else if ("play" == option) {
                msg_info->option = PLAY;
                msg_info->url = request_url.param("playlink");
                msg_info->format = request_url.param("format");
                msg_info->type   = request_url.param("type");
                msg_info->seek_time = 0;
                if (!request_url.param("seek").empty()) {
                    msg_info->seek_time = atoi(request_url.param("seek").c_str());
                }
                msg_info->write_socket = &get_client_data_stream();
            } else if ("mediainfo" == option) {
                msg_info->option = MEDIAINFO;
                msg_info->url = request_url.param("playlink");
                msg_info->format = request_url.param("format");
                msg_info->type   = request_url.param("type");
                msg_info->write_socket = &get_client_data_stream();
            } else if ("seek" == option) {
                msg_info->option = SEEK;
                msg_info->url = request_url.param("playlink");
                msg_info->seek_time = 0;
                if (!request_url.param("time").empty()) {
                    msg_info->seek_time = atoi(request_url.param("time").c_str());
                }
                msg_info->write_socket = &get_client_data_stream();
            } else if ("pause" == option) {
                msg_info->option = PAUSE;
                msg_info->write_socket = &get_client_data_stream();
            } else if ("resume" == option) {
                msg_info->option = RESUME;
                msg_info->write_socket = &get_client_data_stream();
            } else if ("info" == option) {
                msg_info->option = INFO;
                msg_info->url = request_url.param("playlink");
                msg_info->write_socket = &get_client_data_stream();
            } else if ("error" == option) {
                msg_info->option = ERR;
                msg_info->url = request_url.param("playlink");
                msg_info->write_socket = &get_client_data_stream();
            } else {
                msg_info->option = NODEFINE;
                msg_info->url    = option;
                msg_info->write_socket = &get_client_data_stream();
            }
            if (msg_info->format.empty() && !format.empty()) {
                msg_info->format = format;
            }
            pretreat_msg(ec);
            response().head().version = 0x101;
            if (!ec && option == "play") {
                response().head()["Content-Type"] = std::string("{") + content_type(format) + "}";
                //response().head()["Transfer-Encoding"] = "{chunked}";
            } else {
                response().head()["Content-Type"] = "{text/xml}";
            }
            response().head().connection = http_field::Connection::keep_alive;
            resp(ec);
        }

        void HttpServer::transfer_response_data(transfer_response_type const & resp)
        {
            error_code ec;
            if (ec) {
                error_code lec;
                std::pair<std::size_t, std::size_t> size_pair;
                size_pair.first = 0;
                size_pair.second = 0;
                PlayManager::write_error_respone(
                    msg_info->write_socket,
                    ec.value(),
                    ec.message(),
                    "error",
                    lec);
                resp(lec, size_pair);
            } else {
                msg_info->resp = resp;
                PlayManager::s_msg_queue.push(*msg_info);
            }
            return;
        }

        void HttpServer::on_error(
            boost::system::error_code const & ec)
        {
            LOG_S(Logger::kLevelDebug, "[on_error] ec: " << ec.message());
        }

        bool HttpServer::is_right_format(std::string const & format)
        {
            bool res = false;
            if (strncasecmp(format, "flv") || strncasecmp(format, "ts")) {
                res = true;
            }
            return res;
        }

        bool HttpServer::is_right_type(std::string const & type)
        {
            bool res = false;
            if (strncasecmp(type, "ppvod") || strncasecmp(type, "pplive")
                || strncasecmp(type, "ppfile-asf") || strncasecmp(type, "ppfile-mp4")
                || strncasecmp(type, "ppfile-flv")) {
                res = true;
            }
            return res;
        }

        error_code HttpServer::pretreat_msg(error_code & ec)
        {
            ec.clear();
            if (msg_info->option == NODEFINE) {
                ec = httpd_option_not_support;
            } else if (msg_info->option == PLAY  || msg_info->option == MEDIAINFO) {
                if (msg_info->url.empty()) {
                    ec = httpd_request_error;
                } else {
                    if (msg_info->format.empty()) {
                        msg_info->format = "flv";
                    }
                    if (msg_info->type.empty()) {
                        msg_info->type = "ppvod";
                    }
                    if (!is_right_type(msg_info->type)
                        || !is_right_format(msg_info->format)) {
                            ec = httpd_request_error;
                    }
                }
            }
            return ec;
        }

        HttpMediaServer::HttpMediaServer(boost::asio::io_service & io_srv
                                       , NetName const & addr)
                                       :io_srv_(io_srv)
                                       , addr_(addr)
                                       , mgr_(new HttpProxyManager<HttpServer>(io_srv_, addr_))
        {
        }

        HttpMediaServer::~HttpMediaServer()
        {
            if (mgr_) {
                delete mgr_;
                mgr_ = NULL;
            }
        }

        error_code HttpMediaServer::start()
        {
            error_code ec;
            mgr_->start();
            return ec;
        }

        error_code HttpMediaServer::stop()
        {
            error_code ec;
            return ec;
        }

    } // namespace httpd

} // namespace ppbox

int main(int argc, char *argv[])
{
    framework::configure::Config conf("ppbox_httpd.conf");
    global_logger().load_config(conf);

    ppbox::common::log_versions();

    PP_uint32 nBufferTime = 0;
    PP_uint32 nBufferSize = 0;
    PP_uint32 nDownloadSpeed = 0;

    if (nBufferTime > 0) {
        PPBOX_SetPlayBufferTime(nBufferTime * 1000);
    } else {
        PPBOX_SetPlayBufferTime(4 * 1000);
    }

    if (nBufferSize > 0) {
        PPBOX_SetDownloadBufferSize(nBufferSize * 1024 * 1024);
    } else {
        PPBOX_SetDownloadBufferSize(4 * 1024 * 1024);
    }

    if (nDownloadSpeed > 0) {
        PPBOX_SetDownloadMaxSpeed(nDownloadSpeed);
    } else {
        PPBOX_SetDownloadMaxSpeed(100);
    }

    boost::asio::io_service io_serv;
    PlayManager play_manager(io_serv);
    play_manager.start();

    NetName addr("0.0.0.0:9006");
    ppbox::httpd::HttpMediaServer server(io_serv, addr);
    server.start();
    io_serv.run();

    PPBOX_StopP2PEngine();
}


