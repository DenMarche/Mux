// ============================================================================
//  opache license
// ============================================================================
//
//  Copyright (c) 2026 Den Marché
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
//  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
//  OTHER DEALINGS IN THE SOFTWARE.
//
// ============================================================================

#ifndef MUX_HPP_INCLUDED
#define MUX_HPP_INCLUDED

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
#define MUX_WINDOWS 1
#else
#define MUX_WINDOWS 0
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127)
#endif

#if defined(_MSVC_LANG)
#define MUX_CPLUSPLUS _MSVC_LANG
#else
#define MUX_CPLUSPLUS __cplusplus
#endif

#if defined(__clang__) && MUX_CPLUSPLUS >= 202002L
#undef MUX_CPLUSPLUS
#if defined(__clang_major__) && __clang_major__ >= 15 && __cplusplus > 201703L
#define MUX_CPLUSPLUS 202002L
#else
#define MUX_CPLUSPLUS 201703L
#endif
#endif

#if MUX_CPLUSPLUS >= 201703L
#define MUX_IF_CONSTEXPR if constexpr
#define MUX_NODISCARD [[nodiscard]]
#else
#define MUX_IF_CONSTEXPR if
#define MUX_NODISCARD
#endif

#if MUX_CPLUSPLUS >= 201402L
#define MUX_CONSTEXPR constexpr
#else
#define MUX_CONSTEXPR inline
#endif

#define MUX_CAT_(A, B) A##B
#define MUX_CAT(A, B) MUX_CAT_(A, B)
#define MUX_UNIQUE_(LINE) MUX_CAT(MUXV_, LINE)
#define MUX_UNIQUE(LINE) MUX_CAT(MUX_UNIQUE_(LINE), __LINE__)

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)))
#define MUX_HAS_BUILTIN_BSWAP16 1
#else
#define MUX_HAS_BUILTIN_BSWAP16 0
#endif

namespace mux {

    enum : std::uint32_t { MUX_ABI_VARIANTS = 5 };

    namespace detail {

        constexpr std::uint16_t swap16(std::uint16_t V) {
#if MUX_HAS_BUILTIN_BSWAP16
            return __builtin_bswap16(V);
#else
            return static_cast<std::uint16_t>((V >> 8) | (V << 8));
#endif
        }

        constexpr std::uint32_t swap32(std::uint32_t V) {
#if defined(__clang__) || defined(__GNUC__)
            return __builtin_bswap32(V);
#else
            return ((V & 0x000000FFu) << 24) | ((V & 0x0000FF00u) << 8) |
                ((V & 0x00FF0000u) >> 8) | ((V & 0xFF000000u) >> 24);
#endif
        }

        constexpr std::uint32_t get32(const std::uint8_t* PTR) {
            return static_cast<std::uint32_t>(PTR[0]) | (static_cast<std::uint32_t>(PTR[1]) << 8) |
                (static_cast<std::uint32_t>(PTR[2]) << 16) | (static_cast<std::uint32_t>(PTR[3]) << 24);
        }

        constexpr void set32(std::uint8_t* PTR, std::uint32_t V) {
            PTR[0] = static_cast<std::uint8_t>(V);
            PTR[1] = static_cast<std::uint8_t>(V >> 8);
            PTR[2] = static_cast<std::uint8_t>(V >> 16);
            PTR[3] = static_cast<std::uint8_t>(V >> 24);
        }

        constexpr std::uint16_t get16(const std::uint8_t* PTR) {
            return static_cast<std::uint16_t>(static_cast<std::uint16_t>(PTR[0]) |
                static_cast<std::uint16_t>(PTR[1] << 8));
        }

        constexpr void set16(std::uint8_t* PTR, std::uint16_t V) {
            PTR[0] = static_cast<std::uint8_t>(V);
            PTR[1] = static_cast<std::uint8_t>(V >> 8);
        }

        constexpr std::uint64_t mix(std::uint64_t X) {
            X += 0x9E3779B97F4A7C15ull;
            X = (X ^ (X >> 30)) * 0xBF58476D1CE4E5B9ull;
            X = (X ^ (X >> 27)) * 0x94D049BB133111EBull;
            return X ^ (X >> 31);
        }

        constexpr std::uint64_t mix2(std::uint64_t X) {
            X ^= X >> 33;
            X *= 0xff51afd7ed558ccdull;
            X ^= X >> 33;
            X *= 0xc4ceb9fe1a85ec53ull;
            X ^= X >> 33;
            return X;
        }

