/******************************************************************************
 * Copyright 2020 The Firmament Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include <Controller.h>
#include <firmament.h>

#define TAG "Controller"

/* controller input topic */
MCN_DECLARE(fms_output);
MCN_DECLARE(ins_output);

/* controller output topic */
MCN_DEFINE(control_output, sizeof(Control_Out_Bus));

static McnNode_t fms_out_nod;
static McnNode_t ins_out_nod;

fmt_model_info_t control_model_info;

static int control_out_echo(void* param)
{
    Control_Out_Bus control_out;
    if(mcn_copy_from_hub((McnHub*)param, &control_out) == FMT_EOK){
        console_printf("timestamp:%d actuator: %d %d %d %d\n", control_out.timestamp, control_out.actuator_cmd[0], 
            control_out.actuator_cmd[1], control_out.actuator_cmd[2], control_out.actuator_cmd[3]);
    }
    return 0;
}

static void update_parameter(void)
{
    CONTROL_PARAM.VEL_XY_P = PARAM_GET_FLOAT(CONTROL, VEL_XY_P);
    CONTROL_PARAM.VEL_XY_I = PARAM_GET_FLOAT(CONTROL, VEL_XY_I);
    CONTROL_PARAM.VEL_XY_D = PARAM_GET_FLOAT(CONTROL, VEL_XY_D);
    CONTROL_PARAM.VEL_Z_P = PARAM_GET_FLOAT(CONTROL, VEL_Z_P);
    CONTROL_PARAM.VEL_Z_I = PARAM_GET_FLOAT(CONTROL, VEL_Z_I);
    CONTROL_PARAM.VEL_Z_D = PARAM_GET_FLOAT(CONTROL, VEL_Z_D);
    CONTROL_PARAM.VEL_XY_I_MIN = PARAM_GET_FLOAT(CONTROL, VEL_XY_I_MIN);
    CONTROL_PARAM.VEL_XY_I_MAX = PARAM_GET_FLOAT(CONTROL, VEL_XY_I_MAX);
    CONTROL_PARAM.VEL_XY_D_MIN = PARAM_GET_FLOAT(CONTROL, VEL_XY_D_MIN);
    CONTROL_PARAM.VEL_XY_D_MAX = PARAM_GET_FLOAT(CONTROL, VEL_XY_D_MAX);
    CONTROL_PARAM.VEL_Z_I_MIN = PARAM_GET_FLOAT(CONTROL, VEL_Z_I_MIN);
    CONTROL_PARAM.VEL_Z_I_MAX = PARAM_GET_FLOAT(CONTROL, VEL_Z_I_MAX);
    CONTROL_PARAM.VEL_Z_D_MIN = PARAM_GET_FLOAT(CONTROL, VEL_Z_D_MIN);
    CONTROL_PARAM.VEL_Z_D_MAX = PARAM_GET_FLOAT(CONTROL, VEL_Z_D_MAX);
    CONTROL_PARAM.ROLL_P = PARAM_GET_FLOAT(CONTROL, ROLL_P);
    CONTROL_PARAM.PITCH_P = PARAM_GET_FLOAT(CONTROL, PITCH_P);
    CONTROL_PARAM.ROLL_PITCH_CMD_LIM = PARAM_GET_FLOAT(CONTROL, ROLL_PITCH_CMD_LIM);
    CONTROL_PARAM.ROLL_RATE_P = PARAM_GET_FLOAT(CONTROL, ROLL_RATE_P);
    CONTROL_PARAM.PITCH_RATE_P = PARAM_GET_FLOAT(CONTROL, PITCH_RATE_P);
    CONTROL_PARAM.YAW_RATE_P = PARAM_GET_FLOAT(CONTROL, YAW_RATE_P);
    CONTROL_PARAM.ROLL_RATE_I = PARAM_GET_FLOAT(CONTROL, ROLL_RATE_I);
    CONTROL_PARAM.PITCH_RATE_I = PARAM_GET_FLOAT(CONTROL, PITCH_RATE_I);
    CONTROL_PARAM.YAW_RATE_I = PARAM_GET_FLOAT(CONTROL, YAW_RATE_I);
    CONTROL_PARAM.ROLL_RATE_D = PARAM_GET_FLOAT(CONTROL, ROLL_RATE_D);
    CONTROL_PARAM.PITCH_RATE_D = PARAM_GET_FLOAT(CONTROL, PITCH_RATE_D);
    CONTROL_PARAM.YAW_RATE_D = PARAM_GET_FLOAT(CONTROL, YAW_RATE_D);
    CONTROL_PARAM.RATE_I_MIN = PARAM_GET_FLOAT(CONTROL, RATE_I_MIN);
    CONTROL_PARAM.RATE_I_MAX = PARAM_GET_FLOAT(CONTROL, RATE_I_MAX);
    CONTROL_PARAM.RATE_D_MIN = PARAM_GET_FLOAT(CONTROL, RATE_D_MIN);
    CONTROL_PARAM.RATE_D_MAX = PARAM_GET_FLOAT(CONTROL, RATE_D_MAX);
    CONTROL_PARAM.P_Q_CMD_LIM = PARAM_GET_FLOAT(CONTROL, P_Q_CMD_LIM);
    CONTROL_PARAM.R_CMD_LIM = PARAM_GET_FLOAT(CONTROL, R_CMD_LIM);
}

void control_interface_step(uint32_t timestamp)
{
#ifdef FMT_ONLINE_PARAM_TUNING
    update_parameter();
#endif

    if (mcn_poll(fms_out_nod)) {
        mcn_copy(MCN_HUB(fms_output), fms_out_nod, &Controller_U.FMS_Out);
    }

    if (mcn_poll(ins_out_nod)) {
        mcn_copy(MCN_HUB(ins_output), ins_out_nod, &Controller_U.INS_Out);
    }

    Controller_step();

    mcn_publish(MCN_HUB(control_output), &Controller_Y.Control_Out);

    DEFINE_TIMETAG(control_output, 100);
    /* Log Control output bus data */
    if (check_timetag(TIMETAG(control_output))) {
        /* Log Control out data */
        mlog_push_msg((uint8_t*)&Controller_Y.Control_Out, MLOG_CONTROL_OUT_ID, sizeof(Control_Out_Bus));
    }
}

void control_interface_init(void)
{
    control_model_info.period = CONTROL_EXPORT.period;
    control_model_info.info = (char*)CONTROL_EXPORT.model_info;

    mcn_advertise(MCN_HUB(control_output), control_out_echo);

    fms_out_nod = mcn_subscribe(MCN_HUB(fms_output), NULL, NULL);
    ins_out_nod = mcn_subscribe(MCN_HUB(ins_output), NULL, NULL);

    Controller_init();

    update_parameter();
}