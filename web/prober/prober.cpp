/*
 * Copyright (c) 2007-2010 Stefano Sabatini
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * simple media prober based on the FFmpeg libraries
 */

#include <mutex>

// #include "cmdutils.hpp"
#include "prober.hpp"
#include "stream.hpp"

const char *Prober::get_packet_side_data_type(const void *data) {
  const AVPacketSideData *sd = (const AVPacketSideData *)data;
  return (const char *)av_x_if_null(av_packet_side_data_name(sd->type),
                                    "unknown");
}

const char *Prober::get_frame_side_data_type(const void *data) {
  const AVFrameSideData *sd = (const AVFrameSideData *)data;
  return (const char *)av_x_if_null(av_frame_side_data_name(sd->type),
                                    "unknown");
}

const char *Prober::get_raw_string_type(const void *data) {
  return (const char *)data;
}

const char *Prober::get_stream_group_type(const void *data) {
  const AVStreamGroup *stg = (const AVStreamGroup *)data;
  return (const char *)av_x_if_null(avformat_stream_group_name(stg->type),
                                    "unknown");
}

#define put_fmt(k, f, ...)                                                     \
  do {                                                                         \
    av_bprint_clear(&pbuf);                                                    \
    av_bprintf(&pbuf, f, __VA_ARGS__);                                         \
    put_str(k, pbuf.str);                                                      \
  } while (0)

#define put_list_fmt(k, f, n, m, ...)                                          \
  do {                                                                         \
    av_bprint_clear(&pbuf);                                                    \
    for (int idx = 0; idx < n; idx++) {                                        \
      for (int idx2 = 0; idx2 < m; idx2++) {                                   \
        if (idx > 0 || idx2 > 0)                                               \
          av_bprint_chars(&pbuf, ' ', 1);                                      \
        av_bprintf(&pbuf, f, __VA_ARGS__);                                     \
      }                                                                        \
    }                                                                          \
    put_str(k, pbuf.str);                                                      \
  } while (0)

#define REALLOCZ_ARRAY_STREAM(ptr, cur_n, new_n)                               \
  {                                                                            \
    ret = av_reallocp_array(&(ptr), (new_n), sizeof(*(ptr)));                  \
    if (ret < 0)                                                               \
      goto end;                                                                \
    memset((ptr) + (cur_n), 0, ((new_n) - (cur_n)) * sizeof(*(ptr)));          \
  }

int Prober::show_tags(AVDictionary *tags, int section_id) {
  const AVDictionaryEntry *tag = NULL;
  int ret = 0;

  if (!tags)
    return 0;

  while ((tag = av_dict_iterate(tags, tag))) {
    if ((ret = put_str_validate(tag->key, tag->value)) < 0)
      break;
  }

  return ret;
}

void Prober::put_dovi_metadata(const AVDOVIMetadata *dovi) {
  if (!dovi)
    return;

  {
    const AVDOVIRpuDataHeader *hdr = av_dovi_get_header(dovi);
    const AVDOVIDataMapping *mapping = av_dovi_get_mapping(dovi);
    const AVDOVIColorMetadata *color = av_dovi_get_color(dovi);
    AVBPrint pbuf;

    av_bprint_init(&pbuf, 1, AV_BPRINT_SIZE_UNLIMITED);

    // header
    put_int("rpu_type", hdr->rpu_type);
    put_int("rpu_format", hdr->rpu_format);
    put_int("vdr_rpu_profile", hdr->vdr_rpu_profile);
    put_int("vdr_rpu_level", hdr->vdr_rpu_level);
    put_int("chroma_resampling_explicit_filter_flag",
            hdr->chroma_resampling_explicit_filter_flag);
    put_int("coef_data_type", hdr->coef_data_type);
    put_int("coef_log2_denom", hdr->coef_log2_denom);
    put_int("vdr_rpu_normalized_idc", hdr->vdr_rpu_normalized_idc);
    put_int("bl_video_full_range_flag", hdr->bl_video_full_range_flag);
    put_int("bl_bit_depth", hdr->bl_bit_depth);
    put_int("el_bit_depth", hdr->el_bit_depth);
    put_int("vdr_bit_depth", hdr->vdr_bit_depth);
    put_int("spatial_resampling_filter_flag",
            hdr->spatial_resampling_filter_flag);
    put_int("el_spatial_resampling_filter_flag",
            hdr->el_spatial_resampling_filter_flag);
    put_int("disable_residual_flag", hdr->disable_residual_flag);

    // data mapping values
    put_int("vdr_rpu_id", mapping->vdr_rpu_id);
    put_int("mapping_color_space", mapping->mapping_color_space);
    put_int("mapping_chroma_format_idc", mapping->mapping_chroma_format_idc);

    put_int("nlq_method_idc", mapping->nlq_method_idc);
    switch (mapping->nlq_method_idc) {
    case AV_DOVI_NLQ_NONE:
      put_str("nlq_method_idc_name", "none");
      break;
    case AV_DOVI_NLQ_LINEAR_DZ:
      put_str("nlq_method_idc_name", "linear_dz");
      break;
    default:
      put_str("nlq_method_idc_name", "unknown");
      break;
    }

    put_int("num_x_partitions", mapping->num_x_partitions);
    put_int("num_y_partitions", mapping->num_y_partitions);

    for (int c = 0; c < 3; c++) {
      const AVDOVIReshapingCurve *curve = &mapping->curves[c];

      put_list_fmt("pivots", "%" PRIu16, curve->num_pivots, 1,
                   curve->pivots[idx]);

      for (int i = 0; i < curve->num_pivots - 1; i++) {
        AVBPrint piece_buf;

        av_bprint_init(&piece_buf, 0, AV_BPRINT_SIZE_AUTOMATIC);
        switch (curve->mapping_idc[i]) {
        case AV_DOVI_MAPPING_POLYNOMIAL:
          av_bprintf(&piece_buf, "Polynomial");
          break;
        case AV_DOVI_MAPPING_MMR:
          av_bprintf(&piece_buf, "MMR");
          break;
        default:
          av_bprintf(&piece_buf, "Unknown");
          break;
        }
        av_bprintf(&piece_buf, " mapping");

        put_int("mapping_idc", curve->mapping_idc[i]);
        switch (curve->mapping_idc[i]) {
        case AV_DOVI_MAPPING_POLYNOMIAL:
          put_str("mapping_idc_name", "polynomial");
          put_int("poly_order", curve->poly_order[i]);
          put_list_fmt("poly_coef", "%" PRIi64, curve->poly_order[i] + 1, 1,
                       curve->poly_coef[i][idx]);
          break;
        case AV_DOVI_MAPPING_MMR:
          put_str("mapping_idc_name", "mmr");
          put_int("mmr_order", curve->mmr_order[i]);
          put_int("mmr_constant", curve->mmr_constant[i]);
          put_list_fmt("mmr_coef", "%" PRIi64, curve->mmr_order[i], 7,
                       curve->mmr_coef[i][idx][idx2]);
          break;
        default:
          put_str("mapping_idc_name", "unknown");
          break;
        }

        // SECTION_ID_FRAME_SIDE_DATA_PIECE
      }

      // SECTION_ID_FRAME_SIDE_DATA_PIECE_LIST

      if (mapping->nlq_method_idc != AV_DOVI_NLQ_NONE) {
        const AVDOVINLQParams *nlq = &mapping->nlq[c];
        put_int("nlq_offset", nlq->nlq_offset);
        put_int("vdr_in_max", nlq->vdr_in_max);

        switch (mapping->nlq_method_idc) {
        case AV_DOVI_NLQ_LINEAR_DZ:
          put_int("linear_deadzone_slope", nlq->linear_deadzone_slope);
          put_int("linear_deadzone_threshold", nlq->linear_deadzone_threshold);
          break;
        default:
          break;
        }
      }

      // SECTION_ID_FRAME_SIDE_DATA_COMPONENT
    }

    // SECTION_ID_FRAME_SIDE_DATA_COMPONENT_LIST

    // color metadata
    put_int("dm_metadata_id", color->dm_metadata_id);
    put_int("scene_refresh_flag", color->scene_refresh_flag);
    put_list_fmt("ycc_to_rgb_matrix", "%d/%d",
                 FF_ARRAY_ELEMS(color->ycc_to_rgb_matrix), 1,
                 color->ycc_to_rgb_matrix[idx].num,
                 color->ycc_to_rgb_matrix[idx].den);
    put_list_fmt("ycc_to_rgb_offset", "%d/%d",
                 FF_ARRAY_ELEMS(color->ycc_to_rgb_offset), 1,
                 color->ycc_to_rgb_offset[idx].num,
                 color->ycc_to_rgb_offset[idx].den);
    put_list_fmt("rgb_to_lms_matrix", "%d/%d",
                 FF_ARRAY_ELEMS(color->rgb_to_lms_matrix), 1,
                 color->rgb_to_lms_matrix[idx].num,
                 color->rgb_to_lms_matrix[idx].den);
    put_int("signal_eotf", color->signal_eotf);
    put_int("signal_eotf_param0", color->signal_eotf_param0);
    put_int("signal_eotf_param1", color->signal_eotf_param1);
    put_int("signal_eotf_param2", color->signal_eotf_param2);
    put_int("signal_bit_depth", color->signal_bit_depth);
    put_int("signal_color_space", color->signal_color_space);
    put_int("signal_chroma_format", color->signal_chroma_format);
    put_int("signal_full_range_flag", color->signal_full_range_flag);
    put_int("source_min_pq", color->source_min_pq);
    put_int("source_max_pq", color->source_max_pq);
    put_int("source_diagonal", color->source_diagonal);

    av_bprint_finalize(&pbuf, NULL);
  }
}

