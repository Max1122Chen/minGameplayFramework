---
name: workflow-bootstrap
description: Recover collaboration context at session start. Load workflow profile, project context, bootstrap digest, recent progress, active work, tech debt, and feature registry. Use when starting a new session or when context may be stale.
---

# Workflow: Session Bootstrap

## Read Order

1. `docs/ai/WORKFLOW_PROFILE.md` — collaboration posture
2. `docs/ai/PROJECT_CONTEXT.md` — stable project snapshot
3. `docs/ai/BOOTSTRAP_DIGEST.md` — one-page session recovery
4. `docs/ai/ACTIVE_WORK.md` — current short backlog
5. `docs/ai/FEATURE_REGISTRY.md` (In Progress / Planned)
6. `docs/ai/TECH_DEBT.md` (Open)
7. `docs/ai/PROGRESS_LOG.md` (recent entries only)
8. Task-specific design doc under `docs/ai/DOMAIN/` (if linked by ACTIVE_WORK or explicitly named by the user)

Protocol reference: `docs/ai/WORKING_WITH_AI.md`.

## Profile gate

- If profile `status` is `unconfigured`: ask the meta-question once (configure now / apply serious-engineering default / skip). Catalog: `docs/ai/templates/WORKFLOW_PRESETS.md`.
- If `deferred`: do not re-ask unless the user says `reconfigure workflow`.
- If `configured` or `default-applied`: apply the recorded behavior silently. This repo is currently `configured` with preset `serious-engineering`.

## Output

After reading, produce a summary covering:
- current workflow posture (preset / key dimensions)
- current project phase and milestone
- active feature/slice being worked on (or "empty queue")
- last verification state, including the current verification bar
- first concrete next action with its verification command

## When to Use

- Starting a new agent session
- Context has become stale (long idle, many external changes)
- User explicitly asks "where were we?"
