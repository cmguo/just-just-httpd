// RtpDispatcher.h

#ifndef _PPBOX_MP4_HTTPD_DISPATCHER_H_
#define _PPBOX_MP4_HTTPD_DISPATCHER_H_

#include "ppbox/httpd/HttpDispatcher.h"
#include <ppbox/vod/BigMp4.h>

namespace ppbox
{

    namespace httpd
    {  

        class MyStream 
            : public std::basic_streambuf<char>
        {
        public:
            MyStream(util::protocol::HttpSocket& sock);
        protected:
            virtual int xsputn(
                const char *_Ptr,
                int _Count
                );
        private:
            util::protocol::HttpSocket& sock_;
        };
        
        class Mp4HttpDispatcher 
            : public HttpDispatcher
        {
        public:

            Mp4HttpDispatcher(
                util::daemon::Daemon & daemon);

            ~Mp4HttpDispatcher();

            boost::system::error_code open_mediainfo(
                boost::uint32_t& session_id,
                std::string const & play_link,
                std::string const & format,
                std::string & body,
                ppbox::mux::session_callback_respone const &);

            boost::system::error_code open_playinfo(
                boost::uint32_t& session_id,
                std::string const & play_link,
                std::string const & format,
                std::string & body,
                ppbox::mux::session_callback_respone const &);

            boost::system::error_code open_for_play(
                boost::uint32_t& session_id,
                std::string const & play_link,
                std::string const & format,
                ppbox::mux::session_callback_respone const &);

            boost::system::error_code setup(
                boost::uint32_t session_id, 
                util::protocol::HttpSocket& sock,
                bool bChunked,
                ppbox::mux::session_callback_respone const & resp);

            boost::system::error_code play(
                boost::uint32_t size_beg, 
                ppbox::mux::session_callback_respone const & resp);

            boost::system::error_code get_file_length(boost::uint32_t& len);

            std::string parse_url(std::string const &url,boost::system::error_code& ec);

        private:
            ppbox::vod::BigMp4 bigmp4_;
            std::stringstream stream_tmp_;
            std::ostream *stream_;
            MyStream *streambuf_;
        };

    } // namespace rtspd
} // namespace ppbox

#endif // _PPBOX_RTSP_RTP_DISPATCHER_H_