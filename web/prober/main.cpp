#include "prober.hpp"

const char *input_filename;

int main(int argc, char **argv) {
  input_filename = av_strdup(argv[1]);
  if (!input_filename) {
    return AVERROR(ENOMEM);
  }

  Prober *p = new Prober();
  const AVTextFormatter *f;
  AVTextFormatContext *tctx;
  AVTextWriterContext *wctx;
  char *buf;
  char *f_name = NULL, *f_args = NULL;
  int ret, i;

  setvbuf(stderr, NULL, _IONBF, 0); /* win32 runtime needs this */

  av_log_set_flags(AV_LOG_SKIP_REPEATED);

  avformat_network_init();
#if CONFIG_AVDEVICE
  avdevice_register_all();
#endif

  p->output_format = av_strdup("default");
  if (!p->output_format) {
    ret = AVERROR(ENOMEM);
    goto end;
  }
  f_name = av_strtok(p->output_format, "=", &buf);
  if (!f_name) {
    av_log(NULL, AV_LOG_ERROR, "No name specified for the output format\n");
    ret = AVERROR(EINVAL);
    goto end;
  }
  f_args = buf;

  f = &avtextformatter_default;

  ret = avtextwriter_create_stdout(&wctx);

  if (ret < 0) {
    goto end;
  }
  if ((ret = avtext_context_open(
           &tctx, f, wctx, f_args, p->sections, FF_ARRAY_ELEMS(p->sections), 0,
           0, 0, 0, p->show_optional_fields, p->show_data_hash)) >= 0) {
    avtext_print_section_header(tctx, NULL, SECTION_ID_ROOT);

    if (!input_filename) {
      av_log(NULL, AV_LOG_ERROR, "You have to specify one input file.\n");
      ret = AVERROR(EINVAL);
    } else if (input_filename) {
      ret = p->probe_file(tctx, input_filename, p->print_input_filename);
      if (ret < 0 && 0) {
        p->show_error(tctx, ret);
      }
    }

    avtext_print_section_footer(tctx);

    avtextwriter_context_close(&wctx);

    avtext_context_close(&tctx);
  }

end:
  av_freep(&p->output_format);
  av_freep(&p->output_filename);
  av_freep(&input_filename);
  av_freep(&p->print_input_filename);
  av_freep(&p->read_intervals);

  for (i = 0; i < FF_ARRAY_ELEMS(p->sections); i++)
    av_dict_free(&(p->sections[i].entries_to_show));

  avformat_network_deinit();

  return ret < 0;
}
