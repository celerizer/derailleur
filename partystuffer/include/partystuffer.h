#ifndef PARTYSTUFFER_H
#define PARTYSTUFFER_H
#ifdef __cplusplus
extern "C" {
#endif
#define PARTYSTUFFER_OK 0
#define PARTYSTUFFER_ERROR 1
/* Creates or atomically replaces target_rom after successful conversion.
 * source_rom may equal target_rom. Character IDs are zero based.
 * Calls are synchronous and must be serialized (not thread safe).
 * No process exit or console output; resources are released on failure.
 */
int partystuffer_inject_character(const char *source_rom,
    const char *target_rom, const char *mod_folder, unsigned int character_id);
/* Last call's error, or an empty string after success. Library-owned storage,
 * valid until the next injection. Copy it if needed beyond that point. */
const char *partystuffer_last_error(void);
#ifdef __cplusplus
}
#endif
#endif