void Prober::put_dynamic_hdr10_plus(const AVDynamicHDRPlus *metadata) {
  if (!metadata)
    return;
  put_int("application version", metadata->application_version);
  put_int("num_windows", metadata->num_windows);
  for (int n = 1; n < metadata->num_windows; n++) {
    const AVHDRPlusColorTransformParams *params = &metadata->params[n];
    put_q("window_upper_left_corner_x", params->window_upper_left_corner_x,
          '/');
    put_q("window_upper_left_corner_y", params->window_upper_left_corner_y,
          '/');
    put_q("window_lower_right_corner_x", params->window_lower_right_corner_x,
          '/');
    put_q("window_lower_right_corner_y", params->window_lower_right_corner_y,
          '/');
    put_q("window_upper_left_corner_x", params->window_upper_left_corner_x,
          '/');
    put_q("window_upper_left_corner_y", params->window_upper_left_corner_y,
          '/');
    put_int("center_of_ellipse_x", params->center_of_ellipse_x);
    put_int("center_of_ellipse_y", params->center_of_ellipse_y);
    put_int("rotation_angle", params->rotation_angle);
    put_int("semimajor_axis_internal_ellipse",
            params->semimajor_axis_internal_ellipse);
    put_int("semimajor_axis_external_ellipse",
            params->semimajor_axis_external_ellipse);
    put_int("semiminor_axis_external_ellipse",
            params->semiminor_axis_external_ellipse);
    put_int("overlap_process_option", params->overlap_process_option);
  }
  put_q("targeted_system_display_maximum_luminance",
        metadata->targeted_system_display_maximum_luminance, '/');
  if (metadata->targeted_system_display_actual_peak_luminance_flag) {
    put_int("num_rows_targeted_system_display_actual_peak_luminance",
            metadata->num_rows_targeted_system_display_actual_peak_luminance);
    put_int("num_cols_targeted_system_display_actual_peak_luminance",
            metadata->num_cols_targeted_system_display_actual_peak_luminance);
    for (int i = 0;
         i < metadata->num_rows_targeted_system_display_actual_peak_luminance;
         i++) {
      for (int j = 0;
           j < metadata->num_cols_targeted_system_display_actual_peak_luminance;
           j++) {
        put_q("targeted_system_display_actual_peak_luminance",
              metadata->targeted_system_display_actual_peak_luminance[i][j],
              '/');
      }
    }
  }
  for (int n = 0; n < metadata->num_windows; n++) {
    const AVHDRPlusColorTransformParams *params = &metadata->params[n];
    for (int i = 0; i < 3; i++) {
      put_q("maxscl", params->maxscl[i], '/');
    }
    put_q("average_maxrgb", params->average_maxrgb, '/');
    put_int("num_distribution_maxrgb_percentiles",
            params->num_distribution_maxrgb_percentiles);
    for (int i = 0; i < params->num_distribution_maxrgb_percentiles; i++) {
      put_int("distribution_maxrgb_percentage",
              params->distribution_maxrgb[i].percentage);
      put_q("distribution_maxrgb_percentile",
            params->distribution_maxrgb[i].percentile, '/');
    }
    put_q("fraction_bright_pixels", params->fraction_bright_pixels, '/');
  }
  if (metadata->mastering_display_actual_peak_luminance_flag) {
    put_int("num_rows_mastering_display_actual_peak_luminance",
            metadata->num_rows_mastering_display_actual_peak_luminance);
    put_int("num_cols_mastering_display_actual_peak_luminance",
            metadata->num_cols_mastering_display_actual_peak_luminance);
    for (int i = 0;
         i < metadata->num_rows_mastering_display_actual_peak_luminance; i++) {
      for (int j = 0;
           j < metadata->num_cols_mastering_display_actual_peak_luminance;
           j++) {
        put_q("mastering_display_actual_peak_luminance",
              metadata->mastering_display_actual_peak_luminance[i][j], '/');
      }
    }
  }

  for (int n = 0; n < metadata->num_windows; n++) {
    const AVHDRPlusColorTransformParams *params = &metadata->params[n];
    if (params->tone_mapping_flag) {
      put_q("knee_point_x", params->knee_point_x, '/');
      put_q("knee_point_y", params->knee_point_y, '/');
      put_int("num_bezier_curve_anchors", params->num_bezier_curve_anchors);
      for (int i = 0; i < params->num_bezier_curve_anchors; i++) {
        put_q("bezier_curve_anchors", params->bezier_curve_anchors[i], '/');
      }
    }
    if (params->color_saturation_mapping_flag) {
      put_q("color_saturation_weight", params->color_saturation_weight, '/');
    }
  }
}

void Prober::put_dynamic_hdr_vivid(const AVDynamicHDRVivid *metadata) {
  if (!metadata) {
    return;
  }
  put_int("system_start_code", metadata->system_start_code);
  put_int("num_windows", metadata->num_windows);

  for (int n = 0; n < metadata->num_windows; n++) {
    const AVHDRVividColorTransformParams *params = &metadata->params[n];

    put_q("minimum_maxrgb", params->minimum_maxrgb, '/');
    put_q("average_maxrgb", params->average_maxrgb, '/');
    put_q("variance_maxrgb", params->variance_maxrgb, '/');
    put_q("maximum_maxrgb", params->maximum_maxrgb, '/');
  }

  for (int n = 0; n < metadata->num_windows; n++) {
    const AVHDRVividColorTransformParams *params = &metadata->params[n];

    put_int("tone_mapping_mode_flag", params->tone_mapping_mode_flag);
    if (params->tone_mapping_mode_flag) {
      put_int("tone_mapping_param_num", params->tone_mapping_param_num);
      for (int i = 0; i < params->tone_mapping_param_num; i++) {
        const AVHDRVividColorToneMappingParams *tm_params =
            &params->tm_params[i];

        put_q("targeted_system_display_maximum_luminance",
              tm_params->targeted_system_display_maximum_luminance, '/');
        put_int("base_enable_flag", tm_params->base_enable_flag);
        if (tm_params->base_enable_flag) {
          put_q("base_param_m_p", tm_params->base_param_m_p, '/');
          put_q("base_param_m_m", tm_params->base_param_m_m, '/');
          put_q("base_param_m_a", tm_params->base_param_m_a, '/');
          put_q("base_param_m_b", tm_params->base_param_m_b, '/');
          put_q("base_param_m_n", tm_params->base_param_m_n, '/');

          put_int("base_param_k1", tm_params->base_param_k1);
          put_int("base_param_k2", tm_params->base_param_k2);
          put_int("base_param_k3", tm_params->base_param_k3);
          put_int("base_param_Delta_enable_mode",
                  tm_params->base_param_Delta_enable_mode);
          put_q("base_param_Delta", tm_params->base_param_Delta, '/');
        }
        put_int("3Spline_enable_flag", tm_params->three_Spline_enable_flag);
        if (tm_params->three_Spline_enable_flag) {
          put_int("3Spline_num", tm_params->three_Spline_num);

          for (int j = 0; j < tm_params->three_Spline_num; j++) {
            const AVHDRVivid3SplineParams *three_spline =
                &tm_params->three_spline[j];
            put_int("3Spline_TH_mode", three_spline->th_mode);
            if (three_spline->th_mode == 0 || three_spline->th_mode == 2)
              put_q("3Spline_TH_enable_MB", three_spline->th_enable_mb, '/');
            put_q("3Spline_TH_enable", three_spline->th_enable, '/');
            put_q("3Spline_TH_Delta1", three_spline->th_delta1, '/');
            put_q("3Spline_TH_Delta2", three_spline->th_delta2, '/');
            put_q("3Spline_enable_Strength", three_spline->enable_strength,
                  '/');
          }
        }
      }
    }

    put_int("color_saturation_mapping_flag",
            params->color_saturation_mapping_flag);
    if (params->color_saturation_mapping_flag) {
      put_int("color_saturation_num", params->color_saturation_num);
      for (int i = 0; i < params->color_saturation_num; i++) {
        put_q("color_saturation_gain", params->color_saturation_gain[i], '/');
      }
    }
  }
}

void Prober::put_ambient_viewing_environment(
    const AVAmbientViewingEnvironment *env) {
  if (!env) {
    return;
  }

  put_q("ambient_illuminance", env->ambient_illuminance, '/');
  put_q("ambient_light_x", env->ambient_light_x, '/');
  put_q("ambient_light_y", env->ambient_light_y, '/');
}

void Prober::put_film_grain_params(const AVFilmGrainParams *fgp) {
  const char *color_range, *color_primaries, *color_trc, *color_space;
  const char *const film_grain_type_names[] = {
      [AV_FILM_GRAIN_PARAMS_NONE] = "none",
      [AV_FILM_GRAIN_PARAMS_AV1] = "av1",
      [AV_FILM_GRAIN_PARAMS_H274] = "h274",
  };

  AVBPrint pbuf;
  if (!fgp || fgp->type >= FF_ARRAY_ELEMS(film_grain_type_names))
    return;

  color_range = av_color_range_name(fgp->color_range);
  color_primaries = av_color_primaries_name(fgp->color_primaries);
  color_trc = av_color_transfer_name(fgp->color_trc);
  color_space = av_color_space_name(fgp->color_space);

  av_bprint_init(&pbuf, 1, AV_BPRINT_SIZE_UNLIMITED);
  put_str("type", film_grain_type_names[fgp->type]);
  put_fmt("seed", "%" PRIu64, fgp->seed);
  put_int("width", fgp->width);
  put_int("height", fgp->height);
  put_int("subsampling_x", fgp->subsampling_x);
  put_int("subsampling_y", fgp->subsampling_y);
  put_str("color_range", color_range ? color_range : "unknown");
  put_str("color_primaries", color_primaries ? color_primaries : "unknown");
  put_str("color_trc", color_trc ? color_trc : "unknown");
  put_str("color_space", color_space ? color_space : "unknown");

  switch (fgp->type) {
  case AV_FILM_GRAIN_PARAMS_NONE:
    break;
  case AV_FILM_GRAIN_PARAMS_AV1: {
    const AVFilmGrainAOMParams *aom = &fgp->codec.aom;
    const int num_ar_coeffs_y = 2 * aom->ar_coeff_lag * (aom->ar_coeff_lag + 1);
    const int num_ar_coeffs_uv = num_ar_coeffs_y + !!aom->num_y_points;
    put_int("chroma_scaling_from_luma", aom->chroma_scaling_from_luma);
    put_int("scaling_shift", aom->scaling_shift);
    put_int("ar_coeff_lag", aom->ar_coeff_lag);
    put_int("ar_coeff_shift", aom->ar_coeff_shift);
    put_int("grain_scale_shift", aom->grain_scale_shift);
    put_int("overlap_flag", aom->overlap_flag);
    put_int("limit_output_range", aom->limit_output_range);

    if (aom->num_y_points) {

      put_int("bit_depth_luma", fgp->bit_depth_luma);
      put_list_fmt("y_points_value", "%" PRIu8, aom->num_y_points, 1,
                   aom->y_points[idx][0]);
      put_list_fmt("y_points_scaling", "%" PRIu8, aom->num_y_points, 1,
                   aom->y_points[idx][1]);
      put_list_fmt("ar_coeffs_y", "%" PRId8, num_ar_coeffs_y, 1,
                   aom->ar_coeffs_y[idx]);

      // SECTION_ID_FRAME_SIDE_DATA_COMPONENT
    }

    for (int uv = 0; uv < 2; uv++) {
      if (!aom->num_uv_points[uv] && !aom->chroma_scaling_from_luma)
        continue;

      put_int("bit_depth_chroma", fgp->bit_depth_chroma);
      put_list_fmt("uv_points_value", "%" PRIu8, aom->num_uv_points[uv], 1,
                   aom->uv_points[uv][idx][0]);
      put_list_fmt("uv_points_scaling", "%" PRIu8, aom->num_uv_points[uv], 1,
                   aom->uv_points[uv][idx][1]);
      put_list_fmt("ar_coeffs_uv", "%" PRId8, num_ar_coeffs_uv, 1,
                   aom->ar_coeffs_uv[uv][idx]);
      put_int("uv_mult", aom->uv_mult[uv]);
      put_int("uv_mult_luma", aom->uv_mult_luma[uv]);
      put_int("uv_offset", aom->uv_offset[uv]);

      // SECTION_ID_FRAME_SIDE_DATA_COMPONENT
    }

    // SECTION_ID_FRAME_SIDE_DATA_COMPONENT_LIST

    break;
  }
  case AV_FILM_GRAIN_PARAMS_H274: {
    const AVFilmGrainH274Params *h274 = &fgp->codec.h274;
    put_int("model_id", h274->model_id);
    put_int("blending_mode_id", h274->blending_mode_id);
    put_int("log2_scale_factor", h274->log2_scale_factor);

    for (int c = 0; c < 3; c++) {
      if (!h274->component_model_present[c])
        continue;

      put_int(c ? "bit_depth_chroma" : "bit_depth_luma",
              c ? fgp->bit_depth_chroma : fgp->bit_depth_luma);

      for (int i = 0; i < h274->num_intensity_intervals[c]; i++) {

        put_int("intensity_interval_lower_bound",
                h274->intensity_interval_lower_bound[c][i]);
        put_int("intensity_interval_upper_bound",
                h274->intensity_interval_upper_bound[c][i]);
        put_list_fmt("comp_model_value", "%" PRId16, h274->num_model_values[c],
                     1, h274->comp_model_value[c][i][idx]);

        // SECTION_ID_FRAME_SIDE_DATA_PIECE
      }

      // SECTION_ID_FRAME_SIDE_DATA_PIECE_LIST

      // SECTION_ID_FRAME_SIDE_DATA_COMPONENT
    }

    // SECTION_ID_FRAME_SIDE_DATA_COMPONENT_LIST

    break;
  }
  }

  av_bprint_finalize(&pbuf, NULL);
}

