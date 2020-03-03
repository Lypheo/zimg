#pragma once

#ifdef ZIMG_ARM

#include <cstdint>
#include "common/ccdep.h"

// A32 compatibility macros
#if defined(_M_ARM) || defined(__arm__)
  #define vmovl_high_u8(x) vmovl_u8(vget_high_u8(x))
  #define vmovl_high_u16(x) vmovl_u16(vget_high_u16(x))
  #define vmovn_high_u16(r, a) vcombine_u8(r, vmovn_u16(a))
  #define vqmovn_high_u32(r, a) vcombine_u16(r, vqmovn_u32(a))
  #define vcvt_high_f16_f32(r, a) vcombine_f16(r, vcvt_f16_f32(a))
  #define vcvt_high_f32_f16(a) vcvt_f32_f16(vget_high_f16(a))
  #define vfmaq_f32(a, b, c) vmlaq_f32(a, b, c)
  #define vdupq_laneq_f32(vec, lane) ((lane) >= 2 ? vdupq_lane_f32(vget_high_f32(vec), (lane) % 2) : vdupq_lane_f32(vget_low_f32(vec), (lane) % 2))
#endif

#if defined(_MSC_VER) && defined(_M_ARM64)
  #define __fp16 uint16_t
  #define float16x8_t uint16x8_t
  #define vld1q_f16 vld1q_u16
  #define vst1q_f16 vst1q_u16
  #define vmovn_high_u16(r, a) neon_xtn2_16(r, a)
  #define vdupq_laneq_f32(vec, lane) neon_dupqe32q(vec, lane)
  #define vreinterpretq_f32_f64(a) a
#endif

namespace zimg {

// The n-th mask vector has the lower n bytes set to all-ones.
extern const uint8_t neon_mask_table alignas(16)[17][16];

// Store from [x] into [dst] the 8-bit elements with index less than [idx].
static inline FORCE_INLINE void neon_store_idxlo_u8(uint8_t *dst, uint8x16_t x, unsigned idx)
{
	uint8x16_t orig = vld1q_u8(dst);
	uint8x16_t mask = vld1q_u8(neon_mask_table[idx]);

	orig = vbicq_u8(orig, mask);
	x = vandq_u8(x, mask);
	x = vorrq_u8(x, orig);

	vst1q_u8(dst, x);
}

// Store from [x] into [dst] the 8-bit elements with index greater than or equal to [idx].
static inline FORCE_INLINE void neon_store_idxhi_u8(uint8_t *dst, uint8x16_t x, unsigned idx)
{
	uint8x16_t orig = vld1q_u8(dst);
	uint8x16_t mask = vld1q_u8(neon_mask_table[idx]);

	orig = vandq_u8(orig, mask);
	x = vbicq_u8(x, mask);
	x = vorrq_u8(x, orig);

	vst1q_u8(dst, x);
}

// Store from [x] into [dst] the 16-bit elements with index less than [idx].
static inline FORCE_INLINE void neon_store_idxlo_u16(uint16_t *dst, uint16x8_t x, unsigned idx)
{
	neon_store_idxlo_u8(reinterpret_cast<uint8_t *>(dst), vreinterpretq_u8_u16(x), idx * 2);
}

// Store from [x] into [dst] the 16-bit elements with index greater than or equal to [idx].
static inline FORCE_INLINE void neon_store_idxhi_u16(uint16_t *dst, uint16x8_t x, unsigned idx)
{
	neon_store_idxhi_u8(reinterpret_cast<uint8_t *>(dst), vreinterpretq_u8_u16(x), idx * 2);
}

#if !defined(_MSC_VER) || defined(_M_ARM64)
// Store from [x] into [dst] the 16-bit elements with index less than [idx].
static inline FORCE_INLINE void neon_store_idxlo_f16(__fp16 *dst, float16x8_t x, unsigned idx)
{
	neon_store_idxlo_u8(reinterpret_cast<uint8_t *>(dst), vreinterpretq_u8_f16(x), idx * 2);
}

// Store from [x] into [dst] the 16-bit elements with index greater than or equal to [idx].
static inline FORCE_INLINE void neon_store_idxhi_f16(__fp16 *dst, float16x8_t x, unsigned idx)
{
	neon_store_idxhi_u8(reinterpret_cast<uint8_t *>(dst), vreinterpretq_u8_f16(x), idx * 2);
}
#endif // !defined(_MSC_VER) || defined(_M_ARM64)

// Store from [x] into [dst] the 32-bit elements with index less than [idx].
static inline FORCE_INLINE void neon_store_idxlo_f32(float *dst, float32x4_t x, unsigned idx)
{
	neon_store_idxlo_u8(reinterpret_cast<uint8_t *>(dst), vreinterpretq_u8_f32(x), idx * 4);
}

// Store from [x] into [dst] the 32-bit elements with index greater than or equal to [idx]
static inline FORCE_INLINE void neon_store_idxhi_f32(float *dst, float32x4_t x, unsigned idx)
{
	neon_store_idxhi_u8(reinterpret_cast<uint8_t *>(dst), vreinterpretq_u8_f32(x), idx * 4);
}

// Stores the elements of [x] into [dst0]-[dst3].
static inline FORCE_INLINE void neon_scatter_f32(float *dst0, float *dst1, float *dst2, float *dst3, float32x4_t x)
{
	*dst0 = vgetq_lane_f32(x, 0);
	*dst1 = vgetq_lane_f32(x, 1);
	*dst2 = vgetq_lane_f32(x, 2);
	*dst3 = vgetq_lane_f32(x, 3);
}

// Transpose in-place the 4x4 matrix stored in [row0]-[row3].
static inline FORCE_INLINE void neon_transpose4_f32(float32x4_t &row0, float32x4_t &row1, float32x4_t &row2, float32x4_t &row3)
{
	float32x4x2_t t0_t1 = vtrnq_f32(row0, row1);
	float32x4x2_t t2_t3 = vtrnq_f32(row2, row3);
	float32x4_t t0 = t0_t1.val[0];
	float32x4_t t1 = t0_t1.val[1];
	float32x4_t t2 = t2_t3.val[0];
	float32x4_t t3 = t2_t3.val[1];
#if defined(__aarch64__) || defined(_M_ARM64)
	row0 = vreinterpretq_f32_f64(vzip1q_f64(vreinterpretq_f64_f32(t0), vreinterpretq_f64_f32(t2)));
	row1 = vreinterpretq_f32_f64(vzip1q_f64(vreinterpretq_f64_f32(t1), vreinterpretq_f64_f32(t3)));
	row2 = vreinterpretq_f32_f64(vzip2q_f64(vreinterpretq_f64_f32(t0), vreinterpretq_f64_f32(t2)));
	row3 = vreinterpretq_f32_f64(vzip2q_f64(vreinterpretq_f64_f32(t1), vreinterpretq_f64_f32(t3)));
#else
	row0 = vcombine_f32(vget_low_f32(t0), vget_low_f32(t2));
	row1 = vcombine_f32(vget_low_f32(t1), vget_low_f32(t3));
	row2 = vcombine_f32(vget_high_f32(t0), vget_high_f32(t2));
	row3 = vcombine_f32(vget_high_f32(t1), vget_high_f32(t3));
#endif
}

} // namespace

#endif // ZIMG_ARM
