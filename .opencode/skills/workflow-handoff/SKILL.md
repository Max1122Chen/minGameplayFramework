---
name: workflow-handoff
description: Prepare a complete handoff for session switch or end-of-day. Create a session note, append a progress entry, mark incomplete slices, and provide the next action.
---

# Workflow: Session Handoff

## Trigger

The user indicates handoff, session switch, or the end of a meaningful batch.

## Steps

1. **Write a session note** using `docs/ai/templates/session-note.template.md`, saved under `docs/ai/sessions/`
   - TL;DR summary for fast recovery
   - Decisions made and rationale
   - Work completed
   - Open issues

2. **Append a progress entry** to `docs/ai/PROGRESS_LOG.md` (append only; never rewrite past entries)
   - Scope: the Feature / Slice ID being worked on (collaboration docs only — IDs never go into source comments)
   - Completed items
   - Verification commands and results
   - Docs updated
   - Next action

3. **Mark incomplete slices**
   - Set status to Blocked or Deferred with an explicit reason in `docs/ai/FEATURE_REGISTRY.md`
   - Document the unblock condition and the next check date

4. **Provide the first concrete next action**
   - One clear step the next session should start with, plus how to verify it

## Output

- Session note saved under `docs/ai/sessions/`
- Progress log updated
- Incomplete slice status updated in the registry and ACTIVE_WORK
- A single next action stated in the reply