void Prober::put_pkt_side_data(AVCodecParameters *par,
                               const AVPacketSideData *sd, SectionID id_data) {
  const char *name = av_packet_side_data_name(sd->type);

  put_str("side_data_type", name ? name : "unknown");
  if (sd->type == AV_PKT_DATA_DISPLAYMATRIX && sd->size >= 9 * 4) {
    double rotation = av_display_rotation_get((int32_t *)sd->data);
    if (isnan(rotation))
      rotation = 0;
    put_int("rotation", rotation);
  } else if (sd->type == AV_PKT_DATA_STEREO3D) {
    const AVStereo3D *stereo = (AVStereo3D *)sd->data;
    put_str("type", av_stereo3d_type_name(stereo->type));
    put_int("inverted", !!(stereo->flags & AV_STEREO3D_FLAG_INVERT));
    put_str("view", av_stereo3d_view_name(stereo->view));
    put_str("primary_eye", av_stereo3d_primary_eye_name(stereo->primary_eye));
    put_int("baseline", stereo->baseline);
    put_q("horizontal_disparity_adjustment",
          stereo->horizontal_disparity_adjustment, '/');
    put_q("horizontal_field_of_view", stereo->horizontal_field_of_view, '/');
  } else if (sd->type == AV_PKT_DATA_SPHERICAL) {
    const AVSphericalMapping *spherical = (AVSphericalMapping *)sd->data;
    put_str("projection", av_spherical_projection_name(spherical->projection));
    if (spherical->projection == AV_SPHERICAL_CUBEMAP) {
      put_int("padding", spherical->padding);
    } else if (spherical->projection == AV_SPHERICAL_EQUIRECTANGULAR_TILE) {
      size_t l, t, r, b;
      av_spherical_tile_bounds(spherical, par->width, par->height, &l, &t, &r,
                               &b);
      put_int("bound_left", l);
      put_int("bound_top", t);
      put_int("bound_right", r);
      put_int("bound_bottom", b);
    }

    put_int("yaw", (double)spherical->yaw / (1 << 16));
    put_int("pitch", (double)spherical->pitch / (1 << 16));
    put_int("roll", (double)spherical->roll / (1 << 16));
  } else if (sd->type == AV_PKT_DATA_SKIP_SAMPLES && sd->size == 10) {
    put_int("skip_samples", AV_RL32(sd->data));
    put_int("discard_padding", AV_RL32(sd->data + 4));
    put_int("skip_reason", AV_RL8(sd->data + 8));
    put_int("discard_reason", AV_RL8(sd->data + 9));
  } else if (sd->type == AV_PKT_DATA_MASTERING_DISPLAY_METADATA) {
    AVMasteringDisplayMetadata *metadata =
        (AVMasteringDisplayMetadata *)sd->data;

    if (metadata->has_primaries) {
      put_q("red_x", metadata->display_primaries[0][0], '/');
      put_q("red_y", metadata->display_primaries[0][1], '/');
      put_q("green_x", metadata->display_primaries[1][0], '/');
      put_q("green_y", metadata->display_primaries[1][1], '/');
      put_q("blue_x", metadata->display_primaries[2][0], '/');
      put_q("blue_y", metadata->display_primaries[2][1], '/');

      put_q("white_point_x", metadata->white_point[0], '/');
      put_q("white_point_y", metadata->white_point[1], '/');
    }

    if (metadata->has_luminance) {
      put_q("min_luminance", metadata->min_luminance, '/');
      put_q("max_luminance", metadata->max_luminance, '/');
    }
  } else if (sd->type == AV_PKT_DATA_CONTENT_LIGHT_LEVEL) {
    AVContentLightMetadata *metadata = (AVContentLightMetadata *)sd->data;
    put_int("max_content", metadata->MaxCLL);
    put_int("max_average", metadata->MaxFALL);
  } else if (sd->type == AV_PKT_DATA_AMBIENT_VIEWING_ENVIRONMENT) {
    put_ambient_viewing_environment(
        (const AVAmbientViewingEnvironment *)sd->data);
  } else if (sd->type == AV_PKT_DATA_DYNAMIC_HDR10_PLUS) {
    AVDynamicHDRPlus *metadata = (AVDynamicHDRPlus *)sd->data;
    put_dynamic_hdr10_plus(metadata);
  } else if (sd->type == AV_PKT_DATA_DOVI_CONF) {
    AVDOVIDecoderConfigurationRecord *dovi =
        (AVDOVIDecoderConfigurationRecord *)sd->data;
    const char *comp = "unknown";
    put_int("dv_version_major", dovi->dv_version_major);
    put_int("dv_version_minor", dovi->dv_version_minor);
    put_int("dv_profile", dovi->dv_profile);
    put_int("dv_level", dovi->dv_level);
    put_int("rpu_present_flag", dovi->rpu_present_flag);
    put_int("el_present_flag", dovi->el_present_flag);
    put_int("bl_present_flag", dovi->bl_present_flag);
    put_int("dv_bl_signal_compatibility_id",
            dovi->dv_bl_signal_compatibility_id);
    switch (dovi->dv_md_compression) {
    case AV_DOVI_COMPRESSION_NONE:
      comp = "none";
      break;
    case AV_DOVI_COMPRESSION_LIMITED:
      comp = "limited";
      break;
    case AV_DOVI_COMPRESSION_RESERVED:
      comp = "reserved";
      break;
    case AV_DOVI_COMPRESSION_EXTENDED:
      comp = "extended";
      break;
    }
    put_str("dv_md_compression", comp);
  } else if (sd->type == AV_PKT_DATA_AUDIO_SERVICE_TYPE) {
    enum AVAudioServiceType *t = (enum AVAudioServiceType *)sd->data;
    put_int("service_type", *t);
  } else if (sd->type == AV_PKT_DATA_MPEGTS_STREAM_ID) {
    put_int("id", *sd->data);
  } else if (sd->type == AV_PKT_DATA_CPB_PROPERTIES) {
    const AVCPBProperties *prop = (AVCPBProperties *)sd->data;
    put_int("max_bitrate", prop->max_bitrate);
    put_int("min_bitrate", prop->min_bitrate);
    put_int("avg_bitrate", prop->avg_bitrate);
    put_int("buffer_size", prop->buffer_size);
    put_int("vbv_delay", prop->vbv_delay);
  } else if (sd->type == AV_PKT_DATA_WEBVTT_IDENTIFIER ||
             sd->type == AV_PKT_DATA_WEBVTT_SETTINGS) {
    // //avtext_put_data_hash("data_hash", sd->data, sd->size);
  } else if (sd->type == AV_PKT_DATA_FRAME_CROPPING &&
             sd->size >= sizeof(uint32_t) * 4) {
    put_int("crop_top", AV_RL32(sd->data));
    put_int("crop_bottom", AV_RL32(sd->data + 4));
    put_int("crop_left", AV_RL32(sd->data + 8));
    put_int("crop_right", AV_RL32(sd->data + 12));
  } else if (sd->type == AV_PKT_DATA_AFD && sd->size > 0) {
    put_int("active_format", *sd->data);
  }
}

void Prober::put_private_data(void *priv_data) {
  const AVOption *opt = NULL;
  while ((opt = av_opt_next(priv_data, opt))) {
    uint8_t *str;
    if (!(opt->flags & AV_OPT_FLAG_EXPORT))
      continue;
    if (av_opt_get(priv_data, opt->name, 0, &str) >= 0) {
      put_str(opt->name, std::string((char *)str));
      av_free(str);
    }
  }
}

void Prober::put_pixel_format(enum AVPixelFormat pix_fmt) {
  const char *s = av_get_pix_fmt_name(pix_fmt);
  enum AVPixelFormat swapped_pix_fmt;

  if (!s) {
    put_str_opt("pix_fmt", "unknown");
  } else if (!0 || (swapped_pix_fmt = av_pix_fmt_swap_endianness(pix_fmt)) ==
                       AV_PIX_FMT_NONE) {
    put_str("pix_fmt", s);
  } else {
    const char *s2 = av_get_pix_fmt_name(swapped_pix_fmt);
    char buf[128];
    size_t i = 0;

    while (s[i] && s[i] == s2[i] && i < sizeof(buf) - 1) {
      buf[i] = s[i];
      i++;
    }
    buf[i] = '\0';

    put_str("pix_fmt", buf);
  }
}

void Prober::put_color_range(enum AVColorRange color_range) {
  const char *val = av_color_range_name(color_range);
  if (!val || color_range == AVCOL_RANGE_UNSPECIFIED) {
    put_str_opt("color_range", "unknown");
  } else {
    put_str("color_range", val);
  }
}

void Prober::put_color_space(enum AVColorSpace color_space) {
  const char *val = av_color_space_name(color_space);
  if (!val || color_space == AVCOL_SPC_UNSPECIFIED) {
    put_str_opt("color_space", "unknown");
  } else {
    put_str("color_space", val);
  }
}

void Prober::put_primaries(enum AVColorPrimaries color_primaries) {
  const char *val = av_color_primaries_name(color_primaries);
  if (!val || color_primaries == AVCOL_PRI_UNSPECIFIED) {
    put_str_opt("color_primaries", "unknown");
  } else {
    put_str("color_primaries", val);
  }
}

void Prober::put_color_trc(enum AVColorTransferCharacteristic color_trc) {
  const char *val = av_color_transfer_name(color_trc);
  if (!val || color_trc == AVCOL_TRC_UNSPECIFIED) {
    put_str_opt("color_transfer", "unknown");
  } else {
    put_str("color_transfer", val);
  }
}

void Prober::put_chroma_location(enum AVChromaLocation chroma_location) {
  const char *val = av_chroma_location_name(chroma_location);
  if (!val || chroma_location == AVCHROMA_LOC_UNSPECIFIED) {
    put_str_opt("chroma_location", "unspecified");
  } else {
    put_str("chroma_location", val);
  }
}

void Prober::clear_log(int need_lock) {
  int i;

  if (need_lock)
    log_mutex.lock();
  for (i = 0; i < log_buffer_size; i++) {
    av_freep(&log_buffer[i].context_name);
    av_freep(&log_buffer[i].parent_name);
    av_freep(&log_buffer[i].log_message);
  }
  log_buffer_size = 0;
  if (need_lock)
    log_mutex.unlock();
}

