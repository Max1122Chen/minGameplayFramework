# Document Governance

Last updated: 2026-10-15
Status: Active

## 1) Purpose

Ensure any contributor (human or agent) can answer:
1. What problem is being solved?
2. What is done vs pending?
3. What is the next verifiable action?
4. Why was work paused, deferred, or cancelled?

## 2) Document types

- Roadmap: sequencing and priorities
- Design Spec: scope, approach, risks
- Implementation Plan: slices and acceptance checks
- ADR: architectural trade-off decision
- Progress Log: factual timeline
- Bug Record: defect lifecycle and regression safety

## 3) ID conventions

- Feature: `<DOMAIN>-F<nn>`
- Slice: `<FeatureID>-S<nn>`
- Bug: `BUG-<DOMAIN>-<nnn>`
- ADR: `ADR-<yyyyMMdd>-<nn>`

New features must be registered in `docs/ai/FEATURE_REGISTRY.md` before implementation work.

Domain codes are ALL CAPS, short, and stable (examples: `TMPL`, `CORE`, `API`, `UI`, `DATA`, `TEST`, `INFRA`). Not a closed enum — add domains as needed, do not reuse retired codes.

## 3.1) File naming (required)

Long collaboration docs that belong to a Feature use:

```text
<FEATURE_ID>_<SLUG>_<DOC_TYPE>.md
```

Rules:
- Filename is **ALL CAPS** (Feature ID, slug, and doc type)
- `<FEATURE_ID>` must match the registered Feature ID exactly
- `<SLUG>` is `A-Z0-9_` only (words separated by `_`)
- `<DOC_TYPE>` is one of: `DESIGN` | `IMPLEMENTATION` | `ROADMAP` | `REFACTOR_PLAN`

Examples:
- `TMPL-F03_DOC_NAMING_AND_BUCKETING_DESIGN.md`
- `CORE-F01_EVENT_BUS_IMPLEMENTATION.md`
- `RND-F02_MODERN_RHI_REFACTOR_PLAN.md`

ADR and Bug filenames:

```text
ADR-<yyyyMMdd>-<nn>_<SLUG>.md
BUG-<DOMAIN>-<nnn>_<SLUG>.md
```

Also ALL CAPS.

## 3.2) Domain bucketing (required)

Design Spec, Implementation Plan, Roadmap, Refactor Plan, and ADR files **must** live under:

```text
docs/ai/<DOMAIN>/
```

where `<DOMAIN>` matches the Feature/ADR/Bug domain segment (ALL CAPS).

Do **not** place these long docs in `docs/ai/` root.

Bug records may use either:
- `docs/ai/bugs/`
- `docs/ai/<DOMAIN>/bugs/`

### Root exceptions

Only core collaboration truth stays at `docs/ai/` root, for example:
`README.md`, `WORKFLOW_PROFILE.md`, `PROJECT_CONTEXT.md`, `BOOTSTRAP_DIGEST.md`, `ACTIVE_WORK.md`, `FEATURE_REGISTRY.md`, `PROGRESS_LOG.md`, `TECH_DEBT.md`, `WORKING_WITH_AI.md`

### Other reserved folders

- `docs/ai/templates/` — templates and governance only
- `docs/ai/sessions/` — session notes (`YYYY-MM-DD-<topic>.md`; topic may be lowercase)

## 4) Required Meta block for long docs

Design, Roadmap, Implementation, and ADR files should include:

```markdown
## Meta
- **ID:** <FeatureID or N/A>
- **Status:** Draft | Planned | In Progress | Review | Done | Blocked | Deferred | Cancelled | Snapshot | Archived | Reference
- **Owner:** <name>
- **Last updated:** YYYY-MM-DD
- **Related:** [link1](./...), [link2](./...)
```

## 5) Agent doc trust

Planning sources:
- `WORKFLOW_PROFILE.md` (for posture; not backlog)
- `ACTIVE_WORK.md`
- `FEATURE_REGISTRY.md` (In Progress / Planned)
- `TECH_DEBT.md` (Open)
- Recent `PROGRESS_LOG.md`
- Code/tests/verify scripts

Reference-only sources:
- old roadmaps
- Snapshot/Archived/Reference docs
- stale checklist fragments

If docs conflict with code/tests, code/tests win.

## 5.1) Workflow profile

- Deploy-time posture lives in `docs/ai/WORKFLOW_PROFILE.md`.
- Preset catalog: `templates/WORKFLOW_PRESETS.md`.
- If profile status is `unconfigured`, ask meta-question once before large work.
- Profile may tune strictness/autonomy/role/language/verification/design ceremony; it cannot disable commit gate, planning trust tiers, or Level-3 architectural safety checks.

## 5.2) Design complexity levels

Agents must assess complexity before large edits and state the level.

| Level | Typical scope | Required path |
|-------|---------------|---------------|
| **L0 Trivial** | typo, obvious one-line fix, local constant | Implement → Verify (no design review) |
| **L1 Local** | single-module logic | Short design optional; Feature ID if substantial |
| **L2 Feature** | multi-module, API, state, or lifetime | Design Spec → Design Review → Implementation Plan → Implement |
| **L3 Architectural** | core abstraction, cross-module deps, persistence, concurrency, public API, migration, major refactor | Full Design → Design Review → **Implementation Readiness** → Plan → Implement |

Skills:
- Design behavior: `.opencode/skills/engineering-design/SKILL.md`
- Design review: `.opencode/skills/design-review/SKILL.md`

Target flow (skip stages allowed only by level):

```text
Request → Discovery → Complexity Assessment
  → Design → Design Review → Implementation Readiness
  → Implementation → Verification → Done
```

Examples:
- L0: `Request → Implement → Verify`
- L2: `Request → Discovery → Design → Review → Plan → Implement → Verify`
- L3: same as L2 with explicit readiness gate before coding

## 5.3) Implementation readiness gate

For L2+ (always for L3), do not start large-scale coding until Design Review verdict is `Ready` or `Ready with deferred items`, and the Design Spec readiness checklist is satisfied:

- requirements understood; constraints identified
- relevant existing architecture inspected
- responsibilities and boundaries defined; key dependencies understood
- ownership/lifetime and major state/invariants resolved **or** marked N/A with reason
- API semantics sufficiently defined; important failure modes considered
- alternatives considered where meaningful; integration impact understood
- verification strategy exists
- unresolved questions resolved **or** explicitly accepted/deferred

`Draft` design never authorizes large-scale coding.  
`Not ready` blocks large implementation until gaps are closed.

## 6) Slice Done Definition (DoD)

DoD has two layers. **Process DoD** = collaboration bookkeeping. **Quality DoD** = artifact substance.  
L2+: both required before Done. L1: Quality recommended. L0: Process-light only (still obey comment/branch ID bans).

Skill: `.opencode/skills/workflow-dod/SKILL.md`.  
Principles card: `templates/DESIGN_PRINCIPLES.md`.

### 6.1 Process DoD (Docs)

- Progress entry appended
- Feature and slice status synchronized
- Design/Plan updated when scope or status changed
- For L2+: design review verdict recorded; implementation slices trace to design decisions
- ADR updated for meaningful architectural trade-off
- Bug record updated for defect fixes
- New design/impl docs follow §3.1 naming and §3.2 domain bucketing

### 6.2 Process DoD (Engineering bookkeeping)

- Verification command executed and recorded (per profile `verification_bar`) — command + result, not assertion alone
- No unrecorded blocking defect discovered during work
- Public API changes reflected in callers or explicitly documented
- Implementation matches reviewed design decisions (or documents approved deviation)
- Touched source comments have no Feature/Slice/Bug/ADR IDs
- If a branch was created/renamed this batch: topic-granular name (`feat/<topic>[/<subtopic>]`), not ID-based

### 6.3 Quality DoD

- Design (when required): a fresh implementer could plan without chat history; no critical TBD on ownership/API/failure the change needs
- Plan (when required): each in-scope slice has paths + observable acceptance + verification signal; no empty steps
- No **shell-product red flag** remains unaddressed (unless explicitly deferred/accepted):
  - sections that only restate headings or leave critical TBD
  - strawman alternatives
  - unverifiable acceptance (“more maintainable”, “properly handled”)
  - plan steps with no path or no checkable result
  - “tests/docs updated” with no what/where
  - “followed SOLID/OCP” with no named structural consequence
  - Done claimed without verification evidence

## 7) Handoff requirements

On handoff/session switch:
1. Create or update a session note
2. Append one progress entry
3. Mark incomplete slice as Blocked/Deferred with reason and unblock condition
4. Provide first concrete next action

## 8) Work boundary

After completing a meaningful batch:
- finish DoD updates
- propose "prepare commit"
- do not start unrelated new feature work unless explicitly requested

## Design Review Comment Area

Every design document under `docs/ai/<DOMAIN>/` follows these rules.

### Title numbering (required)

- The H1 title starts with the Feature ID, e.g. `# CORE-F01 Framework Layering and Cross-Block Contracts`.
- Every section and subsection heading carries a number (`## 5 ...`, `### 5.3 ...`) so reviewers can point at a location instead of quoting prose.
- Contract items inside a design may carry their own stable labels (for example `C11`); cross-references between documents use the Feature ID plus the section number.

### Required closing sections

A design document is not reviewable until it ends with both:

1. **Open questions** — numbered `Q1..Qn`; each entry states the options and a recommendation with one line of reasoning.
2. **Design review — author comments** — the table below, included empty.

```markdown
| # | Location (section / inline reference) | Author comment | Rationale (filled by the designer) | Disposition (accept / rebut + reason / open question Qn) |
|---|--------------------------------------|----------------|-----------------------------------|---------------------------------------------------------|
|   |                                      |                |                                   |                                                         |
```

### Division of labour

- The **author** writes only two columns: *Location* and *Author comment*. A one-line objection is enough; no rationale and no proposed fix is required.
- The **designer** fills *Rationale* (why the design says what it says: contract clause, platform fact, external precedent, cost trade-off) and *Disposition*.
- The author then re-reads only the rows whose disposition is *rebut* or *open question*.

### Disposition is three-way

Exactly one of:

- **Accept** — the document is changed accordingly.
- **Rebutt + reason** — the document stands; the reason must be concrete (a violated constraint, a measured cost, an engine fact), never "it feels cleaner".
- **Convert to an open question** — recorded as a new `Qn` in the open-questions section, with the blocking condition stated.

A row without a disposition is an unfinished review; a review is complete only when every row has one.

