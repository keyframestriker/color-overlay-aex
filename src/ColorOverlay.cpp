/*
    ColorOverlay.cpp — Adobe After Effects Effect Plug-in
    ────────────────────────────────────────────────────────────────────────────
    Purpose
        Overlays a solid fill color on a layer at a given opacity and blend mode,
        WITHOUT touching the layer's own alpha.  Solves the AE "Fill effect has
        no per-color Opacity" problem.

    Parameters
        Color      — color picker (fill color)
        Opacity    — 0–100 % float slider (fill opacity only)
        Blend Mode — Normal / Multiply / Add / Screen / Difference / Overlay

    Bit-depth support
        8 bpc  (always)
        16 bpc (requires PF_OutFlag_DEEP_COLOR_AWARE, set in GlobalSetup)

    Build
        See build.bat / .github/workflows/build.yml
    ────────────────────────────────────────────────────────────────────────────
*/

#include "ColorOverlay.h"
#include <algorithm>    // std::min
#include <cstdlib>      // std::abs (integer)
#include <cmath>        // std::fabs (double)

// ════════════════════════════════════════════════════════════════════════════
// Blend-mode helpers  (all values 0.0–1.0)
// ════════════════════════════════════════════════════════════════════════════

static inline double blend_channel(double s, double c, int mode)
{
    switch (mode) {
        case BM_NORMAL:
            return c;

        case BM_MULTIPLY:
            return s * c;

        case BM_ADD:
            return (s + c < 1.0) ? s + c : 1.0;

        case BM_SCREEN:
            return 1.0 - (1.0 - s) * (1.0 - c);

        case BM_DIFFERENCE:
            return (s > c) ? s - c : c - s;

        case BM_OVERLAY:
            return (s < 0.5)
                ? 2.0 * s * c
                : 1.0 - 2.0 * (1.0 - s) * (1.0 - c);

        default:
            return c;
    }
}

static inline double clamp01(double v)
{
    return (v < 0.0) ? 0.0 : (v > 1.0) ? 1.0 : v;
}

// ════════════════════════════════════════════════════════════════════════════
// Per-pixel render — 8 bpc
// ════════════════════════════════════════════════════════════════════════════
static PF_Err Render8(
    PF_EffectWorld        *inputP,
    PF_LayerDef           *outputW,
    const ColorOverlayInfo *info)
{
    const int    W    = outputW->width;
    const int    H    = outputW->height;
    const double op   = info->opacity;
    const double inv  = 1.0 - op;
    const int    mode = info->blend_mode;
    const double cr   = info->red;
    const double cg   = info->green;
    const double cb   = info->blue;

    for (int y = 0; y < H; ++y)
    {
        const PF_Pixel8 *inRow  = reinterpret_cast<const PF_Pixel8 *>(
                                    static_cast<const char *>(static_cast<const void *>(inputP->data))
                                    + y * inputP->rowbytes);
              PF_Pixel8 *outRow = reinterpret_cast<PF_Pixel8 *>(
                                    static_cast<char *>(static_cast<void *>(outputW->data))
                                    + y * outputW->rowbytes);

        for (int x = 0; x < W; ++x)
        {
            // Alpha: NEVER modified
            outRow[x].alpha = inRow[x].alpha;

            // Normalize src channels to [0,1]
            double sr = inRow[x].red   * (1.0 / 255.0);
            double sg = inRow[x].green * (1.0 / 255.0);
            double sb = inRow[x].blue  * (1.0 / 255.0);

            // Apply blend mode (color vs. src)
            double br = blend_channel(sr, cr, mode);
            double bg = blend_channel(sg, cg, mode);
            double bb = blend_channel(sb, cb, mode);

            // Mix by opacity and convert back to 8-bit
            outRow[x].red   = static_cast<A_u_char>(clamp01(sr * inv + br * op) * 255.0 + 0.5);
            outRow[x].green = static_cast<A_u_char>(clamp01(sg * inv + bg * op) * 255.0 + 0.5);
            outRow[x].blue  = static_cast<A_u_char>(clamp01(sb * inv + bb * op) * 255.0 + 0.5);
        }
    }
    return PF_Err_NONE;
}

// ════════════════════════════════════════════════════════════════════════════
// Per-pixel render — 16 bpc
// ════════════════════════════════════════════════════════════════════════════
static PF_Err Render16(
    PF_EffectWorld        *inputP,
    PF_LayerDef           *outputW,
    const ColorOverlayInfo *info)
{
    const int    W     = outputW->width;
    const int    H     = outputW->height;
    const double op    = info->opacity;
    const double inv   = 1.0 - op;
    const int    mode  = info->blend_mode;
    const double cr    = info->red;
    const double cg    = info->green;
    const double cb    = info->blue;
    // PF_MAX_CHAN16 = 32768
    const double kMax  = static_cast<double>(PF_MAX_CHAN16);

    for (int y = 0; y < H; ++y)
    {
        const PF_Pixel16 *inRow  = reinterpret_cast<const PF_Pixel16 *>(
                                     static_cast<const char *>(static_cast<const void *>(inputP->data))
                                     + y * inputP->rowbytes);
              PF_Pixel16 *outRow = reinterpret_cast<PF_Pixel16 *>(
                                     static_cast<char *>(static_cast<void *>(outputW->data))
                                     + y * outputW->rowbytes);

        for (int x = 0; x < W; ++x)
        {
            outRow[x].alpha = inRow[x].alpha;   // NEVER modified

            double sr = inRow[x].red   / kMax;
            double sg = inRow[x].green / kMax;
            double sb = inRow[x].blue  / kMax;

            double br = blend_channel(sr, cr, mode);
            double bg = blend_channel(sg, cg, mode);
            double bb = blend_channel(sb, cb, mode);

            outRow[x].red   = static_cast<A_u_short>(clamp01(sr * inv + br * op) * kMax + 0.5);
            outRow[x].green = static_cast<A_u_short>(clamp01(sg * inv + bg * op) * kMax + 0.5);
            outRow[x].blue  = static_cast<A_u_short>(clamp01(sb * inv + bb * op) * kMax + 0.5);
        }
    }
    return PF_Err_NONE;
}

