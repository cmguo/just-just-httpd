// Serialize.h

#include <ppbox/data/MediaInfo.h>
#include <ppbox/data/PlayInfo.h>

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
            ppbox::data::MediaInfo & info)
        {
            ar & SERIALIZATION_NVP_1(info, file_size);
            ar & SERIALIZATION_NVP_1(info, duration);
            ar & SERIALIZATION_NVP_1(info, bitrate);
            ar & SERIALIZATION_NVP_1(info, is_live);
        }

        template <
            typename Archive
        >
        void serialize(
            Archive & ar, 
            ppbox::data::PlayRange & range)
        {
            ar & SERIALIZATION_NVP_1(range, beg);
            ar & SERIALIZATION_NVP_1(range, end);
            ar & SERIALIZATION_NVP_1(range, pos);
        }

        template <
            typename Archive
        >
        void serialize(
            Archive & ar, 
            ppbox::data::PlayInfo & info)
        {
            ar & SERIALIZATION_NVP_1(info, byte_range);
            ar & SERIALIZATION_NVP_1(info, time_range);
        }

    }
}
