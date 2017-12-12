//-----------------------------------------------------------------------------
//
//	GraphStudioNext
//
//	Author : cplussharp
//  Originally seen in http://sourceforge.net/projects/h264bitstream/
//
//-----------------------------------------------------------------------------


#include "H264StructReader.h"


// 7.3.2.11 RBSP trailing bits syntax
void CH264StructReader::ReadTrailingBits(CBitStreamReader& bs)
{
    bs.SkipU1(); // rbsp_stop_one_bit
    while (!bs.ByteAligned())
        bs.SkipU1(); // rbsp_alignment_zero_bit
}

bool CH264StructReader::MoreRbspData(CBitStreamReader& bs) 
{
    if (bs.IsEnd()) return false;
    if (bs.PeekU1()) return false; // if next bit is 1, we've reached the stop bit
    return true;
}

void CH264StructReader::ReadVUI(CBitStreamReader& bs, vui_t& vui)
{
    memset(&vui,0,sizeof(vui_t));

    vui.aspect_ratio_info_present_flag = bs.ReadU1();
    if (vui.aspect_ratio_info_present_flag)
    {
        vui.aspect_ratio_idc = bs.ReadU8();
        if (vui.aspect_ratio_idc == 255) // Extended_SAR
        {
            vui.sar_width = bs.ReadU(16);
            vui.sar_height = bs.ReadU(16);
        }
    }

    vui.overscan_info_present_flag = bs.ReadU1();
    if (vui.overscan_info_present_flag)
        vui.overscan_appropriate_flag = bs.ReadU1();

    vui.video_signal_type_present_flag = bs.ReadU1();
    if (vui.video_signal_type_present_flag)
    {
        vui.video_format = bs.ReadU(3);
        vui.video_full_range_flag = bs.ReadU1();
        vui.colour_description_present_flag = bs.ReadU1();
        if (vui.colour_description_present_flag)
        {
            vui.colour_primaries = bs.ReadU8();
            vui.transfer_characteristics = bs.ReadU8();
            vui.matrix_coefficients = bs.ReadU8();
        }
    }

    vui.chroma_loc_info_present_flag = bs.ReadU1();
    if (vui.chroma_loc_info_present_flag)
    {
        vui.chroma_sample_loc_type_top_field = bs.ReadUE();
        vui.chroma_sample_loc_type_bottom_field = bs.ReadUE();
    }
    
    vui.timing_info_present_flag = bs.ReadU1();
    if (vui.timing_info_present_flag)
    {
        vui.num_units_in_tick = bs.ReadU32();
        vui.time_scale = bs.ReadU32();
        vui.fixed_frame_rate_flag = bs.ReadU1();
    }
    
    vui.nal_hrd_parameters_present_flag = bs.ReadU1();
    if (vui.nal_hrd_parameters_present_flag)
        ReadHRD(bs, vui.nal_hrd_parameters);

    vui.vcl_hrd_parameters_present_flag = bs.ReadU1();
    if (vui.vcl_hrd_parameters_present_flag)
        ReadHRD(bs, vui.vcl_hrd_parameters);

    if (vui.nal_hrd_parameters_present_flag || vui.vcl_hrd_parameters_present_flag)
        vui.low_delay_hrd_flag = bs.ReadU1();

    vui.pic_struct_present_flag = bs.ReadU1();

    vui.bitstream_restriction_flag = bs.ReadU1();
    if (vui.bitstream_restriction_flag)
    {
        vui.motion_vectors_over_pic_boundaries_flag = bs.ReadU1();
        vui.max_bytes_per_pic_denom = bs.ReadUE();
        vui.max_bits_per_mb_denom = bs.ReadUE();
        vui.log2_max_mv_length_horizontal = bs.ReadUE();
        vui.log2_max_mv_length_vertical = bs.ReadUE();
        vui.num_reorder_frames = bs.ReadUE();
        vui.max_dec_frame_buffering = bs.ReadUE();
    }
}

