AX101 RUN80 R5 Integration — CI-only portability overlay 0001

Cause observed in GitHub Actions RUN #82:
OpenIndiana Hipster 2026.10 provides illumos /usr/bin/nm, which rejects the
GNU-only --defined-only option used by compile-n80211-taskq-wait.py.

Correction:
The runner applies this exact additive patch only to the CI test helper after
verifying the immutable R5 transfer bundle and its 540-entry manifest. The
helper now uses POSIX nm -P output and checks that n80211_taskq_destroy is a
defined symbol while taskq_destroy is absent.

This is not a production-source patch. It does not modify the local candidate
archive, Solnix source, generated IWX/IWM source, firmware or acceptance logic.
The syntax compile and negative compiler test are unchanged. The helper SHA is
checked before and after overlay application.

Patch SHA-256:
6e0d638baa1531e87b5813c084e06d4d08ba27802420d869e8acd38112f2fb9c

Original helper SHA-256:
5c20e790ef2d4f0a03862bd8f357d90395e8f79751ba4377980ba6788fae5b97

Patched helper SHA-256:
51176a485cb9b09efe4ca98c4eb3a14476b4e926fd6dc730e199a77b6bcd05a9
