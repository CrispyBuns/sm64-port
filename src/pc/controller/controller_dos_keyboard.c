#include <stdbool.h>
#include <ultra64.h>

#include <stdio.h>

#include "controller_api.h"
#include "../configfile.h"

#include "keyboard.h"

#include "controller_dos_keyboard.h"

static int mapping_length = 13;
static int keyboard_mapping[13][2];

static void set_keyboard_mapping(int index, int mask, int scancode) {
    keyboard_mapping[index][0] = scancode;
    keyboard_mapping[index][1] = mask;
}


static void keyboard_init(void) {

    KeyIntHandler();
    SetKb();

    int i;
    set_keyboard_mapping(i++, 0x80000,      configKeyStickUp);
    set_keyboard_mapping(i++, 0x10000,      configKeyStickLeft);
    set_keyboard_mapping(i++, 0x40000,      configKeyStickDown);
    set_keyboard_mapping(i++, 0x20000,      configKeyStickRight);
    set_keyboard_mapping(i++, A_BUTTON,     configKeyA);
    set_keyboard_mapping(i++, B_BUTTON,     configKeyB);
    set_keyboard_mapping(i++, Z_TRIG,       configKeyZ);
    set_keyboard_mapping(i++, U_CBUTTONS,   configKeyCUp);
    set_keyboard_mapping(i++, L_CBUTTONS,   configKeyCLeft);
    set_keyboard_mapping(i++, D_CBUTTONS,   configKeyCDown);
    set_keyboard_mapping(i++, R_CBUTTONS,   configKeyCRight);
    set_keyboard_mapping(i++, R_TRIG,       configKeyR);
    set_keyboard_mapping(i++, START_BUTTON, configKeyStart);
}

static void keyboard_read(OSContPad *pad) {
    for (int i = 0; i < mapping_length; i++)
    {
        int scan_code = keyboard_mapping[i][0];
        int mapping = keyboard_mapping[i][1];

        if (KeyState(scan_code & 0xFF))
        {
            pad-> button |= mapping;
        }
    }
}


struct ControllerAPI controller_dos_keyboard = {
    keyboard_init,
    keyboard_read
};
