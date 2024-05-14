#include "ui.h"
#include "lmath.h"
#include "parameter.h"
#include "procedures.h"
#include "raylib.h"
#include "state.h"

UIToggleMenuData
UIMeasureToggleMenu(HM_Hashmap *params,
                    HM_Hashmap *procs,
                    StateFont   font,
                    F32         font_size,
                    F32         padding,
                    F32         toggle_width) {
    UIToggleMenuData data = {0};

    U32 _, i = 0;

    Parameter *parameter;
    while (ParameterIter(params, &_, &parameter)) {
        if (!parameter) {
            continue;
        }

        F32 offset =
            RayToHMMV2(MeasureTextEx(FontClosestToSize(font, font_size),
                                     parameter->name, font_size, 1))
                .X;

        if (offset > data.max_loffset) {
            data.max_loffset = offset;
        }

        offset =
            RayToHMMV2(MeasureTextEx(FontClosestToSize(font, font_size),
                                     TextFormat("[%.2f]", parameter->value),
                                     font_size, 1))
                .X;

        if (offset > data.max_roffset) {
            data.max_roffset = offset;
        }
    }

    // Measure procedure buttons
    data.proc_button_width = 200;
    Procedure *proc;
    _ = 0;
    i = 0;
    while (ProcedureIter(procs, &_, &proc)) {
        if (!proc) {
            continue;
        }

        F32 width = MeasureTextEx(FontClosestToSize(font, font_size),
                                  TextFormat("%s", proc->name), font_size, 1)
                        .x;

        if (width > data.proc_button_width) {
            data.proc_button_width = width;
        }

        ++i;
    }

    return data;
}
