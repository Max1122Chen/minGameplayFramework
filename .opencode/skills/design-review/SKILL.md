---
name: design-review
description: Review a Design Spec for completeness and implementation readiness before coding. Focus on ambiguity, shell-product red flags, missing decisions, ownership/lifetime gaps, failure gaps, integration risk, overengineering, and concrete design-principle findings — not typos or formatting. Use after engineering-design for Level 2+ work, or when asked to review a design.
---

# Design Review

This is **not** code review.

| Review type | Primary question |
|-------------|------------------|
| Design review | Is this system design complete, unambiguous, implementable, and maintainable? |
| Code review | Does the implementation match intent and quality bars? |

Use after a Design Spec exists for Complexity Level **2+** (and always for Level **3**).
Authority for gates: `docs/ai/templates/DOC_GOVERNANCE.md`.
Companion skill: `engineering-design`.

## How to review

Actively hunt for:

- ambiguity and missing decisions
- hidden assumptions
- contradictory requirements
- undefined ownership / incomplete lifetime
- missing failure behavior
- integration / migration gaps
- unverifiable acceptance criteria
- unnecessary complexity
- **shell-product red flags** (critical TBD, strawman alternatives, unverifiable acceptance, principle theater — see `docs/ai/templates/DOC_GOVERNANCE.md`)
- structural principle issues (cohesion/coupling/SRP/OCP/…) with concrete risk — see `docs/ai/templates/DESIGN_PRINCIPLES.md`

Do **not** spend the review on spelling, formatting, or template cosmetics unless they hide real ambiguity.
Do **not** demand a filled SOLID matrix; cite a principle only when naming a real structural risk.

## Checklist (apply what is relevant)

### Requirements and scope

- Does the design solve the real problem?
- Are critical requirements missing?
- Are proposed solutions mislabeled as requirements?
- Are In / Non-goals clear?

### Architecture

- Are responsibilities and boundaries clear?
- Are dependencies reasonable? Any cycles or responsibility leakage?

### Ownership / lifetime / state

- Who owns state? Who creates/destroys/mutates?
- Are states, transitions, and invariants clear when relevant?

### API semantics

- Are ownership, mutation, error, and sync/async semantics clear enough to implement without guessing?

### Failure and integration

- Are important failure paths considered?
- Does the design fit the existing system?
- Are compatibility / migration issues addressed?

### Alternatives and complexity

- Were meaningful alternatives considered?
- Are trade-offs real (not ceremonial)?
- Is the design over-engineered? What can be deleted?

### Portability (this repo)

- With the engine-specific API removed, does the design still hold?
- Is engine coupling stated as a swappable boundary rather than baked into the contract?

### Verification and readiness

- Can acceptance criteria be observed/tested?
- Is verification strategy present?
- Does it match the profile's verification bar (`docs/ai/WORKFLOW_PROFILE.md`)?
- Is the design **implementation-ready**?

## Implementation readiness verdict

End with one verdict:

| Verdict | Meaning |
|---------|---------|
| `Ready` | Large implementation may proceed (still honor profile autonomy pauses) |
| `Ready with deferred items` | Proceed only if deferred items are explicit, accepted, and non-blocking |
| `Not ready` | Block large coding; list concrete gaps to resolve |

For `Not ready`, list actionable gaps (not vague "needs more detail").

## Output format

```markdown
## Design Review

- **Design doc:** path under docs/ai/DOMAIN/
- **Complexity level:** L0–L3
- **Verdict:** Ready | Ready with deferred items | Not ready

### Findings
- [severity] finding — why it matters — suggested resolution

### Deferred / accepted risks
- item — owner/acceptance

### Next action
- one concrete next step
```

## Profile interaction

- `serious-engineering` (this repo's preset): enforce readiness for L2+/L3; the executor may be terse but cannot skip architectural safety gaps.
- Any other preset: a leaner review is allowed, but ownership and public contracts still gate L3.

## Domain neutrality

Keep findings at the level of contracts and structure. The repo's subject is a gameplay framework carried by minEngine, so gameplay examples are legitimate — but never require a specific engine API or a specific product feature to be assumed.
