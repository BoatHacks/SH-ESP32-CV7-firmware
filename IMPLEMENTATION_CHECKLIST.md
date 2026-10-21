# Implementation Checklist - Quick Reference

**Before implementing a feature, work through this in order:**

## Phase 1: Explore
- [ ] Read the relevant issue fully
- [ ] Read the relevant sections of `SPEC.md`
- [ ] Read the relevant sections of `ARCHITECTURE.md`
- [ ] Explore existing code before writing anything

## Phase 2: Plan
- [ ] Think through the approach and alternatives
- [ ] Write a short implementation plan in `docs/plans/` (see `docs/plans/plan-template.md`)
- [ ] Identify test scenarios up front

## Phase 3: Test
- [ ] Write native unit tests first for hardware-independent code (parser, conditioning, rate limiter)
- [ ] Confirm they fail for the right reason
- [ ] Commit the tests separately from the implementation

## Phase 4: Implement
- [ ] Write code to satisfy the tests / plan
- [ ] Run native tests and the PlatformIO build frequently, not just at the end
- [ ] If a test seems wrong, fix the test deliberately, don't loosen it just to get to green

## Phase 5: Verify
- [ ] Check edge cases (invalid sentences, bad checksums, sensor timeout), not just the happy path
- [ ] Confirm the change matches `SPEC.md`
- [ ] Confirm the change follows `ARCHITECTURE.md`
- [ ] Before cutting a release: test on a real SH-ESP32 with the real CV7

## Phase 6: Document & Commit
- [ ] Update SPEC.md/ARCHITECTURE.md if this change altered what they describe
- [ ] Remove any temporal language from comments ("new", "recently added")
- [ ] All tests pass, build succeeds
- [ ] Commit with a message that explains *why*, referencing the issue

---

## Common Mistakes to Avoid

**Don't:**
- Jump straight to coding before reading SPEC/ARCHITECTURE
- Put hardware calls in the parser or conditioning code, which breaks native testing
- Loosen a test to make it pass instead of fixing the real issue
- Leave SPEC.md/ARCHITECTURE.md stale after a change that contradicts them

**Do:**
- Explore before planning, plan before coding
- Write down the plan somewhere reviewable, even briefly
- Verify against the docs, not just against your memory of the task