void CH264StructReader::ReadHRD(CBitStreamReader& bs, hrd_t& hrd)
{
    hrd.cpb_cnt_minus1 = bs.ReadUE();
    hrd.bit_rate_scale = bs.ReadU(4);
    hrd.cpb_size_scale = bs.ReadU(4);
    for (int i = 0; i <= hrd.cpb_cnt_minus1; i++ )
    {
        hrd.bit_rate_value_minus1[i] = bs.ReadUE();
        hrd.cpb_size_value_minus1[i] = bs.ReadUE();
        hrd.cbr_flag[i] = bs.ReadU1();
    }
    hrd.initial_cpb_removal_delay_length_minus1 = bs.ReadU(5);
    hrd.cpb_removal_delay_length_minus1 = bs.ReadU(5);
    hrd.dpb_output_delay_length_minus1 = bs.ReadU(5);
    hrd.time_offset_length = bs.ReadU(5);
}

// 7.3.2.1.1 Scaling list syntax
void CH264StructReader::ReadScalingList(CBitStreamReader& bs, int* scalingList, int sizeOfScalingList, bool* useDefaultScalingMatrixFlag)
{
    // NOTE need to be able to set useDefaultScalingMatrixFlag when reading, hence passing as pointer
    int lastScale = 8;
    int nextScale = 8;
    int delta_scale;

    for (int j=0; j<sizeOfScalingList; j++)
    {
        if (nextScale != 0)
        {
            delta_scale = bs.ReadSE();
            nextScale = ( lastScale + delta_scale + 256 ) % 256;
            *useDefaultScalingMatrixFlag = (j==0 && nextScale==0);
        }

        int scale = (nextScale == 0) ? lastScale : nextScale;
        if (scalingList != NULL) scalingList[j] = scale;
        lastScale = scale;
    }
}

void CH264StructReader::ReadSPS(CBitStreamReader& bs, sps_t& sps)
{

	volatile int found_h, val;
	memset(&sps,0,sizeof(sps_t));

	found_h = 0;
	do
	{
		if (bs.IsEnd())
			return;
		val = bs.ReadU8();
		switch (found_h)
		{
		case 0:
		case 1:
		case 2:
			found_h = (val == 0) ?   found_h + 1 : 0;
			break;
		case 3:
			found_h = (val == 1) ?   found_h + 1 : 0;
			break;
		default:
			found_h = (val == 0x67)? found_h + 1 : 0;
		} ;
	}
	while (found_h < 5);


    sps.chroma_format_idc = 1;

    sps.profile_idc = bs.ReadU8();
    sps.constraint_set0_flag = bs.ReadU1();
    sps.constraint_set1_flag = bs.ReadU1();
    sps.constraint_set2_flag = bs.ReadU1();
    sps.constraint_set3_flag = bs.ReadU1();
    sps.constraint_set4_flag = bs.ReadU1();
    sps.constraint_set5_flag = bs.ReadU1();
    /* reserved_zero_2bits */ bs.SkipU(2);
    sps.level_idc = bs.ReadU8();
    sps.seq_parameter_set_id = bs.ReadUE();

    if (sps.profile_idc == 100 || sps.profile_idc == 110 ||
        sps.profile_idc == 122 || sps.profile_idc == 144)
    {
        sps.chroma_format_idc = bs.ReadUE();
        if (sps.chroma_format_idc == 3)
            sps.residual_colour_transform_flag = bs.ReadU1();

        sps.bit_depth_luma_minus8 = bs.ReadUE();
        sps.bit_depth_chroma_minus8 = bs.ReadUE();
        sps.qpprime_y_zero_transform_bypass_flag = bs.ReadU1();
        
        sps.seq_scaling_matrix_present_flag = bs.ReadU1();
        if (sps.seq_scaling_matrix_present_flag)
        {
            for (int i=0; i<8; i++)
            {
                sps.seq_scaling_list_present_flag[i] = bs.ReadU1();
                if (sps.seq_scaling_list_present_flag[i])
                {
                    if (i<6)
                        ReadScalingList(bs, sps.ScalingList4x4[i], 16, &(sps.UseDefaultScalingMatrix4x4Flag[i]));
                    else
                        ReadScalingList(bs, sps.ScalingList8x8[i-6], 64, &(sps.UseDefaultScalingMatrix8x8Flag[i-6]));
                }
            }
        }
    }

    sps.log2_max_frame_num_minus4 = bs.ReadUE();
    
    sps.pic_order_cnt_type = bs.ReadUE();
    if (sps.pic_order_cnt_type == 0)
        sps.log2_max_pic_order_cnt_lsb_minus4 = bs.ReadUE();
    else if (sps.pic_order_cnt_type == 1)
    {
        sps.delta_pic_order_always_zero_flag = bs.ReadU1();
        sps.offset_for_non_ref_pic = bs.ReadSE();
        sps.offset_for_top_to_bottom_field = bs.ReadSE();
        sps.num_ref_frames_in_pic_order_cnt_cycle = bs.ReadUE();
        for (int i = 0; i<sps.num_ref_frames_in_pic_order_cnt_cycle; i++)
            sps.offset_for_ref_frame[ i ] = bs.ReadSE();
    }

    sps.num_ref_frames = bs.ReadUE();
    sps.gaps_in_frame_num_value_allowed_flag = bs.ReadU1();
    sps.pic_width_in_mbs_minus1 = bs.ReadUE();
    sps.pic_height_in_map_units_minus1 = bs.ReadUE();
    
    sps.frame_mbs_only_flag = bs.ReadU1();
    if (!sps.frame_mbs_only_flag)
        sps.mb_adaptive_frame_field_flag = bs.ReadU1();

    sps.direct_8x8_inference_flag = bs.ReadU1();
    
    sps.frame_cropping_flag = bs.ReadU1();
    if (sps.frame_cropping_flag)
    {
        sps.frame_crop_left_offset = bs.ReadUE();
        sps.frame_crop_right_offset = bs.ReadUE();
        sps.frame_crop_top_offset = bs.ReadUE();
        sps.frame_crop_bottom_offset = bs.ReadUE();
    }

    sps.vui_parameters_present_flag = bs.ReadU1();
    if (sps.vui_parameters_present_flag)
        ReadVUI(bs, sps.vui);

    ReadTrailingBits(bs);
}

