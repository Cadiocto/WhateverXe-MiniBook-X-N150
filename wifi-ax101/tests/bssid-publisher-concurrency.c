#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Deterministic interleaving model of the production publisher contract.
 * Explicit begin/enqueue/finish steps replace scheduler timing and make every
 * race window reproducible. This complements, but does not replace, native VM
 * module tests and a kernel-thread stress test.
 */
typedef struct model {
        uint64_t pending, published, failed;
        int accepting, callbacks_enabled, stop, registered, freed;
        unsigned callbacks, workers, mac_active, max_mac_active, pdata_calls, tx_calls;
        int callbacks_drained, overflow;
        int pdata_error, unregister_error;
        unsigned char pending_value, published_value, active_value;
} model_t;

static void
init(model_t *m)
{
        memset(m, 0, sizeof (*m));
        m->accepting = 1;
        m->callbacks_enabled = 1;
        m->registered = 1;
        m->workers = 1;
}

static int
enqueue(model_t *m, unsigned char value)
{
        if (!m->accepting || m->stop)
                return (0);
        if (m->pending == UINT64_MAX) {
                m->overflow = 1;
                m->accepting = 0;
                return (0);
        }
        m->pending++;
        m->pending_value = value;
        return (1);
}

static int
begin_publish(model_t *m, uint64_t *generation)
{
        if (m->workers != 1 || m->stop || !m->registered ||
            m->pending == m->published || m->failed == m->pending)
                return (0);
        assert(m->mac_active == 0);
        m->mac_active++;
        if (m->mac_active > m->max_mac_active)
                m->max_mac_active = m->mac_active;
        m->active_value = m->pending_value;
        *generation = m->pending;
        m->pdata_calls++;
        return (1);
}

static void
finish_publish(model_t *m, uint64_t generation)
{
        assert(m->mac_active == 1);
        if (m->pdata_error) {
                m->failed = generation;
                m->mac_active--;
                return;
        }
        m->tx_calls++;
        m->published = generation;
        m->published_value = m->active_value;
        m->failed = 0;
        m->mac_active--;
}

static void
callback_enter(model_t *m)
{
        assert(m->accepting);
        m->callbacks++;
}

static void
callback_exit(model_t *m, unsigned char value)
{
        assert(m->callbacks != 0);
        (void) enqueue(m, value);
        m->callbacks--;
        if (!m->callbacks_enabled && m->callbacks == 0)
                m->callbacks_drained = 1;
}

static void
disable_callbacks(model_t *m)
{
        m->accepting = 0;
        m->callbacks_enabled = 0;
        assert(m->callbacks == 0);
        m->callbacks_drained = 1;
}

static void
stop_and_join(model_t *m)
{
        assert(m->callbacks == 0);
        assert(m->published == m->pending);
        assert(m->mac_active == 0);
        m->stop = 1;
        m->workers = 0;
}

static void
case1(void) { model_t m; uint64_t g; init(&m); assert(enqueue(&m,1)); assert(begin_publish(&m,&g)); finish_publish(&m,g); assert(m.published==1 && m.pdata_calls==1 && m.tx_calls==1); }
static void
case2(void) { model_t m; uint64_t g; init(&m); enqueue(&m,1); enqueue(&m,2); enqueue(&m,3); assert(begin_publish(&m,&g)); finish_publish(&m,g); assert(g==3 && m.published_value==3); }
static void
case3(void) { model_t m; uint64_t g; init(&m); for (unsigned i=1;i<=12;i++) enqueue(&m,(unsigned char)i); assert(begin_publish(&m,&g)); finish_publish(&m,g); assert(g==12 && m.published==12 && m.pdata_calls==1); }
static void
case4(void) { model_t m; uint64_t g; init(&m); enqueue(&m,1); begin_publish(&m,&g); enqueue(&m,2); finish_publish(&m,g); assert(m.published==1); assert(begin_publish(&m,&g)); finish_publish(&m,g); assert(m.published==2 && m.published_value==2); }
static void
case5(void) { model_t m; uint64_t g; init(&m); enqueue(&m,1); begin_publish(&m,&g); enqueue(&m,2); finish_publish(&m,g); assert(m.tx_calls==1); begin_publish(&m,&g); finish_publish(&m,g); assert(m.published==2 && m.tx_calls==2); }
static void
case6(void) { model_t m; uint64_t g; init(&m); enqueue(&m,1); begin_publish(&m,&g); enqueue(&m,2); enqueue(&m,3); finish_publish(&m,g); assert(m.published==1); begin_publish(&m,&g); finish_publish(&m,g); assert(m.published==3); }
static void
case7(void) { model_t m; uint64_t g; init(&m); enqueue(&m,1); begin_publish(&m,&g); for (unsigned i=2;i<=8;i++) enqueue(&m,(unsigned char)i); finish_publish(&m,g); begin_publish(&m,&g); finish_publish(&m,g); assert(m.published==8 && m.published_value==8); }
static void
case8(void) { model_t m; init(&m); disable_callbacks(&m); assert(!enqueue(&m,9)); assert(m.pending==0); }
static void
case9(void) { model_t m; uint64_t g; init(&m); callback_enter(&m); m.callbacks_enabled=0; callback_exit(&m,7); m.accepting=0; assert(m.callbacks==0 && m.pending==1); begin_publish(&m,&g); finish_publish(&m,g); assert(m.published==1); }
static void
case10(void) { model_t m; uint64_t g; init(&m); callback_enter(&m); m.callbacks_enabled=0; assert(m.callbacks==1); callback_exit(&m,4); m.accepting=0; assert(m.callbacks==0 && m.pending==1); begin_publish(&m,&g); finish_publish(&m,g); assert(m.published==1); }
static void
case11(void) { model_t m; uint64_t g; init(&m); enqueue(&m,5); assert(m.published<m.pending); begin_publish(&m,&g); finish_publish(&m,g); assert(m.published==m.pending); }
static void
case12(void) { model_t m; init(&m); disable_callbacks(&m); stop_and_join(&m); assert(m.stop && m.workers==0 && !m.freed); m.freed=1; assert(m.freed); }
static void
case13(void) { model_t m; uint64_t g; init(&m); enqueue(&m,1); begin_publish(&m,&g); disable_callbacks(&m); assert(m.mac_active==1); finish_publish(&m,g); assert(m.published==m.pending); stop_and_join(&m); assert(m.workers==0); }
static void
case14(void) { model_t m; uint64_t g; init(&m); enqueue(&m,1); m.pdata_error=1; begin_publish(&m,&g); finish_publish(&m,g); assert(m.published==0 && m.failed==1 && m.tx_calls==0); }
static void
case15(void) { model_t m; uint64_t g; init(&m); enqueue(&m,1); begin_publish(&m,&g); finish_publish(&m,g); m.unregister_error=1; disable_callbacks(&m); assert(m.published==m.pending); if (m.unregister_error) { m.accepting=1; m.callbacks_enabled=1; m.stop=0; } assert(m.registered && m.workers==1 && !m.freed); }
static void
case16(void) { model_t m; uint64_t g; init(&m); enqueue(&m,1); begin_publish(&m,&g); finish_publish(&m,g); disable_callbacks(&m); assert(m.published==m.pending); m.registered=0; stop_and_join(&m); assert(!m.registered && m.workers==0 && m.stop); m.freed=1; assert(m.freed); }


