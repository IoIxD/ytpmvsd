#include "stream.hpp"

int isalnum(char c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z');
}

int StreamSpecifier::check(AVFormatContext *s, AVStream *st, const char *spec) {
  auto ss = StreamSpecifier();
  int ret;

  ret = ss.parse(spec, 0, NULL);
  if (ret < 0)
    return ret;

  ret = ss.match(s, st, NULL);
  return ret;
}

StreamSpecifier::~StreamSpecifier() {
  av_freep(&this->meta_key);
  av_freep(&this->meta_val);
  av_freep(&this->remainder);

  memset(this, 0, sizeof(*this));
}

int StreamSpecifier::parse(const char *spec, int allow_remainder,
                           void *logctx) {
  char *endptr;
  int ret;

  memset(this, 0, sizeof(*this));

  this->idx = -1;
  this->media_type = AVMEDIA_TYPE_UNKNOWN;
  this->stream_list = STREAM_LIST_ALL;

  av_log(logctx, AV_LOG_TRACE, "Parsing stream specifier: %s\n", spec);

  while (*spec) {
    if (*spec <= '9' && *spec >= '0') { /* opt:index */
      this->idx = strtol(spec, &endptr, 0);

      av_assert0(endptr > spec);
      spec = endptr;

      av_log(logctx, AV_LOG_TRACE, "Parsed index: %d; remainder: %s\n",
             this->idx, spec);

      // this terminates the specifier
      break;
    } else if ((*spec == 'v' || *spec == 'a' || *spec == 's' || *spec == 'd' ||
                *spec == 't' || *spec == 'V') &&
               !isalnum(*(spec + 1))) { /* opt:[vasdtV] */
      if (this->media_type != AVMEDIA_TYPE_UNKNOWN) {
        av_log(logctx, AV_LOG_ERROR, "Stream type specified multiple times\n");
        ret = AVERROR(EINVAL);
        goto fail;
      }

      switch (*spec++) {
      case 'v':
        this->media_type = AVMEDIA_TYPE_VIDEO;
        break;
      case 'a':
        this->media_type = AVMEDIA_TYPE_AUDIO;
        break;
      case 's':
        this->media_type = AVMEDIA_TYPE_SUBTITLE;
        break;
      case 'd':
        this->media_type = AVMEDIA_TYPE_DATA;
        break;
      case 't':
        this->media_type = AVMEDIA_TYPE_ATTACHMENT;
        break;
      case 'V':
        this->media_type = AVMEDIA_TYPE_VIDEO;
        this->no_apic = 1;
        break;
      default:
        av_assert0(0);
      }

      av_log(logctx, AV_LOG_TRACE, "Parsed media type: %s; remainder: %s\n",
             av_get_media_type_string(this->media_type), spec);
    } else if (*spec == 'g' && *(spec + 1) == ':') {
      if (this->stream_list != STREAM_LIST_ALL)
        goto multiple_stream_lists;

      spec += 2;
      if (*spec == '#' || (*spec == 'i' && *(spec + 1) == ':')) {
        this->stream_list = STREAM_LIST_GROUP_ID;

        spec += 1 + (*spec == 'i');
      } else
        this->stream_list = STREAM_LIST_GROUP_IDX;

      this->list_id = strtol(spec, &endptr, 0);
      if (spec == endptr) {
        av_log(logctx, AV_LOG_ERROR, "Expected stream group idx/ID, got: %s\n",
               spec);
        ret = AVERROR(EINVAL);
        goto fail;
      }
      spec = endptr;

      av_log(logctx, AV_LOG_TRACE,
             "Parsed stream group %s: %" PRId64 "; remainder: %s\n",
             this->stream_list == STREAM_LIST_GROUP_ID ? "ID" : "index",
             this->list_id, spec);
    } else if (*spec == 'p' && *(spec + 1) == ':') {
      if (this->stream_list != STREAM_LIST_ALL)
        goto multiple_stream_lists;

      this->stream_list = STREAM_LIST_PROGRAM;

      spec += 2;
      this->list_id = strtol(spec, &endptr, 0);
      if (spec == endptr) {
        av_log(logctx, AV_LOG_ERROR, "Expected program ID, got: %s\n", spec);
        ret = AVERROR(EINVAL);
        goto fail;
      }
      spec = endptr;

      av_log(logctx, AV_LOG_TRACE,
             "Parsed program ID: %" PRId64 "; remainder: %s\n", this->list_id,
             spec);
    } else if (!strncmp(spec, "disp:", 5)) {
      const AVClass *st_class = av_stream_get_class();
      const AVOption *o = av_opt_find(&st_class, "disposition", NULL, 0,
                                      AV_OPT_SEARCH_FAKE_OBJ);
      char *disp = NULL;
      size_t len;

      av_assert0(o);

      if (this->disposition) {
        av_log(logctx, AV_LOG_ERROR, "Multiple disposition specifiers\n");
        ret = AVERROR(EINVAL);
        goto fail;
      }

      spec += 5;

      for (len = 0; isalnum(spec[len]) || spec[len] == '_' || spec[len] == '+';
           len++)
        continue;

      disp = av_strndup(spec, len);
      if (!disp) {
        ret = AVERROR(ENOMEM);
        goto fail;
      }

      ret = av_opt_eval_flags(&st_class, o, disp, &this->disposition);
      av_freep(&disp);
      if (ret < 0) {
        av_log(logctx, AV_LOG_ERROR, "Invalid disposition specifier\n");
        goto fail;
      }

      spec += len;

      av_log(logctx, AV_LOG_TRACE, "Parsed disposition: 0x%x; remainder: %s\n",
             this->disposition, spec);
    } else if (*spec == '#' || (*spec == 'i' && *(spec + 1) == ':')) {
      if (this->stream_list != STREAM_LIST_ALL)
        goto multiple_stream_lists;

      this->stream_list = STREAM_LIST_STREAM_ID;

      spec += 1 + (*spec == 'i');
      this->list_id = strtol(spec, &endptr, 0);
      if (spec == endptr) {
        av_log(logctx, AV_LOG_ERROR, "Expected stream ID, got: %s\n", spec);
        ret = AVERROR(EINVAL);
        goto fail;
      }
      spec = endptr;

      av_log(logctx, AV_LOG_TRACE,
             "Parsed stream ID: %" PRId64 "; remainder: %s\n", this->list_id,
             spec);

      // this terminates the specifier
      break;
    } else if (*spec == 'm' && *(spec + 1) == ':') {
      av_assert0(!this->meta_key && !this->meta_val);

      spec += 2;
      this->meta_key = av_get_token(&spec, ":");
      if (!this->meta_key) {
        ret = AVERROR(ENOMEM);
        goto fail;
      }
      if (*spec == ':') {
        spec++;
        this->meta_val = av_get_token(&spec, ":");
        if (!this->meta_val) {
          ret = AVERROR(ENOMEM);
          goto fail;
        }
      }

      av_log(logctx, AV_LOG_TRACE, "Parsed metadata: %s:%s; remainder: %s",
             this->meta_key, this->meta_val ? this->meta_val : "<any value>",
             spec);

      // this terminates the specifier
      break;
    } else if (*spec == 'u' && (*(spec + 1) == '\0' || *(spec + 1) == ':')) {
      this->usable_only = 1;
      spec++;
      av_log(logctx, AV_LOG_ERROR, "Parsed 'usable only'\n");

      // this terminates the specifier
      break;
    } else
      break;

    if (*spec == ':')
      spec++;
  }

  if (*spec) {
    if (!allow_remainder) {
      av_log(logctx, AV_LOG_ERROR,
             "Trailing garbage at the end of a stream specifier: %s\n", spec);
      ret = AVERROR(EINVAL);
      goto fail;
    }

    if (*spec == ':')
      spec++;

    this->remainder = av_strdup(spec);
    if (!this->remainder) {
      ret = AVERROR(EINVAL);
      goto fail;
    }
  }

  return 0;

multiple_stream_lists:
  av_log(logctx, AV_LOG_ERROR,
         "Cannot combine multiple program/group designators in a "
         "single stream specifier");
  ret = AVERROR(EINVAL);

fail:
  return ret;
}