int Prober::show_log(int section_ids, int section_id, int log_level) {
  int i;
  log_mutex.lock();
  if (!log_buffer_size) {
    log_mutex.unlock();
    return 0;
  }

  for (i = 0; i < log_buffer_size; i++) {
    if (log_buffer[i].log_level <= log_level) {

      put_str("context", log_buffer[i].context_name);
      put_int("level", log_buffer[i].log_level);
      put_int("category", log_buffer[i].category);
      if (log_buffer[i].parent_name) {
        put_str("parent_context", log_buffer[i].parent_name);
        put_int("parent_category", log_buffer[i].parent_category);
      } else {
        put_str_opt("parent_context", "N/A");
        put_str_opt("parent_category", "N/A");
      }
      put_str("message", log_buffer[i].log_message);
    }
  }
  clear_log(0);
  log_mutex.unlock();

  return 0;
}

void Prober::show_packet(InputFile *ifile, AVPacket *pkt, int packet_idx) {
  AVStream *st = ifile->streams[pkt->stream_index].st;
  AVBPrint pbuf;
  const char *s;

  av_bprint_init(&pbuf, 1, AV_BPRINT_SIZE_UNLIMITED);

  s = av_get_media_type_string(st->codecpar->codec_type);
  if (s)
    put_str("codec_type", s);
  else
    put_str_opt("codec_type", "unknown");
  put_int("stream_index", pkt->stream_index);
  put_ts("pts", pkt->pts);
  put_time("pts_time", pkt->pts, &st->time_base);
  put_ts("dts", pkt->dts);
  put_time("dts_time", pkt->dts, &st->time_base);
  put_duration_ts("duration", pkt->duration);
  put_duration_time("duration_time", pkt->duration, &st->time_base);
  put_val("size", pkt->size, unit_byte_str);
  if (pkt->pos != -1)
    put_fmt("pos", "%" PRId64, pkt->pos);
  else
    put_str_opt("pos", "N/A");
  put_fmt("flags", "%c%c%c", pkt->flags & AV_PKT_FLAG_KEY ? 'K' : '_',
          pkt->flags & AV_PKT_FLAG_DISCARD ? 'D' : '_',
          pkt->flags & AV_PKT_FLAG_CORRUPT ? 'C' : '_');
  // avtext_put_data_hash("data_hash", pkt->data, pkt->size);

  if (pkt->side_data_elems) {
    size_t size;
    const uint8_t *side_metadata;

    side_metadata =
        av_packet_get_side_data(pkt, AV_PKT_DATA_STRINGS_METADATA, &size);
    if (side_metadata && size && 0) {
      AVDictionary *dict = NULL;
      if (av_packet_unpack_dictionary(side_metadata, size, &dict) >= 0) {
        show_tags(dict, SECTION_ID_PACKET_TAGS);
      }
      av_dict_free(&dict);
    }

    for (int i = 0; i < pkt->side_data_elems; i++) {
      put_pkt_side_data(st->codecpar, &pkt->side_data[i],
                        SECTION_ID_PACKET_SIDE_DATA);
    }
  }

  av_bprint_finalize(&pbuf, NULL);
  fflush(stdout);
}

void Prober::show_subtitle(AVSubtitle *sub, AVStream *stream,
                           AVFormatContext *fmt_ctx) {
  AVBPrint pbuf;

  av_bprint_init(&pbuf, 1, AV_BPRINT_SIZE_UNLIMITED);

  put_str("media_type", "subtitle");
  put_ts("pts", sub->pts);
  AVRational time_q = AV_TIME_BASE_Q;
  put_time("pts_time", sub->pts, &time_q);
  put_int("format", sub->format);
  put_int("start_display_time", sub->start_display_time);
  put_int("end_display_time", sub->end_display_time);
  put_int("num_rects", sub->num_rects);

  av_bprint_finalize(&pbuf, NULL);
  fflush(stdout);
}

void Prober::put_frame_side_data(const AVFrame *frame, const AVStream *stream) {

  for (int i = 0; i < frame->nb_side_data; i++) {
    const AVFrameSideData *sd = frame->side_data[i];
    const char *name;

    name = av_frame_side_data_name(sd->type);
    put_str("side_data_type", name ? name : "unknown");
    if (sd->type == AV_FRAME_DATA_DISPLAYMATRIX && sd->size >= 9 * 4) {
      double rotation = av_display_rotation_get((int32_t *)sd->data);
      if (isnan(rotation))
        rotation = 0;
      // avtext_put_integers("displaymatrix", sd->data, 9, " %11d", 3, 4, 1);
      put_int("rotation", rotation);
    } else if (sd->type == AV_FRAME_DATA_AFD && sd->size > 0) {
      put_int("active_format", *sd->data);
    } else if (sd->type == AV_FRAME_DATA_GOP_TIMECODE && sd->size >= 8) {
      char tcbuf[AV_TIMECODE_STR_SIZE];
      av_timecode_make_mpeg_tc_string(tcbuf, *(int64_t *)(sd->data));
      put_str("timecode", tcbuf);
    } else if (sd->type == AV_FRAME_DATA_S12M_TIMECODE && sd->size == 16) {
      uint32_t *tc = (uint32_t *)sd->data;
      int m = FFMIN(tc[0], 3);
      for (int j = 1; j <= m; j++) {
        char tcbuf[AV_TIMECODE_STR_SIZE];
        av_timecode_make_smpte_tc_string2(tcbuf, stream->avg_frame_rate, tc[j],
                                          0, 0);

        put_str("value", tcbuf);
      }

    } else if (sd->type == AV_FRAME_DATA_MASTERING_DISPLAY_METADATA) {
      AVMasteringDisplayMetadata *metadata =
          (AVMasteringDisplayMetadata *)sd->data;

      if (metadata->has_primaries) {
        put_q("red_x", metadata->display_primaries[0][0], '/');
        put_q("red_y", metadata->display_primaries[0][1], '/');
        put_q("green_x", metadata->display_primaries[1][0], '/');
        put_q("green_y", metadata->display_primaries[1][1], '/');
        put_q("blue_x", metadata->display_primaries[2][0], '/');
        put_q("blue_y", metadata->display_primaries[2][1], '/');

        put_q("white_point_x", metadata->white_point[0], '/');
        put_q("white_point_y", metadata->white_point[1], '/');
      }

      if (metadata->has_luminance) {
        put_q("min_luminance", metadata->min_luminance, '/');
        put_q("max_luminance", metadata->max_luminance, '/');
      }
    } else if (sd->type == AV_FRAME_DATA_DYNAMIC_HDR_PLUS) {
      AVDynamicHDRPlus *metadata = (AVDynamicHDRPlus *)sd->data;
      put_dynamic_hdr10_plus(metadata);
    } else if (sd->type == AV_FRAME_DATA_CONTENT_LIGHT_LEVEL) {
      AVContentLightMetadata *metadata = (AVContentLightMetadata *)sd->data;
      put_int("max_content", metadata->MaxCLL);
      put_int("max_average", metadata->MaxFALL);
    } else if (sd->type == AV_FRAME_DATA_ICC_PROFILE) {
      const AVDictionaryEntry *tag =
          av_dict_get(sd->metadata, "name", NULL, AV_DICT_MATCH_CASE);
      if (tag)
        put_str(tag->key, tag->value);
      put_int("size", sd->size);
    } else if (sd->type == AV_FRAME_DATA_DOVI_METADATA) {
      put_dovi_metadata((const AVDOVIMetadata *)sd->data);
    } else if (sd->type == AV_FRAME_DATA_DYNAMIC_HDR_VIVID) {
      AVDynamicHDRVivid *metadata = (AVDynamicHDRVivid *)sd->data;
      put_dynamic_hdr_vivid(metadata);
    } else if (sd->type == AV_FRAME_DATA_AMBIENT_VIEWING_ENVIRONMENT) {
      put_ambient_viewing_environment(
          (const AVAmbientViewingEnvironment *)sd->data);
    } else if (sd->type == AV_FRAME_DATA_FILM_GRAIN_PARAMS) {
      AVFilmGrainParams *fgp = (AVFilmGrainParams *)sd->data;
      put_film_grain_params(fgp);
    } else if (sd->type == AV_FRAME_DATA_VIEW_ID) {
      put_int("view_id", *(int *)sd->data);
    }
  }
}

void Prober::show_frame(AVFrame *frame, AVStream *stream,
                        AVFormatContext *fmt_ctx) {
  FrameData *fd =
      frame->opaque_ref ? (FrameData *)frame->opaque_ref->data : NULL;
  AVBPrint pbuf;
  char val_str[128];
  const char *s;

  av_bprint_init(&pbuf, 1, AV_BPRINT_SIZE_UNLIMITED);

  s = av_get_media_type_string(stream->codecpar->codec_type);
  if (s)
    put_str("media_type", s);
  else
    put_str_opt("media_type", "unknown");
  put_int("stream_index", stream->index);
  put_int("key_frame", !!(frame->flags & AV_FRAME_FLAG_KEY));
  put_ts("pts", frame->pts);
  put_time("pts_time", frame->pts, &stream->time_base);
  put_ts("pkt_dts", frame->pkt_dts);
  put_time("pkt_dts_time", frame->pkt_dts, &stream->time_base);
  put_ts("best_effort_timestamp", frame->best_effort_timestamp);
  put_time("best_effort_timestamp_time", frame->best_effort_timestamp,
           &stream->time_base);
  put_duration_ts("duration", frame->duration);
  put_duration_time("duration_time", frame->duration, &stream->time_base);
  if (fd && fd->pkt_pos != -1)
    put_fmt("pkt_pos", "%" PRId64, fd->pkt_pos);
  else
    put_str_opt("pkt_pos", "N/A");
  if (fd && fd->pkt_size != -1)
    put_val("pkt_size", fd->pkt_size, unit_byte_str);
  else
    put_str_opt("pkt_size", "N/A");

  switch (stream->codecpar->codec_type) {
    AVRational sar;

  case AVMEDIA_TYPE_VIDEO:
    put_int("width", frame->width);
    put_int("height", frame->height);
    put_int("crop_top", frame->crop_top);
    put_int("crop_bottom", frame->crop_bottom);
    put_int("crop_left", frame->crop_left);
    put_int("crop_right", frame->crop_right);
    put_pixel_format((enum AVPixelFormat)frame->format);
    sar = av_guess_sample_aspect_ratio(fmt_ctx, stream, frame);
    if (sar.num) {
      put_q("sample_aspect_ratio", sar, ':');
    } else {
      put_str_opt("sample_aspect_ratio", "N/A");
    }
    put_fmt("pict_type", "%c", av_get_picture_type_char(frame->pict_type));
    put_int("interlaced_frame", !!(frame->flags & AV_FRAME_FLAG_INTERLACED));
    put_int("top_field_first",
            !!(frame->flags & AV_FRAME_FLAG_TOP_FIELD_FIRST));
    // put_int("lossless", !!(frame->flags & AV_FRAME_FLAG_LOSSLESS));
    put_int("repeat_pict", frame->repeat_pict);

    put_color_range(frame->color_range);
    put_color_space(frame->colorspace);
    put_primaries(frame->color_primaries);
    put_color_trc(frame->color_trc);
    put_chroma_location(frame->chroma_location);
    break;

  case AVMEDIA_TYPE_AUDIO:
    s = av_get_sample_fmt_name((enum AVSampleFormat)frame->format);
    if (s)
      put_str("sample_fmt", s);
    else
      put_str_opt("sample_fmt", "unknown");
    put_int("nb_samples", frame->nb_samples);
    put_int("channels", frame->ch_layout.nb_channels);
    if (frame->ch_layout.order != AV_CHANNEL_ORDER_UNSPEC) {
      av_channel_layout_describe(&frame->ch_layout, val_str, sizeof(val_str));
      put_str("channel_layout", val_str);
    } else
      put_str_opt("channel_layout", "unknown");
    break;
  default:
    break;
  }
  if (frame->nb_side_data)
    put_frame_side_data(frame, stream);

  av_bprint_finalize(&pbuf, NULL);
  fflush(stdout);
}

