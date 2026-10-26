---
name: engineering-design
description: Guide system design for non-trivial engineering work. Use before large implementation when a feature/refactor needs architecture reasoning — intent playback when ambiguous, discovery, requirements vs solutions, responsibilities, boundaries, ownership, state, failure, alternatives, design-principle lenses, and verification planning. Reject shell products. Not for typos or one-line fixes.
---

# Engineering Design

Use this skill for **non-trivial** engineering tasks (typically Complexity Level 1–3 per `docs/ai/templates/DOC_GOVERNANCE.md`).
Do **not** force a full design for Level 0 trivial work.

Authority for *when* design is required: DOC_GOVERNANCE complexity levels + the preset effects in `docs/ai/WORKFLOW_PROFILE.md`.
Authority for *how* to design: this skill.
Artifact shape: `docs/ai/templates/design-spec.template.md`.
Principle lenses (short card): `docs/ai/templates/DESIGN_PRINCIPLES.md`.

## Goal

Before coding, discover enough design truth that implementation does not invent ownership, boundaries, or failure behavior on the fly. Prefer substance over filled templates (anti-shell).

```text
Intent playback when ambiguous
  → Understand problem
  → Understand existing system
  → Separate requirements from solutions
  → Design responsibilities and boundaries (principle lenses)
  → Reason about state, lifetime, dependencies, failure (when relevant)
  → Compare meaningful alternatives
  → Plan verification
  → Hand off to design-review / readiness gate
```

## 0) Intent playback (when ambiguous)

For non-trivial work (typically L1+) when intent or success criteria are incomplete:

1. Playback ~150–300 words: problem, success, non-goals, key constraints (mark guesses).
2. One clarifying question **or** ask for correction.
3. Then open/expand the Design Spec or large coding.

The user may skip ("just do it"); record assumptions. L0 exempt.

## 1) Separate problem from solution

Explicitly classify inputs:

| Kind | Meaning |
|------|---------|
| Requirements | What must be true for success |
| Constraints | Hard limits (compat, perf budget, API freeze, platform) |
| Preferences | Soft wants (style, familiarity) |
| Assumptions | Believed but unverified |
| Proposed solution | A candidate design — **not** settled architecture |

If the user says "add a Manager / Service / layer", treat it as a **proposed solution** until requirements and existing architecture justify it.

## 2) Discover the existing system

Before inventing structure, inspect relevant code/docs:

- related modules and existing abstractions/APIs
- data flow and control flow
- lifetime / ownership patterns already in use
- dependency direction and extension points
- similar features already present
- existing constraints and tests

In this repo the discovery surface includes the contracts in `docs/ai/PROJECT_CONTEXT.md` and, read-only, the reference implementations (`minEngine`, `minCraft`).

Principle:

> Prefer extending or reusing existing abstractions when appropriate; do not invent parallel abstractions without justification.

## 3) Responsibilities and boundaries

The design must answer:

- Who is responsible for what — and **not** responsible for what?
- Who owns each important piece of state?
- Who creates / destroys / mutates / may only read?
- How do modules communicate (APIs, events, messages)?
- Which dependencies are allowed or forbidden?

Reject vague "one type owns everything" designs unless the problem is truly that small.

## 4) Ownership and lifetime (when relevant)

For stateful objects/resources, consider:

- ownership, sharing, reference semantics
- creation, destruction, invalidation, cleanup
- shutdown / teardown ordering

If lifetime is unimportant for this change, write `Not applicable because ...`.

## 5) State and invariants (when relevant)

- Which states exist and how do they transition?
- Which combinations are illegal?
- Which invariants must always hold, including on failure paths?
- Who maintains each invariant?

## 6) Data flow and control flow

Do not stop at "list of types".

- **Data flow:** input → processing → storage → output
- **Control flow:** who calls whom; for async/events/callbacks/messages, make the control path explicit

## 7) API / interface semantics

When designing APIs, define more than names:

- responsibility, inputs/outputs
- ownership and mutation semantics
- error semantics
- sync/async / blocking behavior
- thread-safety if relevant
- validity and lifetime requirements for arguments/results

## 8) Failure model

Actively judge which failure modes matter for *this* design, for example:

- invalid input, missing dependency, partial failure
- timeout, cancellation, resource exhaustion
- init/shutdown/persistence failure and recovery

Do not paste every failure mode into every design.

## 9) Alternatives and trade-offs

When multiple reasonable options exist, compare at least two on:

complexity, maintainability, extensibility, performance, resource use, correctness, integration cost, migration cost, operational/debug cost.

Choose with a reason. Do not invent strawman options for ceremony.

## 10) Simplicity / overengineering check

Before finishing:

> Is this design more complex than the problem requires?

Check for unnecessary abstractions, speculative "future" hooks, duplicate sources of truth, duplicate lifecycles, or extra managers/layers/interfaces without current need.

> Prefer the simplest architecture that satisfies known requirements and constraints.

Apply YAGNI and the OCP anti-abuse note from `docs/ai/templates/DESIGN_PRINCIPLES.md`: do not invent extension points "for OCP".

## 10b) Portability check (this repo's core question)

With C++ and the minEngine API removed, does the design still hold?

- Which parts are engine-independent contract, and which are adaptation to minEngine's type system, reflection, serialization, or AssetManager?
- Name the seam: what would another engine or language have to provide?
- If the design only makes sense as minEngine code, say so and justify why the idea does not generalize.

## 10c) Design-principle lenses

Use the card as judgment aids mapped to sections you already write (Responsibilities, Boundaries, Dependencies, API, Simplicity). Do **not** add a mandatory SOLID matrix to the Design Spec.

If a principle finding matters, state the structural risk in Responsibilities/Boundaries/Dependencies — not a checkbox.

## 11) Integration and migration

If changing an existing system, define:

- impact on current modules
- API/data compatibility
- migration and rollout
- transitional code, deprecations, convergence path

Avoid long-lived dual systems with no exit plan (register debt in `docs/ai/TECH_DEBT.md` if temporary retention is required).

## 12) Verification in the design phase

Design must include a verification strategy:

- how to prove core behavior
- failure-path coverage when relevant
- integration / compatibility / performance checks when relevant

Verification should not be invented only after coding. Respect the profile's current verification bar (`docs/ai/WORKFLOW_PROFILE.md`): `docs-only` today, `tests-required` once the first code feature lands.

## Anti-shell (before calling design "done")

Reject your own draft if it shows shell-product red flags (critical TBD, strawman alternatives, unverifiable acceptance, principle theater — see `docs/ai/templates/DOC_GOVERNANCE.md`).

## Output

1. Assess complexity level (L0–L3) and state it.
2. Write or update the Design Spec under `docs/ai/DOMAIN/` using `docs/ai/templates/design-spec.template.md` (omit sections with `Not applicable because ...`). Project-owned docs are written in Chinese; identifiers and code stay as-is.
3. For L2+, invoke **design-review** before large implementation.
4. Do not start large coding while the Design status is `Draft` or readiness is unmet (see `docs/ai/templates/DOC_GOVERNANCE.md`).

## Domain

This repo's subject is a gameplay framework (entities/components, attributes, abilities, items, inventories, assets). Gameplay examples are appropriate; requirements must not assume a specific engine API or a specific product's content.