/** 
 Calculate the log base 2 of the argument, rounded up. 
 Zero or negative arguments return zero 
 Idea from http://www.southwindsgames.com/blog/2009/01/19/fast-integer-log2-function-in-cc/
 */
int intlog2(int x)
{
    int log = 0;
    if (x < 0) { x = 0; }
    while ((x >> log) > 0)
    {
        log++;
    }
    if (log > 0 && x == 1<<(log-1)) { log--; }
    return log;
}

void CH264StructReader::ReadPPS(CBitStreamReader& bs, pps_t& pps)
{
	int found_h, val;

    memset(&pps,0,sizeof(pps_t));

	found_h = 0;
	do
	{
		if (bs.IsEnd())
			return;
		val = bs.ReadU8();
		switch (found_h)
		{
		case 0:
		case 1:
		case 2:
			found_h = (val == 0) ?   found_h + 1 : 0;
			break;
		case 3:
			found_h = (val == 1) ?   found_h + 1 : 0;
			break;
		default:
			found_h = (val == 0x68)? found_h + 1 : 0;
		} ;
	}
	while (found_h < 5);


    pps.pic_parameter_set_id = bs.ReadUE();
    pps.seq_parameter_set_id = bs.ReadUE();
    pps.entropy_coding_mode_flag = bs.ReadU1();
    pps.pic_order_present_flag = bs.ReadU1();
    pps.num_slice_groups_minus1 = bs.ReadUE();

    if (pps.num_slice_groups_minus1 > 0)
    {
        pps.slice_group_map_type = bs.ReadUE();
        if (pps.slice_group_map_type == 0)
        {
            for (int i_group = 0; i_group <= pps.num_slice_groups_minus1; i_group++)
                pps.run_length_minus1[i_group] = bs.ReadUE();
        }
        else if (pps.slice_group_map_type == 2)
        {
            for (int i_group = 0; i_group < pps.num_slice_groups_minus1; i_group++)
            {
                pps.top_left[i_group] = bs.ReadUE();
                pps.bottom_right[i_group] = bs.ReadUE();
            }
        }
        else if (pps.slice_group_map_type == 3 ||
                 pps.slice_group_map_type == 4 ||
                 pps.slice_group_map_type == 5)
        {
            pps.slice_group_change_direction_flag = bs.ReadU1();
            pps.slice_group_change_rate_minus1 = bs.ReadUE();
        }
        else if (pps.slice_group_map_type == 6)
        {
            pps.pic_size_in_map_units_minus1 = bs.ReadUE();
            for (int i = 0; i <= pps.pic_size_in_map_units_minus1; i++)
            {
                int v = intlog2(pps.num_slice_groups_minus1 + 1);
                pps.slice_group_id[i] = bs.ReadU(v);
            }
        }
    }
    pps.num_ref_idx_l0_active_minus1 = bs.ReadUE();
    pps.num_ref_idx_l1_active_minus1 = bs.ReadUE();
    pps.weighted_pred_flag = bs.ReadU1();
    pps.weighted_bipred_idc = bs.ReadU(2);
    pps.pic_init_qp_minus26 = bs.ReadSE();
    pps.pic_init_qs_minus26 = bs.ReadSE();
    pps.chroma_qp_index_offset = bs.ReadSE();
    pps.deblocking_filter_control_present_flag = bs.ReadU1();
    pps.constrained_intra_pred_flag = bs.ReadU1();
    pps.redundant_pic_cnt_present_flag = bs.ReadU1();

    pps.more_rbsp_data_present = MoreRbspData(bs);
    if (pps.more_rbsp_data_present)
    {
        pps.transform_8x8_mode_flag = bs.ReadU1();
        pps.pic_scaling_matrix_present_flag = bs.ReadU1();
        if (pps.pic_scaling_matrix_present_flag)
        {
            for (int i=0; i<6 + 2*pps.transform_8x8_mode_flag; i++)
            {
                pps.pic_scaling_list_present_flag[i] = bs.ReadU1();
                if (pps.pic_scaling_list_present_flag[i])
                {
                    if (i<6)
                        ReadScalingList(bs, pps.ScalingList4x4[i], 16, &(pps.UseDefaultScalingMatrix4x4Flag[i]));
                    else
                        ReadScalingList(bs, pps.ScalingList8x8[i-6], 64, &(pps.UseDefaultScalingMatrix8x8Flag[i-6]));
                }
            }
        }
        pps.second_chroma_qp_index_offset = bs.ReadSE();
    }
    
    ReadTrailingBits(bs);
}

