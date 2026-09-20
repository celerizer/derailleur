/* MP2 US rev0 SBF0 bank-0 scripts: sample-select followed by B5 rate.
 * Offsets are relative to effects.bin, not ROM. Delayed entries encode
 * 13 0A sample_hi sample_lo B5 rate_hi rate_lo; others start with 93.
 * Engine func_80012800_13400: command B5 at 80012FB4 reads a u16 Hz
 * value and divides it by the output rate, overriding descriptor pitch.
 * Keep an explicit verified map instead of scanning arbitrary binary bytes.
 */
#ifndef PS_MP2_VOICE_RATES_H
#define PS_MP2_VOICE_RATES_H
typedef struct { size_t offset; unsigned int descriptor, delayed; } PsMp2VoiceRate;
static const PsMp2VoiceRate ps_mp2_voice_rates[] = {
    {0x7D24, 0x005F, 0},
    {0x7D35, 0x005F, 0},
    {0x7D46, 0x0064, 0},
    {0x7D57, 0x006A, 0},
    {0x7D68, 0x006C, 0},
    {0x7D8F, 0x0060, 0},
    {0x7DFD, 0x0060, 0},
    {0x7E0E, 0x0061, 0},
    {0x7E1F, 0x0064, 0},
    {0x7E30, 0x0068, 0},
    {0x7E41, 0x006C, 0},
    {0x7E52, 0x0073, 0},
    {0x7E63, 0x006F, 0},
    {0x7E74, 0x0060, 0},
    {0x7EC7, 0x0060, 0},
    {0x7ED8, 0x0063, 0},
    {0x7EF9, 0x0067, 0},
    {0x7F0F, 0x006B, 0},
    {0x7F4C, 0x0076, 0},
    {0x7F62, 0x0072, 0},
    {0x7F78, 0x0073, 0},
    {0x92F9, 0x0061, 0},
    {0x9307, 0x0064, 0},
    {0x9315, 0x0068, 0},
    {0x9323, 0x006C, 0},
    {0x9331, 0x0073, 0},
    {0x933F, 0x006F, 0},
    {0x96C6, 0x0063, 0},
    {0x96D6, 0x0063, 0},
    {0x96EF, 0x0067, 0},
    {0x9708, 0x006B, 0},
    {0x976A, 0x0072, 0},
    {0xB22A, 0x0060, 0},
    {0xB23A, 0x0060, 0},
    {0xB251, 0x0060, 1},
    {0xB269, 0x0065, 0},
    {0xB280, 0x0065, 1},
    {0xB298, 0x0069, 0},
    {0xB2AF, 0x0069, 1},
    {0xB322, 0x0074, 0},
    {0xB339, 0x0074, 1},
    {0xB351, 0x0070, 0},
    {0xB368, 0x0070, 1},
    {0xF9CD, 0x0064, 0},
    {0xF9E6, 0x0068, 0},
    {0xF9FF, 0x006C, 0},
    {0xFA10, 0x0073, 0},
    {0xFA29, 0x006F, 0},
};
#define PS_MP2_VOICE_RATE_COUNT (sizeof(ps_mp2_voice_rates)/sizeof(ps_mp2_voice_rates[0]))
#endif
