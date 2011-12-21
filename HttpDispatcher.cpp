// RtspSession.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpDispatcher.h"


#include "ppbox/httpd/HttpChunkedSink.h"
#include "ppbox/httpd/HttpSink.h"

#include <ppbox/mux/Muxer.h>

#include <tinyxml/tinyxml.h>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <framework/system/LogicError.h>
using namespace framework::system::logic_error;
using namespace framework::logger;
using namespace boost::system;
using namespace util::protocol;

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("HttpDispatcher", 0)

namespace ppbox
{
    namespace httpd
    {

        HttpDispatcher::HttpDispatcher(
            util::daemon::Daemon & daemon)
            : ppbox::mux::Dispatcher(daemon)
        {

        }

        HttpDispatcher::~HttpDispatcher()
        {

        }

        boost::system::error_code HttpDispatcher::open_mediainfo(
            boost::uint32_t& session_id,
            std::string const & play_link,
            std::string const & format,
            std::string & body,
            ppbox::mux::session_callback_respone const & resp)
        {
            return Dispatcher::open(session_id,play_link,format,false,
                boost::bind(&HttpDispatcher::mediainfo_callback,this,boost::ref(body),resp,_1));
        }

        boost::system::error_code HttpDispatcher::open_playinfo(
            boost::uint32_t& session_id,
            std::string const & play_link,
            std::string const & format,
            std::string & body,
            ppbox::mux::session_callback_respone const & resp)
        {
            return Dispatcher::open(session_id,play_link,format,false,
                boost::bind(&HttpDispatcher::playinfo_callback,this,boost::ref(body),resp,_1));
        }

        boost::system::error_code HttpDispatcher::open_for_play(
            boost::uint32_t& session_id,
            std::string const & play_link,
            std::string const & format,
            ppbox::mux::session_callback_respone const & resp)
        {
            if(format == "m3u8")
			{
				return Dispatcher::open(session_id,play_link,format,false,resp);
			}
			else
			{
				return Dispatcher::open(session_id,play_link,format,true,resp);
			}
		}

        boost::system::error_code HttpDispatcher::setup(
            boost::uint32_t session_id, 
            util::protocol::HttpSocket& sock,
            bool bChunked,
            ppbox::mux::session_callback_respone const & resp)
        {
            ppbox::mux::Sink* sink = NULL;
            
			if (bChunked)
            {
                sink = new HttpChunkedSink(sock);
            }
            else
            {
                sink = new HttpSink(sock);
            }
            return Dispatcher::setup(session_id,sink,resp);
        }

        void HttpDispatcher::playinfo_callback(
            std::string& rtp_info,
            ppbox::mux::session_callback_respone const &resp,
            boost::system::error_code ec)
        {
            if (!ec)
            {
                boost::system::error_code ec1;
                boost::system::error_code ec2;
                boost::uint32_t time = cur_mov_->demuxer->get_buffer_time(ec1,ec2);
                if (ec1 || ec2)
                {
                    resp(ec1?ec1:ec2);
                    return;
                }
                else
                {
                    TiXmlDocument doc;
                    const char* strXMLContent =
                        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                        "<template module=\"playinfo\" version=\"1.0\">"
                        "</template>";

                    doc.Parse( strXMLContent );

                    TiXmlNode* node = 0;
                    TiXmlElement* element = 0;
                    node = doc.FirstChild( "template" );
                    element = node->ToElement();

                    TiXmlElement time_xml("time ");
                    time_xml.SetAttribute("value", format(time).c_str());

                    element->InsertEndChild(time_xml);

                    TiXmlPrinter printer;
                    printer.SetStreamPrinting();
                    doc.Accept( &printer );
                    rtp_info = printer.CStr();
                }
            }
            resp(ec);

        }

        void HttpDispatcher::mediainfo_callback(
            std::string& rtp_info,
            ppbox::mux::session_callback_respone const &resp,
            boost::system::error_code ec)
        {
            if(!ec)
            {
                const ppbox::mux::MediaFileInfo & media_info = cur_mov_->muxer->mediainfo();

                TiXmlDocument doc;
                const char* strXMLContent =
                    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                    "<template module=\"mediainfo\" version=\"1.0\">"
                    "</template>";

                doc.Parse( strXMLContent );
                if ( !doc.Error() )
                {
                    TiXmlNode* node = 0;
                    TiXmlElement* element = 0;
                    node = doc.FirstChild( "template" );
                    element = node->ToElement();

                    TiXmlElement duration("duration");
                    duration.SetAttribute("value", format(media_info.duration).c_str());

                    TiXmlElement video("video");
                    video.SetAttribute("codec", "h264");
                    for (boost::uint32_t i = 0; i < media_info.stream_infos.size(); ++i) {
                        if (media_info.stream_infos[i].type == ppbox::demux::MEDIA_TYPE_VIDE) {
                            ppbox::demux::MediaInfo const & video_stream_info = media_info.stream_infos[i];
                            TiXmlElement videoproperty("property");
                            videoproperty.SetAttribute("frame-rate", format(video_stream_info.video_format.frame_rate).c_str());
                            videoproperty.SetAttribute("width", format(video_stream_info.video_format.width).c_str());
                            videoproperty.SetAttribute("height", format(video_stream_info.video_format.height).c_str());
                            video.InsertEndChild(videoproperty);
                            break;
                        }
                    }

                    TiXmlElement audio("audio");
                    for (boost::uint32_t i = 0; i < media_info.stream_infos.size(); ++i) {
                        if (media_info.stream_infos[i].type == ppbox::demux::MEDIA_TYPE_AUDI) {
                            ppbox::demux::MediaInfo const & audio_stream_info = media_info.stream_infos[i];
                            if (audio_stream_info.sub_type == ppbox::demux::AUDIO_TYPE_MP4A) { 
                                audio.SetAttribute("codec", "aac");
                            } else if (audio_stream_info.sub_type == ppbox::demux::AUDIO_TYPE_MP1A) {
                                audio.SetAttribute("codec", "mpeg_audio");
                            }
                            TiXmlElement audioproperty("property");
                            audioproperty.SetAttribute("channels", format(audio_stream_info.audio_format.channel_count).c_str());
                            audioproperty.SetAttribute("sample-rate", format(audio_stream_info.audio_format.sample_rate).c_str());
                            audioproperty.SetAttribute("sample-size", format(audio_stream_info.audio_format.sample_size).c_str());
                            audio.InsertEndChild(audioproperty);
                            break;
                        }
                    }

                    element->InsertEndChild(duration);
                    element->InsertEndChild(video);
                    element->InsertEndChild(audio);
                }

                TiXmlPrinter printer;
                printer.SetStreamPrinting();
                doc.Accept( &printer );
                rtp_info = printer.CStr();
            }
            else
            {
                rtp_info.clear();// = "";
            }
            resp(ec);
        }

        void HttpDispatcher::set_host(std::string const & host)
        {
            assert(NULL != cur_mov_->muxer);
            cur_mov_->muxer->Config().set("M3U8","full_path", host);
        }

    } // namespace rtspd
} // namespace ppbox