#if 0
// Use 64bit input params to force 64bit calculation and prevent overflow - issue #246
REFERENCE_TIME CH264StructReader::GetAvgTimePerFrame(REFERENCE_TIME num_units_in_tick, REFERENCE_TIME time_scale)
{
    REFERENCE_TIME val = 0;
    // Trick for weird parameters
    // https://github.com/MediaPortal/MediaPortal-1/blob/master/DirectShowFilters/BDReader/source/FrameHeaderParser.cpp
    if ((num_units_in_tick < 1000) || (num_units_in_tick > 1001))
    {
        if  ((time_scale % num_units_in_tick != 0) && ((time_scale*1001) % num_units_in_tick == 0))
        {
            time_scale          = (time_scale * 1001) / num_units_in_tick;
            num_units_in_tick   = 1001;
        }
        else
        {
            time_scale          = (time_scale * 1000) / num_units_in_tick;
            num_units_in_tick   = 1000;
        }
    }

    // VUI consider fields even for progressive stream : multiply by 2!
    if (time_scale)
        val = 2 * (10000000I64*num_units_in_tick)/time_scale;

    return val;
}

RECT CH264StructReader::GetSize(sps_t& sps, bool ignoreCropping)
{
	RECT rc = { 0, 0, 0, 0 };
	rc.right = (sps.pic_width_in_mbs_minus1 + 1) * 16;
	rc.bottom = (2 - (sps.frame_mbs_only_flag ? 1 : 0)) * ((sps.pic_height_in_map_units_minus1 + 1) * 16);

	if (sps.frame_cropping_flag && !ignoreCropping)
	{
		rc.right -= sps.frame_crop_right_offset * 2 + sps.frame_crop_left_offset * 2;
		rc.bottom -= (2 - (sps.frame_mbs_only_flag ? 1 : 0)) * (sps.frame_crop_bottom_offset * 2 + sps.frame_crop_top_offset * 2);
	}

	return rc;
}
#endif

