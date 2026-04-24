// servo_controller.h
// Teensy 4.1 — Servo Controller
// Responsible for: controlling all 8 fold/unfold servos individually
// Runs on: Teensy 4.1
//
// Hardware layout:
//   Big folder subsystem    — 4 servos: 2 top (A+B), 2 bottom (A+B)
//   Small folder subsystem  — 4 servos: 2 left (A+B), 2 right (A+B)
//
// Each folder pair (e.g. top A + top B) always moves together.
// Mirrored partners within a pair may need opposite sign — confirm with mechanical team.
// "Fold"   → servos sweep from REST to FOLD angle
// "Unfold" → servos sweep from FOLD back to REST angle
// All movement is non-blocking; angles advance each update() call.

// ── Servo angle constants (TODO: tune all after physical assembly) ───────────
// Big folder geometry
constexpr int BIG_REST_ANGLE   = 0;    // TODO: degrees, servos at rest (pre-fold)
constexpr int BIG_FOLD_ANGLE   = 90;   // TODO: degrees, servos at full fold

// Small folder geometry (may differ from big due to different arm length/mount)
constexpr int SMALL_REST_ANGLE = 0;    // TODO
constexpr int SMALL_FOLD_ANGLE = 90;   // TODO

// Servo sweep speed — degrees per update tick
// Lower = slower/gentler fold; higher = faster
constexpr int SERVO_SWEEP_SPEED = 2;   // TODO: tune

// ── Servo indices ────────────────────────────────────────────────────────────
constexpr int SERVO_BIG_TOP_A    = 0;
constexpr int SERVO_BIG_TOP_B    = 1;
constexpr int SERVO_BIG_BOT_A   = 2;
constexpr int SERVO_BIG_BOT_B   = 3;
constexpr int SERVO_SMALL_LEFT_A = 4;
constexpr int SERVO_SMALL_LEFT_B = 5;
constexpr int SERVO_SMALL_RIGHT_A= 6;
constexpr int SERVO_SMALL_RIGHT_B= 7;

constexpr int NUM_SERVOS = 8;

class ServoController {
public:

    void init() {
        for (int i = 0; i < NUM_SERVOS; i++) {
            int rest       = restAngleFor(i);
            currentAngle[i] = rest;
            targetAngle[i]  = rest;
            moveComplete[i]  = true;
            // servos[i].attach(SERVO_PINS[i]);
            // servos[i].write(rest);
        }
    }

    // ── Commands from state machine ───────────────────────────────────────────
    // Each folder is actuated individually — only the commanded servo sweeps.
    // Movement is non-blocking; actual angle advances each update() call.

    void foldBottom()  { Serial.println("Servo: foldBottom");
                         startMove(SERVO_BIG_BOT_A,    BIG_FOLD_ANGLE);
                         startMove(SERVO_BIG_BOT_B,    BIG_FOLD_ANGLE);  }

    void foldTop()     { Serial.println("Servo: foldTop");
                         startMove(SERVO_BIG_TOP_A,    BIG_FOLD_ANGLE);
                         startMove(SERVO_BIG_TOP_B,    BIG_FOLD_ANGLE);  }

    void foldLeft()    { Serial.println("Servo: foldLeft");
                         startMove(SERVO_SMALL_LEFT_A,  SMALL_FOLD_ANGLE);
                         startMove(SERVO_SMALL_LEFT_B,  SMALL_FOLD_ANGLE); }

    void foldRight()   { Serial.println("Servo: foldRight");
                         startMove(SERVO_SMALL_RIGHT_A, SMALL_FOLD_ANGLE);
                         startMove(SERVO_SMALL_RIGHT_B, SMALL_FOLD_ANGLE); }

    void unfoldBig()   { Serial.println("Servo: unfoldBig");
                         startMove(SERVO_BIG_TOP_A,    BIG_REST_ANGLE);
                         startMove(SERVO_BIG_TOP_B,    BIG_REST_ANGLE);
                         startMove(SERVO_BIG_BOT_A,    BIG_REST_ANGLE);
                         startMove(SERVO_BIG_BOT_B,    BIG_REST_ANGLE);  }

    void unfoldSmall() { Serial.println("Servo: unfoldSmall");
                         startMove(SERVO_SMALL_LEFT_A,  SMALL_REST_ANGLE);
                         startMove(SERVO_SMALL_LEFT_B,  SMALL_REST_ANGLE);
                         startMove(SERVO_SMALL_RIGHT_A, SMALL_REST_ANGLE);
                         startMove(SERVO_SMALL_RIGHT_B, SMALL_REST_ANGLE); }

    void stopAll()     { for (int i = 0; i < NUM_SERVOS; i++) {
                             targetAngle[i]  = currentAngle[i];
                             moveComplete[i] = true; } }

    // ── Called every loop() ──────────────────────────────────────────────────
    void update() {
        // For each servo, advance currentAngle toward targetAngle by SERVO_SWEEP_SPEED.
        // Note: mirrored servos within a pair may need opposite sign — one sweeps
        // +degrees while its partner sweeps -degrees to produce a symmetric fold.
        // TODO: confirm sign convention with mechanical team and negate as needed.
        for (int i = 0; i < NUM_SERVOS; i++) {
            if (moveComplete[i]) continue;
            int delta = targetAngle[i] - currentAngle[i];
            if (abs(delta) <= SERVO_SWEEP_SPEED) {
                currentAngle[i] = targetAngle[i];
                moveComplete[i] = true;
            } else {
                currentAngle[i] += (delta > 0) ? SERVO_SWEEP_SPEED : -SERVO_SWEEP_SPEED;
            }
            // servos[i].write(currentAngle[i]);
            Serial.println("Servo " + String(i) + " angle: " + String(currentAngle[i]));
        }
    }

    bool allMovesComplete() {
        for (int i = 0; i < NUM_SERVOS; i++) {
            if (!moveComplete[i]) return false;
        }
        return true;
    }

private:
    Servo servos[NUM_SERVOS];
    int   currentAngle[NUM_SERVOS];
    int   targetAngle[NUM_SERVOS];
    bool  moveComplete[NUM_SERVOS];

    void startMove(int servoIndex, int angle) {
        targetAngle[servoIndex]  = angle;
        moveComplete[servoIndex] = false;
    }

    int restAngleFor(int i) {
        return (i < 4) ? BIG_REST_ANGLE : SMALL_REST_ANGLE;
    }
