#pragma once

/* section structure definition */

#include <mutex>
extern "C" {

#include "config.h"
#include "libavutil/pixfmt.h"
#include <libavutil/ffversion.h>

#include "avtextformat.hpp"
#include <libavcodec/avcodec.h>
#include <libavcodec/version.h>
#include <libavdevice/avdevice.h>
#include <libavdevice/version.h>
#include <libavfilter/version.h>
#include <libavformat/avformat.h>
#include <libavformat/version.h>
#include <libavutil/ambient_viewing_environment.h>
#include <libavutil/avassert.h>
#include <libavutil/avstring.h>
#include <libavutil/bprint.h>
#include <libavutil/channel_layout.h>
#include <libavutil/dict.h>
#include <libavutil/display.h>
#include <libavutil/dovi_meta.h>
#include <libavutil/film_grain_params.h>
#include <libavutil/hdr_dynamic_metadata.h>
#include <libavutil/hdr_dynamic_vivid_metadata.h>
#include <libavutil/iamf.h>
#include <libavutil/intreadwrite.h>
#include <libavutil/mastering_display_metadata.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>
#include <libavutil/parseutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/spherical.h>
#include <libavutil/stereo3d.h>
#include <libavutil/timecode.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>
#include <libswresample/version.h>
#include <libswscale/swscale.h>
#include <libswscale/version.h>
#include <math.h>
#include <string.h>
}

typedef enum {
  SECTION_ID_CHAPTER,
  SECTION_ID_CHAPTER_TAGS,
  SECTION_ID_CHAPTERS,
  SECTION_ID_ERROR,
  SECTION_ID_FORMAT,
  SECTION_ID_FORMAT_TAGS,
  SECTION_ID_FRAME,
  SECTION_ID_FRAMES,
  SECTION_ID_FRAME_TAGS,
  SECTION_ID_FRAME_SIDE_DATA_LIST,
  SECTION_ID_FRAME_SIDE_DATA,
  SECTION_ID_FRAME_SIDE_DATA_TIMECODE_LIST,
  SECTION_ID_FRAME_SIDE_DATA_TIMECODE,
  SECTION_ID_FRAME_SIDE_DATA_COMPONENT_LIST,
  SECTION_ID_FRAME_SIDE_DATA_COMPONENT,
  SECTION_ID_FRAME_SIDE_DATA_PIECE_LIST,
  SECTION_ID_FRAME_SIDE_DATA_PIECE,
  SECTION_ID_FRAME_LOG,
  SECTION_ID_FRAME_LOGS,
  SECTION_ID_LIBRARY_VERSION,
  SECTION_ID_LIBRARY_VERSIONS,
  SECTION_ID_PACKET,
  SECTION_ID_PACKET_TAGS,
  SECTION_ID_PACKETS,
  SECTION_ID_PACKETS_AND_FRAMES,
  SECTION_ID_PACKET_SIDE_DATA_LIST,
  SECTION_ID_PACKET_SIDE_DATA,
  SECTION_ID_PIXEL_FORMAT,
  SECTION_ID_PIXEL_FORMAT_FLAGS,
  SECTION_ID_PIXEL_FORMAT_COMPONENT,
  SECTION_ID_PIXEL_FORMAT_COMPONENTS,
  SECTION_ID_PIXEL_FORMATS,
  SECTION_ID_PROGRAM_STREAM_DISPOSITION,
  SECTION_ID_PROGRAM_STREAM_TAGS,
  SECTION_ID_PROGRAM,
  SECTION_ID_PROGRAM_STREAMS,
  SECTION_ID_PROGRAM_STREAM,
  SECTION_ID_PROGRAM_TAGS,
  SECTION_ID_PROGRAM_VERSION,
  SECTION_ID_PROGRAMS,
  SECTION_ID_STREAM_GROUP_STREAM_DISPOSITION,
  SECTION_ID_STREAM_GROUP_STREAM_TAGS,
  SECTION_ID_STREAM_GROUP,
  SECTION_ID_STREAM_GROUP_COMPONENTS,
  SECTION_ID_STREAM_GROUP_COMPONENT,
  SECTION_ID_STREAM_GROUP_SUBCOMPONENTS,
  SECTION_ID_STREAM_GROUP_SUBCOMPONENT,
  SECTION_ID_STREAM_GROUP_PIECES,
  SECTION_ID_STREAM_GROUP_PIECE,
  SECTION_ID_STREAM_GROUP_SUBPIECES,
  SECTION_ID_STREAM_GROUP_SUBPIECE,
  SECTION_ID_STREAM_GROUP_BLOCKS,
  SECTION_ID_STREAM_GROUP_BLOCK,
  SECTION_ID_STREAM_GROUP_STREAMS,
  SECTION_ID_STREAM_GROUP_STREAM,
  SECTION_ID_STREAM_GROUP_DISPOSITION,
  SECTION_ID_STREAM_GROUP_TAGS,
  SECTION_ID_STREAM_GROUPS,
  SECTION_ID_ROOT,
  SECTION_ID_STREAM,
  SECTION_ID_STREAM_DISPOSITION,
  SECTION_ID_STREAMS,
  SECTION_ID_STREAM_TAGS,
  SECTION_ID_STREAM_SIDE_DATA_LIST,
  SECTION_ID_STREAM_SIDE_DATA,
  SECTION_ID_SUBTITLE,
} SectionID;

