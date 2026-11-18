#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned int move_calls;
static void *counting_memmove(void *, const void *, size_t);
#include "../rotfb_copy.h"

static void *
counting_memmove(void *dst, const void *src, size_t len)
{
	move_calls++;
	return (memmove(dst, src, len));
}

static void
init_bytes(uint8_t *buf, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		buf[i] = (uint8_t)i;
}

static int
check(int ok, const char *name)
{
	if (!ok) {
		(void) fprintf(stderr, "FAIL: %s\n", name);
		return (1);
	}
	return (0);
}

int
main(void)
{
	uint8_t buf[16], expected_forward[16] = {
	    4, 5, 6, 7, 8, 9, 10, 11, 8, 9, 10, 11, 12, 13, 14, 15 };
	uint8_t expected_backward[16] = {
	    0, 1, 2, 3, 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15 };
	uint8_t src[8], fb[8], shadow[8];
	unsigned int failures = 0;

	init_bytes(buf, sizeof (buf));
	move_calls = 0;
	rotfb_copy_rows(buf, buf + 4, buf, 4, 2, 4, 1, 1, 1,
	    counting_memmove);
	failures += check(move_calls == 2,
	    "forward alias copies each row once");
	failures += check(memcmp(buf, expected_forward, sizeof (buf)) == 0,
	    "forward overlapping move preserves bytes");

	init_bytes(buf, sizeof (buf));
	move_calls = 0;
	rotfb_copy_rows(buf + 4, buf, buf + 4, 4, 2, 4, 1, 1, 0,
	    counting_memmove);
	failures += check(move_calls == 2,
	    "backward alias copies each row once");
	failures += check(memcmp(buf, expected_backward, sizeof (buf)) == 0,
	    "backward overlapping move preserves bytes");

	init_bytes(src, sizeof (src));
	memset(fb, 0xee, sizeof (fb));
	memset(shadow, 0xdd, sizeof (shadow));
	move_calls = 0;
	rotfb_copy_rows(fb, src, shadow, 4, 2, 4, 1, 1, 1,
	    counting_memmove);
	failures += check(move_calls == 4,
	    "separate framebuffer and shadow both copied");
	failures += check(memcmp(fb, src, sizeof (src)) == 0,
	    "separate framebuffer receives source");
	failures += check(memcmp(shadow, src, sizeof (src)) == 0,
	    "separate shadow receives source");

	memset(fb, 0xee, sizeof (fb));
	memset(shadow, 0xdd, sizeof (shadow));
	move_calls = 0;
	rotfb_copy_rows(fb, src, shadow, 4, 2, 4, 0, 1, 1,
	    counting_memmove);
	failures += check(move_calls == 2, "graphics mode copies shadow only");
	failures += check(fb[0] == 0xee && fb[7] == 0xee,
	    "graphics mode leaves framebuffer destination untouched");
	failures += check(memcmp(shadow, src, sizeof (src)) == 0,
	    "graphics mode updates shadow");

	init_bytes(buf, sizeof (buf));
	move_calls = 0;
	rotfb_copy_rows(buf + 4, buf, buf + 4, 4, 2, 4, 0, 1, 0,
	    counting_memmove);
	failures += check(move_calls == 2,
	    "aliased graphics destination copied once");

	move_calls = 0;
	rotfb_copy_rows(buf, buf, buf, 0, 2, 4, 1, 1, 1,
	    counting_memmove);
	rotfb_copy_rows(buf, buf, buf, 4, 0, 4, 1, 1, 1,
	    counting_memmove);
	rotfb_copy_rows(buf, buf, buf, 5, 2, 4, 1, 1, 1,
	    counting_memmove);
	rotfb_copy_rows(buf, buf, buf, 4, 2, 4, 1, 1, 1, NULL);
	failures += check(move_calls == 0,
	    "invalid geometry or missing mover rejected");

	if (failures != 0)
		return (1);
	(void) puts("ROTFB bitmap copy C tests: PASS");
	return (0);
}
