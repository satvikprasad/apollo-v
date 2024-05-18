#include "ui.h"
#include "lmath.h"
#include "parameter.h"
#include "procedures.h"
#include "raygui.h"
#include "raylib.h"
#include "state.h"
#include <string.h>

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

B8
UIRenderPopUp(F32         border_size,
              F32         height,
              F32         padding,
              F32         offset,
              F32         alpha,
              HMM_Vec2    screen_size,
              StateFont   font,
              StatePopUp *pop_up,
              B8          enabled) {
    char text[512];
    strcpy(text, pop_up->text);

    F32 width = MaxF32(
        100,
        MeasureTextEx(FontClosestToSize(font, 20), pop_up->text, 20, 1).x + 20);

    if (width > screen_size.Width - padding * 2) {
        width = screen_size.Width - padding * 2;

        F32 w = MeasureTextEx(FontClosestToSize(font, 20), "A", 20, 1).x + 1;

        strcpy(text,
               TextFormat("%s...",
                          TextSubtext(pop_up->text, 0,
                                      (screen_size.Width - padding * 2) / w)));
    }

    DrawRectangleRec(
        (Rectangle){screen_size.Width - padding - width - border_size,
                    screen_size.Height - (height + padding) - border_size -
                        offset,
                    width + 2 * border_size, height + 2 * border_size},
        (Color){204, 204, 204, alpha});

    DrawRectangleRec(
        (Rectangle){screen_size.Width - padding - width,
                    screen_size.Height - (height + padding) - offset, width,
                    height},
        (Color){137, 116, 185, alpha});

    RendererDrawTextCenter(
        FontClosestToSize(font, 20), text,
        HMM_V2(screen_size.Width - padding * 1.5 - width / 2,
               screen_size.Height - (height + padding) + height / 2 - offset),
        WHITE);

    if (!enabled) {
        return false;
    }

    return GuiButton((Rectangle){screen_size.Width - padding * 2.5,
                                 screen_size.Height - (height + padding) +
                                     padding / 2 - offset,
                                 padding, padding},
                     "X");
}
