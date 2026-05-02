// fold_library.h
// Teensy 4.1 — Fold Sequence Library
// Responsible for: physical constants, bounds checking, and sequence generation
// Runs on: Teensy 4.1
// Depends on: nothing (pure data + logic)
//
// ── Mechanical model ──────────────────────────────────────────────────────────
//
// Each subsystem has one rack-and-pinion (doubled for strength, functionally one).
// The pinion is driven by a DC motor with encoder. Both racks move symmetrically:
// one folder goes outward while the other goes inward by the same amount.
// Because of this, the system has exactly ONE position value per subsystem —
// the distance from board center to each folder. Both folders are always at
// that same distance from board center (one left, one right / one up, one down).
//
// To fold an off-center item:
//   1. Command the pinion to position A  → fold one side
//   2. Translate pinion inward by 2*offset → both folders shift
//   3. Other side is now at position B  → fold other side
//
// All positions are absolute distances from BOARD CENTER in mm (or encoder counts).
// Positive encoder direction = outward from center.
//
// ── Coordinate conventions ───────────────────────────────────────────────────
//
// centerX_mm: clothing center offset from board center, positive = right
// centerY_mm: clothing center offset from board center, positive = up
//
// Small subsystem (left/right): positions are distances from board center.
//   - Left fold:  command to (half_width + centerX_mm)  — farther if shifted right
//   - Right fold: command to (half_width - centerX_mm)  — after translating 2*centerX_mm inward
//   Wait — if shifted LEFT (centerX negative), left side is farther:
//   - Left fold position  = half_width + abs(centerX_mm) when shifted left
//   More cleanly: left_pos  = half_width - centerX_mm
//                 right_pos = half_width + centerX_mm
//   Translation between folds = 2 * centerX_mm (positive = move outward for right fold)
//   Verified: centered item (centerX=0) → both folds at half_width. Correct.
//   Shifted 20mm left (centerX=-20): left_pos=170, right_pos=130. Correct.
//
// Big subsystem (top/bottom): same logic on Y axis, positions are discrete slots.
//   bottom_pos_mm = half_height - centerY_mm
//   top_pos_mm    = half_height + centerY_mm
//   Translation between folds = 2 * centerY_mm (rounded to nearest slot)

#pragma once
#include <Arduino.h>

// ─── Physical Range Constants ─────────────────────────────────────────────────

constexpr float INCHES_TO_MM     = 25.4f;

// Small subsystem (left/right folders): continuous, 4"–9" from board center
constexpr float SMALL_MIN_MM     = 4.0f * INCHES_TO_MM;   // 101.6 mm — hard minimum
constexpr float SMALL_MAX_MM     = 9.0f * INCHES_TO_MM;   // 228.6 mm — hard maximum

// Big subsystem (top/bottom folders): discrete slots, 4"–8" from board center
constexpr float BIG_MIN_MM       = 4.0f * INCHES_TO_MM;   // 101.6 mm — home position (slot 0)
constexpr float BIG_MAX_MM       = 8.0f * INCHES_TO_MM;   // 203.2 mm — hard maximum
constexpr float BIG_RANGE_MM     = BIG_MAX_MM - BIG_MIN_MM; // 101.6 mm total travel

// ─── Calibration Constants ────────────────────────────────────────────────────
// TODO: measure all on physical hardware before first run.

constexpr float SMALL_COUNTS_PER_MM  = 0.0f;  // TODO: encoder counts per mm, small subsystem
constexpr float BIG_COUNTS_PER_MM    = 0.0f;  // TODO: encoder counts per mm, big subsystem
constexpr float BIG_INCREMENT_MM     = 0.0f;  // TODO: mm per discrete slot (e.g. 12.7 = 0.5")

// Derived — recompute after TODO values above are filled in:
constexpr int   BIG_MAX_INDEX        = 0;     // TODO: (int)(BIG_RANGE_MM / BIG_INCREMENT_MM)

// Converts an absolute mm distance from board center to encoder counts.
// Home (slot 0 for big, minimum for small) = 0 counts after homing sequence.
inline int smallMmToCounts(float mm) {
    return (int)((mm - SMALL_MIN_MM) * SMALL_COUNTS_PER_MM);
}
inline int bigMmToIndex(float mm) {
    if (BIG_INCREMENT_MM == 0.0f) return 0;
    return (int)roundf((mm - BIG_MIN_MM) / BIG_INCREMENT_MM);
}

// ─── Servo Angles ─────────────────────────────────────────────────────────────
// TODO: tune after physical assembly.

constexpr int BIG_REST_ANGLE   = 0;    // TODO
constexpr int BIG_FOLD_ANGLE   = 90;   // TODO
constexpr int SMALL_REST_ANGLE = 0;    // TODO
constexpr int SMALL_FOLD_ANGLE = 90;   // TODO