        struct rot {
            static MUX_CONSTEXPR std::uint32_t rol(std::uint32_t V, std::uint32_t N) {
                return (V << N) | (V >> ((32u - N) & 31u));
            }
            static MUX_CONSTEXPR std::uint32_t ror(std::uint32_t V, std::uint32_t N) {
                return (V >> N) | (V << ((32u - N) & 31u));
            }
        };

        template <std::uint32_t W, std::uint32_t A>
        struct meta {

            static MUX_CONSTEXPR std::uint32_t WIDTH = W;
            static MUX_CONSTEXPR std::uint32_t ABI = A;
            static MUX_CONSTEXPR std::uint32_t ROUNDS = 2u + 2u * A;

            static MUX_CONSTEXPR std::uint32_t DELTA = 0x9E3779B9u;
            static MUX_CONSTEXPR std::uint32_t S0 = 0x61707865u;
            static MUX_CONSTEXPR std::uint32_t S1 = 0x3320646eu;
            static MUX_CONSTEXPR std::uint32_t S2 = 0x79622d32u;
            static MUX_CONSTEXPR std::uint32_t S3 = 0x6b206574u;

            static MUX_CONSTEXPR std::uint32_t R0 = (A * 7u) & 31u;
            static MUX_CONSTEXPR std::uint32_t R1 = (A * 5u) & 31u;
            static MUX_CONSTEXPR std::uint32_t R2 = (A * 3u) & 31u;
            static MUX_CONSTEXPR std::uint32_t R3 = (A * 1u) & 31u;
        };

        /* 
           PLEASE NOTE : key is derived from the address of the payload, if the linker
           folds two identical string literals into the same address,
           they will share a key.
        */

        template <std::uint32_t W, std::uint32_t A>
        MUX_CONSTEXPR void keys(std::uint32_t* KEY, const void* SELF) {
            using M = meta<W, A>;

            const std::uint64_t IDENTITY = (static_cast<std::uint64_t>(W) << 32) |
                (static_cast<std::uint64_t>(A) << 8) | M::ROUNDS;
            const std::uint64_t ROOT = mix2((reinterpret_cast<std::uint64_t>(SELF) >> 3) ^ IDENTITY);

            std::uint64_t MIXED = mix(ROOT);
            KEY[0] = 0x6B206574u ^ M::S3 ^ static_cast<std::uint32_t>(MIXED);
            KEY[1] = 0x79622D32u ^ M::S2 ^ static_cast<std::uint32_t>(MIXED >> 32);
            MIXED = mix(MIXED + 0xD1B54A32D192ED03ull);
            KEY[2] = 0x3320646Eu ^ M::S1 ^ static_cast<std::uint32_t>(MIXED);
            KEY[3] = 0x61707865u ^ M::S0 ^ static_cast<std::uint32_t>(MIXED >> 32);
        }

        template <std::uint32_t W, std::uint32_t A>
        MUX_CONSTEXPR void tweak(std::uint32_t* KEY, std::uint32_t INDEX) {
            const std::uint64_t MIXED =
                mix((static_cast<std::uint64_t>(INDEX) << 32) | static_cast<std::uint64_t>(KEY[0]));
            KEY[0] ^= static_cast<std::uint32_t>(MIXED);
            KEY[1] ^= static_cast<std::uint32_t>(MIXED >> 32);
            KEY[2] = swap32(KEY[2] + static_cast<std::uint32_t>(MIXED >> 11));
            KEY[3] = swap32(KEY[3] - static_cast<std::uint32_t>(MIXED >> 43));
        }

        template <std::uint32_t W, std::uint32_t A>
        MUX_CONSTEXPR std::uint32_t arx(std::uint32_t IN, std::uint32_t KEY, std::uint32_t ROUND) {
            using M = meta<W, A>;
            const std::uint32_t SALT = (W * 0x9E3779B9u) + (A * 0x85EBCA6Bu);
            std::uint32_t T = IN + KEY + (ROUND * M::DELTA) + SALT;
            T ^= rot::rol(T, M::R1);
            T += KEY;
            T ^= rot::rol(T, M::R2);
            return T ^ (KEY * M::DELTA);
        }

