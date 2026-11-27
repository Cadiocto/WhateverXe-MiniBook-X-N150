# RUN106 packaging-only recovery trigger

This marker intentionally causes the RUN106 workflow to select the package-recovery job.
The build-iwx job is skipped for commits containing [packaging-only].

The recovery job reuses:
- source Actions run 46059925320;
- immutable source artifact ID 15229533467;
- source artifact SHA-256 107ce8e4c2381d5c91dece0d0b2ea29539c49389d177af605882ee2a20880f11;
- original native guest evidence and original Actions job logs.

No ISO download, QEMU boot, package installation in the guest, or native compilation is part of this recovery path.
