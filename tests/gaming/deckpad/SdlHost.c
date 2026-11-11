// An independent host for the controller: SDL's own Steam Controller driver,
// the code games and Steam Input are built on. It finds the controller,
// reads everything the emulated Deck pad is holding, and rumbles it. Nothing
// here shares code with deckpad, so a report laid out wrongly fails here.
//
//   deckpad-sdl-host [SECONDS]    exit 0 when SDL sees what the pad holds
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int failures = 0;

static void expect(bool passed, const char* what) {
    printf("%s: %s\n", passed ? "ok" : "FAIL", what);
    if (!passed) { failures++; }
}

static bool near(float value, float wanted, float within) { return fabsf(value - wanted) <= within; }

static SDL_Gamepad* find(int seconds) {
    Uint64 deadline = SDL_GetTicks() + (Uint64)seconds * 1000;
    while (SDL_GetTicks() < deadline) {
        SDL_UpdateGamepads();
        int count = 0;
        SDL_JoystickID* ids = SDL_GetGamepads(&count);
        for (int index = 0; ids != NULL && index < count; index++) {
            if (SDL_GetGamepadVendorForID(ids[index]) == 0x28de && SDL_GetGamepadProductForID(ids[index]) == 0x1302) {
                SDL_Gamepad* pad = SDL_OpenGamepad(ids[index]);
                SDL_free(ids);
                return pad;
            }
        }
        SDL_free(ids);
        SDL_Delay(50);
    }
    return NULL;
}

int main(int argc, char** argv) {
    int seconds = argc > 1 ? atoi(argv[1]) : 20;
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_STEAM, "1");
    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        printf("FAIL: SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Gamepad* pad = find(seconds);
    if (pad == NULL) {
        printf("FAIL: SDL found no Steam Controller (28de:1302)\n");
        return 1;
    }
    const char* name = SDL_GetGamepadName(pad);
    const char* serial = SDL_GetGamepadSerial(pad);
    printf("found: %s, serial %s, %d touchpads\n", name ? name : "?", serial ? serial : "?", SDL_GetNumGamepadTouchpads(pad));
    expect(name != NULL && SDL_strcmp(name, "Steam Controller") == 0, "SDL's Steam Controller driver took the device");
    expect(SDL_GetNumGamepadTouchpads(pad) == 2, "two touchpads");
    expect(SDL_GamepadHasSensor(pad, SDL_SENSOR_GYRO) && SDL_GamepadHasSensor(pad, SDL_SENSOR_ACCEL), "gyro and accelerometer");
    expect(SDL_SetGamepadSensorEnabled(pad, SDL_SENSOR_GYRO, true) && SDL_SetGamepadSensorEnabled(pad, SDL_SENSOR_ACCEL, true), "sensors switch on");

    // Read until everything the pad holds has shown up, counting the sensor
    // updates: SDL raises one for every state report with a new timestamp.
    bool south = false, up = false, east = false, down = false, touching = false;
    float accel[3] = {0, 0, 0}, gyro[3] = {0, 0, 0}, touchX = 0, touchY = 0, pressure = 0;
    int leftX = 0, leftY = 0, rightTrigger = 0, leftTrigger = 0, percent = -1, updates = 0;
    SDL_PowerState power = SDL_POWERSTATE_UNKNOWN;
    Uint64 started = SDL_GetTicks();
    Uint64 deadline = started + (Uint64)seconds * 1000;
    Uint64 counted = 0;
    bool complete = false;
    while (SDL_GetTicks() < deadline) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_GAMEPAD_SENSOR_UPDATE && event.gsensor.sensor == SDL_SENSOR_GYRO) { updates++; }
        }
        south = SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH);
        up = SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_UP);
        east = SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_EAST);
        down = SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
        leftX = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX);
        leftY = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY);
        leftTrigger = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        rightTrigger = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
        SDL_GetGamepadSensorData(pad, SDL_SENSOR_ACCEL, accel, 3);
        SDL_GetGamepadSensorData(pad, SDL_SENSOR_GYRO, gyro, 3);
        SDL_GetGamepadTouchpadFinger(pad, 0, 0, &touching, &touchX, &touchY, &pressure);
        power = SDL_GetGamepadPowerInfo(pad, &percent);
        counted = SDL_GetTicks() - started;
        complete = south && up && touching && near(accel[1], 9.80665f, 0.05f) && percent >= 0 && counted >= 3000;
        if (complete) { break; }
        SDL_Delay(1);
    }
    int rate = counted > 0 ? (int)((Uint64)updates * 1000 / counted) : 0;
    printf("seen: south %d up %d east %d down %d, left stick %d %d, triggers %d %d, accel %.3f %.3f %.3f, gyro %.4f %.4f %.4f, touch %d %.4f %.4f %.4f, power %d %d%%, %d reports/s\n",
        south, up, east, down, leftX, leftY, leftTrigger, rightTrigger, accel[0], accel[1], accel[2], gyro[0], gyro[1], gyro[2], touching, touchX, touchY, pressure, (int)power, percent, rate);
    expect(south && up && !east && !down, "A and d-pad up are held, and nothing else");
    expect(abs(leftX - 12000) <= 8 && abs(leftY - 8000) <= 8, "left stick at 12000 right, 8000 down");
    expect(abs(rightTrigger - 20000) <= 16 && leftTrigger == 0, "right trigger at 20000 of 32767");
    expect(near(accel[0], 0, 0.05f) && near(accel[1], 9.80665f, 0.05f) && near(accel[2], 0, 0.05f), "one g on the accelerometer's vertical axis");
    expect(near(gyro[0], 0.10653f, 0.005f) && near(gyro[1], 0, 0.005f) && near(gyro[2], 0, 0.005f), "100 units of pitch rate on the gyro");
    expect(touching && near(touchX, 0.51526f, 0.001f) && near(touchY, 0.53052f, 0.001f) && near(pressure, 500 / 32768.0f, 0.001f), "a finger on the left pad");
    expect(power == SDL_POWERSTATE_ON_BATTERY && percent == 100, "battery level");
    expect(rate >= 100, "at least 100 reports a second reach SDL");

    // Rumble for a second; SDL repeats the report while it runs.
    expect(SDL_RumbleGamepad(pad, 0x8000, 0x4000, 1000), "SDL accepts a rumble");
    Uint64 until = SDL_GetTicks() + 1200;
    while (SDL_GetTicks() < until) {
        SDL_UpdateGamepads();
        SDL_Delay(5);
    }
    SDL_CloseGamepad(pad);
    SDL_Quit();
    printf(failures == 0 ? "PASS: SDL read the Deck as a Steam Controller\n" : "FAIL: %d expectation(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
