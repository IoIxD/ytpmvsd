#pragma once

/* section structure definition */

#include <mutex>
#include <string>
extern "C" {

#include "libavutil/pixfmt.h"
#include <libavutil/ffversion.h>

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

class Prober {
public:
  AVDictionary *format_opts, *codec_opts;

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

  int put_int(std::string k, int v);
  int put_q(std::string k, AVRational v, char s);
  int put_str(std::string k, std::string v);
  int put_str_opt(std::string k, std::string v);
  int put_str_validate(std::string k, std::string v);
  int put_time(std::string k, int64_t v, AVRational *tb);
  int put_ts(std::string k, int64_t v);
  int put_duration_time(std::string k, int64_t v, AVRational *tb);
  int put_duration_ts(std::string k, int64_t v);
  int put_val(std::string k, int64_t v, const char u[5]);

  int show_tags(AVDictionary *tags, int section_id);
  void put_dovi_metadata(const AVDOVIMetadata *dovi);
  void put_dynamic_hdr10_plus(const AVDynamicHDRPlus *metadata);
  void put_dynamic_hdr_vivid(const AVDynamicHDRVivid *metadata);
  void put_ambient_viewing_environment(const AVAmbientViewingEnvironment *env);
  void put_film_grain_params(const AVFilmGrainParams *fgp);
  void put_pkt_side_data(AVCodecParameters *par, const AVPacketSideData *sd,
                         SectionID id_data);
  void put_private_data(void *priv_data);
  void put_pixel_format(enum AVPixelFormat pix_fmt);
  void put_color_range(enum AVColorRange color_range);
  void put_color_space(enum AVColorSpace color_space);
  void put_primaries(enum AVColorPrimaries color_primaries);
  void put_color_trc(enum AVColorTransferCharacteristic color_trc);
  void put_chroma_location(enum AVChromaLocation chroma_location);
  void clear_log(int need_lock);
  int show_log(int section_ids, int section_id, int log_level);
  void show_packet(InputFile *ifile, AVPacket *pkt, int packet_idx);
  void show_subtitle(AVSubtitle *sub, AVStream *stream,
                     AVFormatContext *fmt_ctx);
  void put_frame_side_data(const AVFrame *frame, const AVStream *stream);
  void show_frame(AVFrame *frame, AVStream *stream, AVFormatContext *fmt_ctx);
  int process_frame(InputFile *ifile, AVFrame *frame, const AVPacket *pkt,
                    int *packet_new);
  void log_read_interval(const ReadInterval *interval, void *log_ctx,
                         int log_level);
  int read_interval_packets(InputFile *ifile, const ReadInterval *interval,
                            int64_t *cur_ts);
  int read_packets(InputFile *ifile);
  void put_dispositions(uint32_t disposition, SectionID section_id);
  int show_stream(AVFormatContext *fmt_ctx, int stream_idx, InputStream *ist,
                  int container);
  int show_streams(InputFile *ifile);
  int show_program(InputFile *ifile, AVProgram *program);

  int show_programs(InputFile *ifile);
  void put_tile_grid_params(const AVStreamGroup *stg,
                            const AVStreamGroupTileGrid *tile_grid);
  void put_iamf_param_definition(const char *name,
                                 const AVIAMFParamDefinition *param,
                                 SectionID section_id);
  void put_iamf_audio_element_params(const AVStreamGroup *stg,
                                     const AVIAMFAudioElement *audio_element);
  void put_iamf_submix_params(const AVIAMFSubmix *submix);
  void put_iamf_mix_presentation_params(
      const AVStreamGroup *stg, const AVIAMFMixPresentation *mix_presentation);
  void put_stream_group_params(AVStreamGroup *stg);
  int show_stream_group(InputFile *ifile, AVStreamGroup *stg);
  int show_stream_groups(InputFile *ifile);
  int show_chapters(InputFile *ifile);
  int show_format(InputFile *ifile);
  void show_error(int err);

  int open_input_file(InputFile *ifile, const char *filename,
                      const char *put_filename);
  int probe_file(const char *filename, const char *put_filename);
  void ffprobe_show_pixel_formats();

  int opt_format(void *optctx, const char *opt, const char *arg);
  int parse_read_interval(const char *interval_spec, ReadInterval *interval);
  int parse_read_intervals(const char *intervals_spec);

  int filter_codec_opts(const AVDictionary *opts, enum AVCodecID codec_id,
                        AVFormatContext *s, AVStream *st, const AVCodec *codec,
                        AVDictionary **dst, AVDictionary **opts_used);

  int setup_find_stream_info_opts(AVFormatContext *s, AVDictionary *codec_opts,
                                  AVDictionary ***dst);
};
