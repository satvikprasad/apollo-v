#pragma once

#include "defines.h"
#include "hashmap.h"
#include "state.h"
typedef struct UIToggleMenuData {
    F32 max_loffset;
    F32 max_roffset;
    F32 proc_button_width;
} UIToggleMenuData;

UIToggleMenuData
UIMeasureToggleMenu(HM_Hashmap *params,
                    HM_Hashmap *procs,
                    StateFont   font,
                    F32         font_size,
                    F32         padding,
                    F32         toggle_width);
B8
UIRenderPopUp(F32         border_size,
              F32         height,
              F32         padding,
              F32         offset,
              F32         alpha,
              HMM_Vec2    screen_size,
              StateFont   font,
              StatePopUp *pop_up,
              B8          enabled);
