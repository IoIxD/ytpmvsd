#pragma once

#include <cstdint>
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avassert.h>
#include <libavutil/avstring.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

enum StreamList {
  STREAM_LIST_ALL,
  STREAM_LIST_STREAM_ID,
  STREAM_LIST_PROGRAM,
  STREAM_LIST_GROUP_ID,
  STREAM_LIST_GROUP_IDX,
};

class StreamSpecifier {
  // trailing stream index - pick idx-th stream that matches
  // all the other constraints; -1 when not present
  int idx;

  // which stream list to consider
  enum StreamList stream_list;

  // STREAM_LIST_STREAM_ID: stream ID
  // STREAM_LIST_GROUP_IDX: group index
  // STREAM_LIST_GROUP_ID:  group ID
  // STREAM_LIST_PROGRAM:   program ID
  int64_t list_id;

  // when not AVMEDIA_TYPE_UNKNOWN, consider only streams of this type
  enum AVMediaType media_type;
  uint8_t no_apic;

  uint8_t usable_only;

  int disposition;

  char *meta_key;
  char *meta_val;

  char *remainder;

public:
  /**
   * Parse a stream specifier string into a form suitable for matching.
   *
   * @param ss Parsed specifier will be stored here; must be uninitialized
   *           with uninit() when no longer needed.
   * @param spec String containing the stream specifier to be parsed.
   * @param allow_remainder When 1, the part of spec that is left after parsing
   *                        the stream specifier is stored into ss->remainder.
   *                        When 0, any remainder will cause parsing to fail.
   */
  int parse(const char *spec, int allow_remainder, void *logctx);

  /**
   * @return 1 if st matches the parsed specifier, 0 if it does not
   */
  const unsigned match(const AVFormatContext *s, const AVStream *st,
                       void *logctx);

  static int check(AVFormatContext *s, AVStream *st, const char *spec);

  ~StreamSpecifier();
};