        template <std::uint32_t W, std::uint32_t A, bool INV = false>
        MUX_CONSTEXPR void block(std::uint8_t* PTR, const std::uint32_t* KEY) {
            using M = meta<W, A>;
            std::uint32_t L = get32(PTR);
            std::uint32_t RR = get32(PTR + 4);
            MUX_IF_CONSTEXPR(INV) {
                const std::uint32_t R2 = get32(PTR + 4);
                L = get32(PTR) - R2;
                RR = R2 ^ rot::rol(L, M::R3);
                for (std::uint32_t I = 4u; I-- != 0;) {
                    const std::uint32_t AV = RR ^ arx<W, A>(L, KEY[I & 3u], I);
                    RR = L;
                    L = AV;
                }
                set32(PTR, L);
                set32(PTR + 4, RR);
            }
else {
    for (std::uint32_t I = 0; I != 4u; ++I) {
        L ^= arx<W, A>(RR, KEY[I & 3u], I);
        const std::uint32_t T = L;
        L = RR;
        RR = T;
    }
    const std::uint32_t R2 = RR ^ rot::rol(L, M::R3);
    set32(PTR, L + R2);
    set32(PTR + 4, R2);
            }
        }

        template <std::uint32_t A, bool INV = false>
        MUX_CONSTEXPR void xtea(std::uint8_t* PTR, const std::uint32_t* KEY) {
            block<1u, A, INV>(PTR, KEY);
        }

        template <std::uint32_t A, bool INV = false>
        MUX_CONSTEXPR void salsa(std::uint8_t* PTR, const std::uint32_t* KEY) {
            block<2u, A, INV>(PTR, KEY);
        }

        template <std::uint32_t A, bool INV = false>
        MUX_CONSTEXPR void chacha(std::uint8_t* PTR, const std::uint32_t* KEY) {
            block<4u, A, INV>(PTR, KEY);
        }

        constexpr std::uint8_t crc(std::uint8_t C) {
            std::uint32_t V = C;
            for (int I = 0; I < 8; ++I)
                V = (V >> 1) ^ (0xEDB88320u & static_cast<std::uint32_t>(0u - (V & 1u)));
            return static_cast<std::uint8_t>(V);
        }

        constexpr std::uint8_t crcbit(int BIT) {
            for (int X = 0; X != 256; ++X)
                if (crc(static_cast<std::uint8_t>(X)) == (1u << BIT))
                    return static_cast<std::uint8_t>(X);
            return 0;
        }

        constexpr std::uint8_t crcinv(std::uint8_t X) {
            std::uint8_t R = 0;
            for (int BIT = 0; BIT != 8; ++BIT)
                if ((X >> BIT) & 1u) R = static_cast<std::uint8_t>(R ^ crcbit(BIT));
            return R;
        }

        template <std::uint32_t A, bool INV = false>
        MUX_CONSTEXPR void stream(std::uint8_t* PTR, std::uint32_t LEN, const std::uint32_t* KEY) {
            std::uint8_t C = 0xFFu;
            for (std::uint32_t I = 0; I != LEN; ++I) {
                const std::uint8_t KEYB = static_cast<std::uint8_t>(KEY[I & 3u]);
                MUX_IF_CONSTEXPR(INV) {
                    const std::uint8_t C1 = static_cast<std::uint8_t>(PTR[I] ^ KEYB);
                    PTR[I] = static_cast<std::uint8_t>(crcinv(C1) ^ C);
                    C = C1;
                }
else {
    C = crc(static_cast<std::uint8_t>(C ^ PTR[I]));
    PTR[I] = static_cast<std::uint8_t>(C ^ KEYB);
                }
            }
        }

        template <std::uint32_t A>
        MUX_CONSTEXPR void rc4(std::uint8_t* PTR, std::uint32_t LEN, const std::uint32_t* KEY) {
            if (LEN == 0) return;
            std::uint8_t S[256];
            for (std::uint32_t I = 0; I < 256; ++I) S[I] = static_cast<std::uint8_t>(I);
            std::uint32_t J = 0;
            for (std::uint32_t I = 0; I < 256; ++I) {
                J = (J + S[I] + ((KEY[I & 3u] >> ((I & 7u) * 4u)) & 0xFFu)) & 0xFFu;
                const std::uint8_t T = S[I];
                S[I] = S[J];
                S[J] = T;
            }
            std::uint32_t IA = 0, IB = 0;
            for (std::uint32_t I = 0; I != LEN; ++I) {
                IA = (IA + 1u) & 0xFFu;
                IB = (IB + S[IA]) & 0xFFu;
                const std::uint8_t T = S[IA];
                S[IA] = S[IB];
                S[IB] = T;
                PTR[I] ^= S[(S[IA] + S[IB]) & 0xFFu];
            }
        }

        template <std::uint32_t W, std::uint32_t A, bool INV = false>
        struct cipher {
            static MUX_CONSTEXPR void apply(std::uint8_t* PTR, const std::uint32_t* KEY) {
                MUX_IF_CONSTEXPR(W == 1u) {
                    xtea<A, INV>(PTR, KEY);
                }
else MUX_IF_CONSTEXPR(W == 2u) {
    salsa<A, INV>(PTR, KEY);
                }
                else MUX_IF_CONSTEXPR(W == 4u) {
                    chacha<A, INV>(PTR, KEY);
                    }
                else {
                        stream<A, INV>(PTR, 8u, KEY);
                        }
            }
        };

