"""
크리스탈 광석용 프로시저럴 텍스처 생성 스크립트 (에디터 밖 일반 Python, numpy+Pillow 필요).

Substance Designer 그래프(DarkCrystalShard)의 마스크 구성을 근사 재현한다.
모든 맵은 타일링(seamless) 보장.

출력 (Content/LostSignal/Textures/CrystalOre/SourceAssets/):
  T_CrystalOre_Packed.png    — R: 크랙 / G: 스트림 / B: 스펙클 / A: 그런지
  T_CrystalOre_Curvature.png — 커버처 대용 그레이스케일 (베이크 전 플레이스홀더)
  T_CrystalOre_N.png         — 디테일 노멀 (DirectX, G 아래 방향)

실행: python tools/generate_crystal_textures.py
생성 후 에디터에서 tools/import_crystal_textures.py 로 임포트한다.
"""

from pathlib import Path

import numpy as np
from PIL import Image

SIZE = 1024
SEED = 7
OUT_DIR = Path(__file__).resolve().parent.parent / "Content/LostSignal/Textures/CrystalOre/SourceAssets"


def hash01(ix, iy, seed):
    # 정수 격자 좌표 → [0,1) 결정적 해시. 호출 전에 격자 수로 wrap해서 타일링 보장.
    h = (ix.astype(np.int64) * 374761393 + iy.astype(np.int64) * 668265263 + seed * 1446648559) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    return ((h ^ (h >> 16)) & 0xFFFFFFFF) / 2.0**32


def value_noise_xy(x, y, cells, seed):
    # 임의 좌표(x, y ∈ [0, cells))에서의 타일링 밸류 노이즈
    x0 = np.floor(x).astype(np.int64)
    y0 = np.floor(y).astype(np.int64)
    fx, fy = x - x0, y - y0
    ux = fx * fx * fx * (fx * (fx * 6 - 15) + 10)
    uy = fy * fy * fy * (fy * (fy * 6 - 15) + 10)

    def h(i, j):
        return hash01((x0 + i) % cells, (y0 + j) % cells, seed)

    top = h(0, 0) * (1 - ux) + h(1, 0) * ux
    bot = h(0, 1) * (1 - ux) + h(1, 1) * ux
    return top * (1 - uy) + bot * uy


def grid_uv(size):
    coords = np.linspace(0.0, 1.0, size, endpoint=False)
    return np.meshgrid(coords, coords)


def fbm_xy(u, v, cells, octaves, seed):
    # u, v ∈ [0,1). 옥타브마다 격자 수를 2배로 올려 합산 후 0~1 정규화.
    total = np.zeros_like(u)
    amp, amp_sum = 1.0, 0.0
    for o in range(octaves):
        c = cells * (2 ** o)
        total += amp * value_noise_xy((u % 1.0) * c, (v % 1.0) * c, c, seed + o * 101)
        amp_sum += amp
        amp *= 0.5
    total /= amp_sum
    lo, hi = total.min(), total.max()
    return (total - lo) / max(hi - lo, 1e-6)


def fbm(size, cells, octaves, seed):
    u, v = grid_uv(size)
    return fbm_xy(u, v, cells, octaves, seed)


def worley(size, cells, seed, warp_u=None, warp_v=None, warp_amp=0.0):
    # 셀당 특징점 1개(지터), 토러스 wrap으로 F1/F2 거리장 계산 (셀 단위 거리)
    # warp_*를 주면 조회 좌표를 fBm으로 비틀어 셀 경계(균열 선)가 유기적으로 구불거린다
    u, v = grid_uv(size)
    if warp_u is not None:
        u = (u + warp_amp * (warp_u - 0.5)) % 1.0
        v = (v + warp_amp * (warp_v - 0.5)) % 1.0
    x, y = u * cells, v * cells
    xi = np.floor(x).astype(np.int64)
    yi = np.floor(y).astype(np.int64)
    f1 = np.full((size, size), 1e9)
    f2 = np.full((size, size), 1e9)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            cx, cy = xi + dx, yi + dy
            px = cx + hash01(cx % cells, cy % cells, seed)
            py = cy + hash01(cx % cells, cy % cells, seed + 77)
            d = np.hypot(x - px, y - py)
            closer = d < f1
            f2 = np.where(closer, f1, np.minimum(f2, d))
            f1 = np.where(closer, d, f1)
    return f1, f2


