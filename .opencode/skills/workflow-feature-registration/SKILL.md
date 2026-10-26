---
name: workflow-feature-registration
description: Register a new Feature ID before implementation work. Follow the ID scheme and create or update design/plan docs. Use when starting a new feature or refactor.
---

# Workflow: Feature Registration

## Steps

1. **Check existing registry** — read `docs/ai/FEATURE_REGISTRY.md` for the next available number in the target domain, and for the registered domain codes.
2. **Pick the domain code** — currently only `CORE` exists. A new domain code (`ITEM`, `INVENTORY`, `ABILITY`, `ATTR`, …) is added to the registry's domain table only when a real requirement demands it; never pre-declare one and never open an empty `docs/ai/DOMAIN/` bucket.
3. **Assign the Feature ID** — `DOMAIN-Fnn` (e.g. `CORE-F01`).
4. **Assess complexity** — L0–L3 per `docs/ai/templates/DOC_GOVERNANCE.md`.
5. **Register in `docs/ai/FEATURE_REGISTRY.md`** — add a row with Feature ID, Title, Domain, Status (Draft), Design Doc link (TBD), Owner. Also append to `docs/ai/ACTIVE_WORK.md`.
6. **Create the Design Spec** (L1+ when substantial; required L2+) — copy `docs/ai/templates/design-spec.template.md` to
   `docs/ai/DOMAIN/FEATUREID_SLUG_DESIGN.md` (ALL CAPS filename; domain bucket required).
   Use the `engineering-design` skill.
7. **Design Review** (L2+) — use the `design-review` skill; record the verdict.
8. **When ready for implementation** — create
   `docs/ai/DOMAIN/FEATUREID_SLUG_IMPLEMENTATION.md` with slices that trace to design decisions.
   Do not large-code while the Design is `Draft` or the review is `Not ready`.

## Slice IDs

- Feature ID + `-Snn` (e.g. `CORE-F01-S01`)
- Each slice should be independently verifiable and, for L2+, map to a design decision
- Slice IDs live in collaboration docs — never in source comments

## Status Guidance

- Draft: concept exists, not implementation-ready
- Planned: approved for implementation (readiness met when required)
- In Progress: active development
- Review: awaiting validation/review
- Done: accepted
- Blocked / Deferred / Cancelled: include the reason and the follow-up condition

## Current queue state

This repo has no registered Feature yet; `docs/ai/ACTIVE_WORK.md` and `docs/ai/FEATURE_REGISTRY.md` are intentionally empty skeletons. Do not invent work to fill them.
