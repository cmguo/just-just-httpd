// Mp4HttpDispatcher.h

#ifndef _PPBOX_MP4_HTTPD_DISPATCHER_H_
#define _PPBOX_MP4_HTTPD_DISPATCHER_H_

#include "ppbox/httpd/HttpDispatcher.h"

#include <ppbox/segment/BigMp4.h>

#include <boost/asio/deadline_timer.hpp>
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
                framework::string::Url const & params,
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
                framework::string::Url const & params,
                std::string const & format,
                ppbox::mux::session_callback_respone const &);

            boost::system::error_code setup(
                boost::uint32_t session_id, 
                util::protocol::HttpSocket& sock,
                bool bChunked,
                ppbox::mux::session_callback_respone const & resp);

            virtual boost::system::error_code play(
                boost::uint32_t session_id, 
                ppbox::mux::session_callback_respone const & resp);

            boost::system::error_code record(
                boost::uint32_t session_id, 
                ppbox::mux::session_callback_respone const & resp);

            virtual boost::system::error_code seek(
                const boost::uint32_t session_id
                ,const boost::uint32_t begin
                ,const boost::uint32_t end
                ,ppbox::mux::session_callback_respone const &);

            virtual boost::system::error_code close(
                const boost::uint32_t session_id);

            boost::system::error_code get_file_length(boost::uint32_t& len);

            std::string parse_url(std::string const &url,boost::system::error_code& ec);
        private:
            boost::system::error_code open(
                boost::uint32_t & session_id,
                std::string const & play_link,
                framework::string::Url const & params,
                std::string const & format,
                ppbox::mux::session_callback_respone const &);

            void handle_timer( boost::system::error_code const & ec );

            void open_callback( 
                ppbox::mux::session_callback_respone const &
                ,boost::system::error_code const & ec );

            void tranfer_callback( 
                ppbox::mux::session_callback_respone const &
                ,boost::system::error_code const & ec );

            void open_list();
        private:
            struct OpenList
            {
                OpenList(
                    boost::uint32_t& a
                    ,std::string const & b
                    ,framework::string::Url const& c
                    ,std::string const & d
                    ,ppbox::mux::session_callback_respone const & e)
                    :session_id(a)
                    ,play_link(b)
                    ,params(c)
                    ,format(d)
                    ,resp(e){}

                boost::uint32_t session_id;
                std::string play_link;
                framework::string::Url params;
                std::string format;
                ppbox::mux::session_callback_respone resp;
            };

        private:

            boost::asio::io_service& io_svc_;
            boost::asio::deadline_timer timer_;

#ifndef PPBOX_DISABLE_VOD 
            ppbox::vod::BigMp4* bigmp4_;
#endif

#ifndef PPBOX_DISABLE_PEER 
            ppbox::peer::BigMp4* bigmp4_;
#endif
            std::stringstream stream_tmp_;
            std::ostream *stream_;
            MyStream *streambuf_;

            boost::uint32_t session_id_;
            boost::uint32_t seek_;
            std::string playlink_;

            OpenList* list_;
            bool playing_;
        };

    } // namespace httpd
} // namespace ppbox

#endif // _PPBOX_HTTPD_MP4_HTTP_DISPATCHER_H_
