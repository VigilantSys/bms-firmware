/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bms/bms.h>

#include "helper.h"

void bms_soc_reset(struct bms_context *bms, int percent)
{
    if (percent <= 100 && percent >= 0) {
        bms->soc = percent;
    }
    else if (bms->ocv_points != NULL) {
        bms->soc = interpolate(bms->ocv_points, soc_pct, ARRAY_SIZE(soc_pct),
                               bms->ic_data.cell_voltage_avg);
    }
    else {
        // no OCV curve specified, use simplified estimation instead
        float ocv_simple[2] = { bms->ic_conf.cell_chg_voltage_limit,
                                bms->ic_conf.cell_dis_voltage_limit };
        float soc_simple[2] = { 100.0F, 0.0F };
        bms->soc = interpolate(ocv_simple, soc_simple, 2, bms->ic_data.cell_voltage_avg);
    }
}

void bms_soc_update(struct bms_context *bms)
{
    static float coulomb_counter_mAs = 0;
    static int64_t last_update = 0;
    int64_t now = k_uptime_get();

    coulomb_counter_mAs += bms->ic_data.current * (now - last_update);
    float soc_delta = coulomb_counter_mAs / (bms->nominal_capacity_Ah * 3.6e4F);

    if (soc_delta > 0.1F || soc_delta < -0.1F) {
        // only update SoC after significant changes to maintain higher resolution
        float soc_tmp = bms->soc + soc_delta;
        bms->soc = CLAMP(soc_tmp, 0.0F, 100.0F);
        coulomb_counter_mAs = 0;
    }

    last_update = now;
}
