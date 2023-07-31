// -----------------------------------------------------------------------------
// Copyright 2006 Snap-on Incorporated. All Rights Reserved.
// -----------------------------------------------------------------------------
#ifndef __BK2_CMD_H
#define __BK2_CMD_H
// -----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
// -----------------------------------------------------------------------------

#define CMD_LOG                 0x00
#define CMD_NAK                 0xFF
#define CMD_INIT                1
#define CMD_RESET               2
#define CMD_LOGON               3
#define CMD_LOGOUT              4
#define CMD_GET_EVENT           5
#define CMD_GET_VERSION         6
#define CMD_GET_BUILD_NAME      7
#define CMD_GET_BUILD_DATE      8
#define CMD_GET_PO_STATUS       9
#define CMD_GET_KEYB            10
#define CMD_GET_DISP            11
#define CMD_GET_ADI             12
#define CMD_GET_ADE             13
#define CMD_BEEP                14
#define CMD_LIGHT               15
#define CMD_SLEEP               16
#define CMD_SLEEPB              17
#define CMD_SERVICE_MODE        18
#define CMD_STANDBY_MODE        19
#define CMD_ENABLE_EVENT        20
#define CMD_DISABLE_EVENT       21
#define CMD_PO_FLUSH            22
#define CMD_PO_FORMAT           23
#define CMD_PO_COPY_EEP1_EEP2   24
#define CMD_PO_COPY_EEP2_EEP1   25
#define CMD_PO_GET_SHORT        26
#define CMD_PO_GET_LONG         27
#define CMD_PO_GET_DOUBLE       28
#define CMD_PO_GET_STRING       29
#define CMD_PO_GET_COMPLEX      30
#define CMD_PO_SET_SHORT        31
#define CMD_PO_SET_LONG         32
#define CMD_PO_SET_DOUBLE       33
#define CMD_PO_SET_STRING       34
#define CMD_PO_SET_COMPLEX      35
#define CMD_STOP                36
#define CMD_ABORT               37
#define CMD_START               38
#define CMD_START_NR            39
#define CMD_START_DS            40
#define CMD_START_DSP           41
#define CMD_START_DSVOR         42
#define CMD_START_DSNSC         43
#define CMD_START_EMBCOND       44
#define CMD_WHEEL_LOCK          45
#define CMD_WHEEL_UNLOCK        46
#define CMD_WHEEL_CLAMP         47
#define CMD_WHEEL_UNCLAMP       48
#define CMD_GET_KC              49
#define CMD_GET_KX              50
#define CMD_GET_KR              51
#define CMD_GET_DWP             52
#define CMD_GET_DWV             53
#define CMD_GET_DWR             54
#define CMD_GET_IEPP            55
#define CMD_GET_IEPO            56
#define CMD_GET_IEM_STATUS      57
#define CMD_GET_IEP_STATUS      58
#define CMD_GET_TEMPERATURE     59
#define CMD_GET_LINE_FREQ       60
#define CMD_GET_LINE_VOLT       61
#define CMD_GET_VCC             62
#define CMD_RESET_CNT_ALL       63
#define CMD_RESET_CNT_CAL       64
#define CMD_GET_CNT_SPIN        65
#define CMD_GET_CNT_SPIN_CAL    66
#define CMD_GET_CNT_SPIN_SRV    67
#define CMD_GET_CNT_SPIN_USR    68
#define CMD_GET_CNT_SPIN_OK     69
#define CMD_INC_CNT_SPIN_OK     70
#define CMD_INC_CNT_SPIN        71
#define CMD_CLEAR_ERROR         72
#define CMD_GET_ERROR           73
#define CMD_SET_ERROR           74
#define CMD_GET_LAST_ERROR      75
#define CMD_ACK_LAST_ERROR      76
#define CMD_GET_DS              77
#define CMD_GET_DC              78
#define CMD_SET_DC              79
#define CMD_GET_MS              80
#define CMD_GET_MC              81
#define CMD_SET_MC              82
#define CMD_GET_MDASP           83
#define CMD_SET_MDASP           84
#define CMD_GET_DPS             85
#define CMD_GET_DPC             86
#define CMD_SET_DPC             87
#define CMD_GET_MSPEED          88
#define CMD_GET_DWVOR           89
#define CMD_SET_DWVOR           90
#define CMD_GET_ASSP            91
#define CMD_SET_ASSP            92
#define CMD_GET_ASSS            93
#define CMD_GET_ASSC            94
#define CMD_SET_ASSC            95
#define CMD_GET_BRAKE_DC        96
#define CMD_SET_BRAKE_DC        97
#define CMD_GET_DBP             98
#define CMD_SET_DBP             99
#define CMD_GET_DBPC            100
#define CMD_SET_DBPC            101
#define CMD_GET_CDC             102
#define CMD_SET_CDC             103
#define CMD_GET_CMIN            104
#define CMD_SET_CMIN            105
#define CMD_GET_CMAX            106
#define CMD_SET_CMAX            107
#define CMD_GET_CS              108
#define CMD_GET_CN              109
#define CMD_GET_CT              110
#define CMD_GET_C1              111
#define CMD_GET_C2              112
#define CMD_GET_C1x             113
#define CMD_GET_C2x             114
#define CMD_RIM_CLEAR           115
#define CMD_RIM_BACKUP          116
#define CMD_RIM_RESTORE         117
#define CMD_RIM_PROF_LOAD       118
#define CMD_RIM_PROF_SAVE       119
#define CMD_GET_RS              120
#define CMD_GET_RTW             121
#define CMD_SET_RTW             122
#define CMD_GET_RTP             123
#define CMD_SET_RTP             124
#define CMD_GET_RN              125
#define CMD_SET_RN              126
#define CMD_GET_RM              127
#define CMD_SET_RM              128
#define CMD_GET_RCP             129
#define CMD_GET_RCPS            130
#define CMD_GET_RFLNG           131
#define CMD_SET_RFLNG           132
#define CMD_GET_S1S             133
#define CMD_GET_S2S             134
#define CMD_GET_S1E             135
#define CMD_GET_S11             136
#define CMD_GET_S12             137
#define CMD_GET_S1R             138
#define CMD_GET_S2E             139
#define CMD_GET_S21             140
#define CMD_GET_S22             141
#define CMD_GET_S2R             142
#define CMD_SET_S11             143
#define CMD_SET_S12             144
#define CMD_GET_S1TE            145
#define CMD_CAL_SAPE_1D         146
#define CMD_CAL_SAPE_2D         147
#define CMD_CAL_SAPE_3D         148
#define CMD_CAL_SAPE_GEO        149
#define CMD_CAL_SAPE_STORE      150
#define CMD_CAL_BAL             151
#define CMD_CAL_BAL_GAIN        152
#define CMD_CAL_BAL_USER        153
#define CMD_CAL_BAL_STORE       154
#define CMD_GET_PHASE_REAR      155
#define CMD_SET_PHASE_REAR      156
#define CMD_GET_PHASE_FRONT     157
#define CMD_SET_PHASE_FRONT     158
#define CMD_GET_GAIN_LEFT       159
#define CMD_GET_GAIN_RIGHT      160
#define CMD_GET_VIRTUAL_LEFT    161
#define CMD_GET_VIRTUAL_DIST    162
#define CMD_BAL                 163
#define CMD_SPLIT_2             164
#define CMD_SPLIT_N             165
#define CMD_OPT_CALC            166
#define CMD_OPT_RECOM           167
#define CMD_OPT_CHECK           168
#define CMD_REQUEST_TIMER       169
#define CMD_RELEASE_TIMER       170
#define CMD_TEST                171
#define CMD_GET_TS              172
#define CMD_GET_TAIC            173
#define CMD_GET_BELT            174
#define CMD_GET_IESTAT          175
#define CMD_DISPLAY             176
#define CMD_WGCLOSE             177
#define CMD_AWP_STATUS          178
#define CMD_AWP_BRIDGE          179
#define CMD_AWP_RAWFD           180
#define CMD_CAL_BAL_PHASE       181
#define CMD_LASER_KIT           182
#define CMD_SONAR_KIT           183
#define CMD_LASER_KIT_CAL1      184
#define CMD_LASER_KIT_CAL2      185
#define CMD_GET_SN              186
#define CMD_SET_SN              187
#define CMD_PO_USAGE            188
#define CMD_STM32_SEND          189
#define CMD_STM32_RECV          190
#define CMD_LIFT                191
#define CMD_SET_LP              192
#define CMD_REAR_SCANNER        193
#define CMD_SONAR_VALUE         194
#define CMD_REVERSE_LOW         195
#define CMD_REVERSE_CAP         196
#define CMD_REVERSE_PULSE       197
#define CMD_SET_SMIN            198
#define CMD_GET_SMIN            199
#define CMD_SET_SEXTEND         200
#define CMD_GET_SEXTEND         201
#define CMD_INC_CNT_SPIN_LESS_5  202
#define CMD_INC_CNT_SPIN_LESS_10 203
#define CMD_GET_CNT_SPIN_LESS_5  204
#define CMD_GET_CNT_SPIN_LESS_10 205