void CH264StructReader::ReadSEI(CBitStreamReader& bs, sei_t& sei)
{
    memset(&sei,0,sizeof(sei_t));

    uint32_t val = bs.ReadU8();
    while (val == 0Xff)
    {
        sei.payloadType += 255;
        val = bs.ReadU8();
    }
    sei.payloadType += val;

    unsigned long payloadSize = 0;
    val = bs.ReadU8();
    while (val == 0Xff)
    {
        sei.payloadSize += 255;
        val = bs.ReadU8();
    }
    sei.payloadSize += val;

    sei.payload = new char[sei.payloadSize];
    //CopyMemory(sei.payload, bs.Pointer(), sei.payloadSize);
    for (val=0; val < sei.payloadSize; val++)
    	sei.payload[val]=*((char*)bs.Pointer() + val);
}

void CH264StructReader::ReadSliceHeader(CBitStreamReader& bs, slice_header_t& sh, sps_t& sps, bool isNonIDR)
{
    memset(&sh, 0, sizeof(slice_header_t));;

    sh.first_mb_in_slice = bs.ReadUE();
    sh.slice_type = bs.ReadUE();
    sh.pic_parameter_set_id = bs.ReadUE();

    sh.frame_num = bs.ReadU(sps.log2_max_frame_num_minus4 + 4 );
    if (!sps.frame_mbs_only_flag)
    {
        sh.field_pic_flag = bs.ReadU1();
        if (sh.field_pic_flag)
            sh.bottom_field_flag = bs.ReadU1();
    }

    if (isNonIDR)
        sh.idr_pic_id = bs.ReadUE();

    // we don't need to know all
}



void nal_start_code_prefix(wbitstream *bs)
{
    bitstream_put_ui(bs, 0x00000001, 32);
}

#if 0
static void nal_header(wbitstream *bs, int nal_ref_idc, int nal_unit_type)
{
    bitstream_put_ui(bs, 0, 1);                /* forbidden_zero_bit: 0 */
    bitstream_put_ui(bs, nal_ref_idc, 2);
    bitstream_put_ui(bs, nal_unit_type, 5);
}
#endif


