#ifndef _PPBOX_HTTPD_SINK_H_
#define _PPBOX_HTTPD_SINK_H_

#include <util/stream/Sink.h>

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
        class HttpSink : public util::stream::Sink
        {
        public:
            HttpSink(
                boost::asio::io_service & io_svc
                , util::protocol::HttpSocket& sock);

            virtual ~HttpSink();

        private:
            virtual std::size_t private_write_some(
                util::stream::StreamConstBuffers const & buffers,
                boost::system::error_code & ec);

        private:
            util::protocol::HttpSocket& socket_;

        };

    } // namespace httpd
} // namespace ppbox

#endif 
