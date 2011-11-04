// RtpDispatcher.h

#ifndef _PPBOX_HTTPD_DISPATCHER_H_
#define _PPBOX_HTTPD_DISPATCHER_H_

#include <ppbox/mux/tool/Dispatcher.h>

namespace util
{
    namespace protocol
    {
        class HttpSocket;
    }
}

namespace ppbox
{
    namespace mux
    {
        struct MuxTag;
    }
    namespace rtspd
    {
        class RtpSink;
    }


    namespace httpd
    {  
        
        class HttpDispatcher 
            : public ppbox::mux::Dispatcher
        {
        public:

            HttpDispatcher(
                util::daemon::Daemon & daemon);

            ~HttpDispatcher();

            boost::system::error_code open_mediainfo(
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
                ppbox::mux::session_callback_respone const & resp);

            void set_host(std::string const & host);
        private:
            void mediainfo_callback(
                std::string& rtp_info,
                ppbox::mux::session_callback_respone const &resp,
                boost::system::error_code ec);



            //void on_open(
            //    std::string& rtp_info,
            //    ppbox::rtspd::session_callback_respone const &resp,
            //    boost::system::error_code ec);

        };

    } // namespace rtspd
} // namespace ppbox

#endif // _PPBOX_RTSP_RTP_DISPATCHER_H_