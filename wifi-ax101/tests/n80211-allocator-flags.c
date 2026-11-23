#include <stdio.h>

#define M_WAITOK 0x0001
#define M_NOWAIT 0x0002
#define M_CANFAIL 0x0004
#define M_ZERO 0x0100

#define KM_SLEEP 0x10
#define KM_NOSLEEP 0x20

#include <sys/n80211_alloc.h>

static unsigned int checks;
static unsigned int failures;

#define CHECK(expr, label) do { \
    ++checks; \
    if (!(expr)) { \
        fprintf(stderr, "FAIL: %s\n", label); \
        ++failures; \
    } else { \
        printf("PASS: %s\n", label); \
    } \
} while (0)

int
main(void)
{
    CHECK(iwx_bsd_kmem_flags(M_WAITOK) == KM_SLEEP,
        "M_WAITOK maps to KM_SLEEP");
    CHECK(iwx_bsd_kmem_flags(M_NOWAIT) == KM_NOSLEEP,
        "M_NOWAIT maps to KM_NOSLEEP");
    CHECK(iwx_bsd_kmem_flags(M_WAITOK | M_CANFAIL) == KM_NOSLEEP,
        "M_WAITOK|M_CANFAIL preserves a return-NULL path");
    CHECK(iwx_bsd_kmem_flags(M_NOWAIT | M_CANFAIL) == KM_NOSLEEP,
        "M_CANFAIL does not change M_NOWAIT");
    CHECK(iwx_bsd_kmem_flags(M_WAITOK | M_ZERO) == KM_SLEEP,
        "M_ZERO does not change waitability");
    CHECK(iwx_bsd_kmem_flags(M_NOWAIT | M_ZERO) == KM_NOSLEEP,
        "M_ZERO does not change non-waitability");

    printf("%s: %u checks, %u failures\n",
        failures == 0 ? "PASS" : "FAIL", checks, failures);
    return (failures == 0 ? 0 : 1);
}