static int
model_valid(const model_t *m)
{
        if (m->mac_active > 1 || m->max_mac_active > 1)
                return (0);
        if (m->freed && m->workers != 0)
                return (0);
        if (m->callbacks_drained && m->callbacks != 0)
                return (0);
        if (!m->registered && m->mac_active != 0)
                return (0);
        if (m->stop && m->pending != m->published)
                return (0);
        if (m->overflow && m->accepting)
                return (0);
        return (1);
}

static void
case17(void)
{
        model_t m; uint64_t g;
        init(&m); m.pending = UINT64_MAX; m.published = UINT64_MAX - 1;
        assert(!enqueue(&m, 0xaa));
        assert(m.overflow && !m.accepting && m.pending == UINT64_MAX);
        assert(begin_publish(&m, &g)); finish_publish(&m, g);
        assert(m.published == UINT64_MAX && model_valid(&m));
}

static void
case18(void)
{
        model_t m; uint64_t g;
        init(&m); assert(enqueue(&m, 1)); assert(begin_publish(&m, &g));
        assert(!begin_publish(&m, &g)); /* duplicate wakeup cannot start a second publisher */
        finish_publish(&m, 1);
        assert(m.max_mac_active == 1 && !begin_publish(&m, &g));
}

static void
case19(void)
{
        model_t m; uint64_t g;
        init(&m); callback_enter(&m); m.accepting = 0; m.callbacks_enabled = 0;
        callback_exit(&m, 0x42);
        assert(m.callbacks == 0 && m.callbacks_drained && m.pending == 1);
        assert(begin_publish(&m, &g)); finish_publish(&m, g);
        assert(m.published == 1 && model_valid(&m));
}

static void
case20(void)
{
        model_t m; uint64_t g;
        init(&m); assert(enqueue(&m, 1)); assert(begin_publish(&m, &g));
        finish_publish(&m, g);
        m.registered = 0;
        assert(!begin_publish(&m, &g));
        assert(model_valid(&m));
}

static void
case21(void)
{
        model_t m; init(&m);
        m.callbacks_enabled = 0; m.callbacks_drained = 1; m.callbacks = 1;
        assert(!model_valid(&m)); /* injected callback-after-drain defect */
        init(&m); m.mac_active = 2;
        assert(!model_valid(&m)); /* injected concurrent-publisher defect */
        init(&m); m.registered = 0; m.mac_active = 1;
        assert(!model_valid(&m)); /* injected publication-after-unregister defect */
        init(&m); m.freed = 1;
        assert(!model_valid(&m)); /* injected free-before-join defect */
        init(&m); m.stop = 1; m.pending = 2; m.published = 1;
        assert(!model_valid(&m)); /* injected lost-final-generation defect */
        (void) puts("publisher-negative-controls=PASS (5/5)");
}

int
main(void)
{
        void (*cases[])(void) = { case1,case2,case3,case4,case5,case6,case7,case8,
            case9,case10,case11,case12,case13,case14,case15,case16,case17,case18,case19,case20,case21 };
        for (unsigned i=0; i<sizeof (cases)/sizeof (cases[0]); i++) {
                cases[i]();
                (void) printf("publisher-scenario-%02u=PASS\n", i+1);
        }
        case21();
        (void) puts("BSSID_PUBLISHER_INTERLEAVING_MODEL=PASS (21/21)");
        return (0);
}