/* StefanoV: new commands definitions - begin */
#define CMD_GET_APP_VERSION     210
#define CMD_SET_APP_VERSION     211
#define CMD_REGISTER_CCODE      212
#define CMD_GET_CCODE           213
#define CMD_SET_BALANCING_EVENT 214
#define CMD_GET_BALANCING_EVENT 215
#define CMD_LOGON_RO            216 /* This is a special logon command which do NOT touch the current event queue, it has to be used when we use commands to get system infos */
#define CMD_LOGOUT_RO           217 /* This is a special logon command which do NOT touch the current event queue, it has to be used when we use commands to get system infos */
#define CMD_GET_ASYNC_EVT_TYPE  218 /* Return the type of async event currently available (it is a bitmask):
                                        - if it contains 'ASYNC_EVT_BAL' you can call 'CMD_GET_BALANCING_EVENT';
                                        - if it contains 'ASYNC_EVT_CCODE' you can call 'CMD_GET_CCODE'.
                                        - if it contains 'ASYNC_EVT_UIERR' you can call 'CMD_GET_UIERR'. */
#define CMD_RESET_BALANCING_PROGR 219
#define CMD_SET_MACHINE_ENABLE    220 /* Set the machine enable flag: 0 -> DISABLED; 1 -> ENABLED */
#define CMD_GET_MACHINE_ENABLE    221 /* Get the current machine enable flag */
#define CMD_SET_IOT_MODULE_FEED   222 /* Set the feed to the WB 0..65535; this value is decremented by the WB at each launch, if reach 0 the launches are disabled */
#define CMD_GET_IOT_MODULE_FEED   223 /* Get the current feed */
#define CMD_SET_IOT_MODULE_HAVE_INTERNET 224 /* Set the have_internet flag on the WB, if the flag is 0 the launches are disabled */
#define CMD_GET_IOT_MODULE_HAVE_INTERNET 225 /* Get the have_internet flag */
#define CMD_SET_FORCE_MACHINE_ENABLE    226 /* Set the force machine enable flag: 0 -> DISABLED; 1 -> ENABLED */
#define CMD_GET_FORCE_MACHINE_ENABLE    227 /* Get the current force machine enable flag */
#define CMD_GET_HAVE_PPU_FEATURES       228 /* Get the PPU feature presence flag */
#define CMD_SET_CUSTOM_ERROR_CONDITION  229 /* Set an error condition received by the client */
#define CMD_SET_UIERR                   230 /* Register a new UI error */
#define CMD_GET_UIERR                   231 /* Get the UI registered error */