def smoothstep(edge0, edge1, x):
    t = np.clip((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def make_crack(size, seed):
    # 셀 경계 거리(F2-F1) 기반 균열. 중심이 1.0, 가장자리로 갈수록 부드럽게 감쇠.
    warp_u = fbm(size, 9, 4, seed + 50)
    warp_v = fbm(size, 9, 4, seed + 51)
    f1a, f2a = worley(size, 4, seed, warp_u, warp_v, 0.15)  # 셀 수 축소 → 균열 수 감소
    w = (f2a - f1a) * (0.75 + 0.5 * fbm(size, 10, 3, seed + 5))  # 폭 흔들림
    glow = (1.0 - smoothstep(0.0, 0.30, w)) ** 1.8  # 넓고 부드러운 폭 감쇠
    core = 1.0 - smoothstep(0.0, 0.09, w)           # 중심 코어
    crack_big = 0.6 * glow + 0.4 * core
    f1b, f2b = worley(size, 9, seed + 9, warp_u, warp_v, 0.10)
    crack_small = (1.0 - smoothstep(0.0, 0.16, f2b - f1b)) ** 2.0
    region = smoothstep(0.45, 0.8, fbm(size, 3, 4, seed + 3))  # 지역 컬링 강화
    crack = np.clip(crack_big + 0.3 * crack_small, 0.0, 1.0) * (0.25 + 0.75 * region)
    return np.clip(crack, 0.0, 1.0)


def make_stream(size, seed):
    # 도메인 워프한 fBm의 릿지 → 흐르는 에너지 줄기
    u, v = grid_uv(size)
    warp_u = fbm(size, 4, 4, seed + 20) - 0.5
    warp_v = fbm(size, 4, 4, seed + 21) - 0.5
    base = fbm_xy(u + 0.35 * warp_u, v + 0.35 * warp_v, 5, 5, seed + 22)
    ridge = 1.0 - np.abs(2.0 * base - 1.0)
    wisps = ridge ** 2.4
    body = smoothstep(0.35, 0.8, fbm(size, 3, 4, seed + 25))  # 줄기 덩어리 분리
    return np.clip(wisps * (0.25 + 0.75 * body), 0.0, 1.0)


def make_speckle(size, seed):
    # 고밀도 워리 F1로 작은 점, 저밀도로 큰 점 소량. 밝기는 셀 스케일 노이즈로 변주.
    f1_fine, _ = worley(size, 48, seed + 40)
    dots_fine = 1.0 - smoothstep(0.08, 0.22, f1_fine)
    f1_big, _ = worley(size, 16, seed + 44)
    dots_big = 1.0 - smoothstep(0.05, 0.14, f1_big)
    bright = 0.3 + 0.7 * fbm(size, 48, 1, seed + 41) ** 1.5
    return np.clip(dots_fine * bright + dots_big * 0.9, 0.0, 1.0)


def make_grunge(size, seed):
    # 러프니스 변주용 중간 주파수 그런지
    g = fbm(size, 6, 5, seed + 60)
    ridged = (1.0 - np.abs(2.0 * fbm(size, 5, 4, seed + 61) - 1.0)) ** 1.5
    return np.clip(0.6 * g + 0.4 * ridged, 0.0, 1.0)


def make_normal(size, crack, seed, strength=2.2):
    # 표면 요철 + 균열 파임 높이맵에서 노멀 계산 (DirectX 규약: G = 아래)
    h = fbm(size, 8, 5, seed + 70) - 0.45 * crack
    gx = (np.roll(h, -1, axis=1) - np.roll(h, 1, axis=1)) * 0.5 * size / 100.0
    gy = (np.roll(h, -1, axis=0) - np.roll(h, 1, axis=0)) * 0.5 * size / 100.0
    nx, ny, nz = -gx * strength, gy * strength, np.ones_like(h)
    length = np.sqrt(nx * nx + ny * ny + nz * nz)
    return np.stack([nx / length, ny / length, nz / length], axis=-1) * 0.5 + 0.5


def to_u8(arr):
    return (np.clip(arr, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    print(f"[CrystalTex] {SIZE}x{SIZE}, seed={SEED} 생성 시작")

    crack = make_crack(SIZE, SEED)
    stream = make_stream(SIZE, SEED)
    speckle = make_speckle(SIZE, SEED)
    grunge = make_grunge(SIZE, SEED)

    packed = np.stack([to_u8(crack), to_u8(stream), to_u8(speckle), to_u8(grunge)], axis=-1)
    Image.fromarray(packed, "RGBA").save(OUT_DIR / "T_CrystalOre_Packed.png")
    print("[CrystalTex] T_CrystalOre_Packed.png 저장")

    curvature = np.clip(0.5 * grunge + 0.5 * (1.0 - np.abs(2.0 * fbm(SIZE, 7, 4, SEED + 80) - 1.0)), 0, 1)
    Image.fromarray(to_u8(curvature), "L").save(OUT_DIR / "T_CrystalOre_Curvature.png")
    print("[CrystalTex] T_CrystalOre_Curvature.png 저장")

    normal = make_normal(SIZE, crack, SEED)
    Image.fromarray(to_u8(normal), "RGB").save(OUT_DIR / "T_CrystalOre_N.png")
    print("[CrystalTex] T_CrystalOre_N.png 저장")

    print(f"[CrystalTex] 완료: {OUT_DIR}")


if __name__ == "__main__":
    main()
