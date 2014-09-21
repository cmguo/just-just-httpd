// Serialize.h

#include <ppbox/avbase/MediaInfo.h>
#include <ppbox/avbase/StreamStatus.h>

#include <util/serialization/Serialization.h>
#include <util/serialization/NVPair.h>

namespace util
{
    namespace serialization
    {

        template <
            typename Archive
        >
        void serialize(
            Archive & ar, 
            ppbox::avbase::MediaInfo & info)
        {
            ar & SERIALIZATION_NVP_1(info, file_size);
            ar & SERIALIZATION_NVP_1(info, duration);
            ar & SERIALIZATION_NVP_1(info, bitrate);
            ar & SERIALIZATION_NVP_1(info, type);
        }

        template <
            typename Archive
        >
        void serialize(
            Archive & ar, 
            ppbox::avbase::StreamRange & range)
        {
            ar & SERIALIZATION_NVP_1(range, beg);
            ar & SERIALIZATION_NVP_1(range, end);
            ar & SERIALIZATION_NVP_1(range, pos);
            ar & SERIALIZATION_NVP_1(range, buf);
        }

        template <
            typename Archive
        >
        void serialize(
            Archive & ar, 
            ppbox::avbase::StreamStatus & info)
        {
            ar & SERIALIZATION_NVP_1(info, byte_range);
            ar & SERIALIZATION_NVP_1(info, time_range);
            ar & SERIALIZATION_NVP_1(info, buf_ec);
        }

    }
}