int Prober::process_frame(InputFile *ifile, AVFrame *frame, const AVPacket *pkt,
                          int *packet_new) {
  AVFormatContext *fmt_ctx = ifile->fmt_ctx;
  AVCodecContext *dec_ctx = ifile->streams[pkt->stream_index].dec_ctx;
  AVCodecParameters *par = ifile->streams[pkt->stream_index].st->codecpar;
  AVSubtitle sub;
  int ret = 0, got_frame = 0;

  clear_log(1);
  if (dec_ctx) {
    switch (par->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
    case AVMEDIA_TYPE_AUDIO:
      if (*packet_new) {
        ret = avcodec_send_packet(dec_ctx, pkt);
        if (ret == AVERROR(EAGAIN)) {
          ret = 0;
        } else if (ret >= 0 || ret == AVERROR_EOF) {
          ret = 0;
          *packet_new = 0;
        }
      }
      if (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret >= 0) {
          got_frame = 1;
        } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          ret = 0;
        }
      }
      break;

    case AVMEDIA_TYPE_SUBTITLE:
      if (*packet_new)
        ret = avcodec_decode_subtitle2(dec_ctx, &sub, &got_frame, pkt);
      *packet_new = 0;
      break;
    default:
      *packet_new = 0;
    }
  } else {
    *packet_new = 0;
  }

  if (ret < 0)
    return ret;
  if (got_frame) {
    int is_sub = (par->codec_type == AVMEDIA_TYPE_SUBTITLE);
    nb_streams_frames[pkt->stream_index]++;
    if (!is_sub && 1) {
      for (int i = 0; i < frame->nb_side_data; i++) {
        if (frame->side_data[i]->type == AV_FRAME_DATA_A53_CC)
          streams_with_closed_captions[pkt->stream_index] = 1;
        else if (frame->side_data[i]->type == AV_FRAME_DATA_FILM_GRAIN_PARAMS)
          streams_with_film_grain[pkt->stream_index] = 1;
      }
    }

    if (is_sub)
      avsubtitle_free(&sub);
  }
  return got_frame || *packet_new;
}

void Prober::log_read_interval(const ReadInterval *interval, void *log_ctx,
                               int log_level) {
  av_log(log_ctx, log_level, "id:%d", interval->id);

  if (interval->has_start) {

    AVRational time_q = AV_TIME_BASE_Q;
    av_log(log_ctx, log_level, " start:%s%s",
           interval->start_is_offset ? "+" : "",
           av_ts2timestr(interval->start, &time_q));
  } else {
    av_log(log_ctx, log_level, " start:N/A");
  }

  if (interval->has_end) {
    av_log(log_ctx, log_level, " end:%s", interval->end_is_offset ? "+" : "");
    if (interval->duration_frames) {
      av_log(log_ctx, log_level, "#%" PRId64, interval->end);
    } else {

      AVRational time_q = AV_TIME_BASE_Q;
      av_log(log_ctx, log_level, "%s", av_ts2timestr(interval->end, &time_q));
    }
  } else {
    av_log(log_ctx, log_level, " end:N/A");
  }

  av_log(log_ctx, log_level, "\n");
}

int Prober::read_interval_packets(InputFile *ifile,
                                  const ReadInterval *interval,
                                  int64_t *cur_ts) {
  AVFormatContext *fmt_ctx = ifile->fmt_ctx;
  AVPacket *pkt = NULL;
  AVFrame *frame = NULL;
  int ret = 0, i = 0, frame_count = 0;
  int64_t start = -INT64_MAX, end = interval->end;
  int has_start = 0, has_end = interval->has_end && !interval->end_is_offset;

  av_log(NULL, AV_LOG_VERBOSE, "Processing read interval ");
  log_read_interval(interval, NULL, AV_LOG_VERBOSE);

  if (interval->has_start) {
    int64_t target;
    if (interval->start_is_offset) {
      if (*cur_ts == AV_NOPTS_VALUE) {
        av_log(NULL, AV_LOG_ERROR,
               "Could not seek to relative position since current "
               "timestamp is not defined\n");
        ret = AVERROR(EINVAL);
        goto end;
      }
      target = *cur_ts + interval->start;
    } else {
      target = interval->start;
    }
    AVRational time_q = AV_TIME_BASE_Q;

    av_log(NULL, AV_LOG_VERBOSE, "Seeking to read interval start point %s\n",
           av_ts2timestr(target, &time_q));
    if ((ret = avformat_seek_file(fmt_ctx, -1, -INT64_MAX, target, INT64_MAX,
                                  0)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Could not seek to position %" PRId64 ": %s\n",
             interval->start, av_err2str(ret));
      goto end;
    }
  }

  frame = av_frame_alloc();
  if (!frame) {
    ret = AVERROR(ENOMEM);
    goto end;
  }
  pkt = av_packet_alloc();
  if (!pkt) {
    ret = AVERROR(ENOMEM);
    goto end;
  }
  while (!av_read_frame(fmt_ctx, pkt)) {
    if (fmt_ctx->nb_streams > nb_streams) {
      REALLOCZ_ARRAY_STREAM(nb_streams_frames, nb_streams, fmt_ctx->nb_streams);
      REALLOCZ_ARRAY_STREAM(nb_streams_packets, nb_streams,
                            fmt_ctx->nb_streams);
      REALLOCZ_ARRAY_STREAM(selected_streams, nb_streams, fmt_ctx->nb_streams);
      REALLOCZ_ARRAY_STREAM(streams_with_closed_captions, nb_streams,
                            fmt_ctx->nb_streams);
      REALLOCZ_ARRAY_STREAM(streams_with_film_grain, nb_streams,
                            fmt_ctx->nb_streams);
      nb_streams = fmt_ctx->nb_streams;
    }
    if (selected_streams[pkt->stream_index]) {
      AVRational tb = ifile->streams[pkt->stream_index].st->time_base;
      int64_t pts = pkt->pts != AV_NOPTS_VALUE ? pkt->pts : pkt->dts;

      if (pts != AV_NOPTS_VALUE) {
        *cur_ts = av_rescale_q(pts, tb, AV_TIME_BASE_Q);
      }

      if (!has_start && *cur_ts != AV_NOPTS_VALUE) {
        start = *cur_ts;
        has_start = 1;
      }

      if (has_start && !has_end && interval->end_is_offset) {
        end = start + interval->end;
        has_end = 1;
      }

      if (interval->end_is_offset && interval->duration_frames) {
        if (frame_count >= interval->end)
          break;
      } else if (has_end && *cur_ts != AV_NOPTS_VALUE && *cur_ts >= end) {
        break;
      }

      frame_count++;
      nb_streams_packets[pkt->stream_index]++;
      int packet_new = 1;
      FrameData *fd;

      pkt->opaque_ref = av_buffer_allocz(sizeof(*fd));
      if (!pkt->opaque_ref) {
        ret = AVERROR(ENOMEM);
        goto end;
      }
      fd = (FrameData *)pkt->opaque_ref->data;
      fd->pkt_pos = pkt->pos;
      fd->pkt_size = pkt->size;

      while (process_frame(ifile, frame, pkt, &packet_new) > 0)
        ;
    }
    av_packet_unref(pkt);
  }
  av_packet_unref(pkt);
  // Flush remaining frames that are cached in the decoder
  for (i = 0; i < ifile->nb_streams; i++) {
    pkt->stream_index = i;
    auto wh = (int){1};
    while (process_frame(ifile, frame, pkt, &wh) > 0)
      ;
    if (ifile->streams[i].dec_ctx)
      avcodec_flush_buffers(ifile->streams[i].dec_ctx);
  }

end:
  av_frame_free(&frame);
  av_packet_free(&pkt);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Could not read packets in interval ");
    log_read_interval(interval, NULL, AV_LOG_ERROR);
  }
  return ret;
}

int Prober::read_packets(InputFile *ifile) {
  AVFormatContext *fmt_ctx = ifile->fmt_ctx;
  int i, ret = 0;
  int64_t cur_ts = fmt_ctx->start_time;

  if (read_intervals_nb == 0) {
    ReadInterval interval = (ReadInterval){.has_start = 0, .has_end = 0};
    ret = read_interval_packets(ifile, &interval, &cur_ts);
  } else {
    for (i = 0; i < read_intervals_nb; i++) {
      ret = read_interval_packets(ifile, &read_intervals[i], &cur_ts);
      if (ret < 0)
        break;
    }
  }

  return ret;
}

void Prober::put_dispositions(uint32_t disposition, SectionID section_id) {

  for (int i = 0; i < sizeof(disposition) * CHAR_BIT; i++) {
    const char *disposition_str = av_disposition_to_string(1U << i);

    if (disposition_str)
      put_int(disposition_str, !!(disposition & (1U << i)));
  }
}