// ════════════════════════════════════════════════════════════════════════════
// AE command handlers
// ════════════════════════════════════════════════════════════════════════════

static PF_Err About(PF_InData *in_data, PF_OutData *out_data,
                    PF_ParamDef *params[], PF_LayerDef *output)
{
    PF_SPRINTF(out_data->return_msg,
        "%s v%d.%d\r%s",
        PLUGIN_NAME, MAJOR_VERSION, MINOR_VERSION, DESCRIPTION);
    return PF_Err_NONE;
}

// ─────────────────────────────────────────────────────────────────────────────
static PF_Err GlobalSetup(PF_InData *in_data, PF_OutData *out_data,
                          PF_ParamDef *params[], PF_LayerDef *output)
{
    out_data->my_version = PF_VERSION(
        MAJOR_VERSION, MINOR_VERSION,
        BUG_VERSION, STAGE_VERSION, BUILD_VERSION);

    // DEEP_COLOR_AWARE → AE hands us 16-bpc worlds when the comp is in 16-bpc mode
    out_data->out_flags  = PF_OutFlag_DEEP_COLOR_AWARE;
    out_data->out_flags2 = 0;

    return PF_Err_NONE;
}

// ─────────────────────────────────────────────────────────────────────────────
static PF_Err ParamsSetup(PF_InData *in_data, PF_OutData *out_data,
                          PF_ParamDef *params[], PF_LayerDef *output)
{
    PF_Err      err = PF_Err_NONE;
    PF_ParamDef def;

    // ── Color picker (default: white) ─────────────────────────────────────
    AEFX_CLR_STRUCT(def);
    PF_ADD_COLOR("Color",
        /*R*/ 255, /*G*/ 255, /*B*/ 255,
        PARAM_COLOR);

    // ── Opacity 0–100 % ───────────────────────────────────────────────────
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(
        "Opacity",
        /*valid_min*/  0.0,   /*valid_max*/  100.0,
        /*slider_min*/ 0.0,   /*slider_max*/ 100.0,
        /*default*/    100.0,
        PF_Precision_TENTHS,
        PF_ValueDisplayFlag_PERCENT,
        /*flags*/ 0,
        PARAM_OPACITY);

    // ── Blend Mode drop-down ──────────────────────────────────────────────
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(
        "Blend Mode",
        /*num_choices*/ 6,
        /*default*/     1,          // 1-indexed → Normal
        "Normal|Multiply|Add|Screen|Difference|Overlay",
        PARAM_BLEND_MODE);

    out_data->num_params = NUM_PARAMS;
    return err;
}

// ─────────────────────────────────────────────────────────────────────────────
static PF_Err Render(PF_InData *in_data, PF_OutData *out_data,
                     PF_ParamDef *params[], PF_LayerDef *output)
{
    // ── Unpack parameters ─────────────────────────────────────────────────
    ColorOverlayInfo info;

    // Color picker gives PF_Pixel (8-bit ARGB); normalize to [0,1]
    const PF_Pixel8 &col = params[PARAM_COLOR]->u.cd.value;
    info.red   = col.red   * (1.0 / 255.0);
    info.green = col.green * (1.0 / 255.0);
    info.blue  = col.blue  * (1.0 / 255.0);

    // Float slider value is in [0.0, 100.0]
    info.opacity    = params[PARAM_OPACITY]->u.fs_d.value * (1.0 / 100.0);
    info.blend_mode = static_cast<int>(params[PARAM_BLEND_MODE]->u.pd.value);

    // ── Render ────────────────────────────────────────────────────────────
    PF_EffectWorld *inputP = &params[INPUT_LAYER]->u.ld;

    if (PF_WORLD_IS_DEEP(output)) {
        return Render16(inputP, output, &info);
    } else {
        return Render8(inputP, output, &info);
    }
}

// ════════════════════════════════════════════════════════════════════════════
// EffectMain — plug-in entry point (exported via ColorOverlay.def)
// ════════════════════════════════════════════════════════════════════════════
DllExport PF_Err
EffectMain(
    PF_Cmd          cmd,
    PF_InData      *in_data,
    PF_OutData     *out_data,
    PF_ParamDef    *params[],
    PF_LayerDef    *output,
    void           *extra)
{
    PF_Err err = PF_Err_NONE;

    try {
        switch (cmd) {
            case PF_Cmd_ABOUT:
                err = About(in_data, out_data, params, output);
                break;
            case PF_Cmd_GLOBAL_SETUP:
                err = GlobalSetup(in_data, out_data, params, output);
                break;
            case PF_Cmd_PARAMS_SETUP:
                err = ParamsSetup(in_data, out_data, params, output);
                break;
            case PF_Cmd_RENDER:
                err = Render(in_data, out_data, params, output);
                break;
            default:
                break;
        }
    }
    catch (PF_Err &thrown) {
        err = thrown;
    }

    return err;
}