typedef enum
{
    ASYNC_EVT_NOTHING = 0,
    ASYNC_EVT_BAL     = 1,
    ASYNC_EVT_CCODE   = 2,
    ASYNC_EVT_UIERR   = 4,
    ASYNC_EVT_RESERVED= 8 /* reserved for future use */
} ASYNC_EVT_TYPE;

/* This struct is used to pass balancing data from/to the BK2 throught the commands 'CMD_SET_BALANCING_EVENT' and 'CMD_GET_BALANCING_EVENT' */
typedef struct
{
  unsigned char is_data_valid;
  struct {
    double weight_left;
    double weight_right;
    double weight_static;
    unsigned char weight_unit_type; /* 0=none; 1=ounces; 2=grams; 3=kilograms */
  } imbalance;
  struct {
    double offset;
    double diameter;
    double width;
    double aspect_ratio;
    unsigned char length_unit_type; /* 0=none; 1=inches; 2=mm */
    unsigned char vehicle_type; /* 0=none; 1=Car; 2=LTruck; 3=Truck; 4=SUV; 5=Motorcycle; 6=Motorcycle_jbeg; 7=CarWithExt_jbeg; 8=Special; 9=Internal */
  } wheel;
  unsigned char alu_mode;
  unsigned char balancing_progr;
  double flange_offset;
} BALANCING_EVENT;

/* This struct is used to pass UI error codes from/to the BK2 throught the commands 'CMD_SET_UIERR' and 'CMD_GET_UIERR' */
typedef struct
{
  unsigned char is_data_valid;
  unsigned char uierr_type;
  unsigned long uierr_code;
} UIERR_EVENT;

/* StefanoV: new commands definitions - end */

void BK_Host ( void ); // ext command server

// -----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
// -----------------------------------------------------------------------------
#endif //__BK2_CMD_H
// -----------------------------------------------------------------------------
