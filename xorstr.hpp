#pragma once
#include <iostream>
#define XORSTR_FORCEINLINE __forceinline

namespace XOR {
	constexpr auto time = __TIME__;
	constexpr auto seed = static_cast<int>(time[7]) + static_cast<int>(time[6]) * 10 + static_cast<int>(time[4]) * 60 +
						  static_cast<int>(time[3]) * 600 + static_cast<int>(time[1]) * 3600 +
						  static_cast<int>(time[0]) * 36000;

	template <int N> struct RandomGenerator {
	  private:
		static constexpr unsigned a = 16807;	  // 7^5
		static constexpr unsigned m = 2147483647; // 2^31 - 1
		static constexpr unsigned s = RandomGenerator<N - 1>::value;
		static constexpr unsigned lo = a * (s & 0xFFFF);			// Multiply lower 16 bits by 16807
		static constexpr unsigned hi = a * (s >> 16);				// Multiply higher 16 bits by 16807
		static constexpr unsigned lo2 = lo + ((hi & 0x7FFF) << 16); // Combine lower 15 bits of hi with lo's upper bits
		static constexpr unsigned hi2 = hi >> 15;					// Discard lower 15 bits of hi
		static constexpr unsigned lo3 = lo2 + hi;

	  public:
		static constexpr unsigned max = m;
		static constexpr unsigned value = lo3 > m ? lo3 - m : lo3;
	};

	template <> struct RandomGenerator<0> { static constexpr unsigned value = seed; };

	template <int N, int M> struct RandomInt { static constexpr auto value = RandomGenerator<N + 1>::value % M; };

	template <int N> struct RandomChar { static const char value = static_cast<char>(1 + RandomInt<N, 0x7F - 1>::value); };

	template <int N> struct RandomWchar {
		static const wchar_t value = static_cast<wchar_t>(1 + RandomInt<N, 0x7FFF - 1>::value);
	};

	template <size_t N, int K> struct XorString {
	  private:
		const char _key;
		char _encrypted[N + 1];
		constexpr char enc(char c) const { return c ^ _key; }
		char dec(char c) const { return c ^ _key; }

	  public:
		template <size_t... Is>
		constexpr XORSTR_FORCEINLINE XorString(const char *str, std::index_sequence<Is...>)
			: _key(RandomChar<K>::value), _encrypted{enc(str[Is])...} {}

		XORSTR_FORCEINLINE auto decrypt() {
			for (size_t i = 0; i < N; ++i) {
				_encrypted[i] = dec(_encrypted[i]);
			}
			_encrypted[N] = '\0';
			return _encrypted;
		}
	};

	template <size_t N, int K> struct XorWstring {
	  private:
		const wchar_t _key;
		wchar_t _encrypted[N + 1];
		constexpr wchar_t enc(wchar_t c) const { return c ^ _key; }
		wchar_t dec(wchar_t c) const { return c ^ _key; }

	  public:
		template <size_t... Is>
		constexpr XORSTR_FORCEINLINE XorWstring(const wchar_t *str, std::index_sequence<Is...>)
			: _key(RandomWchar<K>::value), _encrypted{enc(str[Is])...} {}

		XORSTR_FORCEINLINE auto decrypt() {
			for (size_t i = 0; i < N; ++i) {
				_encrypted[i] = dec(_encrypted[i]);
			}
			_encrypted[N] = '\0';
			return _encrypted;
		}
	};
} // namespace XOR

#define XORSTR_IMPL(str)                                                                                                       \
	::XOR::XorString<sizeof(str) - 1, __COUNTER__>((str), std::make_index_sequence<sizeof(str) - 1>()).decrypt()
#define XORSTR(str) (XORSTR_IMPL(str))
#define X(str) (XORSTR_IMPL(str))
#define WXORSTR_IMPL(str)                                                                                                      \
	::XOR::XorWstring<sizeof(L##str) - 1, __COUNTER__>((L##str),                                                               \
													   std::make_index_sequence<(sizeof(L##str) / sizeof(wchar_t)) - 1>())     \
		.decrypt()
#define WXORSTR(str) (WXORSTR_IMPL(str))