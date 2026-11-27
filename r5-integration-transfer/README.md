# AX101 RUN80 R5 Integration validation branch

This branch is a CI transport and validation branch only. It does not modify
`wifi-ax101-openindiana` or claim that the integration candidate is merged.

Source of truth:
- Local candidate archive SHA-256:
  `f251541a7c60cc46cdaecc0719e42c576f6f5bd4699dea712a7db5ab9f54a1d0`
- Transfer bundle reconstructed by concatenating `r5-integration-transfer/parts/part-*.b64`:
  SHA-256 `a716503cc1a4fcf42004f459584492144739ea7aa12563e6fa1af5d2bc4dab0a`
- Transfer manifest: 540 entries; each path, type, mode, size and SHA-256/symlink target is checked.
- Candidate manifest: 4,502 entries.
- Pinned Solnix: `1cee7021091f3df92b7816ece2902ff59b37f1be`.
- Pinned illumos-gate: `098e95820891f8a83fae679daece7fcf58b7b3a4`.
- Additive active integration patch: `0027-run80-r5-active-iwx-integration.patch`.

The only pre-existing repository file reused by the workflow is
`wifi-ax101/solnix-1cee7021091f3df92b7816ece2902ff59b37f1be.zip`.
Its SHA-256 and Git blob were verified identical to the archive embedded in
the local candidate. No other old Wi-Fi source, patch stack or generated file
from the base branch is used as a build input.

The workflow checks out its own exact `github.sha`, verifies branch identity,
reconstructs and hashes the transfer bundle, checks both manifests, reconstructs
a fresh checkout of the pinned Solnix commit, replays R5 plus patch 0027, and
requires exact source-tree equality before the OpenIndiana native build.