const unsigned StreamSpecifier::match(const AVFormatContext *s,
                                      const AVStream *st, void *logctx) {
  const AVStreamGroup *g = NULL;
  const AVProgram *p = NULL;
  int start_stream = 0, nb_streams;
  int nb_matched = 0;

  switch (this->stream_list) {
  case STREAM_LIST_STREAM_ID:
    // <n-th> stream with given ID makes no sense and should be impossible to
    // request
    av_assert0(this->idx < 0);
    // return early if we know for sure the stream does not match
    if (st->id != this->list_id)
      return 0;
    start_stream = st->index;
    nb_streams = st->index + 1;
    break;
  case STREAM_LIST_ALL:
    start_stream = this->idx >= 0 ? 0 : st->index;
    nb_streams = st->index + 1;
    break;
  case STREAM_LIST_PROGRAM:
    for (unsigned i = 0; i < s->nb_programs; i++) {
      if (s->programs[i]->id == this->list_id) {
        p = s->programs[i];
        break;
      }
    }
    if (!p) {
      av_log(logctx, AV_LOG_WARNING,
             "No program with ID %" PRId64 " exists,"
             " stream specifier can never match\n",
             this->list_id);
      return 0;
    }
    nb_streams = p->nb_stream_indexes;
    break;
  case STREAM_LIST_GROUP_ID:
    for (unsigned i = 0; i < s->nb_stream_groups; i++) {
      if (this->list_id == s->stream_groups[i]->id) {
        g = s->stream_groups[i];
        break;
      }
    }
    // fall-through
  case STREAM_LIST_GROUP_IDX:
    if (this->stream_list == STREAM_LIST_GROUP_IDX && this->list_id >= 0 &&
        this->list_id < s->nb_stream_groups)
      g = s->stream_groups[this->list_id];

    if (!g) {
      av_log(logctx, AV_LOG_WARNING,
             "No stream group with group %s %" PRId64
             " exists, stream specifier can never match\n",
             this->stream_list == STREAM_LIST_GROUP_ID ? "ID" : "index",
             this->list_id);
      return 0;
    }
    nb_streams = g->nb_streams;
    break;
  default:
    av_assert0(0);
  }

  for (int i = start_stream; i < nb_streams; i++) {
    const AVStream *candidate = s->streams[g   ? g->streams[i]->index
                                           : p ? p->stream_index[i]
                                               : i];

    if (this->media_type != AVMEDIA_TYPE_UNKNOWN &&
        (this->media_type != candidate->codecpar->codec_type ||
         (this->no_apic &&
          (candidate->disposition & AV_DISPOSITION_ATTACHED_PIC))))
      continue;

    if (this->meta_key) {
      const AVDictionaryEntry *tag =
          av_dict_get(candidate->metadata, this->meta_key, NULL, 0);

      if (!tag)
        continue;
      if (this->meta_val && strcmp(tag->value, this->meta_val))
        continue;
    }

    if (this->usable_only) {
      const AVCodecParameters *par = candidate->codecpar;

      switch (par->codec_type) {
      case AVMEDIA_TYPE_AUDIO:
        if (!par->sample_rate || !par->ch_layout.nb_channels ||
            par->format == AV_SAMPLE_FMT_NONE)
          continue;
        break;
      case AVMEDIA_TYPE_VIDEO:
        if (!par->width || !par->height || par->format == AV_PIX_FMT_NONE)
          continue;
        break;
      case AVMEDIA_TYPE_UNKNOWN:
        continue;
      default:
        break;
      }
    }

    if (this->disposition &&
        (candidate->disposition & this->disposition) != this->disposition)
      continue;

    if (st == candidate)
      return this->idx < 0 || this->idx == nb_matched;

    nb_matched++;
  }

  return 0;
}