#define NAL_REF_IDC_NONE        0
#define NAL_REF_IDC_LOW         1
#define NAL_REF_IDC_MEDIUM      2
#define NAL_REF_IDC_HIGH        3
#define NAL_NON_IDR             1
#define NAL_IDR                 5
#define NAL_SPS                 7
#define NAL_PPS                 8
#define NAL_SEI			6
#define SLICE_TYPE_P            0
#define SLICE_TYPE_B            1
#define SLICE_TYPE_I            2
#define ENTROPY_MODE_CAVLC      0
#define ENTROPY_MODE_CABAC      1
#define PROFILE_IDC_BASELINE    66
#define PROFILE_IDC_MAIN        77
#define PROFILE_IDC_HIGH        100
//static void sps_rbsp(wbitstream *bs, VAProfile profile, int frame_bit_rate, VAEncSequenceParameterBufferH264 *seq_param)
void sps_rbsp(wbitstream *bs, /*VAProfile profile,*/ int frame_bit_rate, int fps, sps_t* seq_param)
{
    int profile_idc;
    int constraint_set_flag;
#if 0
    if (profile == VAProfileH264High) {
        profile_idc = PROFILE_IDC_HIGH;
        constraint_set_flag |= (1 << 3); /* Annex A.2.4 */
    }
    else if (profile == VAProfileH264Main) {
        profile_idc = PROFILE_IDC_MAIN;
        constraint_set_flag |= (1 << 1); /* Annex A.2.2 */
    } else {
        profile_idc = PROFILE_IDC_BASELINE;
        constraint_set_flag |= (1 << 0); /* Annex A.2.1 */
    }
#endif
    profile_idc = PROFILE_IDC_MAIN;
    constraint_set_flag |= (1 << 1); /* Annex A.2.2 */

    bitstream_put_ui(bs, profile_idc, 8);               /* profile_idc */
#if 0
    bitstream_put_ui(bs, !!(constraint_set_flag & 1), 1);                         /* constraint_set0_flag */
    bitstream_put_ui(bs, !!(constraint_set_flag & 2), 1);                         /* constraint_set1_flag */
    bitstream_put_ui(bs, !!(constraint_set_flag & 4), 1);                         /* constraint_set2_flag */
    bitstream_put_ui(bs, !!(constraint_set_flag & 8), 1);                         /* constraint_set3_flag */
#endif


    bitstream_put_ui(bs, seq_param->constraint_set0_flag, 1);
    bitstream_put_ui(bs, seq_param->constraint_set1_flag, 1);
    bitstream_put_ui(bs, seq_param->constraint_set2_flag, 1);
    bitstream_put_ui(bs, seq_param->constraint_set3_flag, 1);
    bitstream_put_ui(bs, seq_param->constraint_set4_flag, 1);
    bitstream_put_ui(bs, seq_param->constraint_set5_flag, 1);

    bitstream_put_ui(bs, 0, 2);                         /* reserved_zero_2bits ( equal to 0 ) */
    bitstream_put_ui(bs, seq_param->level_idc, 8);      /* level_idc */
    bitstream_put_ue(bs, seq_param->seq_parameter_set_id);      /* seq_parameter_set_id */
#if 0
    if ( profile_idc == PROFILE_IDC_HIGH) {
        bitstream_put_ue(bs, 1);        /* chroma_format_idc = 1, 4:2:0 */
        bitstream_put_ue(bs, 0);        /* bit_depth_luma_minus8 */
        bitstream_put_ue(bs, 0);        /* bit_depth_chroma_minus8 */
        bitstream_put_ui(bs, 0, 1);     /* qpprime_y_zero_transform_bypass_flag */
        bitstream_put_ui(bs, 0, 1);     /* seq_scaling_matrix_present_flag */
    }
#endif
    bitstream_put_ue(bs, seq_param->log2_max_frame_num_minus4); /* log2_max_frame_num_minus4 */
    bitstream_put_ue(bs, seq_param->pic_order_cnt_type);        /* pic_order_cnt_type */
    if (seq_param->pic_order_cnt_type == 0)
        bitstream_put_ue(bs, seq_param->log2_max_pic_order_cnt_lsb_minus4);     /* log2_max_pic_order_cnt_lsb_minus4 */
    //else {
    //    assert(0);
    //}
    bitstream_put_ue(bs, seq_param->num_ref_frames);        /* num_ref_frames */
    bitstream_put_ui(bs, 0, 1);                                 /* gaps_in_frame_num_value_allowed_flag */
    bitstream_put_ue(bs, seq_param->pic_width_in_mbs_minus1);  /* pic_width_in_mbs_minus1 */
    bitstream_put_ue(bs, seq_param->pic_height_in_map_units_minus1); /* pic_height_in_map_units_minus1 */

    bitstream_put_ui(bs, seq_param->frame_mbs_only_flag, 1);    /* frame_mbs_only_flag */
    //if (!seq_param->frame_mbs_only_flag) {
    //   assert(0);
    //}
    if( !seq_param->frame_mbs_only_flag)
    	bitstream_put_ui(bs, seq_param->mb_adaptive_frame_field_flag, 1); /* mb_adaptive_frame_field_flag */

    bitstream_put_ui(bs, seq_param->direct_8x8_inference_flag, 1);      /* direct_8x8_inference_flag */
    bitstream_put_ui(bs, seq_param->frame_cropping_flag, 1);            /* frame_cropping_flag */
    if (seq_param->frame_cropping_flag) {
        bitstream_put_ue(bs, seq_param->frame_crop_left_offset);        /* frame_crop_left_offset */
        bitstream_put_ue(bs, seq_param->frame_crop_right_offset);       /* frame_crop_right_offset */
        bitstream_put_ue(bs, seq_param->frame_crop_top_offset);         /* frame_crop_top_offset */
        bitstream_put_ue(bs, seq_param->frame_crop_bottom_offset);      /* frame_crop_bottom_offset */
    }
    if ( frame_bit_rate < 0 ) {
        bitstream_put_ui(bs, 0, 1); /* vui_parameters_present_flag */
    } else {
        bitstream_put_ui(bs, 1, 1); /* vui_parameters_present_flag */
        bitstream_put_ui(bs, 0, 1); /* aspect_ratio_info_present_flag */
        bitstream_put_ui(bs, 0, 1); /* overscan_info_present_flag */
        bitstream_put_ui(bs, 0, 1); /* video_signal_type_present_flag */
        bitstream_put_ui(bs, 0, 1); /* chroma_loc_info_present_flag */
        bitstream_put_ui(bs, 1, 1); /* timing_info_present_flag */
        {
            bitstream_put_ui(bs, 900000/(fps*2), 32); /* num_units_in_tick */
            bitstream_put_ui(bs, 900000, 32); /* time_scale */
            bitstream_put_ui(bs, 1, 1); /* fixed_frame_rate_flag */
        }
#if 0
        bitstream_put_ui(bs, 1, 1); /* nal_hrd_parameters_present_flag */
        {
            // hrd_parameters
            bitstream_put_ue(bs, 0);    /* cpb_cnt_minus1 */
            bitstream_put_ui(bs, 4, 4); /* bit_rate_scale */
            bitstream_put_ui(bs, 6, 4); /* cpb_size_scale */
            bitstream_put_ue(bs, frame_bit_rate - 1); /* bit_rate_value_minus1[0] */
            bitstream_put_ue(bs, frame_bit_rate*8 - 1); /* cpb_size_value_minus1[0] */
            bitstream_put_ui(bs, 1, 1);  /* cbr_flag[0] */
            bitstream_put_ui(bs, 23, 5);   /* initial_cpb_removal_delay_length_minus1 */
            bitstream_put_ui(bs, 23, 5);   /* cpb_removal_delay_length_minus1 */
            bitstream_put_ui(bs, 23, 5);   /* dpb_output_delay_length_minus1 */
            bitstream_put_ui(bs, 23, 5);   /* time_offset_length  */
        }
#else
        bitstream_put_ui(bs, 0, 1); /* nal_hrd_parameters_present_flag */
#endif
        bitstream_put_ui(bs, 0, 1);   /* vcl_hrd_parameters_present_flag */
        bitstream_put_ui(bs, 0, 1);   /* low_delay_hrd_flag */
        bitstream_put_ui(bs, 0, 1); /* pic_struct_present_flag */
        bitstream_put_ui(bs, 0, 1); /* bitstream_restriction_flag */
    }
    rbsp_trailing_bits(bs);     /* rbsp_trailing_bits */
}

