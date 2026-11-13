/*
 * P710 native notice preparation: sgp/clip_state.h
 * New path extracting/adapting inherited engine material.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: declare original shared clipping state.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Inherited extraction/translation source: sgp/vobject_blitters.h.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

#ifndef SGP_CLIP_STATE_H
#define SGP_CLIP_STATE_H

#include "core_types.h"

// Shared original sprite clipping state. Right and bottom are exclusive.
extern SGPRect ClippingRect;
void SetClippingRect(SGPRect* clip);
void GetClippingRect(SGPRect* clip);

#endif