        template <std::uint32_t W, std::uint32_t A, bool ENC>
        struct driver {
            static MUX_CONSTEXPR void run(std::uint8_t* PTR, std::uint32_t LEN, std::uint32_t* KEY) {
                const std::uint32_t TOTAL = LEN & ~std::uint32_t(7);
                std::uint32_t INDEX = 0;
                for (std::uint32_t OFF = 0; OFF != TOTAL; OFF += 8) {
                    tweak<W, A>(KEY, INDEX);
                    cipher<W, A, !ENC>::apply(PTR + OFF, KEY);
                    ++INDEX;
                }
                std::uint32_t T[4];
                for (std::uint32_t I = 0; I < 4; ++I) T[I] = KEY[I];
                tweak<W, A>(T, 0xFFFFFFFFu);
                T[0] ^= INDEX * meta<W, A>::DELTA;
                for (std::uint32_t OFF = TOTAL; OFF != LEN; ++OFF) {
                    const std::uint32_t SLOT = (OFF >> 2) & 3u;
                    const std::uint32_t SHIFT = (OFF & 3u) * 8u;
                    T[SLOT] ^= (OFF * 0x85EBCA6Bu) + SHIFT;
                    PTR[OFF] ^= static_cast<std::uint8_t>(T[SLOT] >> SHIFT);
                }
                for (std::uint32_t I = 0; I < 4; ++I) T[I] = 0;
            }
        };

        template <std::uint32_t A, bool ENC>
        struct driver<0u, A, ENC> {
            static MUX_CONSTEXPR void run(std::uint8_t* PTR, std::uint32_t LEN, std::uint32_t* KEY) {
                rc4<A>(PTR, LEN, KEY);
            }
        };

        template <std::uint32_t W, std::uint32_t A, bool ENC>
        MUX_CONSTEXPR void crypt(std::uint8_t* PTR, std::uint32_t LEN, const void* SELF) {
            std::uint32_t KEY[4];
            keys<W, A>(KEY, SELF);
            driver<W, A, ENC>::run(PTR, LEN, KEY);
            KEY[0] = KEY[1] = KEY[2] = KEY[3] = 0;
        }

        template <std::uint32_t W, std::uint32_t A, bool ENC>
        MUX_CONSTEXPR void seed(std::uint8_t* PTR, std::uint32_t LEN, std::uint32_t SEED) {
            std::uint32_t KEY[4];
            const std::uint64_t IDENTITY = (static_cast<std::uint64_t>(W) << 32) |
                (static_cast<std::uint64_t>(A) << 8) |
                meta<W, A>::ROUNDS;
            std::uint64_t MIXED = mix(mix2(SEED) ^ IDENTITY);
            KEY[0] = 0x6B206574u ^ meta<W, A>::S3^ static_cast<std::uint32_t>(MIXED);
            KEY[1] = 0x79622D32u ^ meta<W, A>::S2^ static_cast<std::uint32_t>(MIXED >> 32);
            MIXED = mix(MIXED + 0xD1B54A32D192ED03ull);
            KEY[2] = 0x3320646Eu ^ meta<W, A>::S1^ static_cast<std::uint32_t>(MIXED);
            KEY[3] = 0x61707865u ^ meta<W, A>::S0^ static_cast<std::uint32_t>(MIXED >> 32);
            driver<W, A, ENC>::run(PTR, LEN, KEY);
            KEY[0] = KEY[1] = KEY[2] = KEY[3] = 0;
        }

        using wipe_fn = void (*)(void*);

        struct reg_entry {
            void* OBJECT;
            wipe_fn WIPE;
        };

        inline std::vector<reg_entry>& registry() {
            static std::vector<reg_entry> REG;
            return REG;
        }

        inline std::mutex& lock() {
            static std::mutex MUTEX;
            return MUTEX;
        }

        inline void reg(void* OBJECT, wipe_fn WIPE) {
            const std::lock_guard<std::mutex> GUARD(lock());
            registry().push_back(reg_entry{ OBJECT, WIPE });
        }

        inline void unreg(const void* OBJECT) {
            const std::lock_guard<std::mutex> GUARD(lock());
            std::vector<reg_entry>& REG = registry();
            for (std::size_t I = REG.size(); I-- != 0;)
                if (REG[I].OBJECT == OBJECT) {
                    REG[I] = REG.back();
                    REG.pop_back();
                    return;
                }
        }

