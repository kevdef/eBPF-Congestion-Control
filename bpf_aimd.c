// SPDX-License-Identifier: GPL-2.0-only

/* WARNING: This implemenation is not necessarily the same
 * as the tcp_cubic.c.  The purpose is mainly for testing
 * the kernel BPF logic.
 *
 * Highlights:
 * 1. CONFIG_HZ .kconfig map is used.
 * 2. In bictcp_update(), calculation is changed to use usec
 *    resolution (i.e. USEC_PER_JIFFY) instead of using jiffies.
 *    Thus, usecs_to_jiffies() is not used in the bpf_cubic.c.
 * 3. In bitctcp_update() [under tcp_friendliness], the original
 *    "while (ca->ack_cnt > delta)" loop is changed to the equivalent
 *    "ca->ack_cnt / delta" operation.
 */

#include <linux/bpf.h>
#include "bpf_tcp_helpers.h"

char _license[] SEC("license") = "GPL";

#define clamp(val, lo, hi) min((typeof(val))max(val, lo), hi)

#define BICTCP_BETA_SCALE    1024	/* Scale factor beta calculation
					 * max_cwnd = snd_cwnd * beta
					 */
#define	BICTCP_HZ		10	/* BIC HZ 2^10 = 1024 */

/* Two methods of hybrid slow start */
#define HYSTART_ACK_TRAIN	0x1
#define HYSTART_DELAY		0x2

/* Number of delay samples for detecting the increase of delay */
#define HYSTART_MIN_SAMPLES	8
#define HYSTART_DELAY_MIN	(4000U)	/* 4ms */
#define HYSTART_DELAY_MAX	(16000U)	/* 16 ms */
#define HYSTART_DELAY_THRESH(x)	clamp(x, HYSTART_DELAY_MIN, HYSTART_DELAY_MAX)

static int fast_convergence = 1;
static const int beta = 717;	/* = 717/1024 (BICTCP_BETA_SCALE) */
static int initial_ssthresh;
static const int bic_scale = 41;
static int tcp_friendliness = 1;

static int hystart = 0;
static int hystart_detect = HYSTART_ACK_TRAIN | HYSTART_DELAY;
static int hystart_low_window = 16;
static int hystart_ack_delta_us = 2000;

static const __u32 beta_scale = 8*(BICTCP_BETA_SCALE+beta) / 3
				/ (BICTCP_BETA_SCALE - beta);

/* BIC TCP Parameters */
struct bictcp {
	__u32	cnt;		/* increase cwnd by 1 after ACKs */
	__u32	last_max_cwnd;	/* last maximum snd_cwnd */
	__u32	last_cwnd;	/* the last snd_cwnd */
	__u32	last_time;	/* time when updated last_cwnd */
	__u32	bic_origin_point;/* origin point of bic function */
	__u32	bic_K;		/* time to origin point
				   from the beginning of the current epoch */
	__u32	delay_min;	/* min delay (usec) */
	__u32	epoch_start;	/* beginning of an epoch */
	__u32	ack_cnt;	/* number of acks */
	__u32	tcp_cwnd;	/* estimated tcp cwnd */
	__u16	unused;
	__u8	sample_cnt;	/* number of samples to decide curr_rtt */
	__u8	found;		/* the exit point is found? */
	__u32	round_start;	/* beginning of each round */
	__u32	end_seq;	/* end_seq of the round */
	__u32	last_ack;	/* last time when the ACK spacing is close */
	__u32	curr_rtt;	/* the minimum rtt of current round */
};

static inline void bictcp_reset(struct bictcp *ca)
{
	ca->cnt = 0;
	ca->last_max_cwnd = 0;
	ca->last_cwnd = 0;
	ca->last_time = 0;
	ca->bic_origin_point = 0;
	ca->bic_K = 0;
	ca->delay_min = 0;
	ca->epoch_start = 0;
	ca->ack_cnt = 0;
	ca->tcp_cwnd = 0;
	ca->found = 0;
}

extern unsigned long CONFIG_HZ __kconfig;
#define HZ CONFIG_HZ
#define USEC_PER_MSEC	1000UL
#define USEC_PER_SEC	1000000UL
#define USEC_PER_JIFFY	(USEC_PER_SEC / HZ)

