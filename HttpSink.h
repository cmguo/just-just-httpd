#ifndef _PPBOX_HTTPD_RTPSINK_H_
#define _PPBOX_HTTPD_RTPSINK_H_

#include <ppbox/mux/tool/Sink.h>

namespace util
{
    namespace protocol
    {
        class HttpSocket;
    }
}

namespace ppbox
{
    namespace httpd
    {
        class Transport;

        class HttpSink : public ppbox::mux::Sink
        {
        public:
            HttpSink(util::protocol::HttpSocket& sock);

            virtual ~HttpSink();

        private:
            virtual boost::system::error_code write(
                boost::posix_time::ptime const & time_send, 
                ppbox::demux::Sample&);

        private:
            util::protocol::HttpSocket& socket_;

        };

    } // namespace httpd
} // namespace ppbox

#endif 