        template <class T>
        inline void wipe(void* OBJECT) {
            static_cast<T*>(OBJECT)->wipe();
        }

        inline std::size_t count() {
            const std::lock_guard<std::mutex> GUARD(lock());
            return registry().size();
        }

        template <typename C, std::size_t N>
        constexpr std::size_t len(const C(&)[N]) {
            return N;
        }

        template <typename C, std::size_t N, std::uint32_t W, std::uint32_t A>
        struct blob {
            static_assert(N > 0, "mux threw a runtime error! string literal must contain at least a NUL");
            using char_type = C;
            static MUX_CONSTEXPR std::uint32_t LENGTH = static_cast<std::uint32_t>(N);
            static MUX_CONSTEXPR std::size_t CAPACITY = N;
            static MUX_CONSTEXPR std::uint32_t BYTES = static_cast<std::uint32_t>(N * sizeof(C));
            static MUX_CONSTEXPR std::uint32_t WIDTH = W;
            static MUX_CONSTEXPR std::uint32_t ABI = A;

            C PAYLOAD[N];
            std::uint8_t STATE;

            blob(const blob&) = delete;
            blob& operator=(const blob&) = delete;

            blob(const C(&SRC)[N]) : PAYLOAD{}, STATE(0) {
                for (std::size_t I = 0; I != N; ++I)
                    PAYLOAD[I] = SRC[I];
                crypt<W, A, true>(reinterpret_cast<std::uint8_t*>(PAYLOAD), BYTES, PAYLOAD);
                reg(PAYLOAD, &::mux::detail::wipe<blob>);
            }

            ~blob() {
                destroy();
                unreg(PAYLOAD);
            }

            void wipe() {
                if (!STATE) return;
                crypt<W, A, true>(reinterpret_cast<std::uint8_t*>(PAYLOAD), BYTES, PAYLOAD);
                STATE = 0;
            }

            void destroy() {
                if (STATE) {
                    crypt<W, A, true>(reinterpret_cast<std::uint8_t*>(PAYLOAD), BYTES, PAYLOAD);
                }
                for (std::size_t I = 0; I != N; ++I) PAYLOAD[I] = C();
                STATE = 2;
            }

            void arm() {
                if (STATE != 0) return;
                crypt<W, A, false>(reinterpret_cast<std::uint8_t*>(PAYLOAD), BYTES, PAYLOAD);
                STATE = 1;
            }

            const C* c_str() const { return PAYLOAD; }
            const C* data() const { return PAYLOAD; }
            std::size_t size() const { return static_cast<std::size_t>(LENGTH); }
            std::basic_string<C> str() const { return std::basic_string<C>(PAYLOAD); }
        };

        template <std::uint32_t W, std::uint32_t A, std::uint32_t CAP>
        struct rt {
            using char_type = char;
            static MUX_CONSTEXPR std::uint32_t CAPACITY = CAP;

            std::uint32_t LENGTH;
            alignas(2) mutable std::uint8_t PAYLOAD[CAP];
            mutable std::uint8_t STATE;

            rt() : LENGTH(0), PAYLOAD{}, STATE(0) {}

            void wipe() {
                std::memset(PAYLOAD, 0, sizeof(PAYLOAD));
                LENGTH = 0;
                STATE = 0;
            }

            void set(const char* SRC, std::uint32_t LEN) {
                wipe();
                if (LEN > CAPACITY - 1u) LEN = CAPACITY - 1u;
                for (std::uint32_t I = 0; I < LEN; ++I) PAYLOAD[I] = static_cast<std::uint8_t>(SRC[I]);
                LENGTH = LEN;
                crypt<W, A, true>(PAYLOAD, LEN, PAYLOAD);
                STATE = 0;
            }

            void arm() const {
                if (STATE) return;
                if (LENGTH) crypt<W, A, false>(PAYLOAD, LENGTH, PAYLOAD);
                STATE = 1;
            }

            const char* c_str() const {
                arm();
                PAYLOAD[LENGTH] = '\0';
                return reinterpret_cast<const char*>(PAYLOAD);
            }
            std::size_t size() const { return LENGTH; }
        };

        template <typename C, std::size_t N>
        constexpr std::size_t count_of(const C(&)[N]) { return N; }

        template <std::uint32_t W, std::uint32_t A, bool ENC>
        MUX_CONSTEXPR void crypt_seed(std::uint8_t* PTR, std::uint32_t LEN, std::uint32_t SEED) {
            seed<W, A, ENC>(PTR, LEN, SEED);
        }