static __always_inline __u64 div64_u64(__u64 dividend, __u64 divisor)
{
	return dividend / divisor;
}

#define div64_ul div64_u64

#define BITS_PER_U64 (sizeof(__u64) * 8)
static __always_inline int fls64(__u64 x)
{
	int num = BITS_PER_U64 - 1;

	if (x == 0)
		return 0;

	if (!(x & (~0ull << (BITS_PER_U64-32)))) {
		num -= 32;
		x <<= 32;
	}
	if (!(x & (~0ull << (BITS_PER_U64-16)))) {
		num -= 16;
		x <<= 16;
	}
	if (!(x & (~0ull << (BITS_PER_U64-8)))) {
		num -= 8;
		x <<= 8;
	}
	if (!(x & (~0ull << (BITS_PER_U64-4)))) {
		num -= 4;
		x <<= 4;
	}
	if (!(x & (~0ull << (BITS_PER_U64-2)))) {
		num -= 2;
		x <<= 2;
	}
	if (!(x & (~0ull << (BITS_PER_U64-1))))
		num -= 1;

	return num + 1;
}

static __always_inline __u32 bictcp_clock_us(const struct sock *sk)
{
	return tcp_sk(sk)->tcp_mstamp;
}

static __always_inline void bictcp_hystart_reset(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct bictcp *ca = inet_csk_ca(sk);

	ca->round_start = ca->last_ack = bictcp_clock_us(sk);
	ca->end_seq = tp->snd_nxt;
	ca->curr_rtt = ~0U;
	ca->sample_cnt = 0;
}

/* "struct_ops/" prefix is not a requirement
 * It will be recognized as BPF_PROG_TYPE_STRUCT_OPS
 * as long as it is used in one of the func ptr
 * under SEC(".struct_ops").
 */
SEC("struct_ops/bictcp_init")
void BPF_PROG(bictcp_init, struct sock *sk)
{
	struct bictcp *ca = inet_csk_ca(sk);

	bictcp_reset(ca);

	if (hystart)
		bictcp_hystart_reset(sk);

	if (!hystart && initial_ssthresh)
		tcp_sk(sk)->snd_ssthresh = initial_ssthresh;
}

/* No prefix in SEC will also work.
 * The remaining tcp-cubic functions have an easier way.
 *
SEC("no-sec-prefix-bictcp_cwnd_event")
void BPF_PROG(bictcp_cwnd_event, struct sock *sk, enum tcp_ca_event event)
{
	*
	 * CA_EVENT_TX_START -> First Transmit when no packet in flight
	 *
	if (event == CA_EVENT_TX_START) {
		struct bictcp *ca = inet_csk_ca(sk);
		__u32 now = tcp_jiffies32;
		__s32 delta;
		//lsndtime -> time of last sent datapacket
		delta = now - tcp_sk(sk)->lsndtime;

		* We were application limited (idle) for a while.
		 * Shift epoch_start to keep cwnd growth to cubic curve.
		 *
		if (ca->epoch_start && delta > 0) {
			ca->epoch_start += delta;
			if (after(ca->epoch_start, now))
				ca->epoch_start = now;
		}
		return;
	}
}
*/

void BPF_STRUCT_OPS(reno_cong_avoid, struct sock *sk, __u32 ack, __u32 acked)
{
	struct tcp_sock *tp = tcp_sk(sk);
	/*
	struct bictcp *ca = inet_csk_ca(sk);

	tcp_cong_avoid_ai(tp, ca->cnt, acked);

	if (!tcp_is_cwnd_limited(sk))
		return;
	*/
	/* In "safe" area, increase. */
	if (tcp_in_slow_start(tp)) {
		acked = tcp_slow_start(tp, acked);
		if (!acked)
			return;
	}
	/* In dangerous area, increase slowly. */
	tcp_cong_avoid_ai(tp, tp->snd_cwnd, acked);
}

__u32 BPF_STRUCT_OPS(aimd_ssthresh, struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	return max(tp->snd_cwnd >> 1U, 2U);
}
/*
void BPF_STRUCT_OPS(bictcp_state, struct sock *sk, __u8 new_state)
{
	if (new_state == TCP_CA_Loss) {
		bictcp_reset(inet_csk_ca(sk));
		bictcp_hystart_reset(sk);
	}
}
*/
#define GSO_MAX_SIZE		65536

