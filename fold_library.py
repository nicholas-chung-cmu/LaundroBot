# fold_library.py
# Raspberry Pi — Fold Sequence Library
# Responsible for: storing all garment/size parameters and fold step order
# Runs on: Raspberry Pi
# Depends on: nothing (pure data)

# ─── Fold Order ─────────────────────────────────────────────────────────────
# Defines which subsystem folds first for each garment type.
# "BIG" = top/bottom folders, "SMALL" = left/right folders.

FOLD_ORDER = {
    "SHIRT": ["BIG", "SMALL"],
    "PANTS": ["SMALL", "BIG"],   # adjust to actual mechanical requirements
}

# ─── Big Folder Positions ───────────────────────────────────────────────────
# Big folder DC motors move in discrete fixed increments.
# POS_0 = fully inward (home), POS_1, POS_2, POS_3 = evenly spaced outward.
# INCREMENT_MM defines the step size in mm (same for all positions).
# Actual encoder count = position_index * INCREMENT_MM * COUNTS_PER_MM
#
# TODO: measure and fill in INCREMENT_MM and COUNTS_PER_MM after hardware calibration

BIG_INCREMENT_MM = 0        # TODO: physical increment between discrete positions (mm)
BIG_COUNTS_PER_MM = 0       # TODO: encoder counts per mm for big folder DC motors

BIG_POSITIONS = {
    # Maps (garment, size) → position index (0 = home, 1/2/3 = outward slots)
    "SHIRT": {"S": 1, "M": 2, "L": 3},
    "PANTS": {"S": 1, "M": 2, "L": 3},  # TODO: tune these indices per garment
}

# ─── Small Folder Distances ─────────────────────────────────────────────────
# Small folder DC motors accept a continuous target position in mm.
# Final encoder count = distance_mm * COUNTS_PER_MM + offset_correction
#
# TODO: fill in distances and COUNTS_PER_MM after hardware calibration

SMALL_COUNTS_PER_MM = 0     # TODO: encoder counts per mm for small folder DC motors

SMALL_DISTANCES_MM = {
    # Maps (garment, size) → target shift distance in mm (before offset correction)
    "SHIRT": {"S": 0, "M": 0, "L": 0},   # TODO: tune per garment
    "PANTS": {"S": 0, "M": 0, "L": 0},   # TODO: tune per garment
}

# ─── Servo Angles ───────────────────────────────────────────────────────────
# Fold servos move from a resting angle to a fold angle.
# Each subsystem may have different geometry, so angles are stored separately.
#
# TODO: tune all angles after physical assembly

SERVO_ANGLES = {
    "BIG":   {"rest": 0, "fold": 90},   # TODO: tune
    "SMALL": {"rest": 0, "fold": 90},   # TODO: tune
}

# ─── Helper: build_sequence ─────────────────────────────────────────────────
def build_sequence(garment: str, size: str, offset_x: int, offset_y: int) -> list:
    """
    Returns an ordered list of command dicts for the given garment/size/offset.
    Called by sequencer.py after SCAN results are received from Teensy.

    Each command dict has the form:
        {"cmd": "SHIFT_BIG",   "pos_index": int}
        {"cmd": "SHIFT_SMALL", "counts": int}
        {"cmd": "FOLD_BIG"}
        {"cmd": "FOLD_SMALL"}
        {"cmd": "UNFOLD_BIG"}
        {"cmd": "UNFOLD_SMALL"}

    Offset correction:
        offset_x (left/right, mm) is added to the small folder distance
        offset_y (top/bottom, mm) is used to shift big folder position if it
          pushes into the next discrete slot — otherwise clamp to nearest slot.
        TODO: define clamping/rounding behaviour for offset_y with team.
    """

    sequence = []
    fold_order = FOLD_ORDER[garment]

    # Compute big folder position index
    big_pos_index = BIG_POSITIONS[garment][size]
    # TODO: apply offset_y — round to nearest valid index, clamp to [0, max_index]

    # Compute small folder encoder count
    small_dist_mm = SMALL_DISTANCES_MM[garment][size]
    small_dist_mm += offset_x          # continuous offset correction
    small_counts = int(small_dist_mm * SMALL_COUNTS_PER_MM)

    # Build shift commands first (position folders before folding)
    sequence.append({"cmd": "SHIFT_BIG",   "pos_index": big_pos_index})
    sequence.append({"cmd": "SHIFT_SMALL", "counts": small_counts})

    # Build fold commands in garment-specific order
    for subsystem in fold_order:
        sequence.append({"cmd": f"FOLD_{subsystem}"})
        # TODO: add delay or WAIT_ACK step between folds if needed

    # Unfold in reverse order after folding complete
    for subsystem in reversed(fold_order):
        sequence.append({"cmd": f"UNFOLD_{subsystem}"})

    return sequence