        template <std::uint32_t W, std::uint32_t A, std::uint32_t CAP>
        using runtime_blob = rt<W, A, CAP>;

    }

    MUX_NODISCARD inline std::size_t wipe_all() {
        std::vector<detail::reg_entry> SNAP;
        {
            std::lock_guard<std::mutex> GUARD(detail::lock());
            SNAP = detail::registry();
        }
        const std::size_t N = SNAP.size();
        for (std::size_t I = 0; I < N; ++I) {
            SNAP[I].WIPE(SNAP[I].OBJECT);
        }
        return N;
    }

    template <class B>
    inline void arm(B& BLOB) {
        BLOB.arm();
    }

    template <class B>
    MUX_NODISCARD inline const typename B::char_type* get(B& BLOB) {
        BLOB.arm();
        return BLOB.c_str();
    }

    template <class B>
    MUX_NODISCARD inline std::basic_string<typename B::char_type> to_string(B& BLOB) {
        return std::basic_string<typename B::char_type>(get(BLOB));
    }

    template <class B>
    MUX_NODISCARD inline std::basic_string<typename B::char_type> get_copy(B& BLOB) {
        return to_string(BLOB);
    }

    template <typename C, std::size_t CAP>
    class tmp {
    public:
        tmp() : LENGTH(0) { wipe(); }
        explicit tmp(const C* SRC) : LENGTH(0) { copy(SRC); }
        tmp(const tmp&) = delete;
        tmp& operator=(const tmp&) = delete;
        tmp(tmp&& OTHER) noexcept : LENGTH(OTHER.LENGTH) {
            std::memmove(DATA, OTHER.DATA, (OTHER.LENGTH + 1) * sizeof(C));
            OTHER.wipe();
        }
        tmp& operator=(tmp&& OTHER) noexcept {
            if (this != &OTHER) {
                std::memmove(DATA, OTHER.DATA, (OTHER.LENGTH + 1) * sizeof(C));
                LENGTH = OTHER.LENGTH;
                OTHER.wipe();
            }
            return *this;
        }
        ~tmp() { wipe(); }

        void copy(const C* SRC) {
            wipe();
            std::size_t N2 = 0;
            while (N2 + 1 < CAP && SRC[N2]) ++N2;
            for (std::size_t I = 0; I < N2; ++I) DATA[I] = SRC[I];
            DATA[N2] = C();
            LENGTH = static_cast<std::size_t>(N2);
        }
        void wipe() {
            volatile C* PTR = const_cast<volatile C*>(DATA);
            for (std::size_t I = 0; I < CAP; ++I) PTR[I] = C();
            LENGTH = 0;
        }
        const C* c_str() const { return DATA; }
        std::size_t size() const { return LENGTH; }
        operator const C* () const { return DATA; }

    private:
        C DATA[CAP];
        std::size_t LENGTH;
    };

    template <class B>
    MUX_NODISCARD inline tmp<typename B::char_type, B::CAPACITY> snap(B& BLOB) {
        BLOB.arm();
        return tmp<typename B::char_type, B::CAPACITY>(BLOB.c_str());
    }

}

#define MUX_S(STR)                                                              \
    ([]() -> const char* {                                                     \
        static ::mux::detail::blob<char, ::mux::detail::len(STR), 2u, 1u>       \
            MUX_UNIQUE(_mux_s){STR};                                            \
        return ::mux::get(MUX_UNIQUE(_mux_s));                                 \
    }())

#define MUX_S2(STR, WIDTH, ABI)                                                 \
    ([]() -> const char* {                                                     \
        static ::mux::detail::blob<char, ::mux::detail::len(STR), (WIDTH),      \
                                   (ABI)>                                       \
            MUX_UNIQUE(_mux_s){STR};                                            \
        return ::mux::get(MUX_UNIQUE(_mux_s));                                 \
    }())

#define MUX_SW(STR)                                                             \
    ([]() -> const wchar_t* {                                                   \
        static ::mux::detail::blob<wchar_t, ::mux::detail::len(STR), 2u,        \
                                   1u>                                          \
            MUX_UNIQUE(_mux_w){STR};                                            \
        return ::mux::get(MUX_UNIQUE(_mux_w));                                 \
    }())

#define MUX_SW2(STR, WIDTH, ABI)                                                \
    ([]() -> const wchar_t* {                                                   \
        static ::mux::detail::blob<wchar_t, ::mux::detail::len(STR),            \
                                   (WIDTH), (ABI)>                              \
            MUX_UNIQUE(_mux_w){STR};                                            \
        return ::mux::get(MUX_UNIQUE(_mux_w));                                 \
    }())

