#pragma once

// ─── Standard Windows + AE SDK includes ──────────────────────────────────────
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "AEConfig.h"
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"

// ─── Version ──────────────────────────────────────────────────────────────────
#define MAJOR_VERSION   1
#define MINOR_VERSION   0
#define BUG_VERSION     0
#define STAGE_VERSION   PF_Stage_DEVELOP
#define BUILD_VERSION   1

// ─── Plugin identity ──────────────────────────────────────────────────────────
// MATCH_NAME must be globally unique — change prefix if you fork this
#define PLUGIN_NAME     "Color Overlay"
#define MATCH_NAME      "KFSK Color Overlay"
#define CATEGORY        "Stylize"
#define DESCRIPTION     "Overlays a color onto the layer. " \
                        "Layer alpha is never modified."

// ─── Parameter indices  (INPUT_LAYER must stay 0) ─────────────────────────────
enum {
    INPUT_LAYER = 0,
    PARAM_COLOR,        // 1 — color picker
    PARAM_OPACITY,      // 2 — float slider 0–100 %
    PARAM_BLEND_MODE,   // 3 — popup: Normal / Multiply / Add / Screen / Difference / Overlay
    NUM_PARAMS          // 4
};

// ─── Blend mode values (1-indexed, matching popup order) ──────────────────────
enum BlendMode {
    BM_NORMAL     = 1,
    BM_MULTIPLY   = 2,
    BM_ADD        = 3,
    BM_SCREEN     = 4,
    BM_DIFFERENCE = 5,
    BM_OVERLAY    = 6
};

// ─── Data passed to per-pixel helpers ─────────────────────────────────────────
struct ColorOverlayInfo {
    double red, green, blue;  // fill color, 0.0–1.0  (converted from 8-bit picker)
    double opacity;           // 0.0–1.0
    int    blend_mode;        // BlendMode value
};

// ─── Entry point ──────────────────────────────────────────────────────────────
DllExport PF_Err EffectMain(
    PF_Cmd          cmd,
    PF_InData      *in_data,
    PF_OutData     *out_data,
    PF_ParamDef    *params[],
    PF_LayerDef    *output,
    void           *extra);
