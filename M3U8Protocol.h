// M3u8Protocol.h

#ifndef   _PPBOX_HTTP_M3U8_PROTOCOL_H_
#define   _PPBOX_HTTP_M3U8_PROTOCOL_H_

#include <framework/string/Format.h>
#include <sstream>
#include <iostream>

std::string const M3U8_BEGIN = "#EXTM3U";
std::string const M3U8_TARGETDURATION = "#EXT-X-TARGETDURATION:";
std::string const M3U8_SEQUENCE = "#EXT-X-MEDIA-SEQUENCE:";
std::string const M3U8_EXTINF = "#EXTINF:";
std::string const M3U8_END  = "#EXT-X-ENDLIST";
std::string const M3U8_ENDLINE = "\n";

namespace ppbox
{
    namespace httpd
    {
        class M3U8Protocol
        {
        public:
            M3U8Protocol(
                boost::uint32_t seg_duration,
                boost::uint32_t duration)
            {
                boost::uint32_t lines = boost::uint32_t(duration / seg_duration);
                last_segment_index = lines;
                std::string line = M3U8_BEGIN;
                data_ = line + M3U8_ENDLINE;
                line = M3U8_TARGETDURATION + framework::string::format(seg_duration);
                data_+= line; data_ += M3U8_ENDLINE;
                line = M3U8_SEQUENCE + framework::string::format(1);
                data_+= line; data_ += M3U8_ENDLINE;
                for (boost::uint32_t i = 1; i <= lines; ++i) {
                    std::string segment_line = M3U8_EXTINF + framework::string::format(seg_duration);
                    segment_line += ",";
                    data_+= segment_line; data_ += M3U8_ENDLINE;
                    segment_line = std::string("/") + framework::string::format(i) + std::string(".ts");
                    data_+= segment_line; data_ += M3U8_ENDLINE;
                }
                line = M3U8_END;
                data_+= line; data_ += M3U8_ENDLINE;
            }

            std::string const & read_buffer()
            {
                return data_;
            }

            ~M3U8Protocol()
            {
            }

        private:
            //std::ostringstream data_;
            std::string data_;
        public:
            static boost::uint32_t last_segment_index;
        };

        boost::uint32_t M3U8Protocol::last_segment_index = 0;
    }
}

#endif