#define MUX_STR(STR) std::string(MUX_S(STR))
#define MUX_WSTR(STR) std::wstring(MUX_SW(STR))

#define MUX_TYPE(STR, WIDTH, ABI) \
    ::mux::detail::blob<char, ::mux::detail::len(STR), (WIDTH), (ABI)>
#define MUX_WTYPE(STR, WIDTH, ABI) \
    ::mux::detail::blob<wchar_t, ::mux::detail::len(STR), (WIDTH), (ABI)>

#define MUX_TMP(STR)                                                            \
    ([]() {                                                                     \
        static ::mux::detail::blob<char, ::mux::detail::len(STR), 2u, 1u>       \
            MUX_UNIQUE(_mux_t){STR};                                            \
        return ::mux::snap(MUX_UNIQUE(_mux_t));                                \
    }())

#define MUX_RC4_ 0u
#define MUX_XTEA 1u
#define MUX_SALSA 2u
#define MUX_CHACHA 4u
#define MUX_CRC 8u

#define MUX_RT(WIDTH, ABI, CAP) \
    ::mux::detail::rt<(WIDTH), (ABI), (CAP)>

#if MUX_WINDOWS

namespace mux {
    namespace win {

        enum : unsigned int { MUX_DEFAULT = 3u };

        namespace detail {

            constexpr std::uint32_t ror13(std::uint32_t V) { return (V >> 13) | (V << 19); }

            constexpr std::uint32_t hash(const char* S) {
                std::uint32_t H = 0;
                for (; *S; ++S) {
                    H = ror13(H);
                    const char C = (*S >= 'a' && *S <= 'z') ? static_cast<char>(*S - 32) : *S;
                    H += static_cast<std::uint32_t>(static_cast<unsigned char>(C));
                }
                return H;
            }

            struct unicode_string {
                std::uint16_t LENGTH;
                std::uint16_t MAXIMUM;
                std::uint32_t PAD;
                wchar_t* BUFFER;
            };

#if defined(_M_X64) || defined(__x86_64__)
#define MUX_X64 1
            constexpr int MUX_PEB_LDR = 0x18;
            constexpr int MUX_LDR_MEMORY_ORDER = 0x30;
            constexpr int MUX_ENTRY_MEMORY_ORDER = 0x20;
            constexpr int MUX_ENTRY_LOAD_ORDER = 0x10;
            constexpr int MUX_ENTRY_BASE_NAME = 0x58;
            constexpr int MUX_ENTRY_DLL_BASE = 0x30;
#else
#define MUX_X64 0
            constexpr int MUX_PEB_LDR = 0x0C;
            constexpr int MUX_LDR_MEMORY_ORDER = 0x14;
            constexpr int MUX_ENTRY_MEMORY_ORDER = 0x10;
            constexpr int MUX_ENTRY_LOAD_ORDER = 0x0C;
            constexpr int MUX_ENTRY_BASE_NAME = 0x2E;
            constexpr int MUX_ENTRY_DLL_BASE = 0x18;
#endif

            constexpr int MUX_MAX_DEPTH = 8;

            inline void* peb() {
#if MUX_X64
                return reinterpret_cast<void*>(__readgsqword(0x60));
#else
                return reinterpret_cast<void*>(__readfsdword(0x30));
#endif
            }

            inline bool ieq(const wchar_t* WIDE, const char* ASCII) {
                for (; *ASCII; ++ASCII, ++WIDE) {
                    wchar_t C = *WIDE;
                    if (C >= L'A' && C <= L'Z') C = static_cast<wchar_t>(C + 32);
                    char A2 = *ASCII;
                    if (A2 >= 'A' && A2 <= 'Z') A2 = static_cast<char>(A2 + 32);
                    if (C != static_cast<wchar_t>(static_cast<unsigned char>(A2))) return false;
                }
                return *WIDE == L'\0';
            }

            inline std::uint8_t* base(const char* NAME) {
                auto* P = static_cast<std::uint8_t*>(peb());
                if (!P) return nullptr;
                auto* LDR = P + MUX_PEB_LDR;
                auto* HEAD = reinterpret_cast<LIST_ENTRY*>(LDR + MUX_LDR_MEMORY_ORDER);
                for (LIST_ENTRY* LINK = HEAD->Flink; LINK && LINK != HEAD; LINK = LINK->Flink) {
                    auto* ENTRY = reinterpret_cast<std::uint8_t*>(LINK) - MUX_ENTRY_MEMORY_ORDER;
                    const auto* BASE_NAME =
                        reinterpret_cast<const unicode_string*>(ENTRY + MUX_ENTRY_BASE_NAME);
                    if (BASE_NAME->BUFFER && ieq(BASE_NAME->BUFFER, NAME))
                        return *reinterpret_cast<std::uint8_t**>(ENTRY + MUX_ENTRY_DLL_BASE);
                }
                return nullptr;
            }