int Prober::show_stream(AVFormatContext *fmt_ctx, int stream_idx,
                        InputStream *ist, int container) {
  AVStream *stream = ist->st;
  AVCodecParameters *par;
  AVCodecContext *dec_ctx;
  char val_str[128];
  const char *s;
  AVRational sar, dar;
  AVBPrint pbuf;
  const AVCodecDescriptor *cd;
  const SectionID section_header[] = {
      SECTION_ID_STREAM,
      SECTION_ID_PROGRAM_STREAM,
      SECTION_ID_STREAM_GROUP_STREAM,
  };
  const SectionID section_disposition[] = {
      SECTION_ID_STREAM_DISPOSITION,
      SECTION_ID_PROGRAM_STREAM_DISPOSITION,
      SECTION_ID_STREAM_GROUP_STREAM_DISPOSITION,
  };
  const SectionID section_tags[] = {
      SECTION_ID_STREAM_TAGS,
      SECTION_ID_PROGRAM_STREAM_TAGS,
      SECTION_ID_STREAM_GROUP_STREAM_TAGS,
  };
  int ret = 0;
  const char *profile = NULL;

  av_assert0(container < FF_ARRAY_ELEMS(section_header));

  av_bprint_init(&pbuf, 1, AV_BPRINT_SIZE_UNLIMITED);

  put_int("index", stream->index);

  par = stream->codecpar;
  dec_ctx = ist->dec_ctx;
  if ((cd = avcodec_descriptor_get(par->codec_id))) {
    put_str("codec_name", cd->name);
    if (!0) {
      put_str("codec_long_name", cd->long_name ? cd->long_name : "unknown");
    }
  } else {
    put_str_opt("codec_name", "unknown");
    if (!0) {
      put_str_opt("codec_long_name", "unknown");
    }
  }

  if (!0 && (profile = avcodec_profile_name(par->codec_id, par->profile)))
    put_str("profile", profile);
  else {
    if (par->profile != AV_PROFILE_UNKNOWN) {
      char profile_num[12];
      snprintf(profile_num, sizeof(profile_num), "%d", par->profile);
      put_str("profile", profile_num);
    } else
      put_str_opt("profile", "unknown");
  }

  s = av_get_media_type_string(par->codec_type);
  if (s)
    put_str("codec_type", s);
  else
    put_str_opt("codec_type", "unknown");

  /* print AVI/FourCC tag */
  put_str("codec_tag_string", av_fourcc2str(par->codec_tag));
  put_fmt("codec_tag", "0x%04" PRIx32, par->codec_tag);

  switch (par->codec_type) {
  case AVMEDIA_TYPE_VIDEO:
    put_int("width", par->width);
    put_int("height", par->height);
    if (dec_ctx) {
      put_int("coded_width", dec_ctx->coded_width);
      put_int("coded_height", dec_ctx->coded_height);

      put_int("closed_captions", streams_with_closed_captions[stream->index]);
      put_int("film_grain", streams_with_film_grain[stream->index]);
    }
    put_int("has_b_frames", par->video_delay);
    // sar = av_guess_sample_aspect_ratio(fmt_ctx, stream, NULL);
    // if (sar.num) {
    //   put_q("sample_aspect_ratio", sar, ':');
    //   av_reduce(&dar.num, &dar.den, (int64_t)par->width * sar.num,
    //             (int64_t)par->height * sar.den, 1024 * 1024);
    //   put_q("display_aspect_ratio", dar, ':');
    // } else {
    //   put_str_opt("sample_aspect_ratio", "N/A");
    //   put_str_opt("display_aspect_ratio", "N/A");
    // }
    put_pixel_format((enum AVPixelFormat)par->format);
    put_int("level", par->level);

    put_color_range(par->color_range);
    put_color_space(par->color_space);
    put_color_trc(par->color_trc);
    put_primaries(par->color_primaries);
    put_chroma_location(par->chroma_location);

    if (par->field_order == AV_FIELD_PROGRESSIVE)
      put_str("field_order", "progressive");
    else if (par->field_order == AV_FIELD_TT)
      put_str("field_order", "tt");
    else if (par->field_order == AV_FIELD_BB)
      put_str("field_order", "bb");
    else if (par->field_order == AV_FIELD_TB)
      put_str("field_order", "tb");
    else if (par->field_order == AV_FIELD_BT)
      put_str("field_order", "bt");
    else
      put_str_opt("field_order", "unknown");

    if (dec_ctx)
      put_int("refs", dec_ctx->refs);
    break;

  case AVMEDIA_TYPE_AUDIO:
    s = av_get_sample_fmt_name((enum AVSampleFormat)par->format);
    if (s)
      put_str("sample_fmt", s);
    else
      put_str_opt("sample_fmt", "unknown");
    put_val("sample_rate", par->sample_rate, unit_hertz_str);
    put_int("channels", par->ch_layout.nb_channels);

    if (par->ch_layout.order != AV_CHANNEL_ORDER_UNSPEC) {
      av_channel_layout_describe(&par->ch_layout, val_str, sizeof(val_str));
      put_str("channel_layout", val_str);
    } else {
      put_str_opt("channel_layout", "unknown");
    }

    put_int("bits_per_sample", av_get_bits_per_sample(par->codec_id));

    put_int("initial_padding", par->initial_padding);
    break;

  case AVMEDIA_TYPE_SUBTITLE:
    if (par->width)
      put_int("width", par->width);
    else
      put_str_opt("width", "N/A");
    if (par->height)
      put_int("height", par->height);
    else
      put_str_opt("height", "N/A");
    break;
  default:
    break;
  }

  if (fmt_ctx->iformat->flags & AVFMT_SHOW_IDS)
    put_fmt("id", "0x%x", stream->id);
  else
    put_str_opt("id", "N/A");
  put_q("r_frame_rate", stream->r_frame_rate, '/');
  put_q("avg_frame_rate", stream->avg_frame_rate, '/');
  put_q("time_base", stream->time_base, '/');
  put_ts("start_pts", stream->start_time);
  put_time("start_time", stream->start_time, &stream->time_base);
  put_ts("duration_ts", stream->duration);
  put_time("duration", stream->duration, &stream->time_base);
  if (par->bit_rate > 0)
    put_val("bit_rate", par->bit_rate, unit_bit_per_second_str);
  else
    put_str_opt("bit_rate", "N/A");
  if (dec_ctx && dec_ctx->rc_max_rate > 0)
    put_val("max_bit_rate", dec_ctx->rc_max_rate, unit_bit_per_second_str);
  else
    put_str_opt("max_bit_rate", "N/A");
  if (dec_ctx && dec_ctx->bits_per_raw_sample > 0)
    put_fmt("bits_per_raw_sample", "%d", dec_ctx->bits_per_raw_sample);
  else
    put_str_opt("bits_per_raw_sample", "N/A");
  if (stream->nb_frames)
    put_fmt("nb_frames", "%" PRId64, stream->nb_frames);
  else
    put_str_opt("nb_frames", "N/A");
  if (nb_streams_frames[stream_idx])
    put_fmt("nb_read_frames", "%" PRIu64, nb_streams_frames[stream_idx]);
  else
    put_str_opt("nb_read_frames", "N/A");
  if (nb_streams_packets[stream_idx])
    put_fmt("nb_read_packets", "%" PRIu64, nb_streams_packets[stream_idx]);
  else
    put_str_opt("nb_read_packets", "N/A");

  if (par->extradata_size > 0) {
    put_int("extradata_size", par->extradata_size);
    // avtext_put_data_hash("extradata_hash", par->extradata,
    // par->extradata_size);
  }

  if (stream->codecpar->nb_coded_side_data) {

    for (int i = 0; i < stream->codecpar->nb_coded_side_data; i++) {
      put_pkt_side_data(stream->codecpar, &stream->codecpar->coded_side_data[i],
                        SECTION_ID_STREAM_SIDE_DATA);
    }
  }

  av_bprint_finalize(&pbuf, NULL);
  fflush(stdout);

  return ret;
}

int Prober::show_streams(InputFile *ifile) {
  AVFormatContext *fmt_ctx = ifile->fmt_ctx;
  int i, ret = 0;

  for (i = 0; i < ifile->nb_streams; i++)
    if (selected_streams[i]) {
      ret = show_stream(fmt_ctx, i, &ifile->streams[i], 0);
      if (ret < 0)
        break;
    }

  return ret;
}

int Prober::show_program(InputFile *ifile, AVProgram *program) {
  AVFormatContext *fmt_ctx = ifile->fmt_ctx;
  int i, ret = 0;

  put_int("program_id", program->id);
  put_int("program_num", program->program_num);
  put_int("nb_streams", program->nb_stream_indexes);
  put_int("pmt_pid", program->pmt_pid);
  put_int("pcr_pid", program->pcr_pid);
  if (ret < 0)
    goto end;

  for (i = 0; i < program->nb_stream_indexes; i++) {
    if (selected_streams[program->stream_index[i]]) {
      ret = show_stream(fmt_ctx, program->stream_index[i],
                        &ifile->streams[program->stream_index[i]], IN_PROGRAM);
      if (ret < 0)
        break;
    }
  }

end:

  return ret;
}

int Prober::show_programs(InputFile *ifile) {
  AVFormatContext *fmt_ctx = ifile->fmt_ctx;
  int i, ret = 0;

  for (i = 0; i < fmt_ctx->nb_programs; i++) {
    AVProgram *program = fmt_ctx->programs[i];
    if (!program)
      continue;
    ret = show_program(ifile, program);
    if (ret < 0)
      break;
  }

  return ret;
}

void Prober::put_tile_grid_params(const AVStreamGroup *stg,
                                  const AVStreamGroupTileGrid *tile_grid) {

  put_int("nb_tiles", tile_grid->nb_tiles);
  put_int("coded_width", tile_grid->coded_width);
  put_int("coded_height", tile_grid->coded_height);
  put_int("horizontal_offset", tile_grid->horizontal_offset);
  put_int("vertical_offset", tile_grid->vertical_offset);
  put_int("width", tile_grid->width);
  put_int("height", tile_grid->height);

  for (int i = 0; i < tile_grid->nb_tiles; i++) {
    put_int("stream_index", tile_grid->offsets[i].idx);
    put_int("tile_horizontal_offset", tile_grid->offsets[i].horizontal);
    put_int("tile_vertical_offset", tile_grid->offsets[i].vertical);
  }
}

void Prober::put_iamf_param_definition(const char *name,
                                       const AVIAMFParamDefinition *param,
                                       SectionID section_id) {
  put_str("name", name);
  put_int("nb_subblocks", param->nb_subblocks);
  put_int("type", param->type);
  put_int("parameter_id", param->parameter_id);
  put_int("parameter_rate", param->parameter_rate);
  put_int("duration", param->duration);
  put_int("constant_subblock_duration", param->constant_subblock_duration);
  if (param->nb_subblocks > 0)

    for (int i = 0; i < param->nb_subblocks; i++) {
      const void *subblock = av_iamf_param_definition_get_subblock(param, i);
      switch (param->type) {
      case AV_IAMF_PARAMETER_DEFINITION_MIX_GAIN: {
        const AVIAMFMixGain *mix = (const AVIAMFMixGain *)subblock;
        put_int("subblock_duration", mix->subblock_duration);
        put_int("animation_type", mix->animation_type);
        put_q("start_point_value", mix->start_point_value, '/');
        put_q("end_point_value", mix->end_point_value, '/');
        put_q("control_point_value", mix->control_point_value, '/');
        put_q("control_point_relative_time", mix->control_point_relative_time,
              '/');
        // parameter_section_id
        break;
      }
      case AV_IAMF_PARAMETER_DEFINITION_DEMIXING: {
        const AVIAMFDemixingInfo *demix = (const AVIAMFDemixingInfo *)subblock;

        put_int("subblock_duration", demix->subblock_duration);
        put_int("dmixp_mode", demix->dmixp_mode);
        // parameter_section_id
        break;
      }
      case AV_IAMF_PARAMETER_DEFINITION_RECON_GAIN: {
        const AVIAMFReconGain *recon = (const AVIAMFReconGain *)subblock;

        put_int("subblock_duration", recon->subblock_duration);
        // parameter_section_id
        break;
      }
      }
    }
  // if (param->nb_subblocks > 0)
  // subsection_id
  // section_id
}

void Prober::put_iamf_audio_element_params(
    const AVStreamGroup *stg, const AVIAMFAudioElement *audio_element) {

  put_int("nb_layers", audio_element->nb_layers);
  put_int("audio_element_type", audio_element->audio_element_type);
  put_int("default_w", audio_element->default_w);

  for (int i = 0; i < audio_element->nb_layers; i++) {
    const AVIAMFLayer *layer = audio_element->layers[i];
    char val_str[128];
    av_channel_layout_describe(&layer->ch_layout, val_str, sizeof(val_str));
    put_str("channel_layout", val_str);
    if (audio_element->audio_element_type ==
        AV_IAMF_AUDIO_ELEMENT_TYPE_CHANNEL) {
      put_int("output_gain_flags", layer->output_gain_flags);
      put_q("output_gain", layer->output_gain, '/');
    } else if (audio_element->audio_element_type ==
               AV_IAMF_AUDIO_ELEMENT_TYPE_SCENE)
      put_int("ambisonics_mode", layer->ambisonics_mode);
    // SECTION_ID_STREAM_GROUP_SUBCOMPONENT
  }
  if (audio_element->demixing_info)
    put_iamf_param_definition("demixing_info", audio_element->demixing_info,
                              SECTION_ID_STREAM_GROUP_SUBCOMPONENT);
  if (audio_element->recon_gain_info)
    put_iamf_param_definition("recon_gain_info", audio_element->recon_gain_info,
                              SECTION_ID_STREAM_GROUP_SUBCOMPONENT);
  // SECTION_ID_STREAM_GROUP_SUBCOMPONENTS
  // SECTION_ID_STREAM_GROUP_COMPONENT
}