// ─── Garment and Size Types ───────────────────────────────────────────────────

enum GarmentType {
    GARMENT_SHIRT,
    GARMENT_PANTS_LEFT,   // pants centered on left side of board
    GARMENT_PANTS_RIGHT,  // pants centered on right side of board
    GARMENT_UNKNOWN
};
enum FoldSize { SIZE_S, SIZE_M, SIZE_L };

// ─── Fold Target Dimensions ───────────────────────────────────────────────────
// half_width  = half the target rectangle's width  (small subsystem travel from clothing center)
// half_height = half the target rectangle's height (big subsystem travel from clothing center)
// These are in mm, measured from the CLOTHING CENTER, not the board center.
// The board-center position is computed by adding centerX/Y_mm at sequence build time.
//
// TODO: fill in after deciding on S/M/L rectangle dimensions.

struct FoldDimensions {
    float half_width_mm;    // small subsystem: each folder sits this far from clothing center
    float half_height_mm;   // big subsystem:   each folder sits this far from clothing center
};

// Shirt dimensions per size
constexpr FoldDimensions SHIRT_DIMS[3] = {
    { 0.0f, 0.0f },   // SIZE_S — TODO
    { 0.0f, 0.0f },   // SIZE_M — TODO
    { 0.0f, 0.0f },   // SIZE_L — TODO
};

// Pants dimensions per size (only half_height used; width is a single fold)
constexpr FoldDimensions PANTS_DIMS[3] = {
    { 0.0f, 0.0f },   // SIZE_S — TODO
    { 0.0f, 0.0f },   // SIZE_M — TODO
    { 0.0f, 0.0f },   // SIZE_L — TODO
};

// ─── Step Types ───────────────────────────────────────────────────────────────

enum StepType {
    STEP_SMALL_RESET,       // drive small folders inward to SMALL_MIN_MM (4") — start of every cycle
    STEP_SHIFT_BIG,         // translate big pinion to absolute slot index
    STEP_SHIFT_SMALL,       // translate small pinion to absolute encoder counts
    STEP_FOLD_BIG_BOTTOM,   // actuate bottom folder servo (sweeps up)
    STEP_FOLD_BIG_TOP,      // actuate top folder servo (sweeps down)
    STEP_FOLD_SMALL_LEFT,   // actuate left folder servo (sweeps right)
    STEP_FOLD_SMALL_RIGHT,  // actuate right folder servo (sweeps left)
    STEP_UNFOLD_BIG,        // return big servos to rest
    STEP_UNFOLD_SMALL,      // return small servos to rest
};

struct FoldStep {
    StepType type;
    int      arg;   // slot index for SHIFT_BIG, encoder counts for SHIFT_SMALL, unused otherwise
};

constexpr int MAX_STEPS = 16;  // generous upper bound

struct FoldSequence {
    FoldStep steps[MAX_STEPS];
    int      count       = 0;
    bool     valid       = false;
    char     errorReason[48] = "";
};

// ─── Bounds Checking ──────────────────────────────────────────────────────────

inline bool smallInBounds(float mm) {
    return mm >= SMALL_MIN_MM && mm <= SMALL_MAX_MM;
}
inline bool bigInBounds(int slotIndex) {
    return slotIndex >= 0 && slotIndex <= BIG_MAX_INDEX;
}

// ─── Sequence Builder ─────────────────────────────────────────────────────────

