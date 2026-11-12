#ifndef CONFIG_H
#define CONFIG_H

// Teeny configuration

// Pins
#define OLED_SDA    21   // I2C data  -> OLED SDA
#define OLED_SCL    22   // I2C clock -> OLED SCL
#define OLED_RST    16   // OLED reset (-1 if not wired)
#define OLED_ADDR   0x3C // SSD1306 I2C address (try 0x3D if this fails)

#define JOY_VRX     34   // joystick X axis (analog)
#define JOY_VRY     35   // joystick Y axis (analog)
#define JOY_SW      32   // joystick push button (digital, INPUT_PULLUP)

// Display
#define SCREEN_W    128  // SSD1306 width
#define SCREEN_H    64   // SSD1306 height

// Serial link. Used to stream the screen to the PC client in remote mode.
#define SERIAL_BAUD  921600  // fast serial link for screen streaming

// Modes
#define MODE_HARDWARE 0   // OLED + hardware joystick
#define MODE_REMOTE   1   // Serial display + serial joystick (PC client)

// Timing
#define TARGET_FPS   30
#define FRAME_MS     (1000 / TARGET_FPS)

// Joystick deadzone (center is ~512 on a 10-bit ADC).
#define JOY_DEADZONE 60

#endif