void Prober::put_iamf_submix_params(const AVIAMFSubmix *submix) {
  put_int("nb_elements", submix->nb_elements);
  put_int("nb_layouts", submix->nb_layouts);
  put_q("default_mix_gain", submix->default_mix_gain, '/');

  for (int i = 0; i < submix->nb_elements; i++) {
    const AVIAMFSubmixElement *element = submix->elements[i];
    put_int("stream_id", element->audio_element_id);
    put_q("default_mix_gain", element->default_mix_gain, '/');
    put_int("headphones_rendering_mode", element->headphones_rendering_mode);

    if (element->annotations) {
      const AVDictionaryEntry *annotation = NULL;
      while ((annotation = av_dict_iterate(element->annotations, annotation)))
        put_str(annotation->key, annotation->value);
      // SECTION_ID_STREAM_GROUP_SUBPIECE
    }
    if (element->element_mix_config)
      put_iamf_param_definition("element_mix_config",
                                element->element_mix_config,
                                SECTION_ID_STREAM_GROUP_SUBPIECE);
    // SECTION_ID_STREAM_GROUP_SUBPIECES
    // SECTION_ID_STREAM_GROUP_PIECE
  }
  if (submix->output_mix_config)
    put_iamf_param_definition("output_mix_config", submix->output_mix_config,
                              SECTION_ID_STREAM_GROUP_PIECE);
  for (int i = 0; i < submix->nb_layouts; i++) {
    const AVIAMFSubmixLayout *layout = submix->layouts[i];
    char val_str[128];
    av_channel_layout_describe(&layout->sound_system, val_str, sizeof(val_str));
    put_str("sound_system", val_str);
    put_q("integrated_loudness", layout->integrated_loudness, '/');
    put_q("digital_peak", layout->digital_peak, '/');
    put_q("true_peak", layout->true_peak, '/');
    put_q("dialogue_anchored_loudness", layout->dialogue_anchored_loudness,
          '/');
    put_q("album_anchored_loudness", layout->album_anchored_loudness, '/');
    // SECTION_ID_STREAM_GROUP_PIECE
  }
  // SECTION_ID_STREAM_GROUP_PIECES
  // SECTION_ID_STREAM_GROUP_SUBCOMPONENT
}

void Prober::put_iamf_mix_presentation_params(
    const AVStreamGroup *stg, const AVIAMFMixPresentation *mix_presentation) {

  put_int("nb_submixes", mix_presentation->nb_submixes);

  if (mix_presentation->annotations) {
    const AVDictionaryEntry *annotation = NULL;
    while ((annotation =
                av_dict_iterate(mix_presentation->annotations, annotation)))
      put_str(annotation->key, annotation->value);
    // SECTION_ID_STREAM_GROUP_SUBCOMPONENT
  }
  for (int i = 0; i < mix_presentation->nb_submixes; i++)
    put_iamf_submix_params(mix_presentation->submixes[i]);
  // SECTION_ID_STREAM_GROUP_SUBCOMPONENTS
  // SECTION_ID_STREAM_GROUP_COMPONENT
}

void Prober::put_stream_group_params(AVStreamGroup *stg) {

  if (stg->type == AV_STREAM_GROUP_PARAMS_TILE_GRID)
    put_tile_grid_params(stg, stg->params.tile_grid);
  else if (stg->type == AV_STREAM_GROUP_PARAMS_IAMF_AUDIO_ELEMENT)
    put_iamf_audio_element_params(stg, stg->params.iamf_audio_element);
  else if (stg->type == AV_STREAM_GROUP_PARAMS_IAMF_MIX_PRESENTATION)
    put_iamf_mix_presentation_params(stg, stg->params.iamf_mix_presentation);
  // SECTION_ID_STREAM_GROUP_COMPONENTS
}

int Prober::show_stream_group(InputFile *ifile, AVStreamGroup *stg) {
  AVFormatContext *fmt_ctx = ifile->fmt_ctx;
  AVBPrint pbuf;
  int i, ret = 0;

  av_bprint_init(&pbuf, 1, AV_BPRINT_SIZE_UNLIMITED);

  put_int("index", stg->index);
  if (fmt_ctx->iformat->flags & AVFMT_SHOW_IDS)
    put_fmt("id", "0x%" PRIx64, stg->id);
  else
    put_str_opt("id", "N/A");
  put_int("nb_streams", stg->nb_streams);
  // if (stg->type != AV_STREAM_GROUP_PARAMS_NONE)
  //   put_str("type",
  //           av_x_if_null(avformat_stream_group_name(stg->type), "unknown"));
  // else
  //   put_str_opt("type", "unknown");

  if (ret < 0)
    goto end;

  for (i = 0; i < stg->nb_streams; i++) {
    if (selected_streams[stg->streams[i]->index]) {
      ret =
          show_stream(fmt_ctx, stg->streams[i]->index,
                      &ifile->streams[stg->streams[i]->index], IN_STREAM_GROUP);
      if (ret < 0)
        break;
    }
  }

end:
  av_bprint_finalize(&pbuf, NULL);

  return ret;
}

int Prober::show_stream_groups(InputFile *ifile) {
  AVFormatContext *fmt_ctx = ifile->fmt_ctx;
  int i, ret = 0;

  for (i = 0; i < fmt_ctx->nb_stream_groups; i++) {
    AVStreamGroup *stg = fmt_ctx->stream_groups[i];

    ret = show_stream_group(ifile, stg);
    if (ret < 0)
      break;
  }

  return ret;
}

int Prober::show_chapters(InputFile *ifile) {
  AVFormatContext *fmt_ctx = ifile->fmt_ctx;
  int i, ret = 0;

  for (i = 0; i < fmt_ctx->nb_chapters; i++) {
    AVChapter *chapter = fmt_ctx->chapters[i];

    put_int("id", chapter->id);
    put_q("time_base", chapter->time_base, '/');
    put_int("start", chapter->start);
    put_time("start_time", chapter->start, &chapter->time_base);
    put_int("end", chapter->end);
    put_time("end_time", chapter->end, &chapter->time_base);
  }

  return ret;
}

int Prober::show_format(InputFile *ifile) {
  AVFormatContext *fmt_ctx = ifile->fmt_ctx;
  int64_t size = fmt_ctx->pb ? avio_size(fmt_ctx->pb) : -1;
  int ret = 0;

  put_str_validate("filename", fmt_ctx->url);
  put_int("nb_streams", fmt_ctx->nb_streams);
  put_int("nb_programs", fmt_ctx->nb_programs);
  put_int("nb_stream_groups", fmt_ctx->nb_stream_groups);
  put_str("format_name", fmt_ctx->iformat->name);
  if (!0) {
    if (fmt_ctx->iformat->long_name)
      put_str("format_long_name", fmt_ctx->iformat->long_name);
    else
      put_str_opt("format_long_name", "unknown");
  }
  AVRational time_q = AV_TIME_BASE_Q;

  put_time("start_time", fmt_ctx->start_time, &time_q);
  put_time("duration", fmt_ctx->duration, &time_q);
  if (size >= 0)
    put_val("size", size, unit_byte_str);
  else
    put_str_opt("size", "N/A");
  if (fmt_ctx->bit_rate > 0)
    put_val("bit_rate", fmt_ctx->bit_rate, unit_bit_per_second_str);
  else
    put_str_opt("bit_rate", "N/A");
  put_int("probe_score", fmt_ctx->probe_score);

  fflush(stdout);
  return ret;
}

void Prober::show_error(int err) {

  put_int("code", err);
  put_str("string", av_err2str(err));
}

int Prober::open_input_file(InputFile *ifile, const char *filename,
                            const char *put_filename) {
  int err, i;
  AVFormatContext *fmt_ctx = NULL;
  const AVDictionaryEntry *t = NULL;
  int scan_all_pmts_set = 0;

  fmt_ctx = avformat_alloc_context();
  if (!fmt_ctx)
    return AVERROR(ENOMEM);

  if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
    av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
    scan_all_pmts_set = 1;
  }
  if ((err = avformat_open_input(&fmt_ctx, filename, iformat, &format_opts)) <
      0) {
    av_log(NULL, AV_LOG_ERROR, "%s: %s\n", filename, err);
    return err;
  }
  if (put_filename) {
    av_freep(&fmt_ctx->url);
    fmt_ctx->url = av_strdup(put_filename);
  }
  ifile->fmt_ctx = fmt_ctx;
  if (scan_all_pmts_set)
    av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);
  while ((t = av_dict_iterate(format_opts, t)))
    av_log(NULL, AV_LOG_WARNING, "Option %s skipped - not known to demuxer.\n",
           t->key);

  if (find_stream_info) {
    AVDictionary **opts;
    int orig_nb_streams = fmt_ctx->nb_streams;

    err = setup_find_stream_info_opts(fmt_ctx, codec_opts, &opts);
    if (err < 0)
      return err;

    err = avformat_find_stream_info(fmt_ctx, opts);

    for (i = 0; i < orig_nb_streams; i++)
      av_dict_free(&opts[i]);
    av_freep(&opts);

    if (err < 0) {
      av_log(NULL, AV_LOG_ERROR, "%s: %s\n", filename, err);
      return err;
    }
  }

  av_dump_format(fmt_ctx, 0, filename, 0);

  ifile->streams =
      (InputStream *)av_calloc(fmt_ctx->nb_streams, sizeof(*ifile->streams));
  if (!ifile->streams)
    exit(1);
  ifile->nb_streams = fmt_ctx->nb_streams;

  /* bind a decoder to each input stream */
  for (i = 0; i < fmt_ctx->nb_streams; i++) {
    InputStream *ist = &ifile->streams[i];
    AVStream *stream = fmt_ctx->streams[i];
    const AVCodec *codec;

    ist->st = stream;

    if (stream->codecpar->codec_id == AV_CODEC_ID_PROBE) {
      av_log(NULL, AV_LOG_WARNING,
             "Failed to probe codec for input stream %d\n", stream->index);
      continue;
    }

    codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
      av_log(NULL, AV_LOG_WARNING,
             "Unsupported codec with id %d for input stream %d\n",
             stream->codecpar->codec_id, stream->index);
      continue;
    }
    {
      AVDictionary *opts;

      err = filter_codec_opts(codec_opts, stream->codecpar->codec_id, fmt_ctx,
                              stream, codec, &opts, NULL);
      if (err < 0)
        exit(1);

      ist->dec_ctx = avcodec_alloc_context3(codec);
      if (!ist->dec_ctx)
        exit(1);

      err = avcodec_parameters_to_context(ist->dec_ctx, stream->codecpar);
      if (err < 0)
        exit(1);

      av_dict_set(&opts, "flags", "+copy_opaque", AV_DICT_MULTIKEY);

      ist->dec_ctx->pkt_timebase = stream->time_base;

      if (avcodec_open2(ist->dec_ctx, codec, &opts) < 0) {
        av_log(NULL, AV_LOG_WARNING,
               "Could not open codec for input stream %d\n", stream->index);
        exit(1);
      }

      if ((t = av_dict_iterate(opts, NULL))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s for input stream %d not found\n",
               t->key, stream->index);
        return AVERROR_OPTION_NOT_FOUND;
      }
    }
  }

  ifile->fmt_ctx = fmt_ctx;
  return 0;
}