            inline void* find(const char* MODULE, std::uint32_t NAME_HASH, int DEPTH = 0) {
                if (DEPTH >= MUX_MAX_DEPTH) return nullptr;
                std::uint8_t* BASE = base(MODULE);
                if (!BASE) return nullptr;

                auto* DOS = reinterpret_cast<IMAGE_DOS_HEADER*>(BASE);
                if (DOS->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;
                auto* NT = reinterpret_cast<IMAGE_NT_HEADERS*>(BASE + DOS->e_lfanew);
                if (NT->Signature != IMAGE_NT_SIGNATURE) return nullptr;

                const IMAGE_DATA_DIRECTORY& DIR =
                    NT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
                if (!DIR.VirtualAddress || !DIR.Size) return nullptr;

                auto* EXP = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(BASE + DIR.VirtualAddress);
                auto* NAMES = reinterpret_cast<std::uint32_t*>(BASE + EXP->AddressOfNames);
                auto* ORDS = reinterpret_cast<std::uint16_t*>(BASE + EXP->AddressOfNameOrdinals);
                auto* FUNCS = reinterpret_cast<std::uint32_t*>(BASE + EXP->AddressOfFunctions);

                for (std::uint32_t I = 0; I < EXP->NumberOfNames; ++I) {
                    const char* CANDIDATE = reinterpret_cast<const char*>(BASE + NAMES[I]);
                    if (hash(CANDIDATE) != NAME_HASH) continue;
                    const std::uint32_t RVA = FUNCS[ORDS[I]];

                    if (RVA >= DIR.VirtualAddress && RVA < DIR.VirtualAddress + DIR.Size) {
                        const char* FWD = reinterpret_cast<const char*>(BASE + RVA);
                        char MODULE_NAME[64] = {};
                        std::size_t J = 0;
                        for (; FWD[J] && FWD[J] != '.' && J + 1 < sizeof(MODULE_NAME); ++J)
                            MODULE_NAME[J] = FWD[J];
                        std::uint32_t H = 0;
                        for (const char* S = FWD + J + 1; *S; ++S) {
                            H = ror13(H);
                            const char C = (*S >= 'a' && *S <= 'z') ? static_cast<char>(*S - 32) : *S;
                            H += static_cast<std::uint32_t>(static_cast<unsigned char>(C));
                        }
                        if (J == 0 || H == 0) return nullptr;
                        return find(MODULE_NAME, H, DEPTH + 1);
                    }
                    return BASE + RVA;
                }
                return nullptr;
            }

            using syscall1_fn = std::intptr_t(__fastcall*)(void*);

            inline void scrub(void* ADDRESS, std::size_t BYTES) {
                volatile unsigned char* PTR = static_cast<volatile unsigned char*>(ADDRESS);
                for (std::size_t I = 0; I < BYTES; ++I) PTR[I] = 0;
            }

            template <std::size_t CAP>
            class wide {
            public:
                wide() { std::memset(BUF, 0, sizeof(BUF)); }
                wide(const wide&) = delete;
                wide& operator=(const wide&) = delete;
                ~wide() {
                    volatile wchar_t* PTR = const_cast<volatile wchar_t*>(BUF);
                    for (std::size_t I = 0; I < CAP; ++I) PTR[I] = L'\0';
                }

                template <typename... Chars>
                wide& put(Chars... CS) {
                    const int SWALLOW[] = { 0, (append(CS), 0)... };
                    (void)SWALLOW;
                    return *this;
                }
                const wchar_t* done() {
                    if (LEN < CAP) BUF[LEN] = L'\0';
                    return BUF;
                }

            private:
                void append(char C) { append(static_cast<wchar_t>(static_cast<unsigned char>(C))); }
                void append(char16_t C) { append(static_cast<wchar_t>(C)); }
                void append(wchar_t C) {
                    if (LEN + 1 < CAP) {
                        BUF[LEN] = C;
                        BUF[LEN + 1] = L'\0';
                        ++LEN;
                    }
                }
                void append(const char* S) {
                    for (; *S; ++S) append(*S);
                }
                void append(const wchar_t* S) {
                    for (; *S; ++S) append(*S);
                }

                wchar_t BUF[CAP];
                std::size_t LEN = 0;
            };

        }
    }

}

#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif