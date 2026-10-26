---
name: workflow-dod
description: Verify Process and Quality Definition of Done after a meaningful batch. Check docs sync, evidence-based verification, comment/branch ID hygiene, and shell-product red flags before proposing a commit.
---

# Workflow: Definition of Done (DoD) Check

## When to Use

After completing a meaningful batch of work, before proposing "prepare commit".

Authority: `docs/ai/templates/DOC_GOVERNANCE.md`.

## Process DoD (Docs)

- [ ] Progress entry appended to `docs/ai/PROGRESS_LOG.md`
- [ ] Feature and slice status synchronized in `docs/ai/FEATURE_REGISTRY.md`
- [ ] Design/Plan updated when scope or status changed
- [ ] For L2+: design review verdict recorded; slices trace to design decisions
- [ ] ADR updated for a meaningful architectural trade-off
- [ ] Bug record updated for defect fixes
- [ ] New design/impl docs follow the ALL-CAPS naming and domain bucketing (`docs/ai/DOMAIN/`)
- [ ] No new domain code added without a real requirement behind it

## Process DoD (Engineering bookkeeping)

- [ ] Verification **command + result** recorded (per the profile `verification_bar`) — not "verified" alone
- [ ] No unrecorded blocking defect discovered during work
- [ ] Public API changes reflected in callers or explicitly documented
- [ ] Implementation matches reviewed design decisions (or documents an approved deviation)
- [ ] Touched source comments carry no Feature / Slice / Bug / ADR / TechDebt ID, no design section number, and no pointer into collaboration docs
- [ ] Public contracts carry English Doxygen (`///`); internal notes use `//`
- [ ] If a branch was created/renamed: `feat/<topic>` or `feat/<topic>/<subtopic>` — never ID-based names

## Quality DoD

Required for L2+ before Done; recommended for L1.

- [ ] Design (if required): a fresh implementer could plan without the chat; no critical TBD on needed ownership/API/failure
- [ ] Plan (if required): paths + observable acceptance + verification signal; no empty steps
- [ ] No shell-product red flag left unaddressed (see `docs/ai/templates/DOC_GOVERNANCE.md`), unless explicitly deferred
- [ ] The portability question is answered: the design still reads correctly with the engine API removed

## After DoD Passes

- Propose "prepare commit" (draft the message only, do not execute the commit)
- Do not start unrelated new feature work unless explicitly requested
