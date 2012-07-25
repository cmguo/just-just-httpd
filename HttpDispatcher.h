// HttpDispatcher.h

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

            virtual boost::system::error_code open_mediainfo(
                boost::uint32_t& session_id,
                std::string const & play_link,
                framework::string::Url const & params,
                std::string const & format,
                std::string & body,
                ppbox::mux::session_callback_respone const &);

            virtual boost::system::error_code open_playinfo(
                boost::uint32_t& session_id,
                std::string const & play_link,
                framework::string::Url const & params,
                std::string const & format,
                std::string & body,
                ppbox::mux::session_callback_respone const &);

            virtual boost::system::error_code open_for_play(
                boost::uint32_t& session_id,
                std::string const & play_link,
                framework::string::Url const & params,
                std::string const & format,
                ppbox::mux::session_callback_respone const &);

            virtual boost::system::error_code setup(
                boost::uint32_t session_id, 
                util::protocol::HttpSocket& sock,
                bool bChunked,
                ppbox::mux::session_callback_respone const & resp);

            virtual boost::system::error_code play(
                boost::uint32_t session_id, 
                ppbox::mux::session_callback_respone const & resp);

            virtual boost::system::error_code record(
                boost::uint32_t session_id, 
                ppbox::mux::session_callback_respone const & resp);

            virtual boost::system::error_code seek(
                const boost::uint32_t session_id,
                const boost::uint32_t begin,
                const boost::uint32_t end,
                ppbox::mux::session_callback_respone const &);

            virtual boost::system::error_code close(
                const boost::uint32_t session_id);

            void set_host(std::string const & host);

            virtual boost::system::error_code get_file_length(boost::uint32_t& len);

        private:
            void mediainfo_callback(
                std::string& rtp_info,
                ppbox::mux::session_callback_respone const &resp,
                boost::system::error_code ec);

            void playinfo_callback(
                std::string& rtp_info,
                ppbox::mux::session_callback_respone const &resp,
                boost::system::error_code ec);



            //void on_open(
            //    std::string& rtp_info,
            //    ppbox::rtspd::session_callback_respone const &resp,
            //    boost::system::error_code ec);

        };

    } // namespace httpd
} // namespace ppbox

#endif // _PPBOX_HTTPD_HTTP_DISPATCHER_H_