/* Account for TSO/GRO delays.
 * Otherwise short RTT flows could get too small ssthresh, since during
 * slow start we begin with small TSO packets and ca->delay_min would
 * not account for long aggregation delay when TSO packets get bigger.
 * Ideally even with a very small RTT we would like to have at least one
 * TSO packet being sent and received by GRO, and another one in qdisc layer.
 * We apply another 100% factor because @rate is doubled at this point.
 * We cap the cushion to 1ms.
 */
static __always_inline __u32 hystart_ack_delay(struct sock *sk)
{
	unsigned long rate;

	rate = sk->sk_pacing_rate;
	if (!rate)
		return 0;
	return min((__u64)USEC_PER_MSEC,
		   div64_ul((__u64)GSO_MAX_SIZE * 4 * USEC_PER_SEC, rate));
}

static __always_inline void hystart_update(struct sock *sk, __u32 delay)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct bictcp *ca = inet_csk_ca(sk);
	__u32 threshold;

	if (hystart_detect & HYSTART_ACK_TRAIN) {
		__u32 now = bictcp_clock_us(sk);

		/* first detection parameter - ack-train detection */
		if ((__s32)(now - ca->last_ack) <= hystart_ack_delta_us) {
			ca->last_ack = now;

			threshold = ca->delay_min + hystart_ack_delay(sk);

			/* Hystart ack train triggers if we get ack past
			 * ca->delay_min/2.
			 * Pacing might have delayed packets up to RTT/2
			 * during slow start.
			 */
			if (sk->sk_pacing_status == SK_PACING_NONE)
				threshold >>= 1;

			if ((__s32)(now - ca->round_start) > threshold) {
				ca->found = 1;
				tp->snd_ssthresh = tp->snd_cwnd;
			}
		}
	}

	if (hystart_detect & HYSTART_DELAY) {
		/* obtain the minimum delay of more than sampling packets */
		if (ca->curr_rtt > delay)
			ca->curr_rtt = delay;
		if (ca->sample_cnt < HYSTART_MIN_SAMPLES) {
			ca->sample_cnt++;
		} else {
			if (ca->curr_rtt > ca->delay_min +
			    HYSTART_DELAY_THRESH(ca->delay_min >> 3)) {
				ca->found = 1;
				tp->snd_ssthresh = tp->snd_cwnd;
			}
		}
	}
}

void BPF_STRUCT_OPS(bictcp_acked, struct sock *sk,
		    const struct ack_sample *sample)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	struct bictcp *ca = inet_csk_ca(sk);
	__u32 delay;

	/* Some calls are for duplicates without timetamps */
	if (sample->rtt_us < 0)
		return;

	/* Discard delay samples right after fast recovery */
	if (ca->epoch_start && (__s32)(tcp_jiffies32 - ca->epoch_start) < HZ)
		return;

	delay = sample->rtt_us;
	if (delay == 0)
		delay = 1;

	/* first time call or link delay decreases */
	if (ca->delay_min == 0 || ca->delay_min > delay)
		ca->delay_min = delay;

	/* hystart triggers when cwnd is larger than some threshold */
	if (!ca->found && tcp_in_slow_start(tp) && hystart &&
	    tp->snd_cwnd >= hystart_low_window)
		hystart_update(sk, delay);
}

__u32 BPF_STRUCT_OPS(tcp_reno_undo_cwnd, struct sock *sk)
{
	const struct tcp_sock *tp = tcp_sk(sk);

	return max(tp->snd_cwnd, tp->prior_cwnd);
}

SEC(".struct_ops")
struct tcp_congestion_ops aimd = {
	.init		= (void *)bictcp_init,
	.ssthresh	= (void *)aimd_ssthresh,
	.cong_avoid	= (void *)reno_cong_avoid,
	.undo_cwnd	= (void *)tcp_reno_undo_cwnd,
//	.cwnd_event	= (void *)bictcp_cwnd_event,
	.pkts_acked     = (void *)bictcp_acked,
	.name		= "bpf_aimd",
};