// attached as opaque_ref to packets/frames
typedef struct FrameData {
  int64_t pkt_pos;
  int pkt_size;
} FrameData;

typedef struct InputStream {
  AVStream *st;

  AVCodecContext *dec_ctx;
} InputStream;

typedef struct InputFile {
  AVFormatContext *fmt_ctx;

  InputStream *streams;
  int nb_streams;
} InputFile;

typedef struct ReadInterval {
  int id;             ///< identifier
  int64_t start, end; ///< start, end in second/AV_TIME_BASE units
  int has_start, has_end;
  int start_is_offset, end_is_offset;
  int duration_frames;
} ReadInterval;

typedef struct LogBuffer {
  char *context_name;
  int log_level;
  char *log_message;
  AVClassCategory category;
  char *parent_name;
  AVClassCategory parent_category;
} LogBuffer;

#define SHOW_OPTIONAL_FIELDS_AUTO -1
#define SHOW_OPTIONAL_FIELDS_NEVER 0
#define SHOW_OPTIONAL_FIELDS_ALWAYS 1

#define IN_PROGRAM 1
#define IN_STREAM_GROUP 2

#define PRINT_PIX_FMT_FLAG(flagname, name)                                     \
  do {                                                                         \
    print_int(name, !!(pixdesc->flags & AV_PIX_FMT_FLAG_##flagname));          \
  } while (0)

class Prober {
public:
  AVDictionary *format_opts, *codec_opts;

  struct AVTextFormatSection sections[66] = {
      [SECTION_ID_CHAPTERS] = {SECTION_ID_CHAPTERS,
                               "chapters",
                               AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                               {SECTION_ID_CHAPTER, -1}},
      [SECTION_ID_CHAPTER] = {SECTION_ID_CHAPTER,
                              "chapter",
                              0,
                              {SECTION_ID_CHAPTER_TAGS, -1}},
      [SECTION_ID_CHAPTER_TAGS] =
          {SECTION_ID_CHAPTER_TAGS,
           "tags",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS,
           {-1},
           .element_name = "tag",
           .unique_name = "chapter_tags"},
      [SECTION_ID_ERROR] = {SECTION_ID_ERROR, "error", 0, {-1}},
      [SECTION_ID_FORMAT] = {SECTION_ID_FORMAT,
                             "format",
                             0,
                             {SECTION_ID_FORMAT_TAGS, -1}},
      [SECTION_ID_FORMAT_TAGS] =
          {SECTION_ID_FORMAT_TAGS,
           "tags",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS,
           {-1},
           .element_name = "tag",
           .unique_name = "format_tags"},
      [SECTION_ID_FRAMES] = {SECTION_ID_FRAMES,
                             "frames",
                             AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                             {SECTION_ID_FRAME, SECTION_ID_SUBTITLE, -1}},
      [SECTION_ID_FRAME] = {SECTION_ID_FRAME,
                            "frame",
                            0,
                            {SECTION_ID_FRAME_TAGS,
                             SECTION_ID_FRAME_SIDE_DATA_LIST,
                             SECTION_ID_FRAME_LOGS, -1}},
      [SECTION_ID_FRAME_TAGS] = {SECTION_ID_FRAME_TAGS,
                                 "tags",
                                 AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS,
                                 {-1},
                                 .element_name = "tag",
                                 .unique_name = "frame_tags"},
      [SECTION_ID_FRAME_SIDE_DATA_LIST] = {SECTION_ID_FRAME_SIDE_DATA_LIST,
                                           "side_data_list",
                                           AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                                           {SECTION_ID_FRAME_SIDE_DATA, -1},
                                           .element_name = "side_data",
                                           .unique_name =
                                               "frame_side_data_list"},
      [SECTION_ID_FRAME_SIDE_DATA] =
          {SECTION_ID_FRAME_SIDE_DATA,
           "side_data",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS |
               AV_TEXTFORMAT_SECTION_FLAG_HAS_TYPE,
           {SECTION_ID_FRAME_SIDE_DATA_TIMECODE_LIST,
            SECTION_ID_FRAME_SIDE_DATA_COMPONENT_LIST, -1},
           .unique_name = "frame_side_data",
           .element_name = "side_datum",
           .get_type = get_frame_side_data_type},
      [SECTION_ID_FRAME_SIDE_DATA_TIMECODE_LIST] =
          {SECTION_ID_FRAME_SIDE_DATA_TIMECODE_LIST,
           "timecodes",
           AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
           {SECTION_ID_FRAME_SIDE_DATA_TIMECODE, -1}},
      [SECTION_ID_FRAME_SIDE_DATA_TIMECODE] =
          {SECTION_ID_FRAME_SIDE_DATA_TIMECODE, "timecode", 0, {-1}},
      [SECTION_ID_FRAME_SIDE_DATA_COMPONENT_LIST] =
          {SECTION_ID_FRAME_SIDE_DATA_COMPONENT_LIST,
           "components",
           AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
           {SECTION_ID_FRAME_SIDE_DATA_COMPONENT, -1},
           .element_name = "component",
           .unique_name = "frame_side_data_components"},
      [SECTION_ID_FRAME_SIDE_DATA_COMPONENT] =
          {SECTION_ID_FRAME_SIDE_DATA_COMPONENT,
           "component",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS |
               AV_TEXTFORMAT_SECTION_FLAG_HAS_TYPE,
           {SECTION_ID_FRAME_SIDE_DATA_PIECE_LIST, -1},
           .unique_name = "frame_side_data_component",
           .element_name = "component_entry",
           .get_type = get_raw_string_type},
      [SECTION_ID_FRAME_SIDE_DATA_PIECE_LIST] =
          {SECTION_ID_FRAME_SIDE_DATA_PIECE_LIST,
           "pieces",
           AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
           {SECTION_ID_FRAME_SIDE_DATA_PIECE, -1},
           .element_name = "piece",
           .unique_name = "frame_side_data_pieces"},
      [SECTION_ID_FRAME_SIDE_DATA_PIECE] =
          {SECTION_ID_FRAME_SIDE_DATA_PIECE,
           "piece",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS |
               AV_TEXTFORMAT_SECTION_FLAG_HAS_TYPE,
           {-1},
           .element_name = "piece_entry",
           .unique_name = "frame_side_data_piece",
           .get_type = get_raw_string_type},
      [SECTION_ID_FRAME_LOGS] = {SECTION_ID_FRAME_LOGS,
                                 "logs",
                                 AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                                 {SECTION_ID_FRAME_LOG, -1}},
      [SECTION_ID_FRAME_LOG] =
          {
              SECTION_ID_FRAME_LOG,
              "log",
              0,
              {-1},
          },
      [SECTION_ID_LIBRARY_VERSIONS] = {SECTION_ID_LIBRARY_VERSIONS,
                                       "library_versions",
                                       AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                                       {SECTION_ID_LIBRARY_VERSION, -1}},
      [SECTION_ID_LIBRARY_VERSION] = {SECTION_ID_LIBRARY_VERSION,
                                      "library_version",
                                      0,
                                      {-1}},
      [SECTION_ID_PACKETS] = {SECTION_ID_PACKETS,
                              "packets",
                              AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                              {SECTION_ID_PACKET, -1}},
      [SECTION_ID_PACKETS_AND_FRAMES] =
          {SECTION_ID_PACKETS_AND_FRAMES,
           "packets_and_frames",
           AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY |
               AV_TEXTFORMAT_SECTION_FLAG_NUMBERING_BY_TYPE,
           {SECTION_ID_PACKET, -1}},
      [SECTION_ID_PACKET] = {SECTION_ID_PACKET,
                             "packet",
                             0,
                             {SECTION_ID_PACKET_TAGS,
                              SECTION_ID_PACKET_SIDE_DATA_LIST, -1}},
      [SECTION_ID_PACKET_TAGS] =
          {SECTION_ID_PACKET_TAGS,
           "tags",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS,
           {-1},
           .element_name = "tag",
           .unique_name = "packet_tags"},
      [SECTION_ID_PACKET_SIDE_DATA_LIST] = {SECTION_ID_PACKET_SIDE_DATA_LIST,
                                            "side_data_list",
                                            AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                                            {SECTION_ID_PACKET_SIDE_DATA, -1},
                                            .element_name = "side_data",
                                            .unique_name =
                                                "packet_side_data_list"},
      [SECTION_ID_PACKET_SIDE_DATA] =
          {SECTION_ID_PACKET_SIDE_DATA,
           "side_data",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS |
               AV_TEXTFORMAT_SECTION_FLAG_HAS_TYPE,
           {-1},
           .unique_name = "packet_side_data",
           .element_name = "side_datum",
           .get_type = get_packet_side_data_type},
      [SECTION_ID_PIXEL_FORMATS] = {SECTION_ID_PIXEL_FORMATS,
                                    "pixel_formats",
                                    AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                                    {SECTION_ID_PIXEL_FORMAT, -1}},
      [SECTION_ID_PIXEL_FORMAT] = {SECTION_ID_PIXEL_FORMAT,
                                   "pixel_format",
                                   0,
                                   {SECTION_ID_PIXEL_FORMAT_FLAGS,
                                    SECTION_ID_PIXEL_FORMAT_COMPONENTS, -1}},
      [SECTION_ID_PIXEL_FORMAT_FLAGS] = {SECTION_ID_PIXEL_FORMAT_FLAGS,
                                         "flags",
                                         0,
                                         {-1},
                                         .unique_name = "pixel_format_flags"},
      [SECTION_ID_PIXEL_FORMAT_COMPONENTS] =
          {SECTION_ID_PIXEL_FORMAT_COMPONENTS,
           "components",
           AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
           {SECTION_ID_PIXEL_FORMAT_COMPONENT, -1},
           .unique_name = "pixel_format_components"},
      [SECTION_ID_PIXEL_FORMAT_COMPONENT] = {SECTION_ID_PIXEL_FORMAT_COMPONENT,
                                             "component",
                                             0,
                                             {-1}},
      [SECTION_ID_PROGRAM_STREAM_DISPOSITION] =
          {SECTION_ID_PROGRAM_STREAM_DISPOSITION,
           "disposition",
           0,
           {-1},
           .unique_name = "program_stream_disposition"},
      [SECTION_ID_PROGRAM_STREAM_TAGS] =
          {SECTION_ID_PROGRAM_STREAM_TAGS,
           "tags",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS,
           {-1},
           .element_name = "tag",
           .unique_name = "program_stream_tags"},
      [SECTION_ID_PROGRAM] = {SECTION_ID_PROGRAM,
                              "program",
                              0,
                              {SECTION_ID_PROGRAM_TAGS,
                               SECTION_ID_PROGRAM_STREAMS, -1}},
      [SECTION_ID_PROGRAM_STREAMS] = {SECTION_ID_PROGRAM_STREAMS,
                                      "streams",
                                      AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                                      {SECTION_ID_PROGRAM_STREAM, -1},
                                      .unique_name = "program_streams"},
      [SECTION_ID_PROGRAM_STREAM] = {SECTION_ID_PROGRAM_STREAM,
                                     "stream",
                                     0,
                                     {SECTION_ID_PROGRAM_STREAM_DISPOSITION,
                                      SECTION_ID_PROGRAM_STREAM_TAGS, -1},
                                     .unique_name = "program_stream"},
      [SECTION_ID_PROGRAM_TAGS] =
          {SECTION_ID_PROGRAM_TAGS,
           "tags",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS,
           {-1},
           .element_name = "tag",
           .unique_name = "program_tags"},
      [SECTION_ID_PROGRAM_VERSION] = {SECTION_ID_PROGRAM_VERSION,
                                      "program_version",
                                      0,
                                      {-1}},
      [SECTION_ID_PROGRAMS] = {SECTION_ID_PROGRAMS,
                               "programs",
                               AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                               {SECTION_ID_PROGRAM, -1}},
      [SECTION_ID_STREAM_GROUP_STREAM_DISPOSITION] =
          {SECTION_ID_STREAM_GROUP_STREAM_DISPOSITION,
           "disposition",
           0,
           {-1},
           .unique_name = "stream_group_stream_disposition"},
      [SECTION_ID_STREAM_GROUP_STREAM_TAGS] =
          {SECTION_ID_STREAM_GROUP_STREAM_TAGS,
           "tags",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS,
           {-1},
           .element_name = "tag",
           .unique_name = "stream_group_stream_tags"},
      [SECTION_ID_STREAM_GROUP] = {SECTION_ID_STREAM_GROUP,
                                   "stream_group",
                                   0,
                                   {SECTION_ID_STREAM_GROUP_TAGS,
                                    SECTION_ID_STREAM_GROUP_DISPOSITION,
                                    SECTION_ID_STREAM_GROUP_COMPONENTS,
                                    SECTION_ID_STREAM_GROUP_STREAMS, -1}},
      [SECTION_ID_STREAM_GROUP_COMPONENTS] =
          {SECTION_ID_STREAM_GROUP_COMPONENTS,
           "components",
           AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
           {SECTION_ID_STREAM_GROUP_COMPONENT, -1},
           .element_name = "component",
           .unique_name = "stream_group_components"},
      [SECTION_ID_STREAM_GROUP_COMPONENT] =
          {SECTION_ID_STREAM_GROUP_COMPONENT,
           "component",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS |
               AV_TEXTFORMAT_SECTION_FLAG_HAS_TYPE,
           {SECTION_ID_STREAM_GROUP_SUBCOMPONENTS, -1},
           .unique_name = "stream_group_component",
           .element_name = "component_entry",
           .get_type = get_stream_group_type},
      [SECTION_ID_STREAM_GROUP_SUBCOMPONENTS] =
          {SECTION_ID_STREAM_GROUP_SUBCOMPONENTS,
           "subcomponents",
           AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
           {SECTION_ID_STREAM_GROUP_SUBCOMPONENT, -1},
           .element_name = "component"},
      [SECTION_ID_STREAM_GROUP_SUBCOMPONENT] =
          {SECTION_ID_STREAM_GROUP_SUBCOMPONENT,
           "subcomponent",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS |
               AV_TEXTFORMAT_SECTION_FLAG_HAS_TYPE,
           {SECTION_ID_STREAM_GROUP_PIECES, -1},
           .element_name = "subcomponent_entry",
           .get_type = get_raw_string_type},
      [SECTION_ID_STREAM_GROUP_PIECES] = {SECTION_ID_STREAM_GROUP_PIECES,
                                          "pieces",
                                          AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                                          {SECTION_ID_STREAM_GROUP_PIECE, -1},
                                          .element_name = "piece",
                                          .unique_name = "stream_group_pieces"},
      [SECTION_ID_STREAM_GROUP_PIECE] =
          {SECTION_ID_STREAM_GROUP_PIECE,
           "piece",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS |
               AV_TEXTFORMAT_SECTION_FLAG_HAS_TYPE,
           {SECTION_ID_STREAM_GROUP_SUBPIECES, -1},
           .unique_name = "stream_group_piece",
           .element_name = "piece_entry",
           .get_type = get_raw_string_type},
      [SECTION_ID_STREAM_GROUP_SUBPIECES] =
          {SECTION_ID_STREAM_GROUP_SUBPIECES,
           "subpieces",
           AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
           {SECTION_ID_STREAM_GROUP_SUBPIECE, -1},
           .element_name = "subpiece"},
      [SECTION_ID_STREAM_GROUP_SUBPIECE] =
          {SECTION_ID_STREAM_GROUP_SUBPIECE,
           "subpiece",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS |
               AV_TEXTFORMAT_SECTION_FLAG_HAS_TYPE,
           {SECTION_ID_STREAM_GROUP_BLOCKS, -1},
           .element_name = "subpiece_entry",
           .get_type = get_raw_string_type},
      [SECTION_ID_STREAM_GROUP_BLOCKS] = {SECTION_ID_STREAM_GROUP_BLOCKS,
                                          "blocks",
                                          AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                                          {SECTION_ID_STREAM_GROUP_BLOCK, -1},
                                          .element_name = "block"},
      [SECTION_ID_STREAM_GROUP_BLOCK] =
          {SECTION_ID_STREAM_GROUP_BLOCK,
           "block",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS |
               AV_TEXTFORMAT_SECTION_FLAG_HAS_TYPE,
           {-1},
           .element_name = "block_entry",
           .get_type = get_raw_string_type},
      [SECTION_ID_STREAM_GROUP_STREAMS] = {SECTION_ID_STREAM_GROUP_STREAMS,
                                           "streams",
                                           AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                                           {SECTION_ID_STREAM_GROUP_STREAM, -1},
                                           .unique_name =
                                               "stream_group_streams"},
      [SECTION_ID_STREAM_GROUP_STREAM] =
          {SECTION_ID_STREAM_GROUP_STREAM,
           "stream",
           0,
           {SECTION_ID_STREAM_GROUP_STREAM_DISPOSITION,
            SECTION_ID_STREAM_GROUP_STREAM_TAGS, -1},
           .unique_name = "stream_group_stream"},
      [SECTION_ID_STREAM_GROUP_DISPOSITION] =
          {SECTION_ID_STREAM_GROUP_DISPOSITION,
           "disposition",
           0,
           {-1},
           .unique_name = "stream_group_disposition"},
      [SECTION_ID_STREAM_GROUP_TAGS] =
          {SECTION_ID_STREAM_GROUP_TAGS,
           "tags",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS,
           {-1},
           .element_name = "tag",
           .unique_name = "stream_group_tags"},
      [SECTION_ID_STREAM_GROUPS] = {SECTION_ID_STREAM_GROUPS,
                                    "stream_groups",
                                    AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                                    {SECTION_ID_STREAM_GROUP, -1}},
      [SECTION_ID_ROOT] =
          {SECTION_ID_ROOT,
           "root",
           AV_TEXTFORMAT_SECTION_FLAG_IS_WRAPPER,
           {SECTION_ID_CHAPTERS, SECTION_ID_FORMAT, SECTION_ID_FRAMES,
            SECTION_ID_PROGRAMS, SECTION_ID_STREAM_GROUPS, SECTION_ID_STREAMS,
            SECTION_ID_PACKETS, SECTION_ID_ERROR, SECTION_ID_PROGRAM_VERSION,
            SECTION_ID_LIBRARY_VERSIONS, SECTION_ID_PIXEL_FORMATS, -1}},
      [SECTION_ID_STREAMS] = {SECTION_ID_STREAMS,
                              "streams",
                              AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                              {SECTION_ID_STREAM, -1}},
      [SECTION_ID_STREAM] = {SECTION_ID_STREAM,
                             "stream",
                             0,
                             {SECTION_ID_STREAM_DISPOSITION,
                              SECTION_ID_STREAM_TAGS,
                              SECTION_ID_STREAM_SIDE_DATA_LIST, -1}},
      [SECTION_ID_STREAM_DISPOSITION] = {SECTION_ID_STREAM_DISPOSITION,
                                         "disposition",
                                         0,
                                         {-1},
                                         .unique_name = "stream_disposition"},
      [SECTION_ID_STREAM_TAGS] =
          {SECTION_ID_STREAM_TAGS,
           "tags",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS,
           {-1},
           .element_name = "tag",
           .unique_name = "stream_tags"},
      [SECTION_ID_STREAM_SIDE_DATA_LIST] = {SECTION_ID_STREAM_SIDE_DATA_LIST,
                                            "side_data_list",
                                            AV_TEXTFORMAT_SECTION_FLAG_IS_ARRAY,
                                            {SECTION_ID_STREAM_SIDE_DATA, -1},
                                            .element_name = "side_data",
                                            .unique_name =
                                                "stream_side_data_list"},
      [SECTION_ID_STREAM_SIDE_DATA] =
          {SECTION_ID_STREAM_SIDE_DATA,
           "side_data",
           AV_TEXTFORMAT_SECTION_FLAG_HAS_TYPE |
               AV_TEXTFORMAT_SECTION_FLAG_HAS_VARIABLE_FIELDS,
           {-1},
           .unique_name = "stream_side_data",
           .element_name = "side_datum",
           .get_type = get_packet_side_data_type},
      [SECTION_ID_SUBTITLE] = {SECTION_ID_SUBTITLE, "subtitle", 0, {-1}},
  };

  int show_optional_fields = SHOW_OPTIONAL_FIELDS_AUTO;
  char *output_format;
  char *stream_specifier;
  char *show_data_hash;
  ReadInterval *read_intervals;
  int read_intervals_nb = 0;
  int find_stream_info = 1;

  static const char *get_packet_side_data_type(const void *data);
  static const char *get_frame_side_data_type(const void *data);
  static const char *get_raw_string_type(const void *data);
  static const char *get_stream_group_type(const void *data);

  const char *print_input_filename;
  const AVInputFormat *iformat = NULL;
  const char *output_filename = NULL;

  const char unit_second_str[2] = "s";
  const char unit_hertz_str[3] = "Hz";
  const char unit_byte_str[5] = "byte";
  const char unit_bit_per_second_str[6] = "bit/s";

  int nb_streams;
  uint64_t *nb_streams_packets;
  uint64_t *nb_streams_frames;
  int *selected_streams;
  int *streams_with_closed_captions;
  int *streams_with_film_grain;

  std::mutex log_mutex;

  LogBuffer *log_buffer;
  int log_buffer_size;

  int show_tags(AVTextFormatContext *tfc, AVDictionary *tags, int section_id);
  void print_dovi_metadata(AVTextFormatContext *tfc,
                           const AVDOVIMetadata *dovi);
  void print_dynamic_hdr10_plus(AVTextFormatContext *tfc,
                                const AVDynamicHDRPlus *metadata);
  void print_dynamic_hdr_vivid(AVTextFormatContext *tfc,
                               const AVDynamicHDRVivid *metadata);
  void
  print_ambient_viewing_environment(AVTextFormatContext *tfc,
                                    const AVAmbientViewingEnvironment *env);
  void print_film_grain_params(AVTextFormatContext *tfc,
                               const AVFilmGrainParams *fgp);
  void print_pkt_side_data(AVTextFormatContext *tfc, AVCodecParameters *par,
                           const AVPacketSideData *sd, SectionID id_data);
  void print_private_data(AVTextFormatContext *tfc, void *priv_data);
  void print_pixel_format(AVTextFormatContext *tfc, enum AVPixelFormat pix_fmt);
  void print_color_range(AVTextFormatContext *tfc,
                         enum AVColorRange color_range);
  void print_color_space(AVTextFormatContext *tfc,
                         enum AVColorSpace color_space);
  void print_primaries(AVTextFormatContext *tfc,
                       enum AVColorPrimaries color_primaries);
  void print_color_trc(AVTextFormatContext *tfc,
                       enum AVColorTransferCharacteristic color_trc);
  void print_chroma_location(AVTextFormatContext *tfc,
                             enum AVChromaLocation chroma_location);
  void clear_log(int need_lock);
  int show_log(AVTextFormatContext *tfc, int section_ids, int section_id,
               int log_level);
  void show_packet(AVTextFormatContext *tfc, InputFile *ifile, AVPacket *pkt,
                   int packet_idx);
  void show_subtitle(AVTextFormatContext *tfc, AVSubtitle *sub,
                     AVStream *stream, AVFormatContext *fmt_ctx);
  void print_frame_side_data(AVTextFormatContext *tfc, const AVFrame *frame,
                             const AVStream *stream);
  void show_frame(AVTextFormatContext *tfc, AVFrame *frame, AVStream *stream,
                  AVFormatContext *fmt_ctx);
  int process_frame(AVTextFormatContext *tfc, InputFile *ifile, AVFrame *frame,
                    const AVPacket *pkt, int *packet_new);
  void log_read_interval(const ReadInterval *interval, void *log_ctx,
                         int log_level);
  int read_interval_packets(AVTextFormatContext *tfc, InputFile *ifile,
                            const ReadInterval *interval, int64_t *cur_ts);
  int read_packets(AVTextFormatContext *tfc, InputFile *ifile);
  void print_dispositions(AVTextFormatContext *tfc, uint32_t disposition,
                          SectionID section_id);
  int show_stream(AVTextFormatContext *tfc, AVFormatContext *fmt_ctx,
                  int stream_idx, InputStream *ist, int container);
  int show_streams(AVTextFormatContext *tfc, InputFile *ifile);
  int show_program(AVTextFormatContext *tfc, InputFile *ifile,
                   AVProgram *program);

  int show_programs(AVTextFormatContext *tfc, InputFile *ifile);
  void print_tile_grid_params(AVTextFormatContext *tfc,
                              const AVStreamGroup *stg,
                              const AVStreamGroupTileGrid *tile_grid);
  void print_iamf_param_definition(AVTextFormatContext *tfc, const char *name,
                                   const AVIAMFParamDefinition *param,
                                   SectionID section_id);
  void print_iamf_audio_element_params(AVTextFormatContext *tfc,
                                       const AVStreamGroup *stg,
                                       const AVIAMFAudioElement *audio_element);
  void print_iamf_submix_params(AVTextFormatContext *tfc,
                                const AVIAMFSubmix *submix);
  void print_iamf_mix_presentation_params(
      AVTextFormatContext *tfc, const AVStreamGroup *stg,
      const AVIAMFMixPresentation *mix_presentation);
  void print_stream_group_params(AVTextFormatContext *tfc, AVStreamGroup *stg);
  int show_stream_group(AVTextFormatContext *tfc, InputFile *ifile,
                        AVStreamGroup *stg);
  int show_stream_groups(AVTextFormatContext *tfc, InputFile *ifile);
  int show_chapters(AVTextFormatContext *tfc, InputFile *ifile);
  int show_format(AVTextFormatContext *tfc, InputFile *ifile);
  void show_error(AVTextFormatContext *tfc, int err);

  int open_input_file(InputFile *ifile, const char *filename,
                      const char *print_filename);
  int probe_file(AVTextFormatContext *tfc, const char *filename,
                 const char *print_filename);
  void ffprobe_show_pixel_formats(AVTextFormatContext *tfc);

  int opt_format(void *optctx, const char *opt, const char *arg);
  void mark_section_show_entries(SectionID section_id, int show_all_entries,
                                 AVDictionary *entries);
  int match_section(const char *section_name, int show_all_entries,
                    AVDictionary *entries);
  int parse_read_interval(const char *interval_spec, ReadInterval *interval);
  int parse_read_intervals(const char *intervals_spec);
  void print_section(SectionID id, int level);
  int check_section_show_entries(int section_id);

  int filter_codec_opts(const AVDictionary *opts, enum AVCodecID codec_id,
                        AVFormatContext *s, AVStream *st, const AVCodec *codec,
                        AVDictionary **dst, AVDictionary **opts_used);

  int setup_find_stream_info_opts(AVFormatContext *s, AVDictionary *codec_opts,
                                  AVDictionary ***dst);
};