void close_input_file(InputFile *ifile) {
  int i;

  /* close decoder for each stream */
  for (i = 0; i < ifile->nb_streams; i++)
    avcodec_free_context(&ifile->streams[i].dec_ctx);

  av_freep(&ifile->streams);
  ifile->nb_streams = 0;

  avformat_close_input(&ifile->fmt_ctx);
}

int Prober::probe_file(const char *filename, const char *put_filename) {
  InputFile ifile = {0};
  int ret, i;
  int section_id;

  ret = open_input_file(&ifile, filename, put_filename);
  if (ret < 0)
    goto end;

#define CHECK_END                                                              \
  if (ret < 0)                                                                 \
  goto end

  nb_streams = ifile.fmt_ctx->nb_streams;
  REALLOCZ_ARRAY_STREAM(nb_streams_frames, 0, ifile.fmt_ctx->nb_streams);
  REALLOCZ_ARRAY_STREAM(nb_streams_packets, 0, ifile.fmt_ctx->nb_streams);
  REALLOCZ_ARRAY_STREAM(selected_streams, 0, ifile.fmt_ctx->nb_streams);
  REALLOCZ_ARRAY_STREAM(streams_with_closed_captions, 0,
                        ifile.fmt_ctx->nb_streams);
  REALLOCZ_ARRAY_STREAM(streams_with_film_grain, 0, ifile.fmt_ctx->nb_streams);

  for (i = 0; i < ifile.fmt_ctx->nb_streams; i++) {
    if (stream_specifier) {
      ret = avformat_match_stream_specifier(
          ifile.fmt_ctx, ifile.fmt_ctx->streams[i], stream_specifier);
      CHECK_END;
      else selected_streams[i] = ret;
      ret = 0;
    } else {
      selected_streams[i] = 1;
    }
    if (!selected_streams[i])
      ifile.fmt_ctx->streams[i]->discard = AVDISCARD_ALL;
  }

  section_id = SECTION_ID_FRAMES;
  ret = read_packets(&ifile);
  CHECK_END;

end:
  if (ifile.fmt_ctx)
    close_input_file(&ifile);
  av_freep(&nb_streams_frames);
  av_freep(&nb_streams_packets);
  av_freep(&selected_streams);
  av_freep(&streams_with_closed_captions);
  av_freep(&streams_with_film_grain);

  return ret;
}

void Prober::ffprobe_show_pixel_formats() {
  const AVPixFmtDescriptor *pixdesc = NULL;
  int i, n;

  while ((pixdesc = av_pix_fmt_desc_next(pixdesc))) {

    put_str("name", pixdesc->name);
    put_int("nb_components", pixdesc->nb_components);
    if ((pixdesc->nb_components >= 3) &&
        !(pixdesc->flags & AV_PIX_FMT_FLAG_RGB)) {
      put_int("log2_chroma_w", pixdesc->log2_chroma_w);
      put_int("log2_chroma_h", pixdesc->log2_chroma_h);
    } else {
      put_str_opt("log2_chroma_w", "N/A");
      put_str_opt("log2_chroma_h", "N/A");
    }
    n = av_get_bits_per_pixel(pixdesc);
    if (n)
      put_int("bits_per_pixel", n);
    else
      put_str_opt("bits_per_pixel", "N/A");

    if ((pixdesc->nb_components > 0)) {

      for (i = 0; i < pixdesc->nb_components; i++) {

        put_int("index", i + 1);
        put_int("bit_depth", pixdesc->comp[i].depth);
      }
    }
  }
}

int Prober::opt_format(void *optctx, const char *opt, const char *arg) {
  iformat = av_find_input_format(arg);
  if (!iformat) {
    av_log(NULL, AV_LOG_ERROR, "Unknown input format: %s\n", arg);
    return AVERROR(EINVAL);
  }
  return 0;
}

/**
 * Parse interval specification, according to the format:
 * INTERVAL ::= [START|+START_OFFSET][%[END|+END_OFFSET]]
 * INTERVALS ::= INTERVAL[,INTERVALS]
 */
int Prober::parse_read_interval(const char *interval_spec,
                                ReadInterval *interval) {
  int ret = 0;
  char *next, *p, *spec = av_strdup(interval_spec);
  if (!spec)
    return AVERROR(ENOMEM);

  if (!*spec) {
    av_log(NULL, AV_LOG_ERROR, "Invalid empty interval specification\n");
    ret = AVERROR(EINVAL);
    goto end;
  }

  p = spec;
  next = strchr(spec, '%');
  if (next)
    *next++ = 0;

  /* parse first part */
  if (*p) {
    interval->has_start = 1;

    if (*p == '+') {
      interval->start_is_offset = 1;
      p++;
    } else {
      interval->start_is_offset = 0;
    }

    ret = av_parse_time(&interval->start, p, 1);
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Invalid interval start specification '%s'\n",
             p);
      goto end;
    }
  } else {
    interval->has_start = 0;
  }

  /* parse second part */
  p = next;
  if (p && *p) {
    int64_t us;
    interval->has_end = 1;

    if (*p == '+') {
      interval->end_is_offset = 1;
      p++;
    } else {
      interval->end_is_offset = 0;
    }

    if (interval->end_is_offset && *p == '#') {
      long long int lli;
      char *tail;
      interval->duration_frames = 1;
      p++;
      lli = strtoll(p, &tail, 10);
      if (*tail || lli < 0) {
        av_log(NULL, AV_LOG_ERROR,
               "Invalid or negative value '%s' for duration number of frames\n",
               p);
        goto end;
      }
      interval->end = lli;
    } else {
      interval->duration_frames = 0;
      ret = av_parse_time(&us, p, 1);
      if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR,
               "Invalid interval end/duration specification '%s'\n", p);
        goto end;
      }
      interval->end = us;
    }
  } else {
    interval->has_end = 0;
  }

end:
  av_free(spec);
  return ret;
}

int Prober::parse_read_intervals(const char *intervals_spec) {
  int ret, n, i;
  char *p, *spec = av_strdup(intervals_spec);
  if (!spec)
    return AVERROR(ENOMEM);

  /* preparse specification, get number of intervals */
  for (n = 0, p = spec; *p; p++)
    if (*p == ',')
      n++;
  n++;

  read_intervals = (ReadInterval *)av_malloc_array(n, sizeof(*read_intervals));
  if (!read_intervals) {
    ret = AVERROR(ENOMEM);
    goto end;
  }
  read_intervals_nb = n;

  /* parse intervals */
  p = spec;
  for (i = 0; p; i++) {
    char *next;

    av_assert0(i < read_intervals_nb);
    next = strchr(p, ',');
    if (next)
      *next++ = 0;

    read_intervals[i].id = i;
    ret = parse_read_interval(p, &read_intervals[i]);
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Error parsing read interval #%d '%s'\n", i,
             p);
      goto end;
    }
    av_log(NULL, AV_LOG_VERBOSE, "Parsed log interval ");
    log_read_interval(&read_intervals[i], NULL, AV_LOG_VERBOSE);
    p = next;
  }
  av_assert0(i == read_intervals_nb);

end:
  av_free(spec);
  return ret;
}

int Prober::filter_codec_opts(const AVDictionary *opts, enum AVCodecID codec_id,
                              AVFormatContext *s, AVStream *st,
                              const AVCodec *codec, AVDictionary **dst,
                              AVDictionary **opts_used) {
  AVDictionary *ret = NULL;
  const AVDictionaryEntry *t = NULL;
  int flags =
      s->oformat ? AV_OPT_FLAG_ENCODING_PARAM : AV_OPT_FLAG_DECODING_PARAM;
  char prefix = 0;
  const AVClass *cc = avcodec_get_class();

  switch (st->codecpar->codec_type) {
  case AVMEDIA_TYPE_VIDEO:
    prefix = 'v';
    flags |= AV_OPT_FLAG_VIDEO_PARAM;
    break;
  case AVMEDIA_TYPE_AUDIO:
    prefix = 'a';
    flags |= AV_OPT_FLAG_AUDIO_PARAM;
    break;
  case AVMEDIA_TYPE_SUBTITLE:
    prefix = 's';
    flags |= AV_OPT_FLAG_SUBTITLE_PARAM;
    break;
  default:
    break;
  }

  while ((t = av_dict_iterate(opts, t))) {
    const AVClass *priv_class;
    char *p = strchr(t->key, ':');
    int used = 0;

    /* check stream specification in opt name */
    if (p) {
      int err = StreamSpecifier::check(s, st, p + 1);
      if (err < 0) {
        av_dict_free(&ret);
        return err;
      } else if (!err)
        continue;

      *p = 0;
    }

    if (av_opt_find(&cc, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) ||
        !codec ||
        ((priv_class = codec->priv_class) &&
         av_opt_find(&priv_class, t->key, NULL, flags,
                     AV_OPT_SEARCH_FAKE_OBJ))) {
      av_dict_set(&ret, t->key, t->value, 0);
      used = 1;
    } else if (t->key[0] == prefix && av_opt_find(&cc, t->key + 1, NULL, flags,
                                                  AV_OPT_SEARCH_FAKE_OBJ)) {
      av_dict_set(&ret, t->key + 1, t->value, 0);
      used = 1;
    }

    if (p)
      *p = ':';

    if (used && opts_used)
      av_dict_set(opts_used, t->key, "", 0);
  }

  *dst = ret;
  return 0;
}

int Prober::setup_find_stream_info_opts(AVFormatContext *s,
                                        AVDictionary *local_codec_opts,
                                        AVDictionary ***dst) {
  int ret;
  AVDictionary **opts;

  *dst = NULL;

  if (!s->nb_streams)
    return 0;

  opts = (AVDictionary **)av_calloc(s->nb_streams, sizeof(*opts));
  if (!opts)
    return AVERROR(ENOMEM);

  for (int i = 0; i < s->nb_streams; i++) {
    ret = filter_codec_opts(local_codec_opts, s->streams[i]->codecpar->codec_id,
                            s, s->streams[i], NULL, &opts[i], NULL);
    if (ret < 0)
      goto fail;
  }
  *dst = opts;
  return 0;
fail:
  for (int i = 0; i < s->nb_streams; i++)
    av_dict_free(&opts[i]);
  av_freep(&opts);
  return ret;
}

int Prober::put_int(std::string k, int v) {
  printf("%s: %d\n", k.c_str(), v);
  return 0;
};
int Prober::put_q(std::string k, AVRational v, char s) { return 0; };
int Prober::put_str(std::string k, std::string v) {
  printf("%s: %s\n", k.c_str(), v.c_str());
  return 0;
};
int Prober::put_str_opt(std::string k, std::string v) {
  printf("%s: %s\n", k.c_str(), v.c_str());
  return 0;
};
int Prober::put_str_validate(std::string k, std::string v) {
  printf("%s: %s\n", k.c_str(), v.c_str());
  return 0;
};
int Prober::put_time(std::string k, int64_t v, AVRational *tb) { return 0; };
int Prober::put_ts(std::string k, int64_t v) { return 0; };
int Prober::put_duration_time(std::string k, int64_t v, AVRational *tb) {
  return 0;
};
int Prober::put_duration_ts(std::string k, int64_t v) { return 0; };
int Prober::put_val(std::string k, int64_t v, const char u[5]) { return 0; };