inline FoldSequence buildSequence(
    GarmentType garment,
    FoldSize    size,
    float       centerX_mm,   // clothing center X from board center, + = right
    float       centerY_mm    // clothing center Y from board center, + = up
) {
    FoldSequence seq;

    if (garment == GARMENT_UNKNOWN) {
        snprintf(seq.errorReason, sizeof(seq.errorReason), "UNKNOWN_GARMENT");
        return seq;   // valid remains false
    }

    int s = (int)size;
    bool isShirt = (garment == GARMENT_SHIRT);
    const FoldDimensions& dims = isShirt ? SHIRT_DIMS[s] : PANTS_DIMS[s];

    // ── Compute big (top/bottom) fold positions ───────────────────────────────
    // bottom_mm and top_mm are absolute distances from board center.
    // bottom fold: clothing center is centerY_mm above board center,
    //   so bottom folder must be (half_height - centerY_mm) from board center.
    // top fold:    top folder must be (half_height + centerY_mm) from board center.
    float bottom_mm = dims.half_height_mm - centerY_mm;
    float top_mm    = dims.half_height_mm + centerY_mm;

    int bottom_slot = bigMmToIndex(bottom_mm);
    int top_slot    = bigMmToIndex(top_mm);

    // Translation between bottom and top fold = 2 * centerY_mm, rounded to slots.
    // After folding bottom at bottom_slot, we shift to top_slot.
    // If centerY == 0, top_slot == bottom_slot and no net translation occurs,
    // but we still issue the SHIFT_BIG command (it will be a no-op on the motor).

    if (!bigInBounds(bottom_slot) || !bigInBounds(top_slot)) {
        snprintf(seq.errorReason, sizeof(seq.errorReason), "PLACEMENT_OUT_OF_RANGE");
        return seq;
    }

    // ── Compute small (left/right) fold positions ─────────────────────────────
    // For shirts: both left and right folds.
    // For pants:  one side only, determined by which side the pants are placed on.
    //
    // left_mm  = half_width - centerX_mm  (farther if item shifted right)
    // right_mm = half_width + centerX_mm  (farther if item shifted left)
    // Wait — re-derive carefully:
    //   Item center is centerX_mm to the right of board center.
    //   Left edge of target rectangle is half_width to the LEFT of item center.
    //   In board coords: left edge is at (centerX_mm - half_width) from board center.
    //   But folders are positioned at absolute distance from board center (always positive outward).
    //   Left folder position from board center = half_width - centerX_mm
    //     (if item is right of center, left folder is closer to board center)
    //   Right folder position from board center = half_width + centerX_mm
    //     (if item is right of center, right folder is farther from board center)
    float left_mm  = dims.half_width_mm - centerX_mm;
    float right_mm = dims.half_width_mm + centerX_mm;

    // ── Step 1: SMALL_RESET ───────────────────────────────────────────────────
    // Always first. Drives small folders to minimum (4") so big folders can move
    // without collision. Happens every cycle regardless of garment or offset.
    seq.steps[seq.count++] = { STEP_SMALL_RESET, 0 };

    // ── Step 2: Big subsystem — SHIFT + FOLD_BOTTOM + (translate) + FOLD_TOP ──
    seq.steps[seq.count++] = { STEP_SHIFT_BIG, bottom_slot };
    seq.steps[seq.count++] = { STEP_FOLD_BIG_BOTTOM, 0 };
    seq.steps[seq.count++] = { STEP_UNFOLD_BIG, 0 };

    if (top_slot != bottom_slot) {
        seq.steps[seq.count++] = { STEP_SHIFT_BIG, top_slot };
    }
    seq.steps[seq.count++] = { STEP_FOLD_BIG_TOP, 0 };
    seq.steps[seq.count++] = { STEP_UNFOLD_BIG, 0 };

    // ── Step 3: Small subsystem — depends on garment type ─────────────────────

    if (isShirt) {
        // Shirt: fold LEFT then RIGHT.
        // Bounds check both positions.
        if (!smallInBounds(left_mm) || !smallInBounds(right_mm)) {
            snprintf(seq.errorReason, sizeof(seq.errorReason), "PLACEMENT_OUT_OF_RANGE");
            return seq;
        }
        seq.steps[seq.count++] = { STEP_SHIFT_SMALL, smallMmToCounts(left_mm) };
        seq.steps[seq.count++] = { STEP_FOLD_SMALL_LEFT, 0 };
        seq.steps[seq.count++] = { STEP_UNFOLD_SMALL, 0 };

        if (left_mm != right_mm) {
            seq.steps[seq.count++] = { STEP_SHIFT_SMALL, smallMmToCounts(right_mm) };
        }
        seq.steps[seq.count++] = { STEP_FOLD_SMALL_RIGHT, 0 };
        seq.steps[seq.count++] = { STEP_UNFOLD_SMALL, 0 };

    } else {
        // Pants: single sideways fold. Which folder depends on which side the
        // pants are placed on (detected by sensor, encoded in garment type).
        // User places pants so that the fold edge is within the small folder range.
        // The folder on the same side as the pants center performs the fold.
        if (garment == GARMENT_PANTS_LEFT) {
            if (!smallInBounds(left_mm)) {
                snprintf(seq.errorReason, sizeof(seq.errorReason), "PLACEMENT_OUT_OF_RANGE");
                return seq;
            }
            seq.steps[seq.count++] = { STEP_SHIFT_SMALL, smallMmToCounts(left_mm) };
            seq.steps[seq.count++] = { STEP_FOLD_SMALL_LEFT, 0 };
            seq.steps[seq.count++] = { STEP_UNFOLD_SMALL, 0 };
        } else {
            // GARMENT_PANTS_RIGHT
            if (!smallInBounds(right_mm)) {
                snprintf(seq.errorReason, sizeof(seq.errorReason), "PLACEMENT_OUT_OF_RANGE");
                return seq;
            }
            seq.steps[seq.count++] = { STEP_SHIFT_SMALL, smallMmToCounts(right_mm) };
            seq.steps[seq.count++] = { STEP_FOLD_SMALL_RIGHT, 0 };
            seq.steps[seq.count++] = { STEP_UNFOLD_SMALL, 0 };
        }
    }

    seq.valid = true;
    return seq;
}