void pps_rbsp(wbitstream *bs, pps_t *pic_param)
{
    bitstream_put_ue(bs, pic_param->pic_parameter_set_id);      /* pic_parameter_set_id */
    bitstream_put_ue(bs, pic_param->seq_parameter_set_id);      /* seq_parameter_set_id */
    bitstream_put_ui(bs, pic_param->entropy_coding_mode_flag, 1);  /* entropy_coding_mode_flag */
    bitstream_put_ui(bs, 0, 1);                         /* pic_order_present_flag: 0 */
    bitstream_put_ue(bs, 0);                            /* num_slice_groups_minus1 */
    bitstream_put_ue(bs, pic_param->num_ref_idx_l0_active_minus1);      /* num_ref_idx_l0_active_minus1 */
    bitstream_put_ue(bs, pic_param->num_ref_idx_l1_active_minus1);      /* num_ref_idx_l1_active_minus1 1 */
    bitstream_put_ui(bs, pic_param->weighted_pred_flag, 1);     /* weighted_pred_flag: 0 */
    bitstream_put_ui(bs, pic_param->weighted_bipred_idc, 2);	/* weighted_bipred_idc: 0 */
    bitstream_put_se(bs, pic_param->pic_init_qp_minus26);  /* pic_init_qp_minus26 */
    bitstream_put_se(bs, 0);                            /* pic_init_qs_minus26 */
    bitstream_put_se(bs, 0);                            /* chroma_qp_index_offset */
    bitstream_put_ui(bs, pic_param->deblocking_filter_control_present_flag, 1); /* deblocking_filter_control_present_flag */
    bitstream_put_ui(bs, 0, 1);                         /* constrained_intra_pred_flag */
    bitstream_put_ui(bs, 0, 1);                         /* redundant_pic_cnt_present_flag */
    /* more_rbsp_data */
    bitstream_put_ui(bs, pic_param->transform_8x8_mode_flag, 1);    /*transform_8x8_mode_flag */
    bitstream_put_ui(bs, 0, 1);                         /* pic_scaling_matrix_present_flag */
    bitstream_put_se(bs, pic_param->second_chroma_qp_index_offset );    /*second_chroma_qp_index_offset */
    rbsp_trailing_bits(bs);
}

