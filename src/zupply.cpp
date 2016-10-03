/********************************************************************//*
 *
 *   Script File: zupply.cpp
 *
 *   Description:
 *
 *   Implementations of core modules
 *
 *
 *   Author: Joshua Zhang
 *   DateTime since: June-2015
 *
 *   Copyright (c) <2015> <JOSHUA Z. ZHANG>
 *
 *	 Open source according to MIT License.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ***************************************************************************/

// Unless you are very confident, don't set either OS flag
#if defined(ZUPPLY_OS_UNIX) && defined(ZUPPLY_OS_WINDOWS)
#error Both Unix and Windows flags are set, which is not allowed!
#elif defined(ZUPPLY_OS_UNIX)
#pragma message Using defined Unix flag
#elif defined(ZUPPLY_OS_WINDOWS)
#pragma message Using defined Windows flag
#else
#if defined(unix)        || defined(__unix)      || defined(__unix__) \
	|| defined(linux) || defined(__linux) || defined(__linux__) \
	|| defined(sun) || defined(__sun) \
	|| defined(BSD) || defined(__OpenBSD__) || defined(__NetBSD__) \
	|| defined(__FreeBSD__) || defined (__DragonFly__) \
	|| defined(sgi) || defined(__sgi) \
	|| (defined(__MACOSX__) || defined(__APPLE__)) \
	|| defined(__CYGWIN__) || defined(__MINGW32__)
#define ZUPPLY_OS_UNIX	1	//!< Unix like OS(POSIX compliant)
#undef ZUPPLY_OS_WINDOWS
#elif defined(_MSC_VER) || defined(WIN32)  || defined(_WIN32) || defined(__WIN32__) \
	|| defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
#define ZUPPLY_OS_WINDOWS	1	//!< Microsoft Windows
#undef ZUPPLY_OS_UNIX
#else
#error Unable to support this unknown OS.
#endif
#endif

#if ZUPPLY_OS_WINDOWS
#if _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#endif
#include <windows.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <io.h>
#elif ZUPPLY_OS_UNIX
#include <unistd.h>	/* POSIX flags */
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ftw.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#endif

#include "zupply.hpp"

#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <functional>
#include <cctype>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <deque>
#include <cstdarg>

// UTF8CPP
#include <stdexcept>
#include <iterator>

namespace zz
{
	// \cond
	// This namespace scopes all thirdparty libraries
	namespace thirdparty
	{
		namespace utf8
		{
			// The typedefs for 8-bit, 16-bit and 32-bit unsigned integers
			// You may need to change them to match your system. 
			// These typedefs have the same names as ones from cstdint, or boost/cstdint
			typedef unsigned char   uint8_t;
			typedef unsigned short  uint16_t;
			typedef unsigned int    uint32_t;

			// Helper code - not intended to be directly called by the library users. May be changed at any time
			namespace internal
			{
				// Unicode constants
				// Leading (high) surrogates: 0xd800 - 0xdbff
				// Trailing (low) surrogates: 0xdc00 - 0xdfff
				const uint16_t LEAD_SURROGATE_MIN = 0xd800u;
				const uint16_t LEAD_SURROGATE_MAX = 0xdbffu;
				const uint16_t TRAIL_SURROGATE_MIN = 0xdc00u;
				const uint16_t TRAIL_SURROGATE_MAX = 0xdfffu;
				const uint16_t LEAD_OFFSET = LEAD_SURROGATE_MIN - (0x10000 >> 10);
				const uint32_t SURROGATE_OFFSET = 0x10000u - (LEAD_SURROGATE_MIN << 10) - TRAIL_SURROGATE_MIN;

				// Maximum valid value for a Unicode code point
				const uint32_t CODE_POINT_MAX = 0x0010ffffu;

				template<typename octet_type>
				inline uint8_t mask8(octet_type oc)
				{
					return static_cast<uint8_t>(0xff & oc);
				}
				template<typename u16_type>
				inline uint16_t mask16(u16_type oc)
				{
					return static_cast<uint16_t>(0xffff & oc);
				}
				template<typename octet_type>
				inline bool is_trail(octet_type oc)
				{
					return ((mask8(oc) >> 6) == 0x2);
				}

				template <typename u16>
				inline bool is_surrogate(u16 cp)
				{
					return (cp >= LEAD_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
				}

				template <typename u32>
				inline bool is_code_point_valid(u32 cp)
				{
					return (cp <= CODE_POINT_MAX && !is_surrogate(cp) && cp != 0xfffe && cp != 0xffff);
				}

				template <typename octet_iterator>
				inline typename std::iterator_traits<octet_iterator>::difference_type
					sequence_length(octet_iterator lead_it)
				{
						uint8_t lead = mask8(*lead_it);
						if (lead < 0x80)
							return 1;
						else if ((lead >> 5) == 0x6)
							return 2;
						else if ((lead >> 4) == 0xe)
							return 3;
						else if ((lead >> 3) == 0x1e)
							return 4;
						else
							return 0;
					}

				enum utf_error { OK, NOT_ENOUGH_ROOM, INVALID_LEAD, INCOMPLETE_SEQUENCE, OVERLONG_SEQUENCE, INVALID_CODE_POINT };

				template <typename octet_iterator>
				utf_error validate_next(octet_iterator& it, octet_iterator end, uint32_t* code_point)
				{
					uint32_t cp = mask8(*it);
					// Check the lead octet
					typedef typename std::iterator_traits<octet_iterator>::difference_type octet_difference_type;
					octet_difference_type length = sequence_length(it);

					// "Shortcut" for ASCII characters
					if (length == 1) {
						if (end - it > 0) {
							if (code_point)
								*code_point = cp;
							++it;
							return OK;
						}
						else
							return NOT_ENOUGH_ROOM;
					}

					// Do we have enough memory?     
					if (std::distance(it, end) < length)
						return NOT_ENOUGH_ROOM;

					// Check trail octets and calculate the code point
					switch (length) {
					case 0:
						return INVALID_LEAD;
						break;
					case 2:
						if (is_trail(*(++it))) {
							cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
						}
						else {
							--it;
							return INCOMPLETE_SEQUENCE;
						}
						break;
					case 3:
						if (is_trail(*(++it))) {
							cp = ((cp << 12) & 0xffff) + ((mask8(*it) << 6) & 0xfff);
							if (is_trail(*(++it))) {
								cp += (*it) & 0x3f;
							}
							else {
								std::advance(it, -2);
								return INCOMPLETE_SEQUENCE;
							}
						}
						else {
							--it;
							return INCOMPLETE_SEQUENCE;
						}
						break;
					case 4:
						if (is_trail(*(++it))) {
							cp = ((cp << 18) & 0x1fffff) + ((mask8(*it) << 12) & 0x3ffff);
							if (is_trail(*(++it))) {
								cp += (mask8(*it) << 6) & 0xfff;
								if (is_trail(*(++it))) {
									cp += (*it) & 0x3f;
								}
								else {
									std::advance(it, -3);
									return INCOMPLETE_SEQUENCE;
								}
							}
							else {
								std::advance(it, -2);
								return INCOMPLETE_SEQUENCE;
							}
						}
						else {
							--it;
							return INCOMPLETE_SEQUENCE;
						}
						break;
					}
					// Is the code point valid?
					if (!is_code_point_valid(cp)) {
						for (octet_difference_type i = 0; i < length - 1; ++i)
							--it;
						return INVALID_CODE_POINT;
					}

					if (code_point)
						*code_point = cp;

					if (cp < 0x80) {
						if (length != 1) {
							std::advance(it, -(length - 1));
							return OVERLONG_SEQUENCE;
						}
					}
					else if (cp < 0x800) {
						if (length != 2) {
							std::advance(it, -(length - 1));
							return OVERLONG_SEQUENCE;
						}
					}
					else if (cp < 0x10000) {
						if (length != 3) {
							std::advance(it, -(length - 1));
							return OVERLONG_SEQUENCE;
						}
					}

					++it;
					return OK;
				}

				template <typename octet_iterator>
				inline utf_error validate_next(octet_iterator& it, octet_iterator end) {
					return validate_next(it, end, 0);
				}

			} // namespace internal 

			/// The library API - functions intended to be called by the users

			// Byte order mark
			const uint8_t bom[] = { 0xef, 0xbb, 0xbf };

			template <typename octet_iterator>
			octet_iterator find_invalid(octet_iterator start, octet_iterator end)
			{
				octet_iterator result = start;
				while (result != end) {
					internal::utf_error err_code = internal::validate_next(result, end);
					if (err_code != internal::OK)
						return result;
				}
				return result;
			}

			template <typename octet_iterator>
			inline bool is_valid(octet_iterator start, octet_iterator end)
			{
				return (find_invalid(start, end) == end);
			}

			template <typename octet_iterator>
			inline bool is_bom(octet_iterator it)
			{
				return (
					(internal::mask8(*it++)) == bom[0] &&
					(internal::mask8(*it++)) == bom[1] &&
					(internal::mask8(*it)) == bom[2]
					);
			}

			// Exceptions that may be thrown from the library functions.
			class invalid_code_point : public std::exception {
				uint32_t cp;
			public:
				invalid_code_point(uint32_t _cp) : cp(_cp) {}
				virtual const char* what() const throw() { return "Invalid code point"; }
				uint32_t code_point() const { return cp; }
			};

			class invalid_utf8 : public std::exception {
				uint8_t u8;
			public:
				invalid_utf8(uint8_t u) : u8(u) {}
				virtual const char* what() const throw() { return "Invalid UTF-8"; }
				uint8_t utf8_octet() const { return u8; }
			};

			class invalid_utf16 : public std::exception {
				uint16_t u16;
			public:
				invalid_utf16(uint16_t u) : u16(u) {}
				virtual const char* what() const throw() { return "Invalid UTF-16"; }
				uint16_t utf16_word() const { return u16; }
			};

			class not_enough_room : public std::exception {
			public:
				virtual const char* what() const throw() { return "Not enough space"; }
			};

			/// The library API - functions intended to be called by the users

			template <typename octet_iterator, typename output_iterator>
			output_iterator replace_invalid(octet_iterator start, octet_iterator end, output_iterator out, uint32_t replacement)
			{
				while (start != end) {
					octet_iterator sequence_start = start;
					internal::utf_error err_code = internal::validate_next(start, end);
					switch (err_code) {
					case internal::OK:
						for (octet_iterator it = sequence_start; it != start; ++it)
							*out++ = *it;
						break;
					case internal::NOT_ENOUGH_ROOM:
						throw not_enough_room();
					case internal::INVALID_LEAD:
						append(replacement, out);
						++start;
						break;
					case internal::INCOMPLETE_SEQUENCE:
					case internal::OVERLONG_SEQUENCE:
					case internal::INVALID_CODE_POINT:
						append(replacement, out);
						++start;
						// just one replacement mark for the sequence
						while (internal::is_trail(*start) && start != end)
							++start;
						break;
					}
				}
				return out;
			}

			template <typename octet_iterator, typename output_iterator>
			inline output_iterator replace_invalid(octet_iterator start, octet_iterator end, output_iterator out)
			{
				static const uint32_t replacement_marker = internal::mask16(0xfffd);
				return replace_invalid(start, end, out, replacement_marker);
			}

			template <typename octet_iterator>
			octet_iterator append(uint32_t cp, octet_iterator result)
			{
				if (!internal::is_code_point_valid(cp))
					throw invalid_code_point(cp);

				if (cp < 0x80)                        // one octet
					*(result++) = static_cast<uint8_t>(cp);
				else if (cp < 0x800) {                // two octets
					*(result++) = static_cast<uint8_t>((cp >> 6) | 0xc0);
					*(result++) = static_cast<uint8_t>((cp & 0x3f) | 0x80);
				}
				else if (cp < 0x10000) {              // three octets
					*(result++) = static_cast<uint8_t>((cp >> 12) | 0xe0);
					*(result++) = static_cast<uint8_t>(((cp >> 6) & 0x3f) | 0x80);
					*(result++) = static_cast<uint8_t>((cp & 0x3f) | 0x80);
				}
				else if (cp <= internal::CODE_POINT_MAX) {      // four octets
					*(result++) = static_cast<uint8_t>((cp >> 18) | 0xf0);
					*(result++) = static_cast<uint8_t>(((cp >> 12) & 0x3f) | 0x80);
					*(result++) = static_cast<uint8_t>(((cp >> 6) & 0x3f) | 0x80);
					*(result++) = static_cast<uint8_t>((cp & 0x3f) | 0x80);
				}
				else
					throw invalid_code_point(cp);

				return result;
			}

			template <typename octet_iterator>
			uint32_t next(octet_iterator& it, octet_iterator end)
			{
				uint32_t cp = 0;
				internal::utf_error err_code = internal::validate_next(it, end, &cp);
				switch (err_code) {
				case internal::OK:
					break;
				case internal::NOT_ENOUGH_ROOM:
					throw not_enough_room();
				case internal::INVALID_LEAD:
				case internal::INCOMPLETE_SEQUENCE:
				case internal::OVERLONG_SEQUENCE:
					throw invalid_utf8(*it);
				case internal::INVALID_CODE_POINT:
					throw invalid_code_point(cp);
				}
				return cp;
			}

			template <typename octet_iterator>
			uint32_t peek_next(octet_iterator it, octet_iterator end)
			{
				return next(it, end);
			}

			template <typename octet_iterator>
			uint32_t prior(octet_iterator& it, octet_iterator start)
			{
				octet_iterator end = it;
				while (internal::is_trail(*(--it)))
				if (it < start)
					throw invalid_utf8(*it); // error - no lead byte in the sequence
				octet_iterator temp = it;
				return next(temp, end);
			}

			/// Deprecated in versions that include "prior"
			template <typename octet_iterator>
			uint32_t previous(octet_iterator& it, octet_iterator pass_start)
			{
				octet_iterator end = it;
				while (internal::is_trail(*(--it)))
				if (it == pass_start)
					throw invalid_utf8(*it); // error - no lead byte in the sequence
				octet_iterator temp = it;
				return next(temp, end);
			}

			template <typename octet_iterator, typename distance_type>
			void advance(octet_iterator& it, distance_type n, octet_iterator end)
			{
				for (distance_type i = 0; i < n; ++i)
					next(it, end);
			}

			template <typename octet_iterator>
			typename std::iterator_traits<octet_iterator>::difference_type
				distance(octet_iterator first, octet_iterator last)
			{
					typename std::iterator_traits<octet_iterator>::difference_type dist;
					for (dist = 0; first < last; ++dist)
						next(first, last);
					return dist;
				}

			template <typename u16bit_iterator, typename octet_iterator>
			octet_iterator utf16to8(u16bit_iterator start, u16bit_iterator end, octet_iterator result)
			{
				while (start != end) {
					uint32_t cp = internal::mask16(*start++);
					// Take care of surrogate pairs first
					if (internal::is_surrogate(cp)) {
						if (start != end) {
							uint32_t trail_surrogate = internal::mask16(*start++);
							if (trail_surrogate >= internal::TRAIL_SURROGATE_MIN && trail_surrogate <= internal::TRAIL_SURROGATE_MAX)
								cp = (cp << 10) + trail_surrogate + internal::SURROGATE_OFFSET;
							else
								throw invalid_utf16(static_cast<uint16_t>(trail_surrogate));
						}
						else
							throw invalid_utf16(static_cast<uint16_t>(*start));

					}
					result = append(cp, result);
				}
				return result;
			}

			template <typename u16bit_iterator, typename octet_iterator>
			u16bit_iterator utf8to16(octet_iterator start, octet_iterator end, u16bit_iterator result)
			{
				while (start != end) {
					uint32_t cp = next(start, end);
					if (cp > 0xffff) { //make a surrogate pair
						*result++ = static_cast<uint16_t>((cp >> 10) + internal::LEAD_OFFSET);
						*result++ = static_cast<uint16_t>((cp & 0x3ff) + internal::TRAIL_SURROGATE_MIN);
					}
					else
						*result++ = static_cast<uint16_t>(cp);
				}
				return result;
			}

			template <typename octet_iterator, typename u32bit_iterator>
			octet_iterator utf32to8(u32bit_iterator start, u32bit_iterator end, octet_iterator result)
			{
				while (start != end)
					result = append(*(start++), result);

				return result;
			}

			template <typename octet_iterator, typename u32bit_iterator>
			u32bit_iterator utf8to32(octet_iterator start, octet_iterator end, u32bit_iterator result)
			{
				while (start < end)
					(*result++) = next(start, end);

				return result;
			}

			// The iterator class
			template <typename octet_iterator>
			class iterator : public std::iterator <std::bidirectional_iterator_tag, uint32_t> {
				octet_iterator it;
				octet_iterator range_start;
				octet_iterator range_end;
			public:
				iterator() {}
				explicit iterator(const octet_iterator& octet_it,
					const octet_iterator& _range_start,
					const octet_iterator& _range_end) :
					it(octet_it), range_start(_range_start), range_end(_range_end)
				{
					if (it < range_start || it > range_end)
						throw std::out_of_range("Invalid utf-8 iterator position");
				}
				// the default "big three" are OK
				octet_iterator base() const { return it; }
				uint32_t operator * () const
				{
					octet_iterator temp = it;
					return next(temp, range_end);
				}
				bool operator == (const iterator& rhs) const
				{
					if (range_start != rhs.range_start || range_end != rhs.range_end)
						throw std::logic_error("Comparing utf-8 iterators defined with different ranges");
					return (it == rhs.it);
				}
				bool operator != (const iterator& rhs) const
				{
					return !(operator == (rhs));
				}
				iterator& operator ++ ()
				{
					next(it, range_end);
					return *this;
				}
				iterator operator ++ (int)
				{
					iterator temp = *this;
					next(it, range_end);
					return temp;
				}
				iterator& operator -- ()
				{
					prior(it, range_start);
					return *this;
				}
				iterator operator -- (int)
				{
					iterator temp = *this;
					prior(it, range_start);
					return temp;
				}
			}; // class iterator

			namespace unchecked
			{
				template <typename octet_iterator>
				octet_iterator append(uint32_t cp, octet_iterator result)
				{
					if (cp < 0x80)                        // one octet
						*(result++) = static_cast<char const>(cp);
					else if (cp < 0x800) {                // two octets
						*(result++) = static_cast<char const>((cp >> 6) | 0xc0);
						*(result++) = static_cast<char const>((cp & 0x3f) | 0x80);
					}
					else if (cp < 0x10000) {              // three octets
						*(result++) = static_cast<char const>((cp >> 12) | 0xe0);
						*(result++) = static_cast<char const>(((cp >> 6) & 0x3f) | 0x80);
						*(result++) = static_cast<char const>((cp & 0x3f) | 0x80);
					}
					else {                                // four octets
						*(result++) = static_cast<char const>((cp >> 18) | 0xf0);
						*(result++) = static_cast<char const>(((cp >> 12) & 0x3f) | 0x80);
						*(result++) = static_cast<char const>(((cp >> 6) & 0x3f) | 0x80);
						*(result++) = static_cast<char const>((cp & 0x3f) | 0x80);
					}
					return result;
				}

				template <typename octet_iterator>
				uint32_t next(octet_iterator& it)
				{
					uint32_t cp = internal::mask8(*it);
					typename std::iterator_traits<octet_iterator>::difference_type length = utf8::internal::sequence_length(it);
					switch (length) {
					case 1:
						break;
					case 2:
						it++;
						cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
						break;
					case 3:
						++it;
						cp = ((cp << 12) & 0xffff) + ((internal::mask8(*it) << 6) & 0xfff);
						++it;
						cp += (*it) & 0x3f;
						break;
					case 4:
						++it;
						cp = ((cp << 18) & 0x1fffff) + ((internal::mask8(*it) << 12) & 0x3ffff);
						++it;
						cp += (internal::mask8(*it) << 6) & 0xfff;
						++it;
						cp += (*it) & 0x3f;
						break;
					}
					++it;
					return cp;
				}

				template <typename octet_iterator>
				uint32_t peek_next(octet_iterator it)
				{
					return next(it);
				}

				template <typename octet_iterator>
				uint32_t prior(octet_iterator& it)
				{
					while (internal::is_trail(*(--it)));
					octet_iterator temp = it;
					return next(temp);
				}

				// Deprecated in versions that include prior, but only for the sake of consistency (see utf8::previous)
				template <typename octet_iterator>
				inline uint32_t previous(octet_iterator& it)
				{
					return prior(it);
				}

				template <typename octet_iterator, typename distance_type>
				void advance(octet_iterator& it, distance_type n)
				{
					for (distance_type i = 0; i < n; ++i)
						next(it);
				}

				template <typename octet_iterator>
				typename std::iterator_traits<octet_iterator>::difference_type
					distance(octet_iterator first, octet_iterator last)
				{
						typename std::iterator_traits<octet_iterator>::difference_type dist;
						for (dist = 0; first < last; ++dist)
							next(first);
						return dist;
					}

				template <typename u16bit_iterator, typename octet_iterator>
				octet_iterator utf16to8(u16bit_iterator start, u16bit_iterator end, octet_iterator result)
				{
					while (start != end) {
						uint32_t cp = internal::mask16(*start++);
						// Take care of surrogate pairs first
						if (internal::is_surrogate(cp)) {
							uint32_t trail_surrogate = internal::mask16(*start++);
							cp = (cp << 10) + trail_surrogate + internal::SURROGATE_OFFSET;
						}
						result = append(cp, result);
					}
					return result;
				}

				template <typename u16bit_iterator, typename octet_iterator>
				u16bit_iterator utf8to16(octet_iterator start, octet_iterator end, u16bit_iterator result)
				{
					while (start != end) {
						uint32_t cp = next(start);
						if (cp > 0xffff) { //make a surrogate pair
							*result++ = static_cast<uint16_t>((cp >> 10) + internal::LEAD_OFFSET);
							*result++ = static_cast<uint16_t>((cp & 0x3ff) + internal::TRAIL_SURROGATE_MIN);
						}
						else
							*result++ = static_cast<uint16_t>(cp);
					}
					return result;
				}

				template <typename octet_iterator, typename u32bit_iterator>
				octet_iterator utf32to8(u32bit_iterator start, u32bit_iterator end, octet_iterator result)
				{
					while (start != end)
						result = append(*(start++), result);

					return result;
				}

				template <typename octet_iterator, typename u32bit_iterator>
				u32bit_iterator utf8to32(octet_iterator start, octet_iterator end, u32bit_iterator result)
				{
					while (start < end)
						(*result++) = next(start);

					return result;
				}

				// The iterator class
				template <typename octet_iterator>
				class iterator : public std::iterator <std::bidirectional_iterator_tag, uint32_t> {
					octet_iterator it;
				public:
					iterator() {}
					explicit iterator(const octet_iterator& octet_it) : it(octet_it) {}
					// the default "big three" are OK
					octet_iterator base() const { return it; }
					uint32_t operator * () const
					{
						octet_iterator temp = it;
						return next(temp);
					}
					bool operator == (const iterator& rhs) const
					{
						return (it == rhs.it);
					}
					bool operator != (const iterator& rhs) const
					{
						return !(operator == (rhs));
					}
					iterator& operator ++ ()
					{
						std::advance(it, internal::sequence_length(it));
						return *this;
					}
					iterator operator ++ (int)
					{
						iterator temp = *this;
						std::advance(it, internal::sequence_length(it));
						return temp;
					}
					iterator& operator -- ()
					{
						prior(it);
						return *this;
					}
					iterator operator -- (int)
					{
						iterator temp = *this;
						prior(it);
						return temp;
					}
				}; // class iterator

			} // namespace utf8::unchecked
		} // namespace utf8

		namespace stbi
		{
			namespace decode
			{
				enum
				{
					STBI_default = 0, // only used for req_comp

					STBI_grey = 1,
					STBI_grey_alpha = 2,
					STBI_rgb = 3,
					STBI_rgb_alpha = 4
				};

				typedef unsigned char stbi_uc;
				//////////////////////////////////////////////////////////////////////////////
				//
				// PRIMARY API - works on images of any type
				//

				//
				// load image by filename, open file, or memory buffer
				//

				typedef struct
				{
					int(*read)  (void *user, char *data, int size);   // fill 'data' with 'size' bytes.  return number of bytes actually read
					void(*skip)  (void *user, int n);                 // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
					int(*eof)   (void *user);                       // returns nonzero if we are at end of file/data
				} stbi_io_callbacks;

				stbi_uc *stbi_load(char              const *filename, int *x, int *y, int *comp, int req_comp);
				stbi_uc *stbi_load_from_memory(stbi_uc           const *buffer, int len, int *x, int *y, int *comp, int req_comp);
				stbi_uc *stbi_load_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp);

#ifndef STBI_NO_STDIO
				stbi_uc *stbi_load_from_file(FILE *f, int *x, int *y, int *comp, int req_comp);
				// for stbi_load_from_file, file pointer is left pointing immediately after image
#endif

#ifndef STBI_NO_LINEAR
				float *stbi_loadf(char const *filename, int *x, int *y, int *comp, int req_comp);
				float *stbi_loadf_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
				float *stbi_loadf_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp);

#ifndef STBI_NO_STDIO
				float *stbi_loadf_from_file(FILE *f, int *x, int *y, int *comp, int req_comp);
#endif
#endif

#ifndef STBI_NO_HDR
				void   stbi_hdr_to_ldr_gamma(float gamma);
				void   stbi_hdr_to_ldr_scale(float scale);
#endif

#ifndef STBI_NO_LINEAR
				void   stbi_ldr_to_hdr_gamma(float gamma);
				void   stbi_ldr_to_hdr_scale(float scale);
#endif // STBI_NO_HDR

				// stbi_is_hdr is always defined, but always returns false if STBI_NO_HDR
				int    stbi_is_hdr_from_callbacks(stbi_io_callbacks const *clbk, void *user);
				int    stbi_is_hdr_from_memory(stbi_uc const *buffer, int len);
#ifndef STBI_NO_STDIO
				int      stbi_is_hdr(char const *filename);
				int      stbi_is_hdr_from_file(FILE *f);
#endif // STBI_NO_STDIO


				// get a VERY brief reason for failure
				// NOT THREADSAFE
				const char *stbi_failure_reason(void);

				// free the loaded image -- this is just free()
				void     stbi_image_free(void *retval_from_stbi_load);

				// get image dimensions & components without fully decoding
				int      stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);
				int      stbi_info_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp);

#ifndef STBI_NO_STDIO
				int      stbi_info(char const *filename, int *x, int *y, int *comp);
				int      stbi_info_from_file(FILE *f, int *x, int *y, int *comp);

#endif



				// for image formats that explicitly notate that they have premultiplied alpha,
				// we just return the colors as stored in the file. set this flag to force
				// unpremultiplication. results are undefined if the unpremultiply overflow.
				void stbi_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply);

				// indicate whether we should process iphone images back to canonical format,
				// or just pass them through "as-is"
				void stbi_convert_iphone_png_to_rgb(int flag_true_if_should_convert);

				// flip the image vertically, so the first pixel in the output array is the bottom left
				void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);

				// ZLIB client - used by PNG, available for other purposes

				char *stbi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen);
				char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header);
				char *stbi_zlib_decode_malloc(const char *buffer, int len, int *outlen);
				int   stbi_zlib_decode_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

				char *stbi_zlib_decode_noheader_malloc(const char *buffer, int len, int *outlen);
				int   stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

				// implementation
#if defined(STBI_ONLY_JPEG) || defined(STBI_ONLY_PNG) || defined(STBI_ONLY_BMP) \
	|| defined(STBI_ONLY_TGA) || defined(STBI_ONLY_GIF) || defined(STBI_ONLY_PSD) \
	|| defined(STBI_ONLY_HDR) || defined(STBI_ONLY_PIC) || defined(STBI_ONLY_PNM) \
	|| defined(STBI_ONLY_ZLIB)
#ifndef STBI_ONLY_JPEG
#define STBI_NO_JPEG
#endif
#ifndef STBI_ONLY_PNG
#define STBI_NO_PNG
#endif
#ifndef STBI_ONLY_BMP
#define STBI_NO_BMP
#endif
#ifndef STBI_ONLY_PSD
#define STBI_NO_PSD
#endif
#ifndef STBI_ONLY_TGA
#define STBI_NO_TGA
#endif
#ifndef STBI_ONLY_GIF
#define STBI_NO_GIF
#endif
#ifndef STBI_ONLY_HDR
#define STBI_NO_HDR
#endif
#ifndef STBI_ONLY_PIC
#define STBI_NO_PIC
#endif
#ifndef STBI_ONLY_PNM
#define STBI_NO_PNM
#endif
#endif

#if defined(STBI_NO_PNG) && !defined(STBI_SUPPORT_ZLIB) && !defined(STBI_NO_ZLIB)
#define STBI_NO_ZLIB
#endif


#include <stdarg.h>
#include <stddef.h> // ptrdiff_t on osx
#include <stdlib.h>
#include <string.h>

#if !defined(STBI_NO_LINEAR) || !defined(STBI_NO_HDR)
#include <math.h>  // ldexp
#endif

#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif

#ifndef STBI_ASSERT
#include <assert.h>
#define STBI_ASSERT(x) assert(x)
#endif


#ifndef _MSC_VER
#ifdef __cplusplus
#define stbi_inline inline
#else
#define stbi_inline
#endif
#else
#define stbi_inline __forceinline
#endif


#ifdef _MSC_VER
				typedef unsigned short stbi__uint16;
				typedef   signed short stbi__int16;
				typedef unsigned int   stbi__uint32;
				typedef   signed int   stbi__int32;
#else
#include <stdint.h>
				typedef uint16_t stbi__uint16;
				typedef int16_t  stbi__int16;
				typedef uint32_t stbi__uint32;
				typedef int32_t  stbi__int32;
#endif

				// should produce compiler error if size is wrong
				typedef unsigned char validate_uint32[sizeof(stbi__uint32) == 4 ? 1 : -1];

#ifdef _MSC_VER
#define STBI_NOTUSED(v)  (void)(v)
#else
#define STBI_NOTUSED(v)  (void)sizeof(v)
#endif

#ifdef _MSC_VER
#define STBI_HAS_LROTL
#endif

#ifdef STBI_HAS_LROTL
#define stbi_lrot(x,y)  _lrotl(x,y)
#else
#define stbi_lrot(x,y)  (((x) << (y)) | ((x) >> (32 - (y))))
#endif

#if defined(STBI_MALLOC) && defined(STBI_FREE) && defined(STBI_REALLOC)
				// ok
#elif !defined(STBI_MALLOC) && !defined(STBI_FREE) && !defined(STBI_REALLOC)
				// ok
#else
#error "Must define all or none of STBI_MALLOC, STBI_FREE, and STBI_REALLOC."
#endif

#ifndef STBI_MALLOC
#define STBI_MALLOC(sz)    malloc(sz)
#define STBI_REALLOC(p,sz) realloc(p,sz)
#define STBI_FREE(p)       free(p)
#endif

				// x86/x64 detection
#if defined(__x86_64__) || defined(_M_X64)
#define STBI__X64_TARGET
#elif defined(__i386) || defined(_M_IX86)
#define STBI__X86_TARGET
#endif

#if defined(__GNUC__) && (defined(STBI__X86_TARGET) || defined(STBI__X64_TARGET)) && !defined(__SSE2__) && !defined(STBI_NO_SIMD)
				// NOTE: not clear do we actually need this for the 64-bit path?
				// gcc doesn't support sse2 intrinsics unless you compile with -msse2,
				// (but compiling with -msse2 allows the compiler to use SSE2 everywhere;
				// this is just broken and gcc are jerks for not fixing it properly
				// http://www.virtualdub.org/blog/pivot/entry.php?id=363 )
#define STBI_NO_SIMD
#endif

#if defined(__MINGW32__) && defined(STBI__X86_TARGET) && !defined(STBI_MINGW_ENABLE_SSE2) && !defined(STBI_NO_SIMD)
				// Note that __MINGW32__ doesn't actually mean 32-bit, so we have to avoid STBI__X64_TARGET
				//
				// 32-bit MinGW wants ESP to be 16-byte aligned, but this is not in the
				// Windows ABI and VC++ as well as Windows DLLs don't maintain that invariant.
				// As a result, enabling SSE2 on 32-bit MinGW is dangerous when not
				// simultaneously enabling "-mstackrealign".
				//
				// See https://github.com/nothings/stb/issues/81 for more information.
				//
				// So default to no SSE2 on 32-bit MinGW. If you've read this far and added
				// -mstackrealign to your build settings, feel free to #define STBI_MINGW_ENABLE_SSE2.
#define STBI_NO_SIMD
#endif

#if !defined(STBI_NO_SIMD) && defined(STBI__X86_TARGET)
#define STBI_SSE2
#include <emmintrin.h>

#ifdef _MSC_VER

#if _MSC_VER >= 1400  // not VC6
#include <intrin.h> // __cpuid
				int stbi__cpuid3(void)
				{
					int info[4];
					__cpuid(info, 1);
					return info[3];
				}
#else
				int stbi__cpuid3(void)
				{
					int res;
					__asm {
						mov  eax, 1
							cpuid
							mov  res, edx
					}
					return res;
				}
#endif

#define STBI_SIMD_ALIGN(type, name) __declspec(align(16)) type name

				int stbi__sse2_available()
				{
					int info3 = stbi__cpuid3();
					return ((info3 >> 26) & 1) != 0;
				}
#else // assume GCC-style if not VC++
#define STBI_SIMD_ALIGN(type, name) type name __attribute__((aligned(16)))

				int stbi__sse2_available()
				{
#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 408 // GCC 4.8 or later
					// GCC 4.8+ has a nice way to do this
					return __builtin_cpu_supports("sse2");
#else
					// portable way to do this, preferably without using GCC inline ASM?
					// just bail for now.
					return 0;
#endif
				}
#endif
#endif

				// ARM NEON
#if defined(STBI_NO_SIMD) && defined(STBI_NEON)
#undef STBI_NEON
#endif

#ifdef STBI_NEON
#include <arm_neon.h>
				// assume GCC or Clang on ARM targets
#define STBI_SIMD_ALIGN(type, name) type name __attribute__((aligned(16)))
#endif

#ifndef STBI_SIMD_ALIGN
#define STBI_SIMD_ALIGN(type, name) type name
#endif

				///////////////////////////////////////////////
				//
				//  stbi__context struct and start_xxx functions

				// stbi__context structure is our basic context used by all images, so it
				// contains all the IO context, plus some basic image information
				typedef struct
				{
					stbi__uint32 img_x, img_y;
					int img_n, img_out_n;

					stbi_io_callbacks io;
					void *io_user_data;

					int read_from_callbacks;
					int buflen;
					stbi_uc buffer_start[128];

					stbi_uc *img_buffer, *img_buffer_end;
					stbi_uc *img_buffer_original, *img_buffer_original_end;
				} stbi__context;


				void stbi__refill_buffer(stbi__context *s);

				// initialize a memory-decode context
				void stbi__start_mem(stbi__context *s, stbi_uc const *buffer, int len)
				{
					s->io.read = NULL;
					s->read_from_callbacks = 0;
					s->img_buffer = s->img_buffer_original = (stbi_uc *)buffer;
					s->img_buffer_end = s->img_buffer_original_end = (stbi_uc *)buffer + len;
				}

				// initialize a callback-based context
				void stbi__start_callbacks(stbi__context *s, stbi_io_callbacks *c, void *user)
				{
					s->io = *c;
					s->io_user_data = user;
					s->buflen = sizeof(s->buffer_start);
					s->read_from_callbacks = 1;
					s->img_buffer_original = s->buffer_start;
					stbi__refill_buffer(s);
					s->img_buffer_original_end = s->img_buffer_end;
				}

#ifndef STBI_NO_STDIO

				int stbi__stdio_read(void *user, char *data, int size)
				{
					return (int)fread(data, 1, size, (FILE*)user);
				}

				void stbi__stdio_skip(void *user, int n)
				{
					fseek((FILE*)user, n, SEEK_CUR);
				}

				int stbi__stdio_eof(void *user)
				{
					return feof((FILE*)user);
				}

				stbi_io_callbacks stbi__stdio_callbacks =
				{
					stbi__stdio_read,
					stbi__stdio_skip,
					stbi__stdio_eof,
				};

				void stbi__start_file(stbi__context *s, FILE *f)
				{
					stbi__start_callbacks(s, &stbi__stdio_callbacks, (void *)f);
				}

				// void stop_file(stbi__context *s) { }

#endif // !STBI_NO_STDIO

				void stbi__rewind(stbi__context *s)
				{
					// conceptually rewind SHOULD rewind to the beginning of the stream,
					// but we just rewind to the beginning of the initial buffer, because
					// we only use it after doing 'test', which only ever looks at at most 92 bytes
					s->img_buffer = s->img_buffer_original;
					s->img_buffer_end = s->img_buffer_original_end;
				}

#ifndef STBI_NO_JPEG
				int      stbi__jpeg_test(stbi__context *s);
				stbi_uc *stbi__jpeg_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__jpeg_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PNG
				int      stbi__png_test(stbi__context *s);
				stbi_uc *stbi__png_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__png_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_BMP
				int      stbi__bmp_test(stbi__context *s);
				stbi_uc *stbi__bmp_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__bmp_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_TGA
				int      stbi__tga_test(stbi__context *s);
				stbi_uc *stbi__tga_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__tga_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PSD
				int      stbi__psd_test(stbi__context *s);
				stbi_uc *stbi__psd_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__psd_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_HDR
				int      stbi__hdr_test(stbi__context *s);
				float   *stbi__hdr_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__hdr_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PIC
				int      stbi__pic_test(stbi__context *s);
				stbi_uc *stbi__pic_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__pic_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_GIF
				int      stbi__gif_test(stbi__context *s);
				stbi_uc *stbi__gif_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__gif_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PNM
				int      stbi__pnm_test(stbi__context *s);
				stbi_uc *stbi__pnm_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__pnm_info(stbi__context *s, int *x, int *y, int *comp);
#endif

				// this is not threadsafe
				const char *stbi__g_failure_reason;

				const char *stbi_failure_reason(void)
				{
					return stbi__g_failure_reason;
				}

				int stbi__err(const char *str)
				{
					stbi__g_failure_reason = str;
					return 0;
				}

				void *stbi__malloc(size_t size)
				{
					return STBI_MALLOC(size);
				}

				// stbi__err - error
				// stbi__errpf - error returning pointer to float
				// stbi__errpuc - error returning pointer to unsigned char

#ifdef STBI_NO_FAILURE_STRINGS
#define stbi__err(x,y)  0
#elif defined(STBI_FAILURE_USERMSG)
#define stbi__err(x,y)  stbi__err(y)
#else
#define stbi__err(x,y)  stbi__err(x)
#endif

#define stbi__errpf(x,y)   ((float *)(size_t) (stbi__err(x,y)?NULL:NULL))
#define stbi__errpuc(x,y)  ((unsigned char *)(size_t) (stbi__err(x,y)?NULL:NULL))

				void stbi_image_free(void *retval_from_stbi_load)
				{
					STBI_FREE(retval_from_stbi_load);
				}

#ifndef STBI_NO_LINEAR
				float   *stbi__ldr_to_hdr(stbi_uc *data, int x, int y, int comp);
#endif

#ifndef STBI_NO_HDR
				stbi_uc *stbi__hdr_to_ldr(float   *data, int x, int y, int comp);
#endif

				int stbi__vertically_flip_on_load = 0;

				void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip)
				{
					stbi__vertically_flip_on_load = flag_true_if_should_flip;
				}

				unsigned char *stbi__load_main(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
#ifndef STBI_NO_JPEG
					if (stbi__jpeg_test(s)) return stbi__jpeg_load(s, x, y, comp, req_comp);
#endif
#ifndef STBI_NO_PNG
					if (stbi__png_test(s))  return stbi__png_load(s, x, y, comp, req_comp);
#endif
#ifndef STBI_NO_BMP
					if (stbi__bmp_test(s))  return stbi__bmp_load(s, x, y, comp, req_comp);
#endif
#ifndef STBI_NO_GIF
					if (stbi__gif_test(s))  return stbi__gif_load(s, x, y, comp, req_comp);
#endif
#ifndef STBI_NO_PSD
					if (stbi__psd_test(s))  return stbi__psd_load(s, x, y, comp, req_comp);
#endif
#ifndef STBI_NO_PIC
					if (stbi__pic_test(s))  return stbi__pic_load(s, x, y, comp, req_comp);
#endif
#ifndef STBI_NO_PNM
					if (stbi__pnm_test(s))  return stbi__pnm_load(s, x, y, comp, req_comp);
#endif

#ifndef STBI_NO_HDR
					if (stbi__hdr_test(s)) {
						float *hdr = stbi__hdr_load(s, x, y, comp, req_comp);
						return stbi__hdr_to_ldr(hdr, *x, *y, req_comp ? req_comp : *comp);
					}
#endif

#ifndef STBI_NO_TGA
					// test tga last because it's a crappy test!
					if (stbi__tga_test(s))
						return stbi__tga_load(s, x, y, comp, req_comp);
#endif

					return stbi__errpuc("unknown image type", "Image not of any known type, or corrupt");
				}

				unsigned char *stbi__load_flip(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					unsigned char *result = stbi__load_main(s, x, y, comp, req_comp);

					if (stbi__vertically_flip_on_load && result != NULL) {
						int w = *x, h = *y;
						int depth = req_comp ? req_comp : *comp;
						int row, col, z;
						stbi_uc temp;

						// @OPTIMIZE: use a bigger temp buffer and memcpy multiple pixels at once
						for (row = 0; row < (h >> 1); row++) {
							for (col = 0; col < w; col++) {
								for (z = 0; z < depth; z++) {
									temp = result[(row * w + col) * depth + z];
									result[(row * w + col) * depth + z] = result[((h - row - 1) * w + col) * depth + z];
									result[((h - row - 1) * w + col) * depth + z] = temp;
								}
							}
						}
					}

					return result;
				}

#ifndef STBI_NO_HDR
				void stbi__float_postprocess(float *result, int *x, int *y, int *comp, int req_comp)
				{
					if (stbi__vertically_flip_on_load && result != NULL) {
						int w = *x, h = *y;
						int depth = req_comp ? req_comp : *comp;
						int row, col, z;
						float temp;

						// @OPTIMIZE: use a bigger temp buffer and memcpy multiple pixels at once
						for (row = 0; row < (h >> 1); row++) {
							for (col = 0; col < w; col++) {
								for (z = 0; z < depth; z++) {
									temp = result[(row * w + col) * depth + z];
									result[(row * w + col) * depth + z] = result[((h - row - 1) * w + col) * depth + z];
									result[((h - row - 1) * w + col) * depth + z] = temp;
								}
							}
						}
					}
				}
#endif

#ifndef STBI_NO_STDIO

				FILE *stbi__fopen(char const *filename, char const *mode)
				{
					FILE *f;
#if defined(_MSC_VER) && _MSC_VER >= 1400
					if (0 != fopen_s(&f, filename, mode))
						f = 0;
#else
					f = fopen(filename, mode);
#endif
					return f;
				}


				stbi_uc *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp)
				{
					FILE *f = stbi__fopen(filename, "rb");
					unsigned char *result;
					if (!f) return stbi__errpuc("can't fopen", "Unable to open file");
					result = stbi_load_from_file(f, x, y, comp, req_comp);
					fclose(f);
					return result;
				}

				stbi_uc *stbi_load_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
				{
					unsigned char *result;
					stbi__context s;
					stbi__start_file(&s, f);
					result = stbi__load_flip(&s, x, y, comp, req_comp);
					if (result) {
						// need to 'unget' all the characters in the IO buffer
						fseek(f, -(int)(s.img_buffer_end - s.img_buffer), SEEK_CUR);
					}
					return result;
				}
#endif //!STBI_NO_STDIO

				stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp)
				{
					stbi__context s;
					stbi__start_mem(&s, buffer, len);
					return stbi__load_flip(&s, x, y, comp, req_comp);
				}

				stbi_uc *stbi_load_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp)
				{
					stbi__context s;
					stbi__start_callbacks(&s, (stbi_io_callbacks *)clbk, user);
					return stbi__load_flip(&s, x, y, comp, req_comp);
				}

#ifndef STBI_NO_LINEAR
				float *stbi__loadf_main(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					unsigned char *data;
#ifndef STBI_NO_HDR
					if (stbi__hdr_test(s)) {
						float *hdr_data = stbi__hdr_load(s, x, y, comp, req_comp);
						if (hdr_data)
							stbi__float_postprocess(hdr_data, x, y, comp, req_comp);
						return hdr_data;
					}
#endif
					data = stbi__load_flip(s, x, y, comp, req_comp);
					if (data)
						return stbi__ldr_to_hdr(data, *x, *y, req_comp ? req_comp : *comp);
					return stbi__errpf("unknown image type", "Image not of any known type, or corrupt");
				}

				float *stbi_loadf_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp)
				{
					stbi__context s;
					stbi__start_mem(&s, buffer, len);
					return stbi__loadf_main(&s, x, y, comp, req_comp);
				}

				float *stbi_loadf_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp)
				{
					stbi__context s;
					stbi__start_callbacks(&s, (stbi_io_callbacks *)clbk, user);
					return stbi__loadf_main(&s, x, y, comp, req_comp);
				}

#ifndef STBI_NO_STDIO
				float *stbi_loadf(char const *filename, int *x, int *y, int *comp, int req_comp)
				{
					float *result;
					FILE *f = stbi__fopen(filename, "rb");
					if (!f) return stbi__errpf("can't fopen", "Unable to open file");
					result = stbi_loadf_from_file(f, x, y, comp, req_comp);
					fclose(f);
					return result;
				}

				float *stbi_loadf_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
				{
					stbi__context s;
					stbi__start_file(&s, f);
					return stbi__loadf_main(&s, x, y, comp, req_comp);
				}
#endif // !STBI_NO_STDIO

#endif // !STBI_NO_LINEAR

				// these is-hdr-or-not is defined independent of whether STBI_NO_LINEAR is
				// defined, for API simplicity; if STBI_NO_LINEAR is defined, it always
				// reports false!

				int stbi_is_hdr_from_memory(stbi_uc const *buffer, int len)
				{
#ifndef STBI_NO_HDR
					stbi__context s;
					stbi__start_mem(&s, buffer, len);
					return stbi__hdr_test(&s);
#else
					STBI_NOTUSED(buffer);
					STBI_NOTUSED(len);
					return 0;
#endif
				}

#ifndef STBI_NO_STDIO
				int      stbi_is_hdr(char const *filename)
				{
					FILE *f = stbi__fopen(filename, "rb");
					int result = 0;
					if (f) {
						result = stbi_is_hdr_from_file(f);
						fclose(f);
					}
					return result;
				}

				int      stbi_is_hdr_from_file(FILE *f)
				{
#ifndef STBI_NO_HDR
					stbi__context s;
					stbi__start_file(&s, f);
					return stbi__hdr_test(&s);
#else
					STBI_NOTUSED(f);
					return 0;
#endif
				}
#endif // !STBI_NO_STDIO

				int      stbi_is_hdr_from_callbacks(stbi_io_callbacks const *clbk, void *user)
				{
#ifndef STBI_NO_HDR
					stbi__context s;
					stbi__start_callbacks(&s, (stbi_io_callbacks *)clbk, user);
					return stbi__hdr_test(&s);
#else
					STBI_NOTUSED(clbk);
					STBI_NOTUSED(user);
					return 0;
#endif
				}

				float stbi__h2l_gamma_i = 1.0f / 2.2f, stbi__h2l_scale_i = 1.0f;
				float stbi__l2h_gamma = 2.2f, stbi__l2h_scale = 1.0f;

#ifndef STBI_NO_LINEAR
				void   stbi_ldr_to_hdr_gamma(float gamma) { stbi__l2h_gamma = gamma; }
				void   stbi_ldr_to_hdr_scale(float scale) { stbi__l2h_scale = scale; }
#endif

				void   stbi_hdr_to_ldr_gamma(float gamma) { stbi__h2l_gamma_i = 1 / gamma; }
				void   stbi_hdr_to_ldr_scale(float scale) { stbi__h2l_scale_i = 1 / scale; }


				//////////////////////////////////////////////////////////////////////////////
				//
				// Common code used by all image loaders
				//

				enum
				{
					STBI__SCAN_load = 0,
					STBI__SCAN_type,
					STBI__SCAN_header
				};

				void stbi__refill_buffer(stbi__context *s)
				{
					int n = (s->io.read)(s->io_user_data, (char*)s->buffer_start, s->buflen);
					if (n == 0) {
						// at end of file, treat same as if from memory, but need to handle case
						// where s->img_buffer isn't pointing to safe memory, e.g. 0-byte file
						s->read_from_callbacks = 0;
						s->img_buffer = s->buffer_start;
						s->img_buffer_end = s->buffer_start + 1;
						*s->img_buffer = 0;
					}
					else {
						s->img_buffer = s->buffer_start;
						s->img_buffer_end = s->buffer_start + n;
					}
				}

				stbi_inline  stbi_uc stbi__get8(stbi__context *s)
				{
					if (s->img_buffer < s->img_buffer_end)
						return *s->img_buffer++;
					if (s->read_from_callbacks) {
						stbi__refill_buffer(s);
						return *s->img_buffer++;
					}
					return 0;
				}

				stbi_inline  int stbi__at_eof(stbi__context *s)
				{
					if (s->io.read) {
						if (!(s->io.eof)(s->io_user_data)) return 0;
						// if feof() is true, check if buffer = end
						// special case: we've only got the special 0 character at the end
						if (s->read_from_callbacks == 0) return 1;
					}

					return s->img_buffer >= s->img_buffer_end;
				}

				void stbi__skip(stbi__context *s, int n)
				{
					if (n < 0) {
						s->img_buffer = s->img_buffer_end;
						return;
					}
					if (s->io.read) {
						int blen = (int)(s->img_buffer_end - s->img_buffer);
						if (blen < n) {
							s->img_buffer = s->img_buffer_end;
							(s->io.skip)(s->io_user_data, n - blen);
							return;
						}
					}
					s->img_buffer += n;
				}

				int stbi__getn(stbi__context *s, stbi_uc *buffer, int n)
				{
					if (s->io.read) {
						int blen = (int)(s->img_buffer_end - s->img_buffer);
						if (blen < n) {
							int res, count;

							memcpy(buffer, s->img_buffer, blen);

							count = (s->io.read)(s->io_user_data, (char*)buffer + blen, n - blen);
							res = (count == (n - blen));
							s->img_buffer = s->img_buffer_end;
							return res;
						}
					}

					if (s->img_buffer + n <= s->img_buffer_end) {
						memcpy(buffer, s->img_buffer, n);
						s->img_buffer += n;
						return 1;
					}
					else
						return 0;
				}

				int stbi__get16be(stbi__context *s)
				{
					int z = stbi__get8(s);
					return (z << 8) + stbi__get8(s);
				}

				stbi__uint32 stbi__get32be(stbi__context *s)
				{
					stbi__uint32 z = stbi__get16be(s);
					return (z << 16) + stbi__get16be(s);
				}

#if defined(STBI_NO_BMP) && defined(STBI_NO_TGA) && defined(STBI_NO_GIF)
				// nothing
#else
				int stbi__get16le(stbi__context *s)
				{
					int z = stbi__get8(s);
					return z + (stbi__get8(s) << 8);
				}
#endif

#ifndef STBI_NO_BMP
				stbi__uint32 stbi__get32le(stbi__context *s)
				{
					stbi__uint32 z = stbi__get16le(s);
					return z + (stbi__get16le(s) << 16);
				}
#endif

#define STBI__BYTECAST(x)  ((stbi_uc) ((x) & 255))  // truncate int to byte without warnings


				//////////////////////////////////////////////////////////////////////////////
				//
				//  generic converter from built-in img_n to req_comp
				//    individual types do this automatically as much as possible (e.g. jpeg
				//    does all cases internally since it needs to colorspace convert anyway,
				//    and it never has alpha, so very few cases ). png can automatically
				//    interleave an alpha=255 channel, but falls back to this for other cases
				//
				//  assume data buffer is malloced, so malloc a new one and free that one
				//  only failure mode is malloc failing

				stbi_uc stbi__compute_y(int r, int g, int b)
				{
					return (stbi_uc)(((r * 77) + (g * 150) + (29 * b)) >> 8);
				}

				unsigned char *stbi__convert_format(unsigned char *data, int img_n, int req_comp, unsigned int x, unsigned int y)
				{
					int i, j;
					unsigned char *good;

					if (req_comp == img_n) return data;
					STBI_ASSERT(req_comp >= 1 && req_comp <= 4);

					good = (unsigned char *)stbi__malloc(req_comp * x * y);
					if (good == NULL) {
						STBI_FREE(data);
						return stbi__errpuc("outofmem", "Out of memory");
					}

					for (j = 0; j < (int)y; ++j) {
						unsigned char *src = data + j * x * img_n;
						unsigned char *dest = good + j * x * req_comp;

#define COMBO(a,b)  ((a)*8+(b))
#define CASE(a,b)   case COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
						// convert source image with img_n components to one with req_comp components;
						// avoid switch per pixel, so use switch per scanline and massive macros
						switch (COMBO(img_n, req_comp)) {
							CASE(1, 2) dest[0] = src[0], dest[1] = 255; break;
							CASE(1, 3) dest[0] = dest[1] = dest[2] = src[0]; break;
							CASE(1, 4) dest[0] = dest[1] = dest[2] = src[0], dest[3] = 255; break;
							CASE(2, 1) dest[0] = src[0]; break;
							CASE(2, 3) dest[0] = dest[1] = dest[2] = src[0]; break;
							CASE(2, 4) dest[0] = dest[1] = dest[2] = src[0], dest[3] = src[1]; break;
							CASE(3, 4) dest[0] = src[0], dest[1] = src[1], dest[2] = src[2], dest[3] = 255; break;
							CASE(3, 1) dest[0] = stbi__compute_y(src[0], src[1], src[2]); break;
							CASE(3, 2) dest[0] = stbi__compute_y(src[0], src[1], src[2]), dest[1] = 255; break;
							CASE(4, 1) dest[0] = stbi__compute_y(src[0], src[1], src[2]); break;
							CASE(4, 2) dest[0] = stbi__compute_y(src[0], src[1], src[2]), dest[1] = src[3]; break;
							CASE(4, 3) dest[0] = src[0], dest[1] = src[1], dest[2] = src[2]; break;
						default: STBI_ASSERT(0);
						}
#undef CASE
					}

					STBI_FREE(data);
					return good;
				}

#ifndef STBI_NO_LINEAR
				float   *stbi__ldr_to_hdr(stbi_uc *data, int x, int y, int comp)
				{
					int i, k, n;
					float *output = (float *)stbi__malloc(x * y * comp * sizeof(float));
					if (output == NULL) { STBI_FREE(data); return stbi__errpf("outofmem", "Out of memory"); }
					// compute number of non-alpha components
					if (comp & 1) n = comp; else n = comp - 1;
					for (i = 0; i < x*y; ++i) {
						for (k = 0; k < n; ++k) {
							output[i*comp + k] = (float)(pow(data[i*comp + k] / 255.0f, stbi__l2h_gamma) * stbi__l2h_scale);
						}
						if (k < comp) output[i*comp + k] = data[i*comp + k] / 255.0f;
					}
					STBI_FREE(data);
					return output;
				}
#endif

#ifndef STBI_NO_HDR
#define stbi__float2int(x)   ((int) (x))
				stbi_uc *stbi__hdr_to_ldr(float   *data, int x, int y, int comp)
				{
					int i, k, n;
					stbi_uc *output = (stbi_uc *)stbi__malloc(x * y * comp);
					if (output == NULL) { STBI_FREE(data); return stbi__errpuc("outofmem", "Out of memory"); }
					// compute number of non-alpha components
					if (comp & 1) n = comp; else n = comp - 1;
					for (i = 0; i < x*y; ++i) {
						for (k = 0; k < n; ++k) {
							float z = (float)pow(data[i*comp + k] * stbi__h2l_scale_i, stbi__h2l_gamma_i) * 255 + 0.5f;
							if (z < 0) z = 0;
							if (z > 255) z = 255;
							output[i*comp + k] = (stbi_uc)stbi__float2int(z);
						}
						if (k < comp) {
							float z = data[i*comp + k] * 255 + 0.5f;
							if (z < 0) z = 0;
							if (z > 255) z = 255;
							output[i*comp + k] = (stbi_uc)stbi__float2int(z);
						}
					}
					STBI_FREE(data);
					return output;
				}
#endif

				//////////////////////////////////////////////////////////////////////////////
				//
				//  "baseline" JPEG/JFIF decoder
				//
				//    simple implementation
				//      - doesn't support delayed output of y-dimension
				//      - simple interface (only one output format: 8-bit interleaved RGB)
				//      - doesn't try to recover corrupt jpegs
				//      - doesn't allow partial loading, loading multiple at once
				//      - still fast on x86 (copying globals into locals doesn't help x86)
				//      - allocates lots of intermediate memory (full size of all components)
				//        - non-interleaved case requires this anyway
				//        - allows good upsampling (see next)
				//    high-quality
				//      - upsampled channels are bilinearly interpolated, even across blocks
				//      - quality integer IDCT derived from IJG's 'slow'
				//    performance
				//      - fast huffman; reasonable integer IDCT
				//      - some SIMD kernels for common paths on targets with SSE2/NEON
				//      - uses a lot of intermediate memory, could cache poorly

#ifndef STBI_NO_JPEG

				// huffman decoding acceleration
#define FAST_BITS   9  // larger handles more cases; smaller stomps less cache

				typedef struct
				{
					stbi_uc  fast[1 << FAST_BITS];
					// weirdly, repacking this into AoS is a 10% speed loss, instead of a win
					stbi__uint16 code[256];
					stbi_uc  values[256];
					stbi_uc  size[257];
					unsigned int maxcode[18];
					int    delta[17];   // old 'firstsymbol' - old 'firstcode'
				} stbi__huffman;

				typedef struct
				{
					stbi__context *s;
					stbi__huffman huff_dc[4];
					stbi__huffman huff_ac[4];
					stbi_uc dequant[4][64];
					stbi__int16 fast_ac[4][1 << FAST_BITS];

					// sizes for components, interleaved MCUs
					int img_h_max, img_v_max;
					int img_mcu_x, img_mcu_y;
					int img_mcu_w, img_mcu_h;

					// definition of jpeg image component
					struct
					{
						int id;
						int h, v;
						int tq;
						int hd, ha;
						int dc_pred;

						int x, y, w2, h2;
						stbi_uc *data;
						void *raw_data, *raw_coeff;
						stbi_uc *linebuf;
						short   *coeff;   // progressive only
						int      coeff_w, coeff_h; // number of 8x8 coefficient blocks
					} img_comp[4];

					stbi__uint32   code_buffer; // jpeg entropy-coded buffer
					int            code_bits;   // number of valid bits
					unsigned char  marker;      // marker seen while filling entropy buffer
					int            nomore;      // flag if we saw a marker so must stop

					int            progressive;
					int            spec_start;
					int            spec_end;
					int            succ_high;
					int            succ_low;
					int            eob_run;

					int scan_n, order[4];
					int restart_interval, todo;

					// kernels
					void(*idct_block_kernel)(stbi_uc *out, int out_stride, short data[64]);
					void(*YCbCr_to_RGB_kernel)(stbi_uc *out, const stbi_uc *y, const stbi_uc *pcb, const stbi_uc *pcr, int count, int step);
					stbi_uc *(*resample_row_hv_2_kernel)(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs);
				} stbi__jpeg;

				int stbi__build_huffman(stbi__huffman *h, int *count)
				{
					int i, j, k = 0, code;
					// build size list for each symbol (from JPEG spec)
					for (i = 0; i < 16; ++i)
					for (j = 0; j < count[i]; ++j)
						h->size[k++] = (stbi_uc)(i + 1);
					h->size[k] = 0;

					// compute actual symbols (from jpeg spec)
					code = 0;
					k = 0;
					for (j = 1; j <= 16; ++j) {
						// compute delta to add to code to compute symbol id
						h->delta[j] = k - code;
						if (h->size[k] == j) {
							while (h->size[k] == j)
								h->code[k++] = (stbi__uint16)(code++);
							if (code - 1 >= (1 << j)) return stbi__err("bad code lengths", "Corrupt JPEG");
						}
						// compute largest code + 1 for this size, preshifted as needed later
						h->maxcode[j] = code << (16 - j);
						code <<= 1;
					}
					h->maxcode[j] = 0xffffffff;

					// build non-spec acceleration table; 255 is flag for not-accelerated
					memset(h->fast, 255, 1 << FAST_BITS);
					for (i = 0; i < k; ++i) {
						int s = h->size[i];
						if (s <= FAST_BITS) {
							int c = h->code[i] << (FAST_BITS - s);
							int m = 1 << (FAST_BITS - s);
							for (j = 0; j < m; ++j) {
								h->fast[c + j] = (stbi_uc)i;
							}
						}
					}
					return 1;
				}

				// build a table that decodes both magnitude and value of small ACs in
				// one go.
				void stbi__build_fast_ac(stbi__int16 *fast_ac, stbi__huffman *h)
				{
					int i;
					for (i = 0; i < (1 << FAST_BITS); ++i) {
						stbi_uc fast = h->fast[i];
						fast_ac[i] = 0;
						if (fast < 255) {
							int rs = h->values[fast];
							int run = (rs >> 4) & 15;
							int magbits = rs & 15;
							int len = h->size[fast];

							if (magbits && len + magbits <= FAST_BITS) {
								// magnitude code followed by receive_extend code
								int k = ((i << len) & ((1 << FAST_BITS) - 1)) >> (FAST_BITS - magbits);
								int m = 1 << (magbits - 1);
								if (k < m) k += (-1 << magbits) + 1;
								// if the result is small enough, we can fit it in fast_ac table
								if (k >= -128 && k <= 127)
									fast_ac[i] = (stbi__int16)((k << 8) + (run << 4) + (len + magbits));
							}
						}
					}
				}

				void stbi__grow_buffer_unsafe(stbi__jpeg *j)
				{
					do {
						int b = j->nomore ? 0 : stbi__get8(j->s);
						if (b == 0xff) {
							int c = stbi__get8(j->s);
							if (c != 0) {
								j->marker = (unsigned char)c;
								j->nomore = 1;
								return;
							}
						}
						j->code_buffer |= b << (24 - j->code_bits);
						j->code_bits += 8;
					} while (j->code_bits <= 24);
				}

				// (1 << n) - 1
				stbi__uint32 stbi__bmask[17] = { 0, 1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383, 32767, 65535 };

				// decode a jpeg huffman value from the bitstream
				stbi_inline  int stbi__jpeg_huff_decode(stbi__jpeg *j, stbi__huffman *h)
				{
					unsigned int temp;
					int c, k;

					if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);

					// look at the top FAST_BITS and determine what symbol ID it is,
					// if the code is <= FAST_BITS
					c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS) - 1);
					k = h->fast[c];
					if (k < 255) {
						int s = h->size[k];
						if (s > j->code_bits)
							return -1;
						j->code_buffer <<= s;
						j->code_bits -= s;
						return h->values[k];
					}

					// naive test is to shift the code_buffer down so k bits are
					// valid, then test against maxcode. To speed this up, we've
					// preshifted maxcode left so that it has (16-k) 0s at the
					// end; in other words, regardless of the number of bits, it
					// wants to be compared against something shifted to have 16;
					// that way we don't need to shift inside the loop.
					temp = j->code_buffer >> 16;
					for (k = FAST_BITS + 1;; ++k)
					if (temp < h->maxcode[k])
						break;
					if (k == 17) {
						// error! code not found
						j->code_bits -= 16;
						return -1;
					}

					if (k > j->code_bits)
						return -1;

					// convert the huffman code to the symbol id
					c = ((j->code_buffer >> (32 - k)) & stbi__bmask[k]) + h->delta[k];
					STBI_ASSERT((((j->code_buffer) >> (32 - h->size[c])) & stbi__bmask[h->size[c]]) == h->code[c]);

					// convert the id to a symbol
					j->code_bits -= k;
					j->code_buffer <<= k;
					return h->values[c];
				}

				// bias[n] = (-1<<n) + 1
				int const stbi__jbias[16] = { 0, -1, -3, -7, -15, -31, -63, -127, -255, -511, -1023, -2047, -4095, -8191, -16383, -32767 };

				// combined JPEG 'receive' and JPEG 'extend', since baseline
				// always extends everything it receives.
				stbi_inline  int stbi__extend_receive(stbi__jpeg *j, int n)
				{
					unsigned int k;
					int sgn;
					if (j->code_bits < n) stbi__grow_buffer_unsafe(j);

					sgn = (stbi__int32)j->code_buffer >> 31; // sign bit is always in MSB
					k = stbi_lrot(j->code_buffer, n);
					STBI_ASSERT(n >= 0 && n < (int)(sizeof(stbi__bmask) / sizeof(*stbi__bmask)));
					j->code_buffer = k & ~stbi__bmask[n];
					k &= stbi__bmask[n];
					j->code_bits -= n;
					return k + (stbi__jbias[n] & ~sgn);
				}

				// get some unsigned bits
				stbi_inline  int stbi__jpeg_get_bits(stbi__jpeg *j, int n)
				{
					unsigned int k;
					if (j->code_bits < n) stbi__grow_buffer_unsafe(j);
					k = stbi_lrot(j->code_buffer, n);
					j->code_buffer = k & ~stbi__bmask[n];
					k &= stbi__bmask[n];
					j->code_bits -= n;
					return k;
				}

				stbi_inline  int stbi__jpeg_get_bit(stbi__jpeg *j)
				{
					unsigned int k;
					if (j->code_bits < 1) stbi__grow_buffer_unsafe(j);
					k = j->code_buffer;
					j->code_buffer <<= 1;
					--j->code_bits;
					return k & 0x80000000;
				}

				// given a value that's at position X in the zigzag stream,
				// where does it appear in the 8x8 matrix coded as row-major?
				stbi_uc stbi__jpeg_dezigzag[64 + 15] =
				{
					0, 1, 8, 16, 9, 2, 3, 10,
					17, 24, 32, 25, 18, 11, 4, 5,
					12, 19, 26, 33, 40, 48, 41, 34,
					27, 20, 13, 6, 7, 14, 21, 28,
					35, 42, 49, 56, 57, 50, 43, 36,
					29, 22, 15, 23, 30, 37, 44, 51,
					58, 59, 52, 45, 38, 31, 39, 46,
					53, 60, 61, 54, 47, 55, 62, 63,
					// let corrupt input sample past end
					63, 63, 63, 63, 63, 63, 63, 63,
					63, 63, 63, 63, 63, 63, 63
				};

				// decode one 64-entry block--
				int stbi__jpeg_decode_block(stbi__jpeg *j, short data[64], stbi__huffman *hdc, stbi__huffman *hac, stbi__int16 *fac, int b, stbi_uc *dequant)
				{
					int diff, dc, k;
					int t;

					if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);
					t = stbi__jpeg_huff_decode(j, hdc);
					if (t < 0) return stbi__err("bad huffman code", "Corrupt JPEG");

					// 0 all the ac values now so we can do it 32-bits at a time
					memset(data, 0, 64 * sizeof(data[0]));

					diff = t ? stbi__extend_receive(j, t) : 0;
					dc = j->img_comp[b].dc_pred + diff;
					j->img_comp[b].dc_pred = dc;
					data[0] = (short)(dc * dequant[0]);

					// decode AC components, see JPEG spec
					k = 1;
					do {
						unsigned int zig;
						int c, r, s;
						if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);
						c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS) - 1);
						r = fac[c];
						if (r) { // fast-AC path
							k += (r >> 4) & 15; // run
							s = r & 15; // combined length
							j->code_buffer <<= s;
							j->code_bits -= s;
							// decode into unzigzag'd location
							zig = stbi__jpeg_dezigzag[k++];
							data[zig] = (short)((r >> 8) * dequant[zig]);
						}
						else {
							int rs = stbi__jpeg_huff_decode(j, hac);
							if (rs < 0) return stbi__err("bad huffman code", "Corrupt JPEG");
							s = rs & 15;
							r = rs >> 4;
							if (s == 0) {
								if (rs != 0xf0) break; // end block
								k += 16;
							}
							else {
								k += r;
								// decode into unzigzag'd location
								zig = stbi__jpeg_dezigzag[k++];
								data[zig] = (short)(stbi__extend_receive(j, s) * dequant[zig]);
							}
						}
					} while (k < 64);
					return 1;
				}

				int stbi__jpeg_decode_block_prog_dc(stbi__jpeg *j, short data[64], stbi__huffman *hdc, int b)
				{
					int diff, dc;
					int t;
					if (j->spec_end != 0) return stbi__err("can't merge dc and ac", "Corrupt JPEG");

					if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);

					if (j->succ_high == 0) {
						// first scan for DC coefficient, must be first
						memset(data, 0, 64 * sizeof(data[0])); // 0 all the ac values now
						t = stbi__jpeg_huff_decode(j, hdc);
						diff = t ? stbi__extend_receive(j, t) : 0;

						dc = j->img_comp[b].dc_pred + diff;
						j->img_comp[b].dc_pred = dc;
						data[0] = (short)(dc << j->succ_low);
					}
					else {
						// refinement scan for DC coefficient
						if (stbi__jpeg_get_bit(j))
							data[0] += (short)(1 << j->succ_low);
					}
					return 1;
				}

				// @OPTIMIZE: store non-zigzagged during the decode passes,
				// and only de-zigzag when dequantizing
				int stbi__jpeg_decode_block_prog_ac(stbi__jpeg *j, short data[64], stbi__huffman *hac, stbi__int16 *fac)
				{
					int k;
					if (j->spec_start == 0) return stbi__err("can't merge dc and ac", "Corrupt JPEG");

					if (j->succ_high == 0) {
						int shift = j->succ_low;

						if (j->eob_run) {
							--j->eob_run;
							return 1;
						}

						k = j->spec_start;
						do {
							unsigned int zig;
							int c, r, s;
							if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);
							c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS) - 1);
							r = fac[c];
							if (r) { // fast-AC path
								k += (r >> 4) & 15; // run
								s = r & 15; // combined length
								j->code_buffer <<= s;
								j->code_bits -= s;
								zig = stbi__jpeg_dezigzag[k++];
								data[zig] = (short)((r >> 8) << shift);
							}
							else {
								int rs = stbi__jpeg_huff_decode(j, hac);
								if (rs < 0) return stbi__err("bad huffman code", "Corrupt JPEG");
								s = rs & 15;
								r = rs >> 4;
								if (s == 0) {
									if (r < 15) {
										j->eob_run = (1 << r);
										if (r)
											j->eob_run += stbi__jpeg_get_bits(j, r);
										--j->eob_run;
										break;
									}
									k += 16;
								}
								else {
									k += r;
									zig = stbi__jpeg_dezigzag[k++];
									data[zig] = (short)(stbi__extend_receive(j, s) << shift);
								}
							}
						} while (k <= j->spec_end);
					}
					else {
						// refinement scan for these AC coefficients

						short bit = (short)(1 << j->succ_low);

						if (j->eob_run) {
							--j->eob_run;
							for (k = j->spec_start; k <= j->spec_end; ++k) {
								short *p = &data[stbi__jpeg_dezigzag[k]];
								if (*p != 0)
								if (stbi__jpeg_get_bit(j))
								if ((*p & bit) == 0) {
									if (*p > 0)
										*p += bit;
									else
										*p -= bit;
								}
							}
						}
						else {
							k = j->spec_start;
							do {
								int r, s;
								int rs = stbi__jpeg_huff_decode(j, hac); // @OPTIMIZE see if we can use the fast path here, advance-by-r is so slow, eh
								if (rs < 0) return stbi__err("bad huffman code", "Corrupt JPEG");
								s = rs & 15;
								r = rs >> 4;
								if (s == 0) {
									if (r < 15) {
										j->eob_run = (1 << r) - 1;
										if (r)
											j->eob_run += stbi__jpeg_get_bits(j, r);
										r = 64; // force end of block
									}
									else {
										// r=15 s=0 should write 16 0s, so we just do
										// a run of 15 0s and then write s (which is 0),
										// so we don't have to do anything special here
									}
								}
								else {
									if (s != 1) return stbi__err("bad huffman code", "Corrupt JPEG");
									// sign bit
									if (stbi__jpeg_get_bit(j))
										s = bit;
									else
										s = -bit;
								}

								// advance by r
								while (k <= j->spec_end) {
									short *p = &data[stbi__jpeg_dezigzag[k++]];
									if (*p != 0) {
										if (stbi__jpeg_get_bit(j))
										if ((*p & bit) == 0) {
											if (*p > 0)
												*p += bit;
											else
												*p -= bit;
										}
									}
									else {
										if (r == 0) {
											*p = (short)s;
											break;
										}
										--r;
									}
								}
							} while (k <= j->spec_end);
						}
					}
					return 1;
				}

				// take a -128..127 value and stbi__clamp it and convert to 0..255
				stbi_inline  stbi_uc stbi__clamp(int x)
				{
					// trick to use a single test to catch both cases
					if ((unsigned int)x > 255) {
						if (x < 0) return 0;
						if (x > 255) return 255;
					}
					return (stbi_uc)x;
				}

#define stbi__f2f(x)  ((int) (((x) * 4096 + 0.5)))
#define stbi__fsh(x)  ((x) << 12)

				// derived from jidctint -- DCT_ISLOW
#define STBI__IDCT_1D(s0,s1,s2,s3,s4,s5,s6,s7) \
	int t0, t1, t2, t3, p1, p2, p3, p4, p5, x0, x1, x2, x3; \
	p2 = s2;                                    \
	p3 = s6;                                    \
	p1 = (p2 + p3) * stbi__f2f(0.5411961f);       \
	t2 = p1 + p3*stbi__f2f(-1.847759065f);      \
	t3 = p1 + p2*stbi__f2f(0.765366865f);      \
	p2 = s0;                                    \
	p3 = s4;                                    \
	t0 = stbi__fsh(p2 + p3);                      \
	t1 = stbi__fsh(p2 - p3);                      \
	x0 = t0 + t3;                                 \
	x3 = t0 - t3;                                 \
	x1 = t1 + t2;                                 \
	x2 = t1 - t2;                                 \
	t0 = s7;                                    \
	t1 = s5;                                    \
	t2 = s3;                                    \
	t3 = s1;                                    \
	p3 = t0 + t2;                                 \
	p4 = t1 + t3;                                 \
	p1 = t0 + t3;                                 \
	p2 = t1 + t2;                                 \
	p5 = (p3 + p4)*stbi__f2f(1.175875602f);      \
	t0 = t0*stbi__f2f(0.298631336f);           \
	t1 = t1*stbi__f2f(2.053119869f);           \
	t2 = t2*stbi__f2f(3.072711026f);           \
	t3 = t3*stbi__f2f(1.501321110f);           \
	p1 = p5 + p1*stbi__f2f(-0.899976223f);      \
	p2 = p5 + p2*stbi__f2f(-2.562915447f);      \
	p3 = p3*stbi__f2f(-1.961570560f);           \
	p4 = p4*stbi__f2f(-0.390180644f);           \
	t3 += p1 + p4;                                \
	t2 += p2 + p3;                                \
	t1 += p2 + p4;                                \
	t0 += p1 + p3;

				void stbi__idct_block(stbi_uc *out, int out_stride, short data[64])
				{
					int i, val[64], *v = val;
					stbi_uc *o;
					short *d = data;

					// columns
					for (i = 0; i < 8; ++i, ++d, ++v) {
						// if all zeroes, shortcut -- this avoids dequantizing 0s and IDCTing
						if (d[8] == 0 && d[16] == 0 && d[24] == 0 && d[32] == 0
							&& d[40] == 0 && d[48] == 0 && d[56] == 0) {
							//    no shortcut                 0     seconds
							//    (1|2|3|4|5|6|7)==0          0     seconds
							//    all separate               -0.047 seconds
							//    1 && 2|3 && 4|5 && 6|7:    -0.047 seconds
							int dcterm = d[0] << 2;
							v[0] = v[8] = v[16] = v[24] = v[32] = v[40] = v[48] = v[56] = dcterm;
						}
						else {
							STBI__IDCT_1D(d[0], d[8], d[16], d[24], d[32], d[40], d[48], d[56])
								// constants scaled things up by 1<<12; let's bring them back
								// down, but keep 2 extra bits of precision
								x0 += 512; x1 += 512; x2 += 512; x3 += 512;
							v[0] = (x0 + t3) >> 10;
							v[56] = (x0 - t3) >> 10;
							v[8] = (x1 + t2) >> 10;
							v[48] = (x1 - t2) >> 10;
							v[16] = (x2 + t1) >> 10;
							v[40] = (x2 - t1) >> 10;
							v[24] = (x3 + t0) >> 10;
							v[32] = (x3 - t0) >> 10;
						}
					}

					for (i = 0, v = val, o = out; i < 8; ++i, v += 8, o += out_stride) {
						// no fast case since the first 1D IDCT spread components out
						STBI__IDCT_1D(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7])
							// constants scaled things up by 1<<12, plus we had 1<<2 from first
							// loop, plus horizontal and vertical each scale by sqrt(8) so together
							// we've got an extra 1<<3, so 1<<17 total we need to remove.
							// so we want to round that, which means adding 0.5 * 1<<17,
							// aka 65536. Also, we'll end up with -128 to 127 that we want
							// to encode as 0..255 by adding 128, so we'll add that before the shift
							x0 += 65536 + (128 << 17);
						x1 += 65536 + (128 << 17);
						x2 += 65536 + (128 << 17);
						x3 += 65536 + (128 << 17);
						// tried computing the shifts into temps, or'ing the temps to see
						// if any were out of range, but that was slower
						o[0] = stbi__clamp((x0 + t3) >> 17);
						o[7] = stbi__clamp((x0 - t3) >> 17);
						o[1] = stbi__clamp((x1 + t2) >> 17);
						o[6] = stbi__clamp((x1 - t2) >> 17);
						o[2] = stbi__clamp((x2 + t1) >> 17);
						o[5] = stbi__clamp((x2 - t1) >> 17);
						o[3] = stbi__clamp((x3 + t0) >> 17);
						o[4] = stbi__clamp((x3 - t0) >> 17);
					}
				}

#ifdef STBI_SSE2
				// sse2 integer IDCT. not the fastest possible implementation but it
				// produces bit-identical results to the generic C version so it's
				// fully "transparent".
				void stbi__idct_simd(stbi_uc *out, int out_stride, short data[64])
				{
					// This is constructed to match our regular (generic) integer IDCT exactly.
					__m128i row0, row1, row2, row3, row4, row5, row6, row7;
					__m128i tmp;

					// dot product constant: even elems=x, odd elems=y
#define dct_const(x,y)  _mm_setr_epi16((x),(y),(x),(y),(x),(y),(x),(y))

					// out(0) = c0[even]*x + c0[odd]*y   (c0, x, y 16-bit, out 32-bit)
					// out(1) = c1[even]*x + c1[odd]*y
#define dct_rot(out0,out1, x,y,c0,c1) \
	__m128i c0##lo = _mm_unpacklo_epi16((x), (y)); \
	__m128i c0##hi = _mm_unpackhi_epi16((x), (y)); \
	__m128i out0##_l = _mm_madd_epi16(c0##lo, c0); \
	__m128i out0##_h = _mm_madd_epi16(c0##hi, c0); \
	__m128i out1##_l = _mm_madd_epi16(c0##lo, c1); \
	__m128i out1##_h = _mm_madd_epi16(c0##hi, c1)

					// out = in << 12  (in 16-bit, out 32-bit)
#define dct_widen(out, in) \
	__m128i out##_l = _mm_srai_epi32(_mm_unpacklo_epi16(_mm_setzero_si128(), (in)), 4); \
	__m128i out##_h = _mm_srai_epi32(_mm_unpackhi_epi16(_mm_setzero_si128(), (in)), 4)

					// wide add
#define dct_wadd(out, a, b) \
	__m128i out##_l = _mm_add_epi32(a##_l, b##_l); \
	__m128i out##_h = _mm_add_epi32(a##_h, b##_h)

					// wide sub
#define dct_wsub(out, a, b) \
	__m128i out##_l = _mm_sub_epi32(a##_l, b##_l); \
	__m128i out##_h = _mm_sub_epi32(a##_h, b##_h)

					// butterfly a/b, add bias, then shift by "s" and pack
#define dct_bfly32o(out0, out1, a,b,bias,s) \
					{
					\
						__m128i abiased_l = _mm_add_epi32(a##_l, bias); \
						__m128i abiased_h = _mm_add_epi32(a##_h, bias); \
						dct_wadd(sum, abiased, b); \
						dct_wsub(dif, abiased, b); \
						out0 = _mm_packs_epi32(_mm_srai_epi32(sum_l, s), _mm_srai_epi32(sum_h, s)); \
						out1 = _mm_packs_epi32(_mm_srai_epi32(dif_l, s), _mm_srai_epi32(dif_h, s)); \
	}

						// 8-bit interleave step (for transposes)
#define dct_interleave8(a, b) \
	tmp = a; \
	a = _mm_unpacklo_epi8(a, b); \
	b = _mm_unpackhi_epi8(tmp, b)

						// 16-bit interleave step (for transposes)
#define dct_interleave16(a, b) \
	tmp = a; \
	a = _mm_unpacklo_epi16(a, b); \
	b = _mm_unpackhi_epi16(tmp, b)

#define dct_pass(bias,shift) \
					{
						\
						/* even part */ \
						dct_rot(t2e, t3e, row2, row6, rot0_0, rot0_1); \
						__m128i sum04 = _mm_add_epi16(row0, row4); \
						__m128i dif04 = _mm_sub_epi16(row0, row4); \
						dct_widen(t0e, sum04); \
						dct_widen(t1e, dif04); \
						dct_wadd(x0, t0e, t3e); \
						dct_wsub(x3, t0e, t3e); \
						dct_wadd(x1, t1e, t2e); \
						dct_wsub(x2, t1e, t2e); \
						/* odd part */ \
						dct_rot(y0o, y2o, row7, row3, rot2_0, rot2_1); \
						dct_rot(y1o, y3o, row5, row1, rot3_0, rot3_1); \
						__m128i sum17 = _mm_add_epi16(row1, row7); \
						__m128i sum35 = _mm_add_epi16(row3, row5); \
						dct_rot(y4o, y5o, sum17, sum35, rot1_0, rot1_1); \
						dct_wadd(x4, y0o, y4o); \
						dct_wadd(x5, y1o, y5o); \
						dct_wadd(x6, y2o, y5o); \
						dct_wadd(x7, y3o, y4o); \
						dct_bfly32o(row0, row7, x0, x7, bias, shift); \
						dct_bfly32o(row1, row6, x1, x6, bias, shift); \
						dct_bfly32o(row2, row5, x2, x5, bias, shift); \
						dct_bfly32o(row3, row4, x3, x4, bias, shift); \
	}

					__m128i rot0_0 = dct_const(stbi__f2f(0.5411961f), stbi__f2f(0.5411961f) + stbi__f2f(-1.847759065f));
					__m128i rot0_1 = dct_const(stbi__f2f(0.5411961f) + stbi__f2f(0.765366865f), stbi__f2f(0.5411961f));
					__m128i rot1_0 = dct_const(stbi__f2f(1.175875602f) + stbi__f2f(-0.899976223f), stbi__f2f(1.175875602f));
					__m128i rot1_1 = dct_const(stbi__f2f(1.175875602f), stbi__f2f(1.175875602f) + stbi__f2f(-2.562915447f));
					__m128i rot2_0 = dct_const(stbi__f2f(-1.961570560f) + stbi__f2f(0.298631336f), stbi__f2f(-1.961570560f));
					__m128i rot2_1 = dct_const(stbi__f2f(-1.961570560f), stbi__f2f(-1.961570560f) + stbi__f2f(3.072711026f));
					__m128i rot3_0 = dct_const(stbi__f2f(-0.390180644f) + stbi__f2f(2.053119869f), stbi__f2f(-0.390180644f));
					__m128i rot3_1 = dct_const(stbi__f2f(-0.390180644f), stbi__f2f(-0.390180644f) + stbi__f2f(1.501321110f));

					// rounding biases in column/row passes, see stbi__idct_block for explanation.
					__m128i bias_0 = _mm_set1_epi32(512);
					__m128i bias_1 = _mm_set1_epi32(65536 + (128 << 17));

					// load
					row0 = _mm_load_si128((const __m128i *) (data + 0 * 8));
					row1 = _mm_load_si128((const __m128i *) (data + 1 * 8));
					row2 = _mm_load_si128((const __m128i *) (data + 2 * 8));
					row3 = _mm_load_si128((const __m128i *) (data + 3 * 8));
					row4 = _mm_load_si128((const __m128i *) (data + 4 * 8));
					row5 = _mm_load_si128((const __m128i *) (data + 5 * 8));
					row6 = _mm_load_si128((const __m128i *) (data + 6 * 8));
					row7 = _mm_load_si128((const __m128i *) (data + 7 * 8));

					// column pass
					dct_pass(bias_0, 10);

					{
						// 16bit 8x8 transpose pass 1
						dct_interleave16(row0, row4);
						dct_interleave16(row1, row5);
						dct_interleave16(row2, row6);
						dct_interleave16(row3, row7);

						// transpose pass 2
						dct_interleave16(row0, row2);
						dct_interleave16(row1, row3);
						dct_interleave16(row4, row6);
						dct_interleave16(row5, row7);

						// transpose pass 3
						dct_interleave16(row0, row1);
						dct_interleave16(row2, row3);
						dct_interleave16(row4, row5);
						dct_interleave16(row6, row7);
					}

					// row pass
					dct_pass(bias_1, 17);

					{
						// pack
						__m128i p0 = _mm_packus_epi16(row0, row1); // a0a1a2a3...a7b0b1b2b3...b7
						__m128i p1 = _mm_packus_epi16(row2, row3);
						__m128i p2 = _mm_packus_epi16(row4, row5);
						__m128i p3 = _mm_packus_epi16(row6, row7);

						// 8bit 8x8 transpose pass 1
						dct_interleave8(p0, p2); // a0e0a1e1...
						dct_interleave8(p1, p3); // c0g0c1g1...

						// transpose pass 2
						dct_interleave8(p0, p1); // a0c0e0g0...
						dct_interleave8(p2, p3); // b0d0f0h0...

						// transpose pass 3
						dct_interleave8(p0, p2); // a0b0c0d0...
						dct_interleave8(p1, p3); // a4b4c4d4...

						// store
						_mm_storel_epi64((__m128i *) out, p0); out += out_stride;
						_mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p0, 0x4e)); out += out_stride;
						_mm_storel_epi64((__m128i *) out, p2); out += out_stride;
						_mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p2, 0x4e)); out += out_stride;
						_mm_storel_epi64((__m128i *) out, p1); out += out_stride;
						_mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p1, 0x4e)); out += out_stride;
						_mm_storel_epi64((__m128i *) out, p3); out += out_stride;
						_mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p3, 0x4e));
					}

#undef dct_const
#undef dct_rot
#undef dct_widen
#undef dct_wadd
#undef dct_wsub
#undef dct_bfly32o
#undef dct_interleave8
#undef dct_interleave16
#undef dct_pass
				}

#endif // STBI_SSE2

#ifdef STBI_NEON

				// NEON integer IDCT. should produce bit-identical
				// results to the generic C version.
				void stbi__idct_simd(stbi_uc *out, int out_stride, short data[64])
				{
					int16x8_t row0, row1, row2, row3, row4, row5, row6, row7;

					int16x4_t rot0_0 = vdup_n_s16(stbi__f2f(0.5411961f));
					int16x4_t rot0_1 = vdup_n_s16(stbi__f2f(-1.847759065f));
					int16x4_t rot0_2 = vdup_n_s16(stbi__f2f(0.765366865f));
					int16x4_t rot1_0 = vdup_n_s16(stbi__f2f(1.175875602f));
					int16x4_t rot1_1 = vdup_n_s16(stbi__f2f(-0.899976223f));
					int16x4_t rot1_2 = vdup_n_s16(stbi__f2f(-2.562915447f));
					int16x4_t rot2_0 = vdup_n_s16(stbi__f2f(-1.961570560f));
					int16x4_t rot2_1 = vdup_n_s16(stbi__f2f(-0.390180644f));
					int16x4_t rot3_0 = vdup_n_s16(stbi__f2f(0.298631336f));
					int16x4_t rot3_1 = vdup_n_s16(stbi__f2f(2.053119869f));
					int16x4_t rot3_2 = vdup_n_s16(stbi__f2f(3.072711026f));
					int16x4_t rot3_3 = vdup_n_s16(stbi__f2f(1.501321110f));

#define dct_long_mul(out, inq, coeff) \
	int32x4_t out##_l = vmull_s16(vget_low_s16(inq), coeff); \
	int32x4_t out##_h = vmull_s16(vget_high_s16(inq), coeff)

#define dct_long_mac(out, acc, inq, coeff) \
	int32x4_t out##_l = vmlal_s16(acc##_l, vget_low_s16(inq), coeff); \
	int32x4_t out##_h = vmlal_s16(acc##_h, vget_high_s16(inq), coeff)

#define dct_widen(out, inq) \
	int32x4_t out##_l = vshll_n_s16(vget_low_s16(inq), 12); \
	int32x4_t out##_h = vshll_n_s16(vget_high_s16(inq), 12)

					// wide add
#define dct_wadd(out, a, b) \
	int32x4_t out##_l = vaddq_s32(a##_l, b##_l); \
	int32x4_t out##_h = vaddq_s32(a##_h, b##_h)

					// wide sub
#define dct_wsub(out, a, b) \
	int32x4_t out##_l = vsubq_s32(a##_l, b##_l); \
	int32x4_t out##_h = vsubq_s32(a##_h, b##_h)

					// butterfly a/b, then shift using "shiftop" by "s" and pack
#define dct_bfly32o(out0,out1, a,b,shiftop,s) \
					{
					\
						dct_wadd(sum, a, b); \
						dct_wsub(dif, a, b); \
						out0 = vcombine_s16(shiftop(sum_l, s), shiftop(sum_h, s)); \
						out1 = vcombine_s16(shiftop(dif_l, s), shiftop(dif_h, s)); \
	}

#define dct_pass(shiftop, shift) \
					{ \
					/* even part */ \
					int16x8_t sum26 = vaddq_s16(row2, row6); \
					dct_long_mul(p1e, sum26, rot0_0); \
					dct_long_mac(t2e, p1e, row6, rot0_1); \
					dct_long_mac(t3e, p1e, row2, rot0_2); \
					int16x8_t sum04 = vaddq_s16(row0, row4); \
					int16x8_t dif04 = vsubq_s16(row0, row4); \
					dct_widen(t0e, sum04); \
					dct_widen(t1e, dif04); \
					dct_wadd(x0, t0e, t3e); \
					dct_wsub(x3, t0e, t3e); \
					dct_wadd(x1, t1e, t2e); \
					dct_wsub(x2, t1e, t2e); \
					/* odd part */ \
					int16x8_t sum15 = vaddq_s16(row1, row5); \
					int16x8_t sum17 = vaddq_s16(row1, row7); \
					int16x8_t sum35 = vaddq_s16(row3, row5); \
					int16x8_t sum37 = vaddq_s16(row3, row7); \
					int16x8_t sumodd = vaddq_s16(sum17, sum35); \
					dct_long_mul(p5o, sumodd, rot1_0); \
					dct_long_mac(p1o, p5o, sum17, rot1_1); \
					dct_long_mac(p2o, p5o, sum35, rot1_2); \
					dct_long_mul(p3o, sum37, rot2_0); \
					dct_long_mul(p4o, sum15, rot2_1); \
					dct_wadd(sump13o, p1o, p3o); \
					dct_wadd(sump24o, p2o, p4o); \
					dct_wadd(sump23o, p2o, p3o); \
					dct_wadd(sump14o, p1o, p4o); \
					dct_long_mac(x4, sump13o, row7, rot3_0); \
					dct_long_mac(x5, sump24o, row5, rot3_1); \
					dct_long_mac(x6, sump23o, row3, rot3_2); \
					dct_long_mac(x7, sump14o, row1, rot3_3); \
					dct_bfly32o(row0, row7, x0, x7, shiftop, shift); \
					dct_bfly32o(row1, row6, x1, x6, shiftop, shift); \
					dct_bfly32o(row2, row5, x2, x5, shiftop, shift); \
					dct_bfly32o(row3, row4, x3, x4, shiftop, shift); \
					}

					// load
					row0 = vld1q_s16(data + 0 * 8);
					row1 = vld1q_s16(data + 1 * 8);
					row2 = vld1q_s16(data + 2 * 8);
					row3 = vld1q_s16(data + 3 * 8);
					row4 = vld1q_s16(data + 4 * 8);
					row5 = vld1q_s16(data + 5 * 8);
					row6 = vld1q_s16(data + 6 * 8);
					row7 = vld1q_s16(data + 7 * 8);

					// add DC bias
					row0 = vaddq_s16(row0, vsetq_lane_s16(1024, vdupq_n_s16(0), 0));

					// column pass
					dct_pass(vrshrn_n_s32, 10);

					// 16bit 8x8 transpose
					{
						// these three map to a single VTRN.16, VTRN.32, and VSWP, respectively.
						// whether compilers actually get this is another story, sadly.
#define dct_trn16(x, y) { int16x8x2_t t = vtrnq_s16(x, y); x = t.val[0]; y = t.val[1]; }
#define dct_trn32(x, y) { int32x4x2_t t = vtrnq_s32(vreinterpretq_s32_s16(x), vreinterpretq_s32_s16(y)); x = vreinterpretq_s16_s32(t.val[0]); y = vreinterpretq_s16_s32(t.val[1]); }
#define dct_trn64(x, y) { int16x8_t x0 = x; int16x8_t y0 = y; x = vcombine_s16(vget_low_s16(x0), vget_low_s16(y0)); y = vcombine_s16(vget_high_s16(x0), vget_high_s16(y0)); }

						// pass 1
						dct_trn16(row0, row1); // a0b0a2b2a4b4a6b6
						dct_trn16(row2, row3);
						dct_trn16(row4, row5);
						dct_trn16(row6, row7);

						// pass 2
						dct_trn32(row0, row2); // a0b0c0d0a4b4c4d4
						dct_trn32(row1, row3);
						dct_trn32(row4, row6);
						dct_trn32(row5, row7);

						// pass 3
						dct_trn64(row0, row4); // a0b0c0d0e0f0g0h0
						dct_trn64(row1, row5);
						dct_trn64(row2, row6);
						dct_trn64(row3, row7);

#undef dct_trn16
#undef dct_trn32
#undef dct_trn64
					}

					// row pass
					// vrshrn_n_s32 only supports shifts up to 16, we need
					// 17. so do a non-rounding shift of 16 first then follow
					// up with a rounding shift by 1.
					dct_pass(vshrn_n_s32, 16);

					{
						// pack and round
						uint8x8_t p0 = vqrshrun_n_s16(row0, 1);
						uint8x8_t p1 = vqrshrun_n_s16(row1, 1);
						uint8x8_t p2 = vqrshrun_n_s16(row2, 1);
						uint8x8_t p3 = vqrshrun_n_s16(row3, 1);
						uint8x8_t p4 = vqrshrun_n_s16(row4, 1);
						uint8x8_t p5 = vqrshrun_n_s16(row5, 1);
						uint8x8_t p6 = vqrshrun_n_s16(row6, 1);
						uint8x8_t p7 = vqrshrun_n_s16(row7, 1);

						// again, these can translate into one instruction, but often don't.
#define dct_trn8_8(x, y) { uint8x8x2_t t = vtrn_u8(x, y); x = t.val[0]; y = t.val[1]; }
#define dct_trn8_16(x, y) { uint16x4x2_t t = vtrn_u16(vreinterpret_u16_u8(x), vreinterpret_u16_u8(y)); x = vreinterpret_u8_u16(t.val[0]); y = vreinterpret_u8_u16(t.val[1]); }
#define dct_trn8_32(x, y) { uint32x2x2_t t = vtrn_u32(vreinterpret_u32_u8(x), vreinterpret_u32_u8(y)); x = vreinterpret_u8_u32(t.val[0]); y = vreinterpret_u8_u32(t.val[1]); }

						// sadly can't use interleaved stores here since we only write
						// 8 bytes to each scan line!

						// 8x8 8-bit transpose pass 1
						dct_trn8_8(p0, p1);
						dct_trn8_8(p2, p3);
						dct_trn8_8(p4, p5);
						dct_trn8_8(p6, p7);

						// pass 2
						dct_trn8_16(p0, p2);
						dct_trn8_16(p1, p3);
						dct_trn8_16(p4, p6);
						dct_trn8_16(p5, p7);

						// pass 3
						dct_trn8_32(p0, p4);
						dct_trn8_32(p1, p5);
						dct_trn8_32(p2, p6);
						dct_trn8_32(p3, p7);

						// store
						vst1_u8(out, p0); out += out_stride;
						vst1_u8(out, p1); out += out_stride;
						vst1_u8(out, p2); out += out_stride;
						vst1_u8(out, p3); out += out_stride;
						vst1_u8(out, p4); out += out_stride;
						vst1_u8(out, p5); out += out_stride;
						vst1_u8(out, p6); out += out_stride;
						vst1_u8(out, p7);

#undef dct_trn8_8
#undef dct_trn8_16
#undef dct_trn8_32
					}

#undef dct_long_mul
#undef dct_long_mac
#undef dct_widen
#undef dct_wadd
#undef dct_wsub
#undef dct_bfly32o
#undef dct_pass
				}

#endif // STBI_NEON

#define STBI__MARKER_none  0xff
				// if there's a pending marker from the entropy stream, return that
				// otherwise, fetch from the stream and get a marker. if there's no
				// marker, return 0xff, which is never a valid marker value
				stbi_uc stbi__get_marker(stbi__jpeg *j)
				{
					stbi_uc x;
					if (j->marker != STBI__MARKER_none) { x = j->marker; j->marker = STBI__MARKER_none; return x; }
					x = stbi__get8(j->s);
					if (x != 0xff) return STBI__MARKER_none;
					while (x == 0xff)
						x = stbi__get8(j->s);
					return x;
				}

				// in each scan, we'll have scan_n components, and the order
				// of the components is specified by order[]
#define STBI__RESTART(x)     ((x) >= 0xd0 && (x) <= 0xd7)

				// after a restart interval, stbi__jpeg_reset the entropy decoder and
				// the dc prediction
				void stbi__jpeg_reset(stbi__jpeg *j)
				{
					j->code_bits = 0;
					j->code_buffer = 0;
					j->nomore = 0;
					j->img_comp[0].dc_pred = j->img_comp[1].dc_pred = j->img_comp[2].dc_pred = 0;
					j->marker = STBI__MARKER_none;
					j->todo = j->restart_interval ? j->restart_interval : 0x7fffffff;
					j->eob_run = 0;
					// no more than 1<<31 MCUs if no restart_interal? that's plenty safe,
					// since we don't even allow 1<<30 pixels
				}

				int stbi__parse_entropy_coded_data(stbi__jpeg *z)
				{
					stbi__jpeg_reset(z);
					if (!z->progressive) {
						if (z->scan_n == 1) {
							int i, j;
							STBI_SIMD_ALIGN(short, data[64]);
							int n = z->order[0];
							// non-interleaved data, we just need to process one block at a time,
							// in trivial scanline order
							// number of blocks to do just depends on how many actual "pixels" this
							// component has, independent of interleaved MCU blocking and such
							int w = (z->img_comp[n].x + 7) >> 3;
							int h = (z->img_comp[n].y + 7) >> 3;
							for (j = 0; j < h; ++j) {
								for (i = 0; i < w; ++i) {
									int ha = z->img_comp[n].ha;
									if (!stbi__jpeg_decode_block(z, data, z->huff_dc + z->img_comp[n].hd, z->huff_ac + ha, z->fast_ac[ha], n, z->dequant[z->img_comp[n].tq])) return 0;
									z->idct_block_kernel(z->img_comp[n].data + z->img_comp[n].w2*j * 8 + i * 8, z->img_comp[n].w2, data);
									// every data block is an MCU, so countdown the restart interval
									if (--z->todo <= 0) {
										if (z->code_bits < 24) stbi__grow_buffer_unsafe(z);
										// if it's NOT a restart, then just bail, so we get corrupt data
										// rather than no data
										if (!STBI__RESTART(z->marker)) return 1;
										stbi__jpeg_reset(z);
									}
								}
							}
							return 1;
						}
						else { // interleaved
							int i, j, k, x, y;
							STBI_SIMD_ALIGN(short, data[64]);
							for (j = 0; j < z->img_mcu_y; ++j) {
								for (i = 0; i < z->img_mcu_x; ++i) {
									// scan an interleaved mcu... process scan_n components in order
									for (k = 0; k < z->scan_n; ++k) {
										int n = z->order[k];
										// scan out an mcu's worth of this component; that's just determined
										// by the basic H and V specified for the component
										for (y = 0; y < z->img_comp[n].v; ++y) {
											for (x = 0; x < z->img_comp[n].h; ++x) {
												int x2 = (i*z->img_comp[n].h + x) * 8;
												int y2 = (j*z->img_comp[n].v + y) * 8;
												int ha = z->img_comp[n].ha;
												if (!stbi__jpeg_decode_block(z, data, z->huff_dc + z->img_comp[n].hd, z->huff_ac + ha, z->fast_ac[ha], n, z->dequant[z->img_comp[n].tq])) return 0;
												z->idct_block_kernel(z->img_comp[n].data + z->img_comp[n].w2*y2 + x2, z->img_comp[n].w2, data);
											}
										}
									}
									// after all interleaved components, that's an interleaved MCU,
									// so now count down the restart interval
									if (--z->todo <= 0) {
										if (z->code_bits < 24) stbi__grow_buffer_unsafe(z);
										if (!STBI__RESTART(z->marker)) return 1;
										stbi__jpeg_reset(z);
									}
								}
							}
							return 1;
						}
					}
					else {
						if (z->scan_n == 1) {
							int i, j;
							int n = z->order[0];
							// non-interleaved data, we just need to process one block at a time,
							// in trivial scanline order
							// number of blocks to do just depends on how many actual "pixels" this
							// component has, independent of interleaved MCU blocking and such
							int w = (z->img_comp[n].x + 7) >> 3;
							int h = (z->img_comp[n].y + 7) >> 3;
							for (j = 0; j < h; ++j) {
								for (i = 0; i < w; ++i) {
									short *data = z->img_comp[n].coeff + 64 * (i + j * z->img_comp[n].coeff_w);
									if (z->spec_start == 0) {
										if (!stbi__jpeg_decode_block_prog_dc(z, data, &z->huff_dc[z->img_comp[n].hd], n))
											return 0;
									}
									else {
										int ha = z->img_comp[n].ha;
										if (!stbi__jpeg_decode_block_prog_ac(z, data, &z->huff_ac[ha], z->fast_ac[ha]))
											return 0;
									}
									// every data block is an MCU, so countdown the restart interval
									if (--z->todo <= 0) {
										if (z->code_bits < 24) stbi__grow_buffer_unsafe(z);
										if (!STBI__RESTART(z->marker)) return 1;
										stbi__jpeg_reset(z);
									}
								}
							}
							return 1;
						}
						else { // interleaved
							int i, j, k, x, y;
							for (j = 0; j < z->img_mcu_y; ++j) {
								for (i = 0; i < z->img_mcu_x; ++i) {
									// scan an interleaved mcu... process scan_n components in order
									for (k = 0; k < z->scan_n; ++k) {
										int n = z->order[k];
										// scan out an mcu's worth of this component; that's just determined
										// by the basic H and V specified for the component
										for (y = 0; y < z->img_comp[n].v; ++y) {
											for (x = 0; x < z->img_comp[n].h; ++x) {
												int x2 = (i*z->img_comp[n].h + x);
												int y2 = (j*z->img_comp[n].v + y);
												short *data = z->img_comp[n].coeff + 64 * (x2 + y2 * z->img_comp[n].coeff_w);
												if (!stbi__jpeg_decode_block_prog_dc(z, data, &z->huff_dc[z->img_comp[n].hd], n))
													return 0;
											}
										}
									}
									// after all interleaved components, that's an interleaved MCU,
									// so now count down the restart interval
									if (--z->todo <= 0) {
										if (z->code_bits < 24) stbi__grow_buffer_unsafe(z);
										if (!STBI__RESTART(z->marker)) return 1;
										stbi__jpeg_reset(z);
									}
								}
							}
							return 1;
						}
					}
				}

				void stbi__jpeg_dequantize(short *data, stbi_uc *dequant)
				{
					int i;
					for (i = 0; i < 64; ++i)
						data[i] *= dequant[i];
				}

				void stbi__jpeg_finish(stbi__jpeg *z)
				{
					if (z->progressive) {
						// dequantize and idct the data
						int i, j, n;
						for (n = 0; n < z->s->img_n; ++n) {
							int w = (z->img_comp[n].x + 7) >> 3;
							int h = (z->img_comp[n].y + 7) >> 3;
							for (j = 0; j < h; ++j) {
								for (i = 0; i < w; ++i) {
									short *data = z->img_comp[n].coeff + 64 * (i + j * z->img_comp[n].coeff_w);
									stbi__jpeg_dequantize(data, z->dequant[z->img_comp[n].tq]);
									z->idct_block_kernel(z->img_comp[n].data + z->img_comp[n].w2*j * 8 + i * 8, z->img_comp[n].w2, data);
								}
							}
						}
					}
				}

				int stbi__process_marker(stbi__jpeg *z, int m)
				{
					int L;
					switch (m) {
					case STBI__MARKER_none: // no marker found
						return stbi__err("expected marker", "Corrupt JPEG");

					case 0xDD: // DRI - specify restart interval
						if (stbi__get16be(z->s) != 4) return stbi__err("bad DRI len", "Corrupt JPEG");
						z->restart_interval = stbi__get16be(z->s);
						return 1;

					case 0xDB: // DQT - define quantization table
						L = stbi__get16be(z->s) - 2;
						while (L > 0) {
							int q = stbi__get8(z->s);
							int p = q >> 4;
							int t = q & 15, i;
							if (p != 0) return stbi__err("bad DQT type", "Corrupt JPEG");
							if (t > 3) return stbi__err("bad DQT table", "Corrupt JPEG");
							for (i = 0; i < 64; ++i)
								z->dequant[t][stbi__jpeg_dezigzag[i]] = stbi__get8(z->s);
							L -= 65;
						}
						return L == 0;

					case 0xC4: // DHT - define huffman table
						L = stbi__get16be(z->s) - 2;
						while (L > 0) {
							stbi_uc *v;
							int sizes[16], i, n = 0;
							int q = stbi__get8(z->s);
							int tc = q >> 4;
							int th = q & 15;
							if (tc > 1 || th > 3) return stbi__err("bad DHT header", "Corrupt JPEG");
							for (i = 0; i < 16; ++i) {
								sizes[i] = stbi__get8(z->s);
								n += sizes[i];
							}
							L -= 17;
							if (tc == 0) {
								if (!stbi__build_huffman(z->huff_dc + th, sizes)) return 0;
								v = z->huff_dc[th].values;
							}
							else {
								if (!stbi__build_huffman(z->huff_ac + th, sizes)) return 0;
								v = z->huff_ac[th].values;
							}
							for (i = 0; i < n; ++i)
								v[i] = stbi__get8(z->s);
							if (tc != 0)
								stbi__build_fast_ac(z->fast_ac[th], z->huff_ac + th);
							L -= n;
						}
						return L == 0;
					}
					// check for comment block or APP blocks
					if ((m >= 0xE0 && m <= 0xEF) || m == 0xFE) {
						stbi__skip(z->s, stbi__get16be(z->s) - 2);
						return 1;
					}
					return 0;
				}

				// after we see SOS
				int stbi__process_scan_header(stbi__jpeg *z)
				{
					int i;
					int Ls = stbi__get16be(z->s);
					z->scan_n = stbi__get8(z->s);
					if (z->scan_n < 1 || z->scan_n > 4 || z->scan_n > (int)z->s->img_n) return stbi__err("bad SOS component count", "Corrupt JPEG");
					if (Ls != 6 + 2 * z->scan_n) return stbi__err("bad SOS len", "Corrupt JPEG");
					for (i = 0; i < z->scan_n; ++i) {
						int id = stbi__get8(z->s), which;
						int q = stbi__get8(z->s);
						for (which = 0; which < z->s->img_n; ++which)
						if (z->img_comp[which].id == id)
							break;
						if (which == z->s->img_n) return 0; // no match
						z->img_comp[which].hd = q >> 4;   if (z->img_comp[which].hd > 3) return stbi__err("bad DC huff", "Corrupt JPEG");
						z->img_comp[which].ha = q & 15;   if (z->img_comp[which].ha > 3) return stbi__err("bad AC huff", "Corrupt JPEG");
						z->order[i] = which;
					}

					{
						int aa;
						z->spec_start = stbi__get8(z->s);
						z->spec_end = stbi__get8(z->s); // should be 63, but might be 0
						aa = stbi__get8(z->s);
						z->succ_high = (aa >> 4);
						z->succ_low = (aa & 15);
						if (z->progressive) {
							if (z->spec_start > 63 || z->spec_end > 63 || z->spec_start > z->spec_end || z->succ_high > 13 || z->succ_low > 13)
								return stbi__err("bad SOS", "Corrupt JPEG");
						}
						else {
							if (z->spec_start != 0) return stbi__err("bad SOS", "Corrupt JPEG");
							if (z->succ_high != 0 || z->succ_low != 0) return stbi__err("bad SOS", "Corrupt JPEG");
							z->spec_end = 63;
						}
					}

					return 1;
				}

				int stbi__process_frame_header(stbi__jpeg *z, int scan)
				{
					stbi__context *s = z->s;
					int Lf, p, i, q, h_max = 1, v_max = 1, c;
					Lf = stbi__get16be(s);         if (Lf < 11) return stbi__err("bad SOF len", "Corrupt JPEG"); // JPEG
					p = stbi__get8(s);            if (p != 8) return stbi__err("only 8-bit", "JPEG format not supported: 8-bit only"); // JPEG baseline
					s->img_y = stbi__get16be(s);   if (s->img_y == 0) return stbi__err("no header height", "JPEG format not supported: delayed height"); // Legal, but we don't handle it--but neither does IJG
					s->img_x = stbi__get16be(s);   if (s->img_x == 0) return stbi__err("0 width", "Corrupt JPEG"); // JPEG requires
					c = stbi__get8(s);
					if (c != 3 && c != 1) return stbi__err("bad component count", "Corrupt JPEG");    // JFIF requires
					s->img_n = c;
					for (i = 0; i < c; ++i) {
						z->img_comp[i].data = NULL;
						z->img_comp[i].linebuf = NULL;
					}

					if (Lf != 8 + 3 * s->img_n) return stbi__err("bad SOF len", "Corrupt JPEG");

					for (i = 0; i < s->img_n; ++i) {
						z->img_comp[i].id = stbi__get8(s);
						if (z->img_comp[i].id != i + 1)   // JFIF requires
						if (z->img_comp[i].id != i)  // some version of jpegtran outputs non-JFIF-compliant files!
							return stbi__err("bad component ID", "Corrupt JPEG");
						q = stbi__get8(s);
						z->img_comp[i].h = (q >> 4);  if (!z->img_comp[i].h || z->img_comp[i].h > 4) return stbi__err("bad H", "Corrupt JPEG");
						z->img_comp[i].v = q & 15;    if (!z->img_comp[i].v || z->img_comp[i].v > 4) return stbi__err("bad V", "Corrupt JPEG");
						z->img_comp[i].tq = stbi__get8(s);  if (z->img_comp[i].tq > 3) return stbi__err("bad TQ", "Corrupt JPEG");
					}

					if (scan != STBI__SCAN_load) return 1;

					if ((1 << 30) / s->img_x / s->img_n < s->img_y) return stbi__err("too large", "Image too large to decode");

					for (i = 0; i < s->img_n; ++i) {
						if (z->img_comp[i].h > h_max) h_max = z->img_comp[i].h;
						if (z->img_comp[i].v > v_max) v_max = z->img_comp[i].v;
					}

					// compute interleaved mcu info
					z->img_h_max = h_max;
					z->img_v_max = v_max;
					z->img_mcu_w = h_max * 8;
					z->img_mcu_h = v_max * 8;
					z->img_mcu_x = (s->img_x + z->img_mcu_w - 1) / z->img_mcu_w;
					z->img_mcu_y = (s->img_y + z->img_mcu_h - 1) / z->img_mcu_h;

					for (i = 0; i < s->img_n; ++i) {
						// number of effective pixels (e.g. for non-interleaved MCU)
						z->img_comp[i].x = (s->img_x * z->img_comp[i].h + h_max - 1) / h_max;
						z->img_comp[i].y = (s->img_y * z->img_comp[i].v + v_max - 1) / v_max;
						// to simplify generation, we'll allocate enough memory to decode
						// the bogus oversized data from using interleaved MCUs and their
						// big blocks (e.g. a 16x16 iMCU on an image of width 33); we won't
						// discard the extra data until colorspace conversion
						z->img_comp[i].w2 = z->img_mcu_x * z->img_comp[i].h * 8;
						z->img_comp[i].h2 = z->img_mcu_y * z->img_comp[i].v * 8;
						z->img_comp[i].raw_data = stbi__malloc(z->img_comp[i].w2 * z->img_comp[i].h2 + 15);

						if (z->img_comp[i].raw_data == NULL) {
							for (--i; i >= 0; --i) {
								STBI_FREE(z->img_comp[i].raw_data);
								z->img_comp[i].raw_data = NULL;
							}
							return stbi__err("outofmem", "Out of memory");
						}
						// align blocks for idct using mmx/sse
						z->img_comp[i].data = (stbi_uc*)(((size_t)z->img_comp[i].raw_data + 15) & ~15);
						z->img_comp[i].linebuf = NULL;
						if (z->progressive) {
							z->img_comp[i].coeff_w = (z->img_comp[i].w2 + 7) >> 3;
							z->img_comp[i].coeff_h = (z->img_comp[i].h2 + 7) >> 3;
							z->img_comp[i].raw_coeff = STBI_MALLOC(z->img_comp[i].coeff_w * z->img_comp[i].coeff_h * 64 * sizeof(short)+15);
							z->img_comp[i].coeff = (short*)(((size_t)z->img_comp[i].raw_coeff + 15) & ~15);
						}
						else {
							z->img_comp[i].coeff = 0;
							z->img_comp[i].raw_coeff = 0;
						}
					}

					return 1;
				}

				// use comparisons since in some cases we handle more than one case (e.g. SOF)
#define stbi__DNL(x)         ((x) == 0xdc)
#define stbi__SOI(x)         ((x) == 0xd8)
#define stbi__EOI(x)         ((x) == 0xd9)
#define stbi__SOF(x)         ((x) == 0xc0 || (x) == 0xc1 || (x) == 0xc2)
#define stbi__SOS(x)         ((x) == 0xda)

#define stbi__SOF_progressive(x)   ((x) == 0xc2)

				int stbi__decode_jpeg_header(stbi__jpeg *z, int scan)
				{
					int m;
					z->marker = STBI__MARKER_none; // initialize cached marker to empty
					m = stbi__get_marker(z);
					if (!stbi__SOI(m)) return stbi__err("no SOI", "Corrupt JPEG");
					if (scan == STBI__SCAN_type) return 1;
					m = stbi__get_marker(z);
					while (!stbi__SOF(m)) {
						if (!stbi__process_marker(z, m)) return 0;
						m = stbi__get_marker(z);
						while (m == STBI__MARKER_none) {
							// some files have extra padding after their blocks, so ok, we'll scan
							if (stbi__at_eof(z->s)) return stbi__err("no SOF", "Corrupt JPEG");
							m = stbi__get_marker(z);
						}
					}
					z->progressive = stbi__SOF_progressive(m);
					if (!stbi__process_frame_header(z, scan)) return 0;
					return 1;
				}

				// decode image to YCbCr format
				int stbi__decode_jpeg_image(stbi__jpeg *j)
				{
					int m;
					for (m = 0; m < 4; m++) {
						j->img_comp[m].raw_data = NULL;
						j->img_comp[m].raw_coeff = NULL;
					}
					j->restart_interval = 0;
					if (!stbi__decode_jpeg_header(j, STBI__SCAN_load)) return 0;
					m = stbi__get_marker(j);
					while (!stbi__EOI(m)) {
						if (stbi__SOS(m)) {
							if (!stbi__process_scan_header(j)) return 0;
							if (!stbi__parse_entropy_coded_data(j)) return 0;
							if (j->marker == STBI__MARKER_none) {
								// handle 0s at the end of image data from IP Kamera 9060
								while (!stbi__at_eof(j->s)) {
									int x = stbi__get8(j->s);
									if (x == 255) {
										j->marker = stbi__get8(j->s);
										break;
									}
									else if (x != 0) {
										return stbi__err("junk before marker", "Corrupt JPEG");
									}
								}
								// if we reach eof without hitting a marker, stbi__get_marker() below will fail and we'll eventually return 0
							}
						}
						else {
							if (!stbi__process_marker(j, m)) return 0;
						}
						m = stbi__get_marker(j);
					}
					if (j->progressive)
						stbi__jpeg_finish(j);
					return 1;
				}

				//  jfif-centered resampling (across block boundaries)

				typedef stbi_uc *(*resample_row_func)(stbi_uc *out, stbi_uc *in0, stbi_uc *in1,
					int w, int hs);

#define stbi__div4(x) ((stbi_uc) ((x) >> 2))

				stbi_uc *resample_row_1(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
				{
					STBI_NOTUSED(out);
					STBI_NOTUSED(in_far);
					STBI_NOTUSED(w);
					STBI_NOTUSED(hs);
					return in_near;
				}

				stbi_uc* stbi__resample_row_v_2(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
				{
					// need to generate two samples vertically for every one in input
					int i;
					STBI_NOTUSED(hs);
					for (i = 0; i < w; ++i)
						out[i] = stbi__div4(3 * in_near[i] + in_far[i] + 2);
					return out;
				}

				stbi_uc*  stbi__resample_row_h_2(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
				{
					// need to generate two samples horizontally for every one in input
					int i;
					stbi_uc *input = in_near;

					if (w == 1) {
						// if only one sample, can't do any interpolation
						out[0] = out[1] = input[0];
						return out;
					}

					out[0] = input[0];
					out[1] = stbi__div4(input[0] * 3 + input[1] + 2);
					for (i = 1; i < w - 1; ++i) {
						int n = 3 * input[i] + 2;
						out[i * 2 + 0] = stbi__div4(n + input[i - 1]);
						out[i * 2 + 1] = stbi__div4(n + input[i + 1]);
					}
					out[i * 2 + 0] = stbi__div4(input[w - 2] * 3 + input[w - 1] + 2);
					out[i * 2 + 1] = input[w - 1];

					STBI_NOTUSED(in_far);
					STBI_NOTUSED(hs);

					return out;
				}

#define stbi__div16(x) ((stbi_uc) ((x) >> 4))

				stbi_uc *stbi__resample_row_hv_2(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
				{
					// need to generate 2x2 samples for every one in input
					int i, t0, t1;
					if (w == 1) {
						out[0] = out[1] = stbi__div4(3 * in_near[0] + in_far[0] + 2);
						return out;
					}

					t1 = 3 * in_near[0] + in_far[0];
					out[0] = stbi__div4(t1 + 2);
					for (i = 1; i < w; ++i) {
						t0 = t1;
						t1 = 3 * in_near[i] + in_far[i];
						out[i * 2 - 1] = stbi__div16(3 * t0 + t1 + 8);
						out[i * 2] = stbi__div16(3 * t1 + t0 + 8);
					}
					out[w * 2 - 1] = stbi__div4(t1 + 2);

					STBI_NOTUSED(hs);

					return out;
				}

#if defined(STBI_SSE2) || defined(STBI_NEON)
				stbi_uc *stbi__resample_row_hv_2_simd(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
				{
					// need to generate 2x2 samples for every one in input
					int i = 0, t0, t1;

					if (w == 1) {
						out[0] = out[1] = stbi__div4(3 * in_near[0] + in_far[0] + 2);
						return out;
					}

					t1 = 3 * in_near[0] + in_far[0];
					// process groups of 8 pixels for as long as we can.
					// note we can't handle the last pixel in a row in this loop
					// because we need to handle the filter boundary conditions.
					for (; i < ((w - 1) & ~7); i += 8) {
#if defined(STBI_SSE2)
						// load and perform the vertical filtering pass
						// this uses 3*x + y = 4*x + (y - x)
						__m128i zero = _mm_setzero_si128();
						__m128i farb = _mm_loadl_epi64((__m128i *) (in_far + i));
						__m128i nearb = _mm_loadl_epi64((__m128i *) (in_near + i));
						__m128i farw = _mm_unpacklo_epi8(farb, zero);
						__m128i nearw = _mm_unpacklo_epi8(nearb, zero);
						__m128i diff = _mm_sub_epi16(farw, nearw);
						__m128i nears = _mm_slli_epi16(nearw, 2);
						__m128i curr = _mm_add_epi16(nears, diff); // current row

						// horizontal filter works the same based on shifted vers of current
						// row. "prev" is current row shifted right by 1 pixel; we need to
						// insert the previous pixel value (from t1).
						// "next" is current row shifted left by 1 pixel, with first pixel
						// of next block of 8 pixels added in.
						__m128i prv0 = _mm_slli_si128(curr, 2);
						__m128i nxt0 = _mm_srli_si128(curr, 2);
						__m128i prev = _mm_insert_epi16(prv0, t1, 0);
						__m128i next = _mm_insert_epi16(nxt0, 3 * in_near[i + 8] + in_far[i + 8], 7);

						// horizontal filter, polyphase implementation since it's convenient:
						// even pixels = 3*cur + prev = cur*4 + (prev - cur)
						// odd  pixels = 3*cur + next = cur*4 + (next - cur)
						// note the shared term.
						__m128i bias = _mm_set1_epi16(8);
						__m128i curs = _mm_slli_epi16(curr, 2);
						__m128i prvd = _mm_sub_epi16(prev, curr);
						__m128i nxtd = _mm_sub_epi16(next, curr);
						__m128i curb = _mm_add_epi16(curs, bias);
						__m128i even = _mm_add_epi16(prvd, curb);
						__m128i odd = _mm_add_epi16(nxtd, curb);

						// interleave even and odd pixels, then undo scaling.
						__m128i int0 = _mm_unpacklo_epi16(even, odd);
						__m128i int1 = _mm_unpackhi_epi16(even, odd);
						__m128i de0 = _mm_srli_epi16(int0, 4);
						__m128i de1 = _mm_srli_epi16(int1, 4);

						// pack and write output
						__m128i outv = _mm_packus_epi16(de0, de1);
						_mm_storeu_si128((__m128i *) (out + i * 2), outv);
#elif defined(STBI_NEON)
						// load and perform the vertical filtering pass
						// this uses 3*x + y = 4*x + (y - x)
						uint8x8_t farb = vld1_u8(in_far + i);
						uint8x8_t nearb = vld1_u8(in_near + i);
						int16x8_t diff = vreinterpretq_s16_u16(vsubl_u8(farb, nearb));
						int16x8_t nears = vreinterpretq_s16_u16(vshll_n_u8(nearb, 2));
						int16x8_t curr = vaddq_s16(nears, diff); // current row

						// horizontal filter works the same based on shifted vers of current
						// row. "prev" is current row shifted right by 1 pixel; we need to
						// insert the previous pixel value (from t1).
						// "next" is current row shifted left by 1 pixel, with first pixel
						// of next block of 8 pixels added in.
						int16x8_t prv0 = vextq_s16(curr, curr, 7);
						int16x8_t nxt0 = vextq_s16(curr, curr, 1);
						int16x8_t prev = vsetq_lane_s16(t1, prv0, 0);
						int16x8_t next = vsetq_lane_s16(3 * in_near[i + 8] + in_far[i + 8], nxt0, 7);

						// horizontal filter, polyphase implementation since it's convenient:
						// even pixels = 3*cur + prev = cur*4 + (prev - cur)
						// odd  pixels = 3*cur + next = cur*4 + (next - cur)
						// note the shared term.
						int16x8_t curs = vshlq_n_s16(curr, 2);
						int16x8_t prvd = vsubq_s16(prev, curr);
						int16x8_t nxtd = vsubq_s16(next, curr);
						int16x8_t even = vaddq_s16(curs, prvd);
						int16x8_t odd = vaddq_s16(curs, nxtd);

						// undo scaling and round, then store with even/odd phases interleaved
						uint8x8x2_t o;
						o.val[0] = vqrshrun_n_s16(even, 4);
						o.val[1] = vqrshrun_n_s16(odd, 4);
						vst2_u8(out + i * 2, o);
#endif

						// "previous" value for next iter
						t1 = 3 * in_near[i + 7] + in_far[i + 7];
					}

					t0 = t1;
					t1 = 3 * in_near[i] + in_far[i];
					out[i * 2] = stbi__div16(3 * t1 + t0 + 8);

					for (++i; i < w; ++i) {
						t0 = t1;
						t1 = 3 * in_near[i] + in_far[i];
						out[i * 2 - 1] = stbi__div16(3 * t0 + t1 + 8);
						out[i * 2] = stbi__div16(3 * t1 + t0 + 8);
					}
					out[w * 2 - 1] = stbi__div4(t1 + 2);

					STBI_NOTUSED(hs);

					return out;
				}
#endif

				stbi_uc *stbi__resample_row_generic(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
				{
					// resample with nearest-neighbor
					int i, j;
					STBI_NOTUSED(in_far);
					for (i = 0; i < w; ++i)
					for (j = 0; j < hs; ++j)
						out[i*hs + j] = in_near[i];
					return out;
				}

#ifdef STBI_JPEG_OLD
				// this is the same YCbCr-to-RGB calculation that stb_image has used
				// historically before the algorithm changes in 1.49
#define float2fixed(x)  ((int) ((x) * 65536 + 0.5))
				void stbi__YCbCr_to_RGB_row(stbi_uc *out, const stbi_uc *y, const stbi_uc *pcb, const stbi_uc *pcr, int count, int step)
				{
					int i;
					for (i = 0; i < count; ++i) {
						int y_fixed = (y[i] << 16) + 32768; // rounding
						int r, g, b;
						int cr = pcr[i] - 128;
						int cb = pcb[i] - 128;
						r = y_fixed + cr*float2fixed(1.40200f);
						g = y_fixed - cr*float2fixed(0.71414f) - cb*float2fixed(0.34414f);
						b = y_fixed + cb*float2fixed(1.77200f);
						r >>= 16;
						g >>= 16;
						b >>= 16;
						if ((unsigned)r > 255) { if (r < 0) r = 0; else r = 255; }
						if ((unsigned)g > 255) { if (g < 0) g = 0; else g = 255; }
						if ((unsigned)b > 255) { if (b < 0) b = 0; else b = 255; }
						out[0] = (stbi_uc)r;
						out[1] = (stbi_uc)g;
						out[2] = (stbi_uc)b;
						out[3] = 255;
						out += step;
					}
				}
#else
				// this is a reduced-precision calculation of YCbCr-to-RGB introduced
				// to make sure the code produces the same results in both SIMD and scalar
#define float2fixed(x)  (((int) ((x) * 4096.0f + 0.5f)) << 8)
				void stbi__YCbCr_to_RGB_row(stbi_uc *out, const stbi_uc *y, const stbi_uc *pcb, const stbi_uc *pcr, int count, int step)
				{
					int i;
					for (i = 0; i < count; ++i) {
						int y_fixed = (y[i] << 20) + (1 << 19); // rounding
						int r, g, b;
						int cr = pcr[i] - 128;
						int cb = pcb[i] - 128;
						r = y_fixed + cr* float2fixed(1.40200f);
						g = y_fixed + (cr*-float2fixed(0.71414f)) + ((cb*-float2fixed(0.34414f)) & 0xffff0000);
						b = y_fixed + cb* float2fixed(1.77200f);
						r >>= 20;
						g >>= 20;
						b >>= 20;
						if ((unsigned)r > 255) { if (r < 0) r = 0; else r = 255; }
						if ((unsigned)g > 255) { if (g < 0) g = 0; else g = 255; }
						if ((unsigned)b > 255) { if (b < 0) b = 0; else b = 255; }
						out[0] = (stbi_uc)r;
						out[1] = (stbi_uc)g;
						out[2] = (stbi_uc)b;
						out[3] = 255;
						out += step;
					}
				}
#endif

#if defined(STBI_SSE2) || defined(STBI_NEON)
				void stbi__YCbCr_to_RGB_simd(stbi_uc *out, stbi_uc const *y, stbi_uc const *pcb, stbi_uc const *pcr, int count, int step)
				{
					int i = 0;

#ifdef STBI_SSE2
					// step == 3 is pretty ugly on the final interleave, and i'm not convinced
					// it's useful in practice (you wouldn't use it for textures, for example).
					// so just accelerate step == 4 case.
					if (step == 4) {
						// this is a fairly straightforward implementation and not super-optimized.
						__m128i signflip = _mm_set1_epi8(-0x80);
						__m128i cr_const0 = _mm_set1_epi16((short)(1.40200f*4096.0f + 0.5f));
						__m128i cr_const1 = _mm_set1_epi16(-(short)(0.71414f*4096.0f + 0.5f));
						__m128i cb_const0 = _mm_set1_epi16(-(short)(0.34414f*4096.0f + 0.5f));
						__m128i cb_const1 = _mm_set1_epi16((short)(1.77200f*4096.0f + 0.5f));
						__m128i y_bias = _mm_set1_epi8((char)(unsigned char)128);
						__m128i xw = _mm_set1_epi16(255); // alpha channel

						for (; i + 7 < count; i += 8) {
							// load
							__m128i y_bytes = _mm_loadl_epi64((__m128i *) (y + i));
							__m128i cr_bytes = _mm_loadl_epi64((__m128i *) (pcr + i));
							__m128i cb_bytes = _mm_loadl_epi64((__m128i *) (pcb + i));
							__m128i cr_biased = _mm_xor_si128(cr_bytes, signflip); // -128
							__m128i cb_biased = _mm_xor_si128(cb_bytes, signflip); // -128

							// unpack to short (and left-shift cr, cb by 8)
							__m128i yw = _mm_unpacklo_epi8(y_bias, y_bytes);
							__m128i crw = _mm_unpacklo_epi8(_mm_setzero_si128(), cr_biased);
							__m128i cbw = _mm_unpacklo_epi8(_mm_setzero_si128(), cb_biased);

							// color transform
							__m128i yws = _mm_srli_epi16(yw, 4);
							__m128i cr0 = _mm_mulhi_epi16(cr_const0, crw);
							__m128i cb0 = _mm_mulhi_epi16(cb_const0, cbw);
							__m128i cb1 = _mm_mulhi_epi16(cbw, cb_const1);
							__m128i cr1 = _mm_mulhi_epi16(crw, cr_const1);
							__m128i rws = _mm_add_epi16(cr0, yws);
							__m128i gwt = _mm_add_epi16(cb0, yws);
							__m128i bws = _mm_add_epi16(yws, cb1);
							__m128i gws = _mm_add_epi16(gwt, cr1);

							// descale
							__m128i rw = _mm_srai_epi16(rws, 4);
							__m128i bw = _mm_srai_epi16(bws, 4);
							__m128i gw = _mm_srai_epi16(gws, 4);

							// back to byte, set up for transpose
							__m128i brb = _mm_packus_epi16(rw, bw);
							__m128i gxb = _mm_packus_epi16(gw, xw);

							// transpose to interleave channels
							__m128i t0 = _mm_unpacklo_epi8(brb, gxb);
							__m128i t1 = _mm_unpackhi_epi8(brb, gxb);
							__m128i o0 = _mm_unpacklo_epi16(t0, t1);
							__m128i o1 = _mm_unpackhi_epi16(t0, t1);

							// store
							_mm_storeu_si128((__m128i *) (out + 0), o0);
							_mm_storeu_si128((__m128i *) (out + 16), o1);
							out += 32;
						}
					}
#endif

#ifdef STBI_NEON
					// in this version, step=3 support would be easy to add. but is there demand?
					if (step == 4) {
						// this is a fairly straightforward implementation and not super-optimized.
						uint8x8_t signflip = vdup_n_u8(0x80);
						int16x8_t cr_const0 = vdupq_n_s16((short)(1.40200f*4096.0f + 0.5f));
						int16x8_t cr_const1 = vdupq_n_s16(-(short)(0.71414f*4096.0f + 0.5f));
						int16x8_t cb_const0 = vdupq_n_s16(-(short)(0.34414f*4096.0f + 0.5f));
						int16x8_t cb_const1 = vdupq_n_s16((short)(1.77200f*4096.0f + 0.5f));

						for (; i + 7 < count; i += 8) {
							// load
							uint8x8_t y_bytes = vld1_u8(y + i);
							uint8x8_t cr_bytes = vld1_u8(pcr + i);
							uint8x8_t cb_bytes = vld1_u8(pcb + i);
							int8x8_t cr_biased = vreinterpret_s8_u8(vsub_u8(cr_bytes, signflip));
							int8x8_t cb_biased = vreinterpret_s8_u8(vsub_u8(cb_bytes, signflip));

							// expand to s16
							int16x8_t yws = vreinterpretq_s16_u16(vshll_n_u8(y_bytes, 4));
							int16x8_t crw = vshll_n_s8(cr_biased, 7);
							int16x8_t cbw = vshll_n_s8(cb_biased, 7);

							// color transform
							int16x8_t cr0 = vqdmulhq_s16(crw, cr_const0);
							int16x8_t cb0 = vqdmulhq_s16(cbw, cb_const0);
							int16x8_t cr1 = vqdmulhq_s16(crw, cr_const1);
							int16x8_t cb1 = vqdmulhq_s16(cbw, cb_const1);
							int16x8_t rws = vaddq_s16(yws, cr0);
							int16x8_t gws = vaddq_s16(vaddq_s16(yws, cb0), cr1);
							int16x8_t bws = vaddq_s16(yws, cb1);

							// undo scaling, round, convert to byte
							uint8x8x4_t o;
							o.val[0] = vqrshrun_n_s16(rws, 4);
							o.val[1] = vqrshrun_n_s16(gws, 4);
							o.val[2] = vqrshrun_n_s16(bws, 4);
							o.val[3] = vdup_n_u8(255);

							// store, interleaving r/g/b/a
							vst4_u8(out, o);
							out += 8 * 4;
						}
					}
#endif

					for (; i < count; ++i) {
						int y_fixed = (y[i] << 20) + (1 << 19); // rounding
						int r, g, b;
						int cr = pcr[i] - 128;
						int cb = pcb[i] - 128;
						r = y_fixed + cr* float2fixed(1.40200f);
						g = y_fixed + cr*-float2fixed(0.71414f) + ((cb*-float2fixed(0.34414f)) & 0xffff0000);
						b = y_fixed + cb* float2fixed(1.77200f);
						r >>= 20;
						g >>= 20;
						b >>= 20;
						if ((unsigned)r > 255) { if (r < 0) r = 0; else r = 255; }
						if ((unsigned)g > 255) { if (g < 0) g = 0; else g = 255; }
						if ((unsigned)b > 255) { if (b < 0) b = 0; else b = 255; }
						out[0] = (stbi_uc)r;
						out[1] = (stbi_uc)g;
						out[2] = (stbi_uc)b;
						out[3] = 255;
						out += step;
					}
				}
#endif

				// set up the kernels
				void stbi__setup_jpeg(stbi__jpeg *j)
				{
					j->idct_block_kernel = stbi__idct_block;
					j->YCbCr_to_RGB_kernel = stbi__YCbCr_to_RGB_row;
					j->resample_row_hv_2_kernel = stbi__resample_row_hv_2;

#ifdef STBI_SSE2
					if (stbi__sse2_available()) {
						j->idct_block_kernel = stbi__idct_simd;
#ifndef STBI_JPEG_OLD
						j->YCbCr_to_RGB_kernel = stbi__YCbCr_to_RGB_simd;
#endif
						j->resample_row_hv_2_kernel = stbi__resample_row_hv_2_simd;
					}
#endif

#ifdef STBI_NEON
					j->idct_block_kernel = stbi__idct_simd;
#ifndef STBI_JPEG_OLD
					j->YCbCr_to_RGB_kernel = stbi__YCbCr_to_RGB_simd;
#endif
					j->resample_row_hv_2_kernel = stbi__resample_row_hv_2_simd;
#endif
				}

				// clean up the temporary component buffers
				void stbi__cleanup_jpeg(stbi__jpeg *j)
				{
					int i;
					for (i = 0; i < j->s->img_n; ++i) {
						if (j->img_comp[i].raw_data) {
							STBI_FREE(j->img_comp[i].raw_data);
							j->img_comp[i].raw_data = NULL;
							j->img_comp[i].data = NULL;
						}
						if (j->img_comp[i].raw_coeff) {
							STBI_FREE(j->img_comp[i].raw_coeff);
							j->img_comp[i].raw_coeff = 0;
							j->img_comp[i].coeff = 0;
						}
						if (j->img_comp[i].linebuf) {
							STBI_FREE(j->img_comp[i].linebuf);
							j->img_comp[i].linebuf = NULL;
						}
					}
				}

				typedef struct
				{
					resample_row_func resample;
					stbi_uc *line0, *line1;
					int hs, vs;   // expansion factor in each axis
					int w_lores; // horizontal pixels pre-expansion
					int ystep;   // how far through vertical expansion we are
					int ypos;    // which pre-expansion row we're on
				} stbi__resample;

				stbi_uc *load_jpeg_image(stbi__jpeg *z, int *out_x, int *out_y, int *comp, int req_comp)
				{
					int n, decode_n;
					z->s->img_n = 0; // make stbi__cleanup_jpeg safe

					// validate req_comp
					if (req_comp < 0 || req_comp > 4) return stbi__errpuc("bad req_comp", "Internal error");

					// load a jpeg image from whichever source, but leave in YCbCr format
					if (!stbi__decode_jpeg_image(z)) { stbi__cleanup_jpeg(z); return NULL; }

					// determine actual number of components to generate
					n = req_comp ? req_comp : z->s->img_n;

					if (z->s->img_n == 3 && n < 3)
						decode_n = 1;
					else
						decode_n = z->s->img_n;

					// resample and color-convert
					{
						int k;
						unsigned int i, j;
						stbi_uc *output;
						stbi_uc *coutput[4];

						stbi__resample res_comp[4];

						for (k = 0; k < decode_n; ++k) {
							stbi__resample *r = &res_comp[k];

							// allocate line buffer big enough for upsampling off the edges
							// with upsample factor of 4
							z->img_comp[k].linebuf = (stbi_uc *)stbi__malloc(z->s->img_x + 3);
							if (!z->img_comp[k].linebuf) { stbi__cleanup_jpeg(z); return stbi__errpuc("outofmem", "Out of memory"); }

							r->hs = z->img_h_max / z->img_comp[k].h;
							r->vs = z->img_v_max / z->img_comp[k].v;
							r->ystep = r->vs >> 1;
							r->w_lores = (z->s->img_x + r->hs - 1) / r->hs;
							r->ypos = 0;
							r->line0 = r->line1 = z->img_comp[k].data;

							if (r->hs == 1 && r->vs == 1) r->resample = resample_row_1;
							else if (r->hs == 1 && r->vs == 2) r->resample = stbi__resample_row_v_2;
							else if (r->hs == 2 && r->vs == 1) r->resample = stbi__resample_row_h_2;
							else if (r->hs == 2 && r->vs == 2) r->resample = z->resample_row_hv_2_kernel;
							else                               r->resample = stbi__resample_row_generic;
						}

						// can't error after this so, this is safe
						output = (stbi_uc *)stbi__malloc(n * z->s->img_x * z->s->img_y + 1);
						if (!output) { stbi__cleanup_jpeg(z); return stbi__errpuc("outofmem", "Out of memory"); }

						// now go ahead and resample
						for (j = 0; j < z->s->img_y; ++j) {
							stbi_uc *out = output + n * z->s->img_x * j;
							for (k = 0; k < decode_n; ++k) {
								stbi__resample *r = &res_comp[k];
								int y_bot = r->ystep >= (r->vs >> 1);
								coutput[k] = r->resample(z->img_comp[k].linebuf,
									y_bot ? r->line1 : r->line0,
									y_bot ? r->line0 : r->line1,
									r->w_lores, r->hs);
								if (++r->ystep >= r->vs) {
									r->ystep = 0;
									r->line0 = r->line1;
									if (++r->ypos < z->img_comp[k].y)
										r->line1 += z->img_comp[k].w2;
								}
							}
							if (n >= 3) {
								stbi_uc *y = coutput[0];
								if (z->s->img_n == 3) {
									z->YCbCr_to_RGB_kernel(out, y, coutput[1], coutput[2], z->s->img_x, n);
								}
								else
								for (i = 0; i < z->s->img_x; ++i) {
									out[0] = out[1] = out[2] = y[i];
									out[3] = 255; // not used if n==3
									out += n;
								}
							}
							else {
								stbi_uc *y = coutput[0];
								if (n == 1)
								for (i = 0; i < z->s->img_x; ++i) out[i] = y[i];
								else
								for (i = 0; i < z->s->img_x; ++i) *out++ = y[i], *out++ = 255;
							}
						}
						stbi__cleanup_jpeg(z);
						*out_x = z->s->img_x;
						*out_y = z->s->img_y;
						if (comp) *comp = z->s->img_n; // report original components, not output
						return output;
					}
				}

				unsigned char *stbi__jpeg_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					stbi__jpeg j;
					j.s = s;
					stbi__setup_jpeg(&j);
					return load_jpeg_image(&j, x, y, comp, req_comp);
				}

				int stbi__jpeg_test(stbi__context *s)
				{
					int r;
					stbi__jpeg j;
					j.s = s;
					stbi__setup_jpeg(&j);
					r = stbi__decode_jpeg_header(&j, STBI__SCAN_type);
					stbi__rewind(s);
					return r;
				}

				int stbi__jpeg_info_raw(stbi__jpeg *j, int *x, int *y, int *comp)
				{
					if (!stbi__decode_jpeg_header(j, STBI__SCAN_header)) {
						stbi__rewind(j->s);
						return 0;
					}
					if (x) *x = j->s->img_x;
					if (y) *y = j->s->img_y;
					if (comp) *comp = j->s->img_n;
					return 1;
				}

				int stbi__jpeg_info(stbi__context *s, int *x, int *y, int *comp)
				{
					stbi__jpeg j;
					j.s = s;
					return stbi__jpeg_info_raw(&j, x, y, comp);
				}
#endif

				// public domain zlib decode    v0.2  Sean Barrett 2006-11-18
				//    simple implementation
				//      - all input must be provided in an upfront buffer
				//      - all output is written to a single output buffer (can malloc/realloc)
				//    performance
				//      - fast huffman

#ifndef STBI_NO_ZLIB

				// fast-way is faster to check than jpeg huffman, but slow way is slower
#define STBI__ZFAST_BITS  9 // accelerate all cases in default tables
#define STBI__ZFAST_MASK  ((1 << STBI__ZFAST_BITS) - 1)

				// zlib-style huffman encoding
				// (jpegs packs from left, zlib from right, so can't share code)
				typedef struct
				{
					stbi__uint16 fast[1 << STBI__ZFAST_BITS];
					stbi__uint16 firstcode[16];
					int maxcode[17];
					stbi__uint16 firstsymbol[16];
					stbi_uc  size[288];
					stbi__uint16 value[288];
				} stbi__zhuffman;

				stbi_inline  int stbi__bitreverse16(int n)
				{
					n = ((n & 0xAAAA) >> 1) | ((n & 0x5555) << 1);
					n = ((n & 0xCCCC) >> 2) | ((n & 0x3333) << 2);
					n = ((n & 0xF0F0) >> 4) | ((n & 0x0F0F) << 4);
					n = ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
					return n;
				}

				stbi_inline  int stbi__bit_reverse(int v, int bits)
				{
					STBI_ASSERT(bits <= 16);
					// to bit reverse n bits, reverse 16 and shift
					// e.g. 11 bits, bit reverse and shift away 5
					return stbi__bitreverse16(v) >> (16 - bits);
				}

				int stbi__zbuild_huffman(stbi__zhuffman *z, stbi_uc *sizelist, int num)
				{
					int i, k = 0;
					int code, next_code[16], sizes[17];

					// DEFLATE spec for generating codes
					memset(sizes, 0, sizeof(sizes));
					memset(z->fast, 0, sizeof(z->fast));
					for (i = 0; i < num; ++i)
						++sizes[sizelist[i]];
					sizes[0] = 0;
					for (i = 1; i < 16; ++i)
					if (sizes[i] >(1 << i))
						return stbi__err("bad sizes", "Corrupt PNG");
					code = 0;
					for (i = 1; i < 16; ++i) {
						next_code[i] = code;
						z->firstcode[i] = (stbi__uint16)code;
						z->firstsymbol[i] = (stbi__uint16)k;
						code = (code + sizes[i]);
						if (sizes[i])
						if (code - 1 >= (1 << i)) return stbi__err("bad codelengths", "Corrupt PNG");
						z->maxcode[i] = code << (16 - i); // preshift for inner loop
						code <<= 1;
						k += sizes[i];
					}
					z->maxcode[16] = 0x10000; // sentinel
					for (i = 0; i < num; ++i) {
						int s = sizelist[i];
						if (s) {
							int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
							stbi__uint16 fastv = (stbi__uint16)((s << 9) | i);
							z->size[c] = (stbi_uc)s;
							z->value[c] = (stbi__uint16)i;
							if (s <= STBI__ZFAST_BITS) {
								int j = stbi__bit_reverse(next_code[s], s);
								while (j < (1 << STBI__ZFAST_BITS)) {
									z->fast[j] = fastv;
									j += (1 << s);
								}
							}
							++next_code[s];
						}
					}
					return 1;
				}

				// zlib-from-memory implementation for PNG reading
				//    because PNG allows splitting the zlib stream arbitrarily,
				//    and it's annoying structurally to have PNG call ZLIB call PNG,
				//    we require PNG read all the IDATs and combine them into a single
				//    memory buffer

				typedef struct
				{
					stbi_uc *zbuffer, *zbuffer_end;
					int num_bits;
					stbi__uint32 code_buffer;

					char *zout;
					char *zout_start;
					char *zout_end;
					int   z_expandable;

					stbi__zhuffman z_length, z_distance;
				} stbi__zbuf;

				stbi_inline  stbi_uc stbi__zget8(stbi__zbuf *z)
				{
					if (z->zbuffer >= z->zbuffer_end) return 0;
					return *z->zbuffer++;
				}

				void stbi__fill_bits(stbi__zbuf *z)
				{
					do {
						STBI_ASSERT(z->code_buffer < (1U << z->num_bits));
						z->code_buffer |= (unsigned int)stbi__zget8(z) << z->num_bits;
						z->num_bits += 8;
					} while (z->num_bits <= 24);
				}

				stbi_inline  unsigned int stbi__zreceive(stbi__zbuf *z, int n)
				{
					unsigned int k;
					if (z->num_bits < n) stbi__fill_bits(z);
					k = z->code_buffer & ((1 << n) - 1);
					z->code_buffer >>= n;
					z->num_bits -= n;
					return k;
				}

				int stbi__zhuffman_decode_slowpath(stbi__zbuf *a, stbi__zhuffman *z)
				{
					int b, s, k;
					// not resolved by fast table, so compute it the slow way
					// use jpeg approach, which requires MSbits at top
					k = stbi__bit_reverse(a->code_buffer, 16);
					for (s = STBI__ZFAST_BITS + 1;; ++s)
					if (k < z->maxcode[s])
						break;
					if (s == 16) return -1; // invalid code!
					// code size is s, so:
					b = (k >> (16 - s)) - z->firstcode[s] + z->firstsymbol[s];
					STBI_ASSERT(z->size[b] == s);
					a->code_buffer >>= s;
					a->num_bits -= s;
					return z->value[b];
				}

				stbi_inline  int stbi__zhuffman_decode(stbi__zbuf *a, stbi__zhuffman *z)
				{
					int b, s;
					if (a->num_bits < 16) stbi__fill_bits(a);
					b = z->fast[a->code_buffer & STBI__ZFAST_MASK];
					if (b) {
						s = b >> 9;
						a->code_buffer >>= s;
						a->num_bits -= s;
						return b & 511;
					}
					return stbi__zhuffman_decode_slowpath(a, z);
				}

				int stbi__zexpand(stbi__zbuf *z, char *zout, int n)  // need to make room for n bytes
				{
					char *q;
					int cur, limit;
					z->zout = zout;
					if (!z->z_expandable) return stbi__err("output buffer limit", "Corrupt PNG");
					cur = (int)(z->zout - z->zout_start);
					limit = (int)(z->zout_end - z->zout_start);
					while (cur + n > limit)
						limit *= 2;
					q = (char *)STBI_REALLOC(z->zout_start, limit);
					if (q == NULL) return stbi__err("outofmem", "Out of memory");
					z->zout_start = q;
					z->zout = q + cur;
					z->zout_end = q + limit;
					return 1;
				}

				int stbi__zlength_base[31] = {
					3, 4, 5, 6, 7, 8, 9, 10, 11, 13,
					15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
					67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0 };

				int stbi__zlength_extra[31] =
				{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0 };

				int stbi__zdist_base[32] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
					257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0, 0 };

				int stbi__zdist_extra[32] =
				{ 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

				int stbi__parse_huffman_block(stbi__zbuf *a)
				{
					char *zout = a->zout;
					for (;;) {
						int z = stbi__zhuffman_decode(a, &a->z_length);
						if (z < 256) {
							if (z < 0) return stbi__err("bad huffman code", "Corrupt PNG"); // error in huffman codes
							if (zout >= a->zout_end) {
								if (!stbi__zexpand(a, zout, 1)) return 0;
								zout = a->zout;
							}
							*zout++ = (char)z;
						}
						else {
							stbi_uc *p;
							int len, dist;
							if (z == 256) {
								a->zout = zout;
								return 1;
							}
							z -= 257;
							len = stbi__zlength_base[z];
							if (stbi__zlength_extra[z]) len += stbi__zreceive(a, stbi__zlength_extra[z]);
							z = stbi__zhuffman_decode(a, &a->z_distance);
							if (z < 0) return stbi__err("bad huffman code", "Corrupt PNG");
							dist = stbi__zdist_base[z];
							if (stbi__zdist_extra[z]) dist += stbi__zreceive(a, stbi__zdist_extra[z]);
							if (zout - a->zout_start < dist) return stbi__err("bad dist", "Corrupt PNG");
							if (zout + len > a->zout_end) {
								if (!stbi__zexpand(a, zout, len)) return 0;
								zout = a->zout;
							}
							p = (stbi_uc *)(zout - dist);
							if (dist == 1) { // run of one byte; common in images.
								stbi_uc v = *p;
								if (len) { do *zout++ = v; while (--len); }
							}
							else {
								if (len) { do *zout++ = *p++; while (--len); }
							}
						}
					}
				}

				int stbi__compute_huffman_codes(stbi__zbuf *a)
				{
					stbi_uc length_dezigzag[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
					stbi__zhuffman z_codelength;
					stbi_uc lencodes[286 + 32 + 137];//padding for maximum single op
					stbi_uc codelength_sizes[19];
					int i, n;

					int hlit = stbi__zreceive(a, 5) + 257;
					int hdist = stbi__zreceive(a, 5) + 1;
					int hclen = stbi__zreceive(a, 4) + 4;

					memset(codelength_sizes, 0, sizeof(codelength_sizes));
					for (i = 0; i < hclen; ++i) {
						int s = stbi__zreceive(a, 3);
						codelength_sizes[length_dezigzag[i]] = (stbi_uc)s;
					}
					if (!stbi__zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;

					n = 0;
					while (n < hlit + hdist) {
						int c = stbi__zhuffman_decode(a, &z_codelength);
						if (c < 0 || c >= 19) return stbi__err("bad codelengths", "Corrupt PNG");
						if (c < 16)
							lencodes[n++] = (stbi_uc)c;
						else if (c == 16) {
							c = stbi__zreceive(a, 2) + 3;
							memset(lencodes + n, lencodes[n - 1], c);
							n += c;
						}
						else if (c == 17) {
							c = stbi__zreceive(a, 3) + 3;
							memset(lencodes + n, 0, c);
							n += c;
						}
						else {
							STBI_ASSERT(c == 18);
							c = stbi__zreceive(a, 7) + 11;
							memset(lencodes + n, 0, c);
							n += c;
						}
					}
					if (n != hlit + hdist) return stbi__err("bad codelengths", "Corrupt PNG");
					if (!stbi__zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
					if (!stbi__zbuild_huffman(&a->z_distance, lencodes + hlit, hdist)) return 0;
					return 1;
				}

				int stbi__parse_uncomperssed_block(stbi__zbuf *a)
				{
					stbi_uc header[4];
					int len, nlen, k;
					if (a->num_bits & 7)
						stbi__zreceive(a, a->num_bits & 7); // discard
					// drain the bit-packed data into header
					k = 0;
					while (a->num_bits > 0) {
						header[k++] = (stbi_uc)(a->code_buffer & 255); // suppress MSVC run-time check
						a->code_buffer >>= 8;
						a->num_bits -= 8;
					}
					STBI_ASSERT(a->num_bits == 0);
					// now fill header the normal way
					while (k < 4)
						header[k++] = stbi__zget8(a);
					len = header[1] * 256 + header[0];
					nlen = header[3] * 256 + header[2];
					if (nlen != (len ^ 0xffff)) return stbi__err("zlib corrupt", "Corrupt PNG");
					if (a->zbuffer + len > a->zbuffer_end) return stbi__err("read past buffer", "Corrupt PNG");
					if (a->zout + len > a->zout_end)
					if (!stbi__zexpand(a, a->zout, len)) return 0;
					memcpy(a->zout, a->zbuffer, len);
					a->zbuffer += len;
					a->zout += len;
					return 1;
				}

				int stbi__parse_zlib_header(stbi__zbuf *a)
				{
					int cmf = stbi__zget8(a);
					int cm = cmf & 15;
					/* int cinfo = cmf >> 4; */
					int flg = stbi__zget8(a);
					if ((cmf * 256 + flg) % 31 != 0) return stbi__err("bad zlib header", "Corrupt PNG"); // zlib spec
					if (flg & 32) return stbi__err("no preset dict", "Corrupt PNG"); // preset dictionary not allowed in png
					if (cm != 8) return stbi__err("bad compression", "Corrupt PNG"); // DEFLATE required for png
					// window = 1 << (8 + cinfo)... but who cares, we fully buffer output
					return 1;
				}

				// @TODO: should statically initialize these for optimal thread safety
				stbi_uc stbi__zdefault_length[288], stbi__zdefault_distance[32];
				void stbi__init_zdefaults(void)
				{
					int i;   // use <= to match clearly with spec
					for (i = 0; i <= 143; ++i)     stbi__zdefault_length[i] = 8;
					for (; i <= 255; ++i)     stbi__zdefault_length[i] = 9;
					for (; i <= 279; ++i)     stbi__zdefault_length[i] = 7;
					for (; i <= 287; ++i)     stbi__zdefault_length[i] = 8;

					for (i = 0; i <= 31; ++i)     stbi__zdefault_distance[i] = 5;
				}

				int stbi__parse_zlib(stbi__zbuf *a, int parse_header)
				{
					int final, type;
					if (parse_header)
					if (!stbi__parse_zlib_header(a)) return 0;
					a->num_bits = 0;
					a->code_buffer = 0;
					do {
						final = stbi__zreceive(a, 1);
						type = stbi__zreceive(a, 2);
						if (type == 0) {
							if (!stbi__parse_uncomperssed_block(a)) return 0;
						}
						else if (type == 3) {
							return 0;
						}
						else {
							if (type == 1) {
								// use fixed code lengths
								if (!stbi__zdefault_distance[31]) stbi__init_zdefaults();
								if (!stbi__zbuild_huffman(&a->z_length, stbi__zdefault_length, 288)) return 0;
								if (!stbi__zbuild_huffman(&a->z_distance, stbi__zdefault_distance, 32)) return 0;
							}
							else {
								if (!stbi__compute_huffman_codes(a)) return 0;
							}
							if (!stbi__parse_huffman_block(a)) return 0;
						}
					} while (!final);
					return 1;
				}

				int stbi__do_zlib(stbi__zbuf *a, char *obuf, int olen, int exp, int parse_header)
				{
					a->zout_start = obuf;
					a->zout = obuf;
					a->zout_end = obuf + olen;
					a->z_expandable = exp;

					return stbi__parse_zlib(a, parse_header);
				}

				char *stbi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen)
				{
					stbi__zbuf a;
					char *p = (char *)stbi__malloc(initial_size);
					if (p == NULL) return NULL;
					a.zbuffer = (stbi_uc *)buffer;
					a.zbuffer_end = (stbi_uc *)buffer + len;
					if (stbi__do_zlib(&a, p, initial_size, 1, 1)) {
						if (outlen) *outlen = (int)(a.zout - a.zout_start);
						return a.zout_start;
					}
					else {
						STBI_FREE(a.zout_start);
						return NULL;
					}
				}

				char *stbi_zlib_decode_malloc(char const *buffer, int len, int *outlen)
				{
					return stbi_zlib_decode_malloc_guesssize(buffer, len, 16384, outlen);
				}

				char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header)
				{
					stbi__zbuf a;
					char *p = (char *)stbi__malloc(initial_size);
					if (p == NULL) return NULL;
					a.zbuffer = (stbi_uc *)buffer;
					a.zbuffer_end = (stbi_uc *)buffer + len;
					if (stbi__do_zlib(&a, p, initial_size, 1, parse_header)) {
						if (outlen) *outlen = (int)(a.zout - a.zout_start);
						return a.zout_start;
					}
					else {
						STBI_FREE(a.zout_start);
						return NULL;
					}
				}

				int stbi_zlib_decode_buffer(char *obuffer, int olen, char const *ibuffer, int ilen)
				{
					stbi__zbuf a;
					a.zbuffer = (stbi_uc *)ibuffer;
					a.zbuffer_end = (stbi_uc *)ibuffer + ilen;
					if (stbi__do_zlib(&a, obuffer, olen, 0, 1))
						return (int)(a.zout - a.zout_start);
					else
						return -1;
				}

				char *stbi_zlib_decode_noheader_malloc(char const *buffer, int len, int *outlen)
				{
					stbi__zbuf a;
					char *p = (char *)stbi__malloc(16384);
					if (p == NULL) return NULL;
					a.zbuffer = (stbi_uc *)buffer;
					a.zbuffer_end = (stbi_uc *)buffer + len;
					if (stbi__do_zlib(&a, p, 16384, 1, 0)) {
						if (outlen) *outlen = (int)(a.zout - a.zout_start);
						return a.zout_start;
					}
					else {
						STBI_FREE(a.zout_start);
						return NULL;
					}
				}

				int stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen)
				{
					stbi__zbuf a;
					a.zbuffer = (stbi_uc *)ibuffer;
					a.zbuffer_end = (stbi_uc *)ibuffer + ilen;
					if (stbi__do_zlib(&a, obuffer, olen, 0, 0))
						return (int)(a.zout - a.zout_start);
					else
						return -1;
				}
#endif

				// public domain "baseline" PNG decoder   v0.10  Sean Barrett 2006-11-18
				//    simple implementation
				//      - only 8-bit samples
				//      - no CRC checking
				//      - allocates lots of intermediate memory
				//        - avoids problem of streaming data between subsystems
				//        - avoids explicit window management
				//    performance
				//      - uses stb_zlib, a PD zlib implementation with fast huffman decoding

#ifndef STBI_NO_PNG
				typedef struct
				{
					stbi__uint32 length;
					stbi__uint32 type;
				} stbi__pngchunk;

				stbi__pngchunk stbi__get_chunk_header(stbi__context *s)
				{
					stbi__pngchunk c;
					c.length = stbi__get32be(s);
					c.type = stbi__get32be(s);
					return c;
				}

				int stbi__check_png_header(stbi__context *s)
				{
					stbi_uc png_sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
					int i;
					for (i = 0; i < 8; ++i)
					if (stbi__get8(s) != png_sig[i]) return stbi__err("bad png sig", "Not a PNG");
					return 1;
				}

				typedef struct
				{
					stbi__context *s;
					stbi_uc *idata, *expanded, *out;
				} stbi__png;


				enum {
					STBI__F_none = 0,
					STBI__F_sub = 1,
					STBI__F_up = 2,
					STBI__F_avg = 3,
					STBI__F_paeth = 4,
					// synthetic filters used for first scanline to avoid needing a dummy row of 0s
					STBI__F_avg_first,
					STBI__F_paeth_first
				};

				stbi_uc first_row_filter[5] =
				{
					STBI__F_none,
					STBI__F_sub,
					STBI__F_none,
					STBI__F_avg_first,
					STBI__F_paeth_first
				};

				int stbi__paeth(int a, int b, int c)
				{
					int p = a + b - c;
					int pa = abs(p - a);
					int pb = abs(p - b);
					int pc = abs(p - c);
					if (pa <= pb && pa <= pc) return a;
					if (pb <= pc) return b;
					return c;
				}

				stbi_uc stbi__depth_scale_table[9] = { 0, 0xff, 0x55, 0, 0x11, 0, 0, 0, 0x01 };

				// create the png data from post-deflated data
				int stbi__create_png_image_raw(stbi__png *a, stbi_uc *raw, stbi__uint32 raw_len, int out_n, stbi__uint32 x, stbi__uint32 y, int depth, int color)
				{
					stbi__context *s = a->s;
					stbi__uint32 i, j, stride = x*out_n;
					stbi__uint32 img_len, img_width_bytes;
					int k;
					int img_n = s->img_n; // copy it into a local for later

					STBI_ASSERT(out_n == s->img_n || out_n == s->img_n + 1);
					a->out = (stbi_uc *)stbi__malloc(x * y * out_n); // extra bytes to write off the end into
					if (!a->out) return stbi__err("outofmem", "Out of memory");

					img_width_bytes = (((img_n * x * depth) + 7) >> 3);
					img_len = (img_width_bytes + 1) * y;
					if (s->img_x == x && s->img_y == y) {
						if (raw_len != img_len) return stbi__err("not enough pixels", "Corrupt PNG");
					}
					else { // interlaced:
						if (raw_len < img_len) return stbi__err("not enough pixels", "Corrupt PNG");
					}

					for (j = 0; j < y; ++j) {
						stbi_uc *cur = a->out + stride*j;
						stbi_uc *prior = cur - stride;
						int filter = *raw++;
						int filter_bytes = img_n;
						int width = x;
						if (filter > 4)
							return stbi__err("invalid filter", "Corrupt PNG");

						if (depth < 8) {
							STBI_ASSERT(img_width_bytes <= x);
							cur += x*out_n - img_width_bytes; // store output to the rightmost img_len bytes, so we can decode in place
							filter_bytes = 1;
							width = img_width_bytes;
						}

						// if first row, use special filter that doesn't sample previous row
						if (j == 0) filter = first_row_filter[filter];

						// handle first byte explicitly
						for (k = 0; k < filter_bytes; ++k) {
							switch (filter) {
							case STBI__F_none: cur[k] = raw[k]; break;
							case STBI__F_sub: cur[k] = raw[k]; break;
							case STBI__F_up: cur[k] = STBI__BYTECAST(raw[k] + prior[k]); break;
							case STBI__F_avg: cur[k] = STBI__BYTECAST(raw[k] + (prior[k] >> 1)); break;
							case STBI__F_paeth: cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(0, prior[k], 0)); break;
							case STBI__F_avg_first: cur[k] = raw[k]; break;
							case STBI__F_paeth_first: cur[k] = raw[k]; break;
							}
						}

						if (depth == 8) {
							if (img_n != out_n)
								cur[img_n] = 255; // first pixel
							raw += img_n;
							cur += out_n;
							prior += out_n;
						}
						else {
							raw += 1;
							cur += 1;
							prior += 1;
						}

						// this is a little gross, so that we don't switch per-pixel or per-component
						if (depth < 8 || img_n == out_n) {
							int nk = (width - 1)*img_n;
#define CASE(f) \
					case f:     \
							for (k = 0; k < nk; ++k)
							switch (filter) {
								// "none" filter turns into a memcpy here; make that explicit.
							case STBI__F_none:         memcpy(cur, raw, nk); break;
								CASE(STBI__F_sub)          cur[k] = STBI__BYTECAST(raw[k] + cur[k - filter_bytes]); break;
								CASE(STBI__F_up)           cur[k] = STBI__BYTECAST(raw[k] + prior[k]); break;
								CASE(STBI__F_avg)          cur[k] = STBI__BYTECAST(raw[k] + ((prior[k] + cur[k - filter_bytes]) >> 1)); break;
								CASE(STBI__F_paeth)        cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - filter_bytes], prior[k], prior[k - filter_bytes])); break;
								CASE(STBI__F_avg_first)    cur[k] = STBI__BYTECAST(raw[k] + (cur[k - filter_bytes] >> 1)); break;
								CASE(STBI__F_paeth_first)  cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - filter_bytes], 0, 0)); break;
							}
#undef CASE
							raw += nk;
						}
						else {
							STBI_ASSERT(img_n + 1 == out_n);
#define CASE(f) \
					case f:     \
							for (i = x - 1; i >= 1; --i, cur[img_n] = 255, raw += img_n, cur += out_n, prior += out_n) \
							for (k = 0; k < img_n; ++k)
							switch (filter) {
								CASE(STBI__F_none)         cur[k] = raw[k]; break;
								CASE(STBI__F_sub)          cur[k] = STBI__BYTECAST(raw[k] + cur[k - out_n]); break;
								CASE(STBI__F_up)           cur[k] = STBI__BYTECAST(raw[k] + prior[k]); break;
								CASE(STBI__F_avg)          cur[k] = STBI__BYTECAST(raw[k] + ((prior[k] + cur[k - out_n]) >> 1)); break;
								CASE(STBI__F_paeth)        cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - out_n], prior[k], prior[k - out_n])); break;
								CASE(STBI__F_avg_first)    cur[k] = STBI__BYTECAST(raw[k] + (cur[k - out_n] >> 1)); break;
								CASE(STBI__F_paeth_first)  cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - out_n], 0, 0)); break;
							}
#undef CASE
						}
					}

					// we make a separate pass to expand bits to pixels; for performance,
					// this could run two scanlines behind the above code, so it won't
					// intefere with filtering but will still be in the cache.
					if (depth < 8) {
						for (j = 0; j < y; ++j) {
							stbi_uc *cur = a->out + stride*j;
							stbi_uc *in = a->out + stride*j + x*out_n - img_width_bytes;
							// unpack 1/2/4-bit into a 8-bit buffer. allows us to keep the common 8-bit path optimal at minimal cost for 1/2/4-bit
							// png guarante byte alignment, if width is not multiple of 8/4/2 we'll decode dummy trailing data that will be skipped in the later loop
							stbi_uc scale = (color == 0) ? stbi__depth_scale_table[depth] : 1; // scale grayscale values to 0..255 range

							// note that the final byte might overshoot and write more data than desired.
							// we can allocate enough data that this never writes out of memory, but it
							// could also overwrite the next scanline. can it overwrite non-empty data
							// on the next scanline? yes, consider 1-pixel-wide scanlines with 1-bit-per-pixel.
							// so we need to explicitly clamp the final ones

							if (depth == 4) {
								for (k = x*img_n; k >= 2; k -= 2, ++in) {
									*cur++ = scale * ((*in >> 4));
									*cur++ = scale * ((*in) & 0x0f);
								}
								if (k > 0) *cur++ = scale * ((*in >> 4));
							}
							else if (depth == 2) {
								for (k = x*img_n; k >= 4; k -= 4, ++in) {
									*cur++ = scale * ((*in >> 6));
									*cur++ = scale * ((*in >> 4) & 0x03);
									*cur++ = scale * ((*in >> 2) & 0x03);
									*cur++ = scale * ((*in) & 0x03);
								}
								if (k > 0) *cur++ = scale * ((*in >> 6));
								if (k > 1) *cur++ = scale * ((*in >> 4) & 0x03);
								if (k > 2) *cur++ = scale * ((*in >> 2) & 0x03);
							}
							else if (depth == 1) {
								for (k = x*img_n; k >= 8; k -= 8, ++in) {
									*cur++ = scale * ((*in >> 7));
									*cur++ = scale * ((*in >> 6) & 0x01);
									*cur++ = scale * ((*in >> 5) & 0x01);
									*cur++ = scale * ((*in >> 4) & 0x01);
									*cur++ = scale * ((*in >> 3) & 0x01);
									*cur++ = scale * ((*in >> 2) & 0x01);
									*cur++ = scale * ((*in >> 1) & 0x01);
									*cur++ = scale * ((*in) & 0x01);
								}
								if (k > 0) *cur++ = scale * ((*in >> 7));
								if (k > 1) *cur++ = scale * ((*in >> 6) & 0x01);
								if (k > 2) *cur++ = scale * ((*in >> 5) & 0x01);
								if (k > 3) *cur++ = scale * ((*in >> 4) & 0x01);
								if (k > 4) *cur++ = scale * ((*in >> 3) & 0x01);
								if (k > 5) *cur++ = scale * ((*in >> 2) & 0x01);
								if (k > 6) *cur++ = scale * ((*in >> 1) & 0x01);
							}
							if (img_n != out_n) {
								int q;
								// insert alpha = 255
								cur = a->out + stride*j;
								if (img_n == 1) {
									for (q = x - 1; q >= 0; --q) {
										cur[q * 2 + 1] = 255;
										cur[q * 2 + 0] = cur[q];
									}
								}
								else {
									STBI_ASSERT(img_n == 3);
									for (q = x - 1; q >= 0; --q) {
										cur[q * 4 + 3] = 255;
										cur[q * 4 + 2] = cur[q * 3 + 2];
										cur[q * 4 + 1] = cur[q * 3 + 1];
										cur[q * 4 + 0] = cur[q * 3 + 0];
									}
								}
							}
						}
					}

					return 1;
				}

				int stbi__create_png_image(stbi__png *a, stbi_uc *image_data, stbi__uint32 image_data_len, int out_n, int depth, int color, int interlaced)
				{
					stbi_uc *final;
					int p;
					if (!interlaced)
						return stbi__create_png_image_raw(a, image_data, image_data_len, out_n, a->s->img_x, a->s->img_y, depth, color);

					// de-interlacing
					final = (stbi_uc *)stbi__malloc(a->s->img_x * a->s->img_y * out_n);
					for (p = 0; p < 7; ++p) {
						int xorig[] = { 0, 4, 0, 2, 0, 1, 0 };
						int yorig[] = { 0, 0, 4, 0, 2, 0, 1 };
						int xspc[] = { 8, 8, 4, 4, 2, 2, 1 };
						int yspc[] = { 8, 8, 8, 4, 4, 2, 2 };
						int i, j, x, y;
						// pass1_x[4] = 0, pass1_x[5] = 1, pass1_x[12] = 1
						x = (a->s->img_x - xorig[p] + xspc[p] - 1) / xspc[p];
						y = (a->s->img_y - yorig[p] + yspc[p] - 1) / yspc[p];
						if (x && y) {
							stbi__uint32 img_len = ((((a->s->img_n * x * depth) + 7) >> 3) + 1) * y;
							if (!stbi__create_png_image_raw(a, image_data, image_data_len, out_n, x, y, depth, color)) {
								STBI_FREE(final);
								return 0;
							}
							for (j = 0; j < y; ++j) {
								for (i = 0; i < x; ++i) {
									int out_y = j*yspc[p] + yorig[p];
									int out_x = i*xspc[p] + xorig[p];
									memcpy(final + out_y*a->s->img_x*out_n + out_x*out_n,
										a->out + (j*x + i)*out_n, out_n);
								}
							}
							STBI_FREE(a->out);
							image_data += img_len;
							image_data_len -= img_len;
						}
					}
					a->out = final;

					return 1;
				}

				int stbi__compute_transparency(stbi__png *z, stbi_uc tc[3], int out_n)
				{
					stbi__context *s = z->s;
					stbi__uint32 i, pixel_count = s->img_x * s->img_y;
					stbi_uc *p = z->out;

					// compute color-based transparency, assuming we've
					// already got 255 as the alpha value in the output
					STBI_ASSERT(out_n == 2 || out_n == 4);

					if (out_n == 2) {
						for (i = 0; i < pixel_count; ++i) {
							p[1] = (p[0] == tc[0] ? 0 : 255);
							p += 2;
						}
					}
					else {
						for (i = 0; i < pixel_count; ++i) {
							if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
								p[3] = 0;
							p += 4;
						}
					}
					return 1;
				}

				int stbi__expand_png_palette(stbi__png *a, stbi_uc *palette, int len, int pal_img_n)
				{
					stbi__uint32 i, pixel_count = a->s->img_x * a->s->img_y;
					stbi_uc *p, *temp_out, *orig = a->out;

					p = (stbi_uc *)stbi__malloc(pixel_count * pal_img_n);
					if (p == NULL) return stbi__err("outofmem", "Out of memory");

					// between here and free(out) below, exitting would leak
					temp_out = p;

					if (pal_img_n == 3) {
						for (i = 0; i < pixel_count; ++i) {
							int n = orig[i] * 4;
							p[0] = palette[n];
							p[1] = palette[n + 1];
							p[2] = palette[n + 2];
							p += 3;
						}
					}
					else {
						for (i = 0; i < pixel_count; ++i) {
							int n = orig[i] * 4;
							p[0] = palette[n];
							p[1] = palette[n + 1];
							p[2] = palette[n + 2];
							p[3] = palette[n + 3];
							p += 4;
						}
					}
					STBI_FREE(a->out);
					a->out = temp_out;

					STBI_NOTUSED(len);

					return 1;
				}

				int stbi__unpremultiply_on_load = 0;
				int stbi__de_iphone_flag = 0;

				void stbi_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply)
				{
					stbi__unpremultiply_on_load = flag_true_if_should_unpremultiply;
				}

				void stbi_convert_iphone_png_to_rgb(int flag_true_if_should_convert)
				{
					stbi__de_iphone_flag = flag_true_if_should_convert;
				}

				void stbi__de_iphone(stbi__png *z)
				{
					stbi__context *s = z->s;
					stbi__uint32 i, pixel_count = s->img_x * s->img_y;
					stbi_uc *p = z->out;

					if (s->img_out_n == 3) {  // convert bgr to rgb
						for (i = 0; i < pixel_count; ++i) {
							stbi_uc t = p[0];
							p[0] = p[2];
							p[2] = t;
							p += 3;
						}
					}
					else {
						STBI_ASSERT(s->img_out_n == 4);
						if (stbi__unpremultiply_on_load) {
							// convert bgr to rgb and unpremultiply
							for (i = 0; i < pixel_count; ++i) {
								stbi_uc a = p[3];
								stbi_uc t = p[0];
								if (a) {
									p[0] = p[2] * 255 / a;
									p[1] = p[1] * 255 / a;
									p[2] = t * 255 / a;
								}
								else {
									p[0] = p[2];
									p[2] = t;
								}
								p += 4;
							}
						}
						else {
							// convert bgr to rgb
							for (i = 0; i < pixel_count; ++i) {
								stbi_uc t = p[0];
								p[0] = p[2];
								p[2] = t;
								p += 4;
							}
						}
					}
				}

#define STBI__PNG_TYPE(a,b,c,d)  (((a) << 24) + ((b) << 16) + ((c) << 8) + (d))

				int stbi__parse_png_file(stbi__png *z, int scan, int req_comp)
				{
					stbi_uc palette[1024], pal_img_n = 0;
					stbi_uc has_trans = 0, tc[3];
					stbi__uint32 ioff = 0, idata_limit = 0, i, pal_len = 0;
					int first = 1, k, interlace = 0, color = 0, depth = 0, is_iphone = 0;
					stbi__context *s = z->s;

					z->expanded = NULL;
					z->idata = NULL;
					z->out = NULL;

					if (!stbi__check_png_header(s)) return 0;

					if (scan == STBI__SCAN_type) return 1;

					for (;;) {
						stbi__pngchunk c = stbi__get_chunk_header(s);
						switch (c.type) {
						case STBI__PNG_TYPE('C', 'g', 'B', 'I'):
							is_iphone = 1;
							stbi__skip(s, c.length);
							break;
						case STBI__PNG_TYPE('I', 'H', 'D', 'R'): {
																	 int comp, filter;
																	 if (!first) return stbi__err("multiple IHDR", "Corrupt PNG");
																	 first = 0;
																	 if (c.length != 13) return stbi__err("bad IHDR len", "Corrupt PNG");
																	 s->img_x = stbi__get32be(s); if (s->img_x > (1 << 24)) return stbi__err("too large", "Very large image (corrupt?)");
																	 s->img_y = stbi__get32be(s); if (s->img_y > (1 << 24)) return stbi__err("too large", "Very large image (corrupt?)");
																	 depth = stbi__get8(s);  if (depth != 1 && depth != 2 && depth != 4 && depth != 8)  return stbi__err("1/2/4/8-bit only", "PNG not supported: 1/2/4/8-bit only");
																	 color = stbi__get8(s);  if (color > 6)         return stbi__err("bad ctype", "Corrupt PNG");
																	 if (color == 3) pal_img_n = 3; else if (color & 1) return stbi__err("bad ctype", "Corrupt PNG");
																	 comp = stbi__get8(s);  if (comp) return stbi__err("bad comp method", "Corrupt PNG");
																	 filter = stbi__get8(s);  if (filter) return stbi__err("bad filter method", "Corrupt PNG");
																	 interlace = stbi__get8(s); if (interlace > 1) return stbi__err("bad interlace method", "Corrupt PNG");
																	 if (!s->img_x || !s->img_y) return stbi__err("0-pixel image", "Corrupt PNG");
																	 if (!pal_img_n) {
																		 s->img_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
																		 if ((1 << 30) / s->img_x / s->img_n < s->img_y) return stbi__err("too large", "Image too large to decode");
																		 if (scan == STBI__SCAN_header) return 1;
																	 }
																	 else {
																		 // if paletted, then pal_n is our final components, and
																		 // img_n is # components to decompress/filter.
																		 s->img_n = 1;
																		 if ((1 << 30) / s->img_x / 4 < s->img_y) return stbi__err("too large", "Corrupt PNG");
																		 // if SCAN_header, have to scan to see if we have a tRNS
																	 }
																	 break;
						}

						case STBI__PNG_TYPE('P', 'L', 'T', 'E'):  {
																	  if (first) return stbi__err("first not IHDR", "Corrupt PNG");
																	  if (c.length > 256 * 3) return stbi__err("invalid PLTE", "Corrupt PNG");
																	  pal_len = c.length / 3;
																	  if (pal_len * 3 != c.length) return stbi__err("invalid PLTE", "Corrupt PNG");
																	  for (i = 0; i < pal_len; ++i) {
																		  palette[i * 4 + 0] = stbi__get8(s);
																		  palette[i * 4 + 1] = stbi__get8(s);
																		  palette[i * 4 + 2] = stbi__get8(s);
																		  palette[i * 4 + 3] = 255;
																	  }
																	  break;
						}

						case STBI__PNG_TYPE('t', 'R', 'N', 'S'): {
																	 if (first) return stbi__err("first not IHDR", "Corrupt PNG");
																	 if (z->idata) return stbi__err("tRNS after IDAT", "Corrupt PNG");
																	 if (pal_img_n) {
																		 if (scan == STBI__SCAN_header) { s->img_n = 4; return 1; }
																		 if (pal_len == 0) return stbi__err("tRNS before PLTE", "Corrupt PNG");
																		 if (c.length > pal_len) return stbi__err("bad tRNS len", "Corrupt PNG");
																		 pal_img_n = 4;
																		 for (i = 0; i < c.length; ++i)
																			 palette[i * 4 + 3] = stbi__get8(s);
																	 }
																	 else {
																		 if (!(s->img_n & 1)) return stbi__err("tRNS with alpha", "Corrupt PNG");
																		 if (c.length != (stbi__uint32)s->img_n * 2) return stbi__err("bad tRNS len", "Corrupt PNG");
																		 has_trans = 1;
																		 for (k = 0; k < s->img_n; ++k)
																			 tc[k] = (stbi_uc)(stbi__get16be(s) & 255) * stbi__depth_scale_table[depth]; // non 8-bit images will be larger
																	 }
																	 break;
						}

						case STBI__PNG_TYPE('I', 'D', 'A', 'T'): {
																	 if (first) return stbi__err("first not IHDR", "Corrupt PNG");
																	 if (pal_img_n && !pal_len) return stbi__err("no PLTE", "Corrupt PNG");
																	 if (scan == STBI__SCAN_header) { s->img_n = pal_img_n; return 1; }
																	 if ((int)(ioff + c.length) < (int)ioff) return 0;
																	 if (ioff + c.length > idata_limit) {
																		 stbi_uc *p;
																		 if (idata_limit == 0) idata_limit = c.length > 4096 ? c.length : 4096;
																		 while (ioff + c.length > idata_limit)
																			 idata_limit *= 2;
																		 p = (stbi_uc *)STBI_REALLOC(z->idata, idata_limit); if (p == NULL) return stbi__err("outofmem", "Out of memory");
																		 z->idata = p;
																	 }
																	 if (!stbi__getn(s, z->idata + ioff, c.length)) return stbi__err("outofdata", "Corrupt PNG");
																	 ioff += c.length;
																	 break;
						}

						case STBI__PNG_TYPE('I', 'E', 'N', 'D'): {
																	 stbi__uint32 raw_len, bpl;
																	 if (first) return stbi__err("first not IHDR", "Corrupt PNG");
																	 if (scan != STBI__SCAN_load) return 1;
																	 if (z->idata == NULL) return stbi__err("no IDAT", "Corrupt PNG");
																	 // initial guess for decoded data size to avoid unnecessary reallocs
																	 bpl = (s->img_x * depth + 7) / 8; // bytes per line, per component
																	 raw_len = bpl * s->img_y * s->img_n /* pixels */ + s->img_y /* filter mode per row */;
																	 z->expanded = (stbi_uc *)stbi_zlib_decode_malloc_guesssize_headerflag((char *)z->idata, ioff, raw_len, (int *)&raw_len, !is_iphone);
																	 if (z->expanded == NULL) return 0; // zlib should set error
																	 STBI_FREE(z->idata); z->idata = NULL;
																	 if ((req_comp == s->img_n + 1 && req_comp != 3 && !pal_img_n) || has_trans)
																		 s->img_out_n = s->img_n + 1;
																	 else
																		 s->img_out_n = s->img_n;
																	 if (!stbi__create_png_image(z, z->expanded, raw_len, s->img_out_n, depth, color, interlace)) return 0;
																	 if (has_trans)
																	 if (!stbi__compute_transparency(z, tc, s->img_out_n)) return 0;
																	 if (is_iphone && stbi__de_iphone_flag && s->img_out_n > 2)
																		 stbi__de_iphone(z);
																	 if (pal_img_n) {
																		 // pal_img_n == 3 or 4
																		 s->img_n = pal_img_n; // record the actual colors we had
																		 s->img_out_n = pal_img_n;
																		 if (req_comp >= 3) s->img_out_n = req_comp;
																		 if (!stbi__expand_png_palette(z, palette, pal_len, s->img_out_n))
																			 return 0;
																	 }
																	 STBI_FREE(z->expanded); z->expanded = NULL;
																	 return 1;
						}

						default:
							// if critical, fail
							if (first) return stbi__err("first not IHDR", "Corrupt PNG");
							if ((c.type & (1 << 29)) == 0) {
#ifndef STBI_NO_FAILURE_STRINGS
								// not threadsafe
								char invalid_chunk[] = "XXXX PNG chunk not known";
								invalid_chunk[0] = STBI__BYTECAST(c.type >> 24);
								invalid_chunk[1] = STBI__BYTECAST(c.type >> 16);
								invalid_chunk[2] = STBI__BYTECAST(c.type >> 8);
								invalid_chunk[3] = STBI__BYTECAST(c.type >> 0);
#endif
								return stbi__err(invalid_chunk, "PNG not supported: unknown PNG chunk type");
							}
							stbi__skip(s, c.length);
							break;
						}
						// end of PNG chunk, read and skip CRC
						stbi__get32be(s);
					}
				}

				unsigned char *stbi__do_png(stbi__png *p, int *x, int *y, int *n, int req_comp)
				{
					unsigned char *result = NULL;
					if (req_comp < 0 || req_comp > 4) return stbi__errpuc("bad req_comp", "Internal error");
					if (stbi__parse_png_file(p, STBI__SCAN_load, req_comp)) {
						result = p->out;
						p->out = NULL;
						if (req_comp && req_comp != p->s->img_out_n) {
							result = stbi__convert_format(result, p->s->img_out_n, req_comp, p->s->img_x, p->s->img_y);
							p->s->img_out_n = req_comp;
							if (result == NULL) return result;
						}
						*x = p->s->img_x;
						*y = p->s->img_y;
						if (n) *n = p->s->img_out_n;
					}
					STBI_FREE(p->out);      p->out = NULL;
					STBI_FREE(p->expanded); p->expanded = NULL;
					STBI_FREE(p->idata);    p->idata = NULL;

					return result;
				}

				unsigned char *stbi__png_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					stbi__png p;
					p.s = s;
					return stbi__do_png(&p, x, y, comp, req_comp);
				}

				int stbi__png_test(stbi__context *s)
				{
					int r;
					r = stbi__check_png_header(s);
					stbi__rewind(s);
					return r;
				}

				int stbi__png_info_raw(stbi__png *p, int *x, int *y, int *comp)
				{
					if (!stbi__parse_png_file(p, STBI__SCAN_header, 0)) {
						stbi__rewind(p->s);
						return 0;
					}
					if (x) *x = p->s->img_x;
					if (y) *y = p->s->img_y;
					if (comp) *comp = p->s->img_n;
					return 1;
				}

				int stbi__png_info(stbi__context *s, int *x, int *y, int *comp)
				{
					stbi__png p;
					p.s = s;
					return stbi__png_info_raw(&p, x, y, comp);
				}
#endif

				// Microsoft/Windows BMP image

#ifndef STBI_NO_BMP
				int stbi__bmp_test_raw(stbi__context *s)
				{
					int r;
					int sz;
					if (stbi__get8(s) != 'B') return 0;
					if (stbi__get8(s) != 'M') return 0;
					stbi__get32le(s); // discard filesize
					stbi__get16le(s); // discard reserved
					stbi__get16le(s); // discard reserved
					stbi__get32le(s); // discard data offset
					sz = stbi__get32le(s);
					r = (sz == 12 || sz == 40 || sz == 56 || sz == 108 || sz == 124);
					return r;
				}

				int stbi__bmp_test(stbi__context *s)
				{
					int r = stbi__bmp_test_raw(s);
					stbi__rewind(s);
					return r;
				}


				// returns 0..31 for the highest set bit
				int stbi__high_bit(unsigned int z)
				{
					int n = 0;
					if (z == 0) return -1;
					if (z >= 0x10000) n += 16, z >>= 16;
					if (z >= 0x00100) n += 8, z >>= 8;
					if (z >= 0x00010) n += 4, z >>= 4;
					if (z >= 0x00004) n += 2, z >>= 2;
					if (z >= 0x00002) n += 1, z >>= 1;
					return n;
				}

				int stbi__bitcount(unsigned int a)
				{
					a = (a & 0x55555555) + ((a >> 1) & 0x55555555); // max 2
					a = (a & 0x33333333) + ((a >> 2) & 0x33333333); // max 4
					a = (a + (a >> 4)) & 0x0f0f0f0f; // max 8 per 4, now 8 bits
					a = (a + (a >> 8)); // max 16 per 8 bits
					a = (a + (a >> 16)); // max 32 per 8 bits
					return a & 0xff;
				}

				int stbi__shiftsigned(int v, int shift, int bits)
				{
					int result;
					int z = 0;

					if (shift < 0) v <<= -shift;
					else v >>= shift;
					result = v;

					z = bits;
					while (z < 8) {
						result += v >> z;
						z += bits;
					}
					return result;
				}

				stbi_uc *stbi__bmp_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					stbi_uc *out;
					unsigned int mr = 0, mg = 0, mb = 0, ma = 0, all_a = 255;
					stbi_uc pal[256][4];
					int psize = 0, i, j, compress = 0, width;
					int bpp, flip_vertically, pad, target, offset, hsz;
					if (stbi__get8(s) != 'B' || stbi__get8(s) != 'M') return stbi__errpuc("not BMP", "Corrupt BMP");
					stbi__get32le(s); // discard filesize
					stbi__get16le(s); // discard reserved
					stbi__get16le(s); // discard reserved
					offset = stbi__get32le(s);
					hsz = stbi__get32le(s);
					if (hsz != 12 && hsz != 40 && hsz != 56 && hsz != 108 && hsz != 124) return stbi__errpuc("unknown BMP", "BMP type not supported: unknown");
					if (hsz == 12) {
						s->img_x = stbi__get16le(s);
						s->img_y = stbi__get16le(s);
					}
					else {
						s->img_x = stbi__get32le(s);
						s->img_y = stbi__get32le(s);
					}
					if (stbi__get16le(s) != 1) return stbi__errpuc("bad BMP", "bad BMP");
					bpp = stbi__get16le(s);
					if (bpp == 1) return stbi__errpuc("monochrome", "BMP type not supported: 1-bit");
					flip_vertically = ((int)s->img_y) > 0;
					s->img_y = abs((int)s->img_y);
					if (hsz == 12) {
						if (bpp < 24)
							psize = (offset - 14 - 24) / 3;
					}
					else {
						compress = stbi__get32le(s);
						if (compress == 1 || compress == 2) return stbi__errpuc("BMP RLE", "BMP type not supported: RLE");
						stbi__get32le(s); // discard sizeof
						stbi__get32le(s); // discard hres
						stbi__get32le(s); // discard vres
						stbi__get32le(s); // discard colorsused
						stbi__get32le(s); // discard max important
						if (hsz == 40 || hsz == 56) {
							if (hsz == 56) {
								stbi__get32le(s);
								stbi__get32le(s);
								stbi__get32le(s);
								stbi__get32le(s);
							}
							if (bpp == 16 || bpp == 32) {
								mr = mg = mb = 0;
								if (compress == 0) {
									if (bpp == 32) {
										mr = 0xffu << 16;
										mg = 0xffu << 8;
										mb = 0xffu << 0;
										ma = 0xffu << 24;
										all_a = 0; // if all_a is 0 at end, then we loaded alpha channel but it was all 0
									}
									else {
										mr = 31u << 10;
										mg = 31u << 5;
										mb = 31u << 0;
									}
								}
								else if (compress == 3) {
									mr = stbi__get32le(s);
									mg = stbi__get32le(s);
									mb = stbi__get32le(s);
									// not documented, but generated by photoshop and handled by mspaint
									if (mr == mg && mg == mb) {
										// ?!?!?
										return stbi__errpuc("bad BMP", "bad BMP");
									}
								}
								else
									return stbi__errpuc("bad BMP", "bad BMP");
							}
						}
						else {
							STBI_ASSERT(hsz == 108 || hsz == 124);
							mr = stbi__get32le(s);
							mg = stbi__get32le(s);
							mb = stbi__get32le(s);
							ma = stbi__get32le(s);
							stbi__get32le(s); // discard color space
							for (i = 0; i < 12; ++i)
								stbi__get32le(s); // discard color space parameters
							if (hsz == 124) {
								stbi__get32le(s); // discard rendering intent
								stbi__get32le(s); // discard offset of profile data
								stbi__get32le(s); // discard size of profile data
								stbi__get32le(s); // discard reserved
							}
						}
						if (bpp < 16)
							psize = (offset - 14 - hsz) >> 2;
					}
					s->img_n = ma ? 4 : 3;
					if (req_comp && req_comp >= 3) // we can directly decode 3 or 4
						target = req_comp;
					else
						target = s->img_n; // if they want monochrome, we'll post-convert
					out = (stbi_uc *)stbi__malloc(target * s->img_x * s->img_y);
					if (!out) return stbi__errpuc("outofmem", "Out of memory");
					if (bpp < 16) {
						int z = 0;
						if (psize == 0 || psize > 256) { STBI_FREE(out); return stbi__errpuc("invalid", "Corrupt BMP"); }
						for (i = 0; i < psize; ++i) {
							pal[i][2] = stbi__get8(s);
							pal[i][1] = stbi__get8(s);
							pal[i][0] = stbi__get8(s);
							if (hsz != 12) stbi__get8(s);
							pal[i][3] = 255;
						}
						stbi__skip(s, offset - 14 - hsz - psize * (hsz == 12 ? 3 : 4));
						if (bpp == 4) width = (s->img_x + 1) >> 1;
						else if (bpp == 8) width = s->img_x;
						else { STBI_FREE(out); return stbi__errpuc("bad bpp", "Corrupt BMP"); }
						pad = (-width) & 3;
						for (j = 0; j < (int)s->img_y; ++j) {
							for (i = 0; i < (int)s->img_x; i += 2) {
								int v = stbi__get8(s), v2 = 0;
								if (bpp == 4) {
									v2 = v & 15;
									v >>= 4;
								}
								out[z++] = pal[v][0];
								out[z++] = pal[v][1];
								out[z++] = pal[v][2];
								if (target == 4) out[z++] = 255;
								if (i + 1 == (int)s->img_x) break;
								v = (bpp == 8) ? stbi__get8(s) : v2;
								out[z++] = pal[v][0];
								out[z++] = pal[v][1];
								out[z++] = pal[v][2];
								if (target == 4) out[z++] = 255;
							}
							stbi__skip(s, pad);
						}
					}
					else {
						int rshift = 0, gshift = 0, bshift = 0, ashift = 0, rcount = 0, gcount = 0, bcount = 0, acount = 0;
						int z = 0;
						int easy = 0;
						stbi__skip(s, offset - 14 - hsz);
						if (bpp == 24) width = 3 * s->img_x;
						else if (bpp == 16) width = 2 * s->img_x;
						else /* bpp = 32 and pad = 0 */ width = 0;
						pad = (-width) & 3;
						if (bpp == 24) {
							easy = 1;
						}
						else if (bpp == 32) {
							if (mb == 0xff && mg == 0xff00 && mr == 0x00ff0000 && ma == 0xff000000)
								easy = 2;
						}
						if (!easy) {
							if (!mr || !mg || !mb) { STBI_FREE(out); return stbi__errpuc("bad masks", "Corrupt BMP"); }
							// right shift amt to put high bit in position #7
							rshift = stbi__high_bit(mr) - 7; rcount = stbi__bitcount(mr);
							gshift = stbi__high_bit(mg) - 7; gcount = stbi__bitcount(mg);
							bshift = stbi__high_bit(mb) - 7; bcount = stbi__bitcount(mb);
							ashift = stbi__high_bit(ma) - 7; acount = stbi__bitcount(ma);
						}
						for (j = 0; j < (int)s->img_y; ++j) {
							if (easy) {
								for (i = 0; i < (int)s->img_x; ++i) {
									unsigned char a;
									out[z + 2] = stbi__get8(s);
									out[z + 1] = stbi__get8(s);
									out[z + 0] = stbi__get8(s);
									z += 3;
									a = (easy == 2 ? stbi__get8(s) : 255);
									all_a |= a;
									if (target == 4) out[z++] = a;
								}
							}
							else {
								for (i = 0; i < (int)s->img_x; ++i) {
									stbi__uint32 v = (bpp == 16 ? (stbi__uint32)stbi__get16le(s) : stbi__get32le(s));
									int a;
									out[z++] = STBI__BYTECAST(stbi__shiftsigned(v & mr, rshift, rcount));
									out[z++] = STBI__BYTECAST(stbi__shiftsigned(v & mg, gshift, gcount));
									out[z++] = STBI__BYTECAST(stbi__shiftsigned(v & mb, bshift, bcount));
									a = (ma ? stbi__shiftsigned(v & ma, ashift, acount) : 255);
									all_a |= a;
									if (target == 4) out[z++] = STBI__BYTECAST(a);
								}
							}
							stbi__skip(s, pad);
						}
					}

					// if alpha channel is all 0s, replace with all 255s
					if (target == 4 && all_a == 0)
					for (i = 4 * s->img_x*s->img_y - 1; i >= 0; i -= 4)
						out[i] = 255;

					if (flip_vertically) {
						stbi_uc t;
						for (j = 0; j < (int)s->img_y >> 1; ++j) {
							stbi_uc *p1 = out + j     *s->img_x*target;
							stbi_uc *p2 = out + (s->img_y - 1 - j)*s->img_x*target;
							for (i = 0; i < (int)s->img_x*target; ++i) {
								t = p1[i], p1[i] = p2[i], p2[i] = t;
							}
						}
					}

					if (req_comp && req_comp != target) {
						out = stbi__convert_format(out, target, req_comp, s->img_x, s->img_y);
						if (out == NULL) return out; // stbi__convert_format frees input on failure
					}

					*x = s->img_x;
					*y = s->img_y;
					if (comp) *comp = s->img_n;
					return out;
				}
#endif

				// Targa Truevision - TGA
				// by Jonathan Dummer
#ifndef STBI_NO_TGA
				int stbi__tga_info(stbi__context *s, int *x, int *y, int *comp)
				{
					int tga_w, tga_h, tga_comp;
					int sz;
					stbi__get8(s);                   // discard Offset
					sz = stbi__get8(s);              // color type
					if (sz > 1) {
						stbi__rewind(s);
						return 0;      // only RGB or indexed allowed
					}
					sz = stbi__get8(s);              // image type
					// only RGB or grey allowed, +/- RLE
					if ((sz != 1) && (sz != 2) && (sz != 3) && (sz != 9) && (sz != 10) && (sz != 11)) return 0;
					stbi__skip(s, 9);
					tga_w = stbi__get16le(s);
					if (tga_w < 1) {
						stbi__rewind(s);
						return 0;   // test width
					}
					tga_h = stbi__get16le(s);
					if (tga_h < 1) {
						stbi__rewind(s);
						return 0;   // test height
					}
					sz = stbi__get8(s);               // bits per pixel
					// only RGB or RGBA or grey allowed
					if ((sz != 8) && (sz != 16) && (sz != 24) && (sz != 32)) {
						stbi__rewind(s);
						return 0;
					}
					tga_comp = sz;
					if (x) *x = tga_w;
					if (y) *y = tga_h;
					if (comp) *comp = tga_comp / 8;
					return 1;                   // seems to have passed everything
				}

				int stbi__tga_test(stbi__context *s)
				{
					int res;
					int sz;
					stbi__get8(s);      //   discard Offset
					sz = stbi__get8(s);   //   color type
					if (sz > 1) return 0;   //   only RGB or indexed allowed
					sz = stbi__get8(s);   //   image type
					if ((sz != 1) && (sz != 2) && (sz != 3) && (sz != 9) && (sz != 10) && (sz != 11)) return 0;   //   only RGB or grey allowed, +/- RLE
					stbi__get16be(s);      //   discard palette start
					stbi__get16be(s);      //   discard palette length
					stbi__get8(s);         //   discard bits per palette color entry
					stbi__get16be(s);      //   discard x origin
					stbi__get16be(s);      //   discard y origin
					if (stbi__get16be(s) < 1) return 0;      //   test width
					if (stbi__get16be(s) < 1) return 0;      //   test height
					sz = stbi__get8(s);   //   bits per pixel
					if ((sz != 8) && (sz != 16) && (sz != 24) && (sz != 32))
						res = 0;
					else
						res = 1;
					stbi__rewind(s);
					return res;
				}

				stbi_uc *stbi__tga_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					//   read in the TGA header stuff
					int tga_offset = stbi__get8(s);
					int tga_indexed = stbi__get8(s);
					int tga_image_type = stbi__get8(s);
					int tga_is_RLE = 0;
					int tga_palette_start = stbi__get16le(s);
					int tga_palette_len = stbi__get16le(s);
					int tga_palette_bits = stbi__get8(s);
					int tga_x_origin = stbi__get16le(s);
					int tga_y_origin = stbi__get16le(s);
					int tga_width = stbi__get16le(s);
					int tga_height = stbi__get16le(s);
					int tga_bits_per_pixel = stbi__get8(s);
					int tga_comp = tga_bits_per_pixel / 8;
					int tga_inverted = stbi__get8(s);
					//   image data
					unsigned char *tga_data;
					unsigned char *tga_palette = NULL;
					int i, j;
					unsigned char raw_data[4];
					int RLE_count = 0;
					int RLE_repeating = 0;
					int read_next_pixel = 1;

					//   do a tiny bit of precessing
					if (tga_image_type >= 8)
					{
						tga_image_type -= 8;
						tga_is_RLE = 1;
					}
					/* int tga_alpha_bits = tga_inverted & 15; */
					tga_inverted = 1 - ((tga_inverted >> 5) & 1);

					//   error check
					if ( //(tga_indexed) ||
						(tga_width < 1) || (tga_height < 1) ||
						(tga_image_type < 1) || (tga_image_type > 3) ||
						((tga_bits_per_pixel != 8) && (tga_bits_per_pixel != 16) &&
						(tga_bits_per_pixel != 24) && (tga_bits_per_pixel != 32))
						)
					{
						return NULL; // we don't report this as a bad TGA because we don't even know if it's TGA
					}

					//   If I'm paletted, then I'll use the number of bits from the palette
					if (tga_indexed)
					{
						tga_comp = tga_palette_bits / 8;
					}

					//   tga info
					*x = tga_width;
					*y = tga_height;
					if (comp) *comp = tga_comp;

					tga_data = (unsigned char*)stbi__malloc((size_t)tga_width * tga_height * tga_comp);
					if (!tga_data) return stbi__errpuc("outofmem", "Out of memory");

					// skip to the data's starting position (offset usually = 0)
					stbi__skip(s, tga_offset);

					if (!tga_indexed && !tga_is_RLE) {
						for (i = 0; i < tga_height; ++i) {
							int row = tga_inverted ? tga_height - i - 1 : i;
							stbi_uc *tga_row = tga_data + row*tga_width*tga_comp;
							stbi__getn(s, tga_row, tga_width * tga_comp);
						}
					}
					else  {
						//   do I need to load a palette?
						if (tga_indexed)
						{
							//   any data to skip? (offset usually = 0)
							stbi__skip(s, tga_palette_start);
							//   load the palette
							tga_palette = (unsigned char*)stbi__malloc(tga_palette_len * tga_palette_bits / 8);
							if (!tga_palette) {
								STBI_FREE(tga_data);
								return stbi__errpuc("outofmem", "Out of memory");
							}
							if (!stbi__getn(s, tga_palette, tga_palette_len * tga_palette_bits / 8)) {
								STBI_FREE(tga_data);
								STBI_FREE(tga_palette);
								return stbi__errpuc("bad palette", "Corrupt TGA");
							}
						}
						//   load the data
						for (i = 0; i < tga_width * tga_height; ++i)
						{
							//   if I'm in RLE mode, do I need to get a RLE stbi__pngchunk?
							if (tga_is_RLE)
							{
								if (RLE_count == 0)
								{
									//   yep, get the next byte as a RLE command
									int RLE_cmd = stbi__get8(s);
									RLE_count = 1 + (RLE_cmd & 127);
									RLE_repeating = RLE_cmd >> 7;
									read_next_pixel = 1;
								}
								else if (!RLE_repeating)
								{
									read_next_pixel = 1;
								}
							}
							else
							{
								read_next_pixel = 1;
							}
							//   OK, if I need to read a pixel, do it now
							if (read_next_pixel)
							{
								//   load however much data we did have
								if (tga_indexed)
								{
									//   read in 1 byte, then perform the lookup
									int pal_idx = stbi__get8(s);
									if (pal_idx >= tga_palette_len)
									{
										//   invalid index
										pal_idx = 0;
									}
									pal_idx *= tga_bits_per_pixel / 8;
									for (j = 0; j * 8 < tga_bits_per_pixel; ++j)
									{
										raw_data[j] = tga_palette[pal_idx + j];
									}
								}
								else
								{
									//   read in the data raw
									for (j = 0; j * 8 < tga_bits_per_pixel; ++j)
									{
										raw_data[j] = stbi__get8(s);
									}
								}
								//   clear the reading flag for the next pixel
								read_next_pixel = 0;
							} // end of reading a pixel

							// copy data
							for (j = 0; j < tga_comp; ++j)
								tga_data[i*tga_comp + j] = raw_data[j];

							//   in case we're in RLE mode, keep counting down
							--RLE_count;
						}
						//   do I need to invert the image?
						if (tga_inverted)
						{
							for (j = 0; j * 2 < tga_height; ++j)
							{
								int index1 = j * tga_width * tga_comp;
								int index2 = (tga_height - 1 - j) * tga_width * tga_comp;
								for (i = tga_width * tga_comp; i > 0; --i)
								{
									unsigned char temp = tga_data[index1];
									tga_data[index1] = tga_data[index2];
									tga_data[index2] = temp;
									++index1;
									++index2;
								}
							}
						}
						//   clear my palette, if I had one
						if (tga_palette != NULL)
						{
							STBI_FREE(tga_palette);
						}
					}

					// swap RGB
					if (tga_comp >= 3)
					{
						unsigned char* tga_pixel = tga_data;
						for (i = 0; i < tga_width * tga_height; ++i)
						{
							unsigned char temp = tga_pixel[0];
							tga_pixel[0] = tga_pixel[2];
							tga_pixel[2] = temp;
							tga_pixel += tga_comp;
						}
					}

					// convert to target component count
					if (req_comp && req_comp != tga_comp)
						tga_data = stbi__convert_format(tga_data, tga_comp, req_comp, tga_width, tga_height);

					//   the things I do to get rid of an error message, and yet keep
					//   Microsoft's C compilers happy... [8^(
					tga_palette_start = tga_palette_len = tga_palette_bits =
						tga_x_origin = tga_y_origin = 0;
					//   OK, done
					return tga_data;
				}
#endif

				// *************************************************************************************************
				// Photoshop PSD loader -- PD by Thatcher Ulrich, integration by Nicolas Schulz, tweaked by STB

#ifndef STBI_NO_PSD
				int stbi__psd_test(stbi__context *s)
				{
					int r = (stbi__get32be(s) == 0x38425053);
					stbi__rewind(s);
					return r;
				}

				stbi_uc *stbi__psd_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					int   pixelCount;
					int channelCount, compression;
					int channel, i, count, len;
					int bitdepth;
					int w, h;
					stbi_uc *out;

					// Check identifier
					if (stbi__get32be(s) != 0x38425053)   // "8BPS"
						return stbi__errpuc("not PSD", "Corrupt PSD image");

					// Check file type version.
					if (stbi__get16be(s) != 1)
						return stbi__errpuc("wrong version", "Unsupported version of PSD image");

					// Skip 6 reserved bytes.
					stbi__skip(s, 6);

					// Read the number of channels (R, G, B, A, etc).
					channelCount = stbi__get16be(s);
					if (channelCount < 0 || channelCount > 16)
						return stbi__errpuc("wrong channel count", "Unsupported number of channels in PSD image");

					// Read the rows and columns of the image.
					h = stbi__get32be(s);
					w = stbi__get32be(s);

					// Make sure the depth is 8 bits.
					bitdepth = stbi__get16be(s);
					if (bitdepth != 8 && bitdepth != 16)
						return stbi__errpuc("unsupported bit depth", "PSD bit depth is not 8 or 16 bit");

					// Make sure the color mode is RGB.
					// Valid options are:
					//   0: Bitmap
					//   1: Grayscale
					//   2: Indexed color
					//   3: RGB color
					//   4: CMYK color
					//   7: Multichannel
					//   8: Duotone
					//   9: Lab color
					if (stbi__get16be(s) != 3)
						return stbi__errpuc("wrong color format", "PSD is not in RGB color format");

					// Skip the Mode Data.  (It's the palette for indexed color; other info for other modes.)
					stbi__skip(s, stbi__get32be(s));

					// Skip the image resources.  (resolution, pen tool paths, etc)
					stbi__skip(s, stbi__get32be(s));

					// Skip the reserved data.
					stbi__skip(s, stbi__get32be(s));

					// Find out if the data is compressed.
					// Known values:
					//   0: no compression
					//   1: RLE compressed
					compression = stbi__get16be(s);
					if (compression > 1)
						return stbi__errpuc("bad compression", "PSD has an unknown compression format");

					// Create the destination image.
					out = (stbi_uc *)stbi__malloc(4 * w*h);
					if (!out) return stbi__errpuc("outofmem", "Out of memory");
					pixelCount = w*h;

					// Initialize the data to zero.
					//memset( out, 0, pixelCount * 4 );

					// Finally, the image data.
					if (compression) {
						// RLE as used by .PSD and .TIFF
						// Loop until you get the number of unpacked bytes you are expecting:
						//     Read the next source byte into n.
						//     If n is between 0 and 127 inclusive, copy the next n+1 bytes literally.
						//     Else if n is between -127 and -1 inclusive, copy the next byte -n+1 times.
						//     Else if n is 128, noop.
						// Endloop

						// The RLE-compressed data is preceeded by a 2-byte data count for each row in the data,
						// which we're going to just skip.
						stbi__skip(s, h * channelCount * 2);

						// Read the RLE data by channel.
						for (channel = 0; channel < 4; channel++) {
							stbi_uc *p;

							p = out + channel;
							if (channel >= channelCount) {
								// Fill this channel with default data.
								for (i = 0; i < pixelCount; i++, p += 4)
									*p = (channel == 3 ? 255 : 0);
							}
							else {
								// Read the RLE data.
								count = 0;
								while (count < pixelCount) {
									len = stbi__get8(s);
									if (len == 128) {
										// No-op.
									}
									else if (len < 128) {
										// Copy next len+1 bytes literally.
										len++;
										count += len;
										while (len) {
											*p = stbi__get8(s);
											p += 4;
											len--;
										}
									}
									else if (len > 128) {
										stbi_uc   val;
										// Next -len+1 bytes in the dest are replicated from next source byte.
										// (Interpret len as a negative 8-bit int.)
										len ^= 0x0FF;
										len += 2;
										val = stbi__get8(s);
										count += len;
										while (len) {
											*p = val;
											p += 4;
											len--;
										}
									}
								}
							}
						}

					}
					else {
						// We're at the raw image data.  It's each channel in order (Red, Green, Blue, Alpha, ...)
						// where each channel consists of an 8-bit value for each pixel in the image.

						// Read the data by channel.
						for (channel = 0; channel < 4; channel++) {
							stbi_uc *p;

							p = out + channel;
							if (channel >= channelCount) {
								// Fill this channel with default data.
								stbi_uc val = channel == 3 ? 255 : 0;
								for (i = 0; i < pixelCount; i++, p += 4)
									*p = val;
							}
							else {
								// Read the data.
								if (bitdepth == 16) {
									for (i = 0; i < pixelCount; i++, p += 4)
										*p = (stbi_uc)(stbi__get16be(s) >> 8);
								}
								else {
									for (i = 0; i < pixelCount; i++, p += 4)
										*p = stbi__get8(s);
								}
							}
						}
					}

					if (req_comp && req_comp != 4) {
						out = stbi__convert_format(out, 4, req_comp, w, h);
						if (out == NULL) return out; // stbi__convert_format frees input on failure
					}

					if (comp) *comp = 4;
					*y = h;
					*x = w;

					return out;
				}
#endif

				// *************************************************************************************************
				// Softimage PIC loader
				// by Tom Seddon
				//
				// See http://softimage.wiki.softimage.com/index.php/INFO:_PIC_file_format
				// See http://ozviz.wasp.uwa.edu.au/~pbourke/dataformats/softimagepic/

#ifndef STBI_NO_PIC
				int stbi__pic_is4(stbi__context *s, const char *str)
				{
					int i;
					for (i = 0; i < 4; ++i)
					if (stbi__get8(s) != (stbi_uc)str[i])
						return 0;

					return 1;
				}

				int stbi__pic_test_core(stbi__context *s)
				{
					int i;

					if (!stbi__pic_is4(s, "\x53\x80\xF6\x34"))
						return 0;

					for (i = 0; i < 84; ++i)
						stbi__get8(s);

					if (!stbi__pic_is4(s, "PICT"))
						return 0;

					return 1;
				}

				typedef struct
				{
					stbi_uc size, type, channel;
				} stbi__pic_packet;

				stbi_uc *stbi__readval(stbi__context *s, int channel, stbi_uc *dest)
				{
					int mask = 0x80, i;

					for (i = 0; i < 4; ++i, mask >>= 1) {
						if (channel & mask) {
							if (stbi__at_eof(s)) return stbi__errpuc("bad file", "PIC file too short");
							dest[i] = stbi__get8(s);
						}
					}

					return dest;
				}

				void stbi__copyval(int channel, stbi_uc *dest, const stbi_uc *src)
				{
					int mask = 0x80, i;

					for (i = 0; i < 4; ++i, mask >>= 1)
					if (channel&mask)
						dest[i] = src[i];
				}

				stbi_uc *stbi__pic_load_core(stbi__context *s, int width, int height, int *comp, stbi_uc *result)
				{
					int act_comp = 0, num_packets = 0, y, chained;
					stbi__pic_packet packets[10];

					// this will (should...) cater for even some bizarre stuff like having data
					// for the same channel in multiple packets.
					do {
						stbi__pic_packet *packet;

						if (num_packets == sizeof(packets) / sizeof(packets[0]))
							return stbi__errpuc("bad format", "too many packets");

						packet = &packets[num_packets++];

						chained = stbi__get8(s);
						packet->size = stbi__get8(s);
						packet->type = stbi__get8(s);
						packet->channel = stbi__get8(s);

						act_comp |= packet->channel;

						if (stbi__at_eof(s))          return stbi__errpuc("bad file", "file too short (reading packets)");
						if (packet->size != 8)  return stbi__errpuc("bad format", "packet isn't 8bpp");
					} while (chained);

					*comp = (act_comp & 0x10 ? 4 : 3); // has alpha channel?

					for (y = 0; y < height; ++y) {
						int packet_idx;

						for (packet_idx = 0; packet_idx < num_packets; ++packet_idx) {
							stbi__pic_packet *packet = &packets[packet_idx];
							stbi_uc *dest = result + y*width * 4;

							switch (packet->type) {
							default:
								return stbi__errpuc("bad format", "packet has bad compression type");

							case 0: {//uncompressed
										int x;

										for (x = 0; x<width; ++x, dest += 4)
										if (!stbi__readval(s, packet->channel, dest))
											return 0;
										break;
							}

							case 1://Pure RLE
							{
									   int left = width, i;

									   while (left>0) {
										   stbi_uc count, value[4];

										   count = stbi__get8(s);
										   if (stbi__at_eof(s))   return stbi__errpuc("bad file", "file too short (pure read count)");

										   if (count > left)
											   count = (stbi_uc)left;

										   if (!stbi__readval(s, packet->channel, value))  return 0;

										   for (i = 0; i<count; ++i, dest += 4)
											   stbi__copyval(packet->channel, dest, value);
										   left -= count;
									   }
							}
								break;

							case 2: {//Mixed RLE
										int left = width;
										while (left>0) {
											int count = stbi__get8(s), i;
											if (stbi__at_eof(s))  return stbi__errpuc("bad file", "file too short (mixed read count)");

											if (count >= 128) { // Repeated
												stbi_uc value[4];

												if (count == 128)
													count = stbi__get16be(s);
												else
													count -= 127;
												if (count > left)
													return stbi__errpuc("bad file", "scanline overrun");

												if (!stbi__readval(s, packet->channel, value))
													return 0;

												for (i = 0; i<count; ++i, dest += 4)
													stbi__copyval(packet->channel, dest, value);
											}
											else { // Raw
												++count;
												if (count>left) return stbi__errpuc("bad file", "scanline overrun");

												for (i = 0; i < count; ++i, dest += 4)
												if (!stbi__readval(s, packet->channel, dest))
													return 0;
											}
											left -= count;
										}
										break;
							}
							}
						}
					}

					return result;
				}

				stbi_uc *stbi__pic_load(stbi__context *s, int *px, int *py, int *comp, int req_comp)
				{
					stbi_uc *result;
					int i, x, y;

					for (i = 0; i < 92; ++i)
						stbi__get8(s);

					x = stbi__get16be(s);
					y = stbi__get16be(s);
					if (stbi__at_eof(s))  return stbi__errpuc("bad file", "file too short (pic header)");
					if ((1 << 28) / x < y) return stbi__errpuc("too large", "Image too large to decode");

					stbi__get32be(s); //skip `ratio'
					stbi__get16be(s); //skip `fields'
					stbi__get16be(s); //skip `pad'

					// intermediate buffer is RGBA
					result = (stbi_uc *)stbi__malloc(x*y * 4);
					memset(result, 0xff, x*y * 4);

					if (!stbi__pic_load_core(s, x, y, comp, result)) {
						STBI_FREE(result);
						result = 0;
					}
					*px = x;
					*py = y;
					if (req_comp == 0) req_comp = *comp;
					result = stbi__convert_format(result, 4, req_comp, x, y);

					return result;
				}

				int stbi__pic_test(stbi__context *s)
				{
					int r = stbi__pic_test_core(s);
					stbi__rewind(s);
					return r;
				}
#endif

				// *************************************************************************************************
				// GIF loader -- public domain by Jean-Marc Lienher -- simplified/shrunk by stb

#ifndef STBI_NO_GIF
				typedef struct
				{
					stbi__int16 prefix;
					stbi_uc first;
					stbi_uc suffix;
				} stbi__gif_lzw;

				typedef struct
				{
					int w, h;
					stbi_uc *out, *old_out;             // output buffer (always 4 components)
					int flags, bgindex, ratio, transparent, eflags, delay;
					stbi_uc  pal[256][4];
					stbi_uc lpal[256][4];
					stbi__gif_lzw codes[4096];
					stbi_uc *color_table;
					int parse, step;
					int lflags;
					int start_x, start_y;
					int max_x, max_y;
					int cur_x, cur_y;
					int line_size;
				} stbi__gif;

				int stbi__gif_test_raw(stbi__context *s)
				{
					int sz;
					if (stbi__get8(s) != 'G' || stbi__get8(s) != 'I' || stbi__get8(s) != 'F' || stbi__get8(s) != '8') return 0;
					sz = stbi__get8(s);
					if (sz != '9' && sz != '7') return 0;
					if (stbi__get8(s) != 'a') return 0;
					return 1;
				}

				int stbi__gif_test(stbi__context *s)
				{
					int r = stbi__gif_test_raw(s);
					stbi__rewind(s);
					return r;
				}

				void stbi__gif_parse_colortable(stbi__context *s, stbi_uc pal[256][4], int num_entries, int transp)
				{
					int i;
					for (i = 0; i < num_entries; ++i) {
						pal[i][2] = stbi__get8(s);
						pal[i][1] = stbi__get8(s);
						pal[i][0] = stbi__get8(s);
						pal[i][3] = transp == i ? 0 : 255;
					}
				}

				int stbi__gif_header(stbi__context *s, stbi__gif *g, int *comp, int is_info)
				{
					stbi_uc version;
					if (stbi__get8(s) != 'G' || stbi__get8(s) != 'I' || stbi__get8(s) != 'F' || stbi__get8(s) != '8')
						return stbi__err("not GIF", "Corrupt GIF");

					version = stbi__get8(s);
					if (version != '7' && version != '9')    return stbi__err("not GIF", "Corrupt GIF");
					if (stbi__get8(s) != 'a')                return stbi__err("not GIF", "Corrupt GIF");

					stbi__g_failure_reason = "";
					g->w = stbi__get16le(s);
					g->h = stbi__get16le(s);
					g->flags = stbi__get8(s);
					g->bgindex = stbi__get8(s);
					g->ratio = stbi__get8(s);
					g->transparent = -1;

					if (comp != 0) *comp = 4;  // can't actually tell whether it's 3 or 4 until we parse the comments

					if (is_info) return 1;

					if (g->flags & 0x80)
						stbi__gif_parse_colortable(s, g->pal, 2 << (g->flags & 7), -1);

					return 1;
				}

				int stbi__gif_info_raw(stbi__context *s, int *x, int *y, int *comp)
				{
					stbi__gif g;
					if (!stbi__gif_header(s, &g, comp, 1)) {
						stbi__rewind(s);
						return 0;
					}
					if (x) *x = g.w;
					if (y) *y = g.h;
					return 1;
				}

				void stbi__out_gif_code(stbi__gif *g, stbi__uint16 code)
				{
					stbi_uc *p, *c;

					// recurse to decode the prefixes, since the linked-list is backwards,
					// and working backwards through an interleaved image would be nasty
					if (g->codes[code].prefix >= 0)
						stbi__out_gif_code(g, g->codes[code].prefix);

					if (g->cur_y >= g->max_y) return;

					p = &g->out[g->cur_x + g->cur_y];
					c = &g->color_table[g->codes[code].suffix * 4];

					if (c[3] >= 128) {
						p[0] = c[2];
						p[1] = c[1];
						p[2] = c[0];
						p[3] = c[3];
					}
					g->cur_x += 4;

					if (g->cur_x >= g->max_x) {
						g->cur_x = g->start_x;
						g->cur_y += g->step;

						while (g->cur_y >= g->max_y && g->parse > 0) {
							g->step = (1 << g->parse) * g->line_size;
							g->cur_y = g->start_y + (g->step >> 1);
							--g->parse;
						}
					}
				}

				stbi_uc *stbi__process_gif_raster(stbi__context *s, stbi__gif *g)
				{
					stbi_uc lzw_cs;
					stbi__int32 len, init_code;
					stbi__uint32 first;
					stbi__int32 codesize, codemask, avail, oldcode, bits, valid_bits, clear;
					stbi__gif_lzw *p;

					lzw_cs = stbi__get8(s);
					if (lzw_cs > 12) return NULL;
					clear = 1 << lzw_cs;
					first = 1;
					codesize = lzw_cs + 1;
					codemask = (1 << codesize) - 1;
					bits = 0;
					valid_bits = 0;
					for (init_code = 0; init_code < clear; init_code++) {
						g->codes[init_code].prefix = -1;
						g->codes[init_code].first = (stbi_uc)init_code;
						g->codes[init_code].suffix = (stbi_uc)init_code;
					}

					// support no starting clear code
					avail = clear + 2;
					oldcode = -1;

					len = 0;
					for (;;) {
						if (valid_bits < codesize) {
							if (len == 0) {
								len = stbi__get8(s); // start new block
								if (len == 0)
									return g->out;
							}
							--len;
							bits |= (stbi__int32)stbi__get8(s) << valid_bits;
							valid_bits += 8;
						}
						else {
							stbi__int32 code = bits & codemask;
							bits >>= codesize;
							valid_bits -= codesize;
							// @OPTIMIZE: is there some way we can accelerate the non-clear path?
							if (code == clear) {  // clear code
								codesize = lzw_cs + 1;
								codemask = (1 << codesize) - 1;
								avail = clear + 2;
								oldcode = -1;
								first = 0;
							}
							else if (code == clear + 1) { // end of stream code
								stbi__skip(s, len);
								while ((len = stbi__get8(s)) > 0)
									stbi__skip(s, len);
								return g->out;
							}
							else if (code <= avail) {
								if (first) return stbi__errpuc("no clear code", "Corrupt GIF");

								if (oldcode >= 0) {
									p = &g->codes[avail++];
									if (avail > 4096)        return stbi__errpuc("too many codes", "Corrupt GIF");
									p->prefix = (stbi__int16)oldcode;
									p->first = g->codes[oldcode].first;
									p->suffix = (code == avail) ? p->first : g->codes[code].first;
								}
								else if (code == avail)
									return stbi__errpuc("illegal code in raster", "Corrupt GIF");

								stbi__out_gif_code(g, (stbi__uint16)code);

								if ((avail & codemask) == 0 && avail <= 0x0FFF) {
									codesize++;
									codemask = (1 << codesize) - 1;
								}

								oldcode = code;
							}
							else {
								return stbi__errpuc("illegal code in raster", "Corrupt GIF");
							}
						}
					}
				}

				void stbi__fill_gif_background(stbi__gif *g, int x0, int y0, int x1, int y1)
				{
					int x, y;
					stbi_uc *c = g->pal[g->bgindex];
					for (y = y0; y < y1; y += 4 * g->w) {
						for (x = x0; x < x1; x += 4) {
							stbi_uc *p = &g->out[y + x];
							p[0] = c[2];
							p[1] = c[1];
							p[2] = c[0];
							p[3] = 0;
						}
					}
				}

				// this function is designed to support animated gifs, although stb_image doesn't support it
				stbi_uc *stbi__gif_load_next(stbi__context *s, stbi__gif *g, int *comp, int req_comp)
				{
					int i;
					stbi_uc *prev_out = 0;

					if (g->out == 0 && !stbi__gif_header(s, g, comp, 0))
						return 0; // stbi__g_failure_reason set by stbi__gif_header

					prev_out = g->out;
					g->out = (stbi_uc *)stbi__malloc(4 * g->w * g->h);
					if (g->out == 0) return stbi__errpuc("outofmem", "Out of memory");

					switch ((g->eflags & 0x1C) >> 2) {
					case 0: // unspecified (also always used on 1st frame)
						stbi__fill_gif_background(g, 0, 0, 4 * g->w, 4 * g->w * g->h);
						break;
					case 1: // do not dispose
						if (prev_out) memcpy(g->out, prev_out, 4 * g->w * g->h);
						g->old_out = prev_out;
						break;
					case 2: // dispose to background
						if (prev_out) memcpy(g->out, prev_out, 4 * g->w * g->h);
						stbi__fill_gif_background(g, g->start_x, g->start_y, g->max_x, g->max_y);
						break;
					case 3: // dispose to previous
						if (g->old_out) {
							for (i = g->start_y; i < g->max_y; i += 4 * g->w)
								memcpy(&g->out[i + g->start_x], &g->old_out[i + g->start_x], g->max_x - g->start_x);
						}
						break;
					}

					for (;;) {
						switch (stbi__get8(s)) {
						case 0x2C: /* Image Descriptor */
						{
									   int prev_trans = -1;
									   stbi__int32 x, y, w, h;
									   stbi_uc *o;

									   x = stbi__get16le(s);
									   y = stbi__get16le(s);
									   w = stbi__get16le(s);
									   h = stbi__get16le(s);
									   if (((x + w) > (g->w)) || ((y + h) > (g->h)))
										   return stbi__errpuc("bad Image Descriptor", "Corrupt GIF");

									   g->line_size = g->w * 4;
									   g->start_x = x * 4;
									   g->start_y = y * g->line_size;
									   g->max_x = g->start_x + w * 4;
									   g->max_y = g->start_y + h * g->line_size;
									   g->cur_x = g->start_x;
									   g->cur_y = g->start_y;

									   g->lflags = stbi__get8(s);

									   if (g->lflags & 0x40) {
										   g->step = 8 * g->line_size; // first interlaced spacing
										   g->parse = 3;
									   }
									   else {
										   g->step = g->line_size;
										   g->parse = 0;
									   }

									   if (g->lflags & 0x80) {
										   stbi__gif_parse_colortable(s, g->lpal, 2 << (g->lflags & 7), g->eflags & 0x01 ? g->transparent : -1);
										   g->color_table = (stbi_uc *)g->lpal;
									   }
									   else if (g->flags & 0x80) {
										   if (g->transparent >= 0 && (g->eflags & 0x01)) {
											   prev_trans = g->pal[g->transparent][3];
											   g->pal[g->transparent][3] = 0;
										   }
										   g->color_table = (stbi_uc *)g->pal;
									   }
									   else
										   return stbi__errpuc("missing color table", "Corrupt GIF");

									   o = stbi__process_gif_raster(s, g);
									   if (o == NULL) return NULL;

									   if (prev_trans != -1)
										   g->pal[g->transparent][3] = (stbi_uc)prev_trans;

									   return o;
						}

						case 0x21: // Comment Extension.
						{
									   int len;
									   if (stbi__get8(s) == 0xF9) { // Graphic Control Extension.
										   len = stbi__get8(s);
										   if (len == 4) {
											   g->eflags = stbi__get8(s);
											   g->delay = stbi__get16le(s);
											   g->transparent = stbi__get8(s);
										   }
										   else {
											   stbi__skip(s, len);
											   break;
										   }
									   }
									   while ((len = stbi__get8(s)) != 0)
										   stbi__skip(s, len);
									   break;
						}

						case 0x3B: // gif stream termination code
							return (stbi_uc *)s; // using '1' causes warning on some compilers

						default:
							return stbi__errpuc("unknown code", "Corrupt GIF");
						}
					}

					STBI_NOTUSED(req_comp);
				}

				stbi_uc *stbi__gif_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					stbi_uc *u = 0;
					stbi__gif g;
					memset(&g, 0, sizeof(g));

					u = stbi__gif_load_next(s, &g, comp, req_comp);
					if (u == (stbi_uc *)s) u = 0;  // end of animated gif marker
					if (u) {
						*x = g.w;
						*y = g.h;
						if (req_comp && req_comp != 4)
							u = stbi__convert_format(u, 4, req_comp, g.w, g.h);
					}
					else if (g.out)
						STBI_FREE(g.out);

					return u;
				}

				int stbi__gif_info(stbi__context *s, int *x, int *y, int *comp)
				{
					return stbi__gif_info_raw(s, x, y, comp);
				}
#endif

				// *************************************************************************************************
				// Radiance RGBE HDR loader
				// originally by Nicolas Schulz
#ifndef STBI_NO_HDR
				int stbi__hdr_test_core(stbi__context *s)
				{
					const char *signature = "#?RADIANCE\n";
					int i;
					for (i = 0; signature[i]; ++i)
					if (stbi__get8(s) != signature[i])
						return 0;
					return 1;
				}

				int stbi__hdr_test(stbi__context* s)
				{
					int r = stbi__hdr_test_core(s);
					stbi__rewind(s);
					return r;
				}

#define STBI__HDR_BUFLEN  1024
				char *stbi__hdr_gettoken(stbi__context *z, char *buffer)
				{
					int len = 0;
					char c = '\0';

					c = (char)stbi__get8(z);

					while (!stbi__at_eof(z) && c != '\n') {
						buffer[len++] = c;
						if (len == STBI__HDR_BUFLEN - 1) {
							// flush to end of line
							while (!stbi__at_eof(z) && stbi__get8(z) != '\n')
								;
							break;
						}
						c = (char)stbi__get8(z);
					}

					buffer[len] = 0;
					return buffer;
				}

				void stbi__hdr_convert(float *output, stbi_uc *input, int req_comp)
				{
					if (input[3] != 0) {
						float f1;
						// Exponent
						f1 = (float)ldexp(1.0f, input[3] - (int)(128 + 8));
						if (req_comp <= 2)
							output[0] = (input[0] + input[1] + input[2]) * f1 / 3;
						else {
							output[0] = input[0] * f1;
							output[1] = input[1] * f1;
							output[2] = input[2] * f1;
						}
						if (req_comp == 2) output[1] = 1;
						if (req_comp == 4) output[3] = 1;
					}
					else {
						switch (req_comp) {
						case 4: output[3] = 1; /* fallthrough */
						case 3: output[0] = output[1] = output[2] = 0;
							break;
						case 2: output[1] = 1; /* fallthrough */
						case 1: output[0] = 0;
							break;
						}
					}
				}

				float *stbi__hdr_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					char buffer[STBI__HDR_BUFLEN];
					char *token;
					int valid = 0;
					int width, height;
					stbi_uc *scanline;
					float *hdr_data;
					int len;
					unsigned char count, value;
					int i, j, k, c1, c2, z;


					// Check identifier
					if (strcmp(stbi__hdr_gettoken(s, buffer), "#?RADIANCE") != 0)
						return stbi__errpf("not HDR", "Corrupt HDR image");

					// Parse header
					for (;;) {
						token = stbi__hdr_gettoken(s, buffer);
						if (token[0] == 0) break;
						if (strcmp(token, "FORMAT=32-bit_rle_rgbe") == 0) valid = 1;
					}

					if (!valid)    return stbi__errpf("unsupported format", "Unsupported HDR format");

					// Parse width and height
					// can't use sscanf() if we're not using stdio!
					token = stbi__hdr_gettoken(s, buffer);
					if (strncmp(token, "-Y ", 3))  return stbi__errpf("unsupported data layout", "Unsupported HDR format");
					token += 3;
					height = (int)strtol(token, &token, 10);
					while (*token == ' ') ++token;
					if (strncmp(token, "+X ", 3))  return stbi__errpf("unsupported data layout", "Unsupported HDR format");
					token += 3;
					width = (int)strtol(token, NULL, 10);

					*x = width;
					*y = height;

					if (comp) *comp = 3;
					if (req_comp == 0) req_comp = 3;

					// Read data
					hdr_data = (float *)stbi__malloc(height * width * req_comp * sizeof(float));

					// Load image data
					// image data is stored as some number of sca
					if (width < 8 || width >= 32768) {
						// Read flat data
						for (j = 0; j < height; ++j) {
							for (i = 0; i < width; ++i) {
								stbi_uc rgbe[4];
							main_decode_loop:
								stbi__getn(s, rgbe, 4);
								stbi__hdr_convert(hdr_data + j * width * req_comp + i * req_comp, rgbe, req_comp);
							}
						}
					}
					else {
						// Read RLE-encoded data
						scanline = NULL;

						for (j = 0; j < height; ++j) {
							c1 = stbi__get8(s);
							c2 = stbi__get8(s);
							len = stbi__get8(s);
							if (c1 != 2 || c2 != 2 || (len & 0x80)) {
								// not run-length encoded, so we have to actually use THIS data as a decoded
								// pixel (note this can't be a valid pixel--one of RGB must be >= 128)
								stbi_uc rgbe[4];
								rgbe[0] = (stbi_uc)c1;
								rgbe[1] = (stbi_uc)c2;
								rgbe[2] = (stbi_uc)len;
								rgbe[3] = (stbi_uc)stbi__get8(s);
								stbi__hdr_convert(hdr_data, rgbe, req_comp);
								i = 1;
								j = 0;
								STBI_FREE(scanline);
								goto main_decode_loop; // yes, this makes no sense
							}
							len <<= 8;
							len |= stbi__get8(s);
							if (len != width) { STBI_FREE(hdr_data); STBI_FREE(scanline); return stbi__errpf("invalid decoded scanline length", "corrupt HDR"); }
							if (scanline == NULL) scanline = (stbi_uc *)stbi__malloc(width * 4);

							for (k = 0; k < 4; ++k) {
								i = 0;
								while (i < width) {
									count = stbi__get8(s);
									if (count > 128) {
										// Run
										value = stbi__get8(s);
										count -= 128;
										for (z = 0; z < count; ++z)
											scanline[i++ * 4 + k] = value;
									}
									else {
										// Dump
										for (z = 0; z < count; ++z)
											scanline[i++ * 4 + k] = stbi__get8(s);
									}
								}
							}
							for (i = 0; i < width; ++i)
								stbi__hdr_convert(hdr_data + (j*width + i)*req_comp, scanline + i * 4, req_comp);
						}
						STBI_FREE(scanline);
					}

					return hdr_data;
				}

				int stbi__hdr_info(stbi__context *s, int *x, int *y, int *comp)
				{
					char buffer[STBI__HDR_BUFLEN];
					char *token;
					int valid = 0;

					if (strcmp(stbi__hdr_gettoken(s, buffer), "#?RADIANCE") != 0) {
						stbi__rewind(s);
						return 0;
					}

					for (;;) {
						token = stbi__hdr_gettoken(s, buffer);
						if (token[0] == 0) break;
						if (strcmp(token, "FORMAT=32-bit_rle_rgbe") == 0) valid = 1;
					}

					if (!valid) {
						stbi__rewind(s);
						return 0;
					}
					token = stbi__hdr_gettoken(s, buffer);
					if (strncmp(token, "-Y ", 3)) {
						stbi__rewind(s);
						return 0;
					}
					token += 3;
					*y = (int)strtol(token, &token, 10);
					while (*token == ' ') ++token;
					if (strncmp(token, "+X ", 3)) {
						stbi__rewind(s);
						return 0;
					}
					token += 3;
					*x = (int)strtol(token, NULL, 10);
					*comp = 3;
					return 1;
				}
#endif // STBI_NO_HDR

#ifndef STBI_NO_BMP
				int stbi__bmp_info(stbi__context *s, int *x, int *y, int *comp)
				{
					int hsz;
					if (stbi__get8(s) != 'B' || stbi__get8(s) != 'M') {
						stbi__rewind(s);
						return 0;
					}
					stbi__skip(s, 12);
					hsz = stbi__get32le(s);
					if (hsz != 12 && hsz != 40 && hsz != 56 && hsz != 108 && hsz != 124) {
						stbi__rewind(s);
						return 0;
					}
					if (hsz == 12) {
						*x = stbi__get16le(s);
						*y = stbi__get16le(s);
					}
					else {
						*x = stbi__get32le(s);
						*y = stbi__get32le(s);
					}
					if (stbi__get16le(s) != 1) {
						stbi__rewind(s);
						return 0;
					}
					*comp = stbi__get16le(s) / 8;
					return 1;
				}
#endif

#ifndef STBI_NO_PSD
				int stbi__psd_info(stbi__context *s, int *x, int *y, int *comp)
				{
					int channelCount;
					if (stbi__get32be(s) != 0x38425053) {
						stbi__rewind(s);
						return 0;
					}
					if (stbi__get16be(s) != 1) {
						stbi__rewind(s);
						return 0;
					}
					stbi__skip(s, 6);
					channelCount = stbi__get16be(s);
					if (channelCount < 0 || channelCount > 16) {
						stbi__rewind(s);
						return 0;
					}
					*y = stbi__get32be(s);
					*x = stbi__get32be(s);
					if (stbi__get16be(s) != 8) {
						stbi__rewind(s);
						return 0;
					}
					if (stbi__get16be(s) != 3) {
						stbi__rewind(s);
						return 0;
					}
					*comp = 4;
					return 1;
				}
#endif

#ifndef STBI_NO_PIC
				int stbi__pic_info(stbi__context *s, int *x, int *y, int *comp)
				{
					int act_comp = 0, num_packets = 0, chained;
					stbi__pic_packet packets[10];

					if (!stbi__pic_is4(s, "\x53\x80\xF6\x34")) {
						stbi__rewind(s);
						return 0;
					}

					stbi__skip(s, 88);

					*x = stbi__get16be(s);
					*y = stbi__get16be(s);
					if (stbi__at_eof(s)) {
						stbi__rewind(s);
						return 0;
					}
					if ((*x) != 0 && (1 << 28) / (*x) < (*y)) {
						stbi__rewind(s);
						return 0;
					}

					stbi__skip(s, 8);

					do {
						stbi__pic_packet *packet;

						if (num_packets == sizeof(packets) / sizeof(packets[0]))
							return 0;

						packet = &packets[num_packets++];
						chained = stbi__get8(s);
						packet->size = stbi__get8(s);
						packet->type = stbi__get8(s);
						packet->channel = stbi__get8(s);
						act_comp |= packet->channel;

						if (stbi__at_eof(s)) {
							stbi__rewind(s);
							return 0;
						}
						if (packet->size != 8) {
							stbi__rewind(s);
							return 0;
						}
					} while (chained);

					*comp = (act_comp & 0x10 ? 4 : 3);

					return 1;
				}
#endif

				// *************************************************************************************************
				// Portable Gray Map and Portable Pixel Map loader
				// by Ken Miller
				//
				// PGM: http://netpbm.sourceforge.net/doc/pgm.html
				// PPM: http://netpbm.sourceforge.net/doc/ppm.html
				//
				// Known limitations:
				//    Does not support comments in the header section
				//    Does not support ASCII image data (formats P2 and P3)
				//    Does not support 16-bit-per-channel

#ifndef STBI_NO_PNM

				int      stbi__pnm_test(stbi__context *s)
				{
					char p, t;
					p = (char)stbi__get8(s);
					t = (char)stbi__get8(s);
					if (p != 'P' || (t != '5' && t != '6')) {
						stbi__rewind(s);
						return 0;
					}
					return 1;
				}

				stbi_uc *stbi__pnm_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					stbi_uc *out;
					if (!stbi__pnm_info(s, (int *)&s->img_x, (int *)&s->img_y, (int *)&s->img_n))
						return 0;
					*x = s->img_x;
					*y = s->img_y;
					*comp = s->img_n;

					out = (stbi_uc *)stbi__malloc(s->img_n * s->img_x * s->img_y);
					if (!out) return stbi__errpuc("outofmem", "Out of memory");
					stbi__getn(s, out, s->img_n * s->img_x * s->img_y);

					if (req_comp && req_comp != s->img_n) {
						out = stbi__convert_format(out, s->img_n, req_comp, s->img_x, s->img_y);
						if (out == NULL) return out; // stbi__convert_format frees input on failure
					}
					return out;
				}

				int      stbi__pnm_isspace(char c)
				{
					return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
				}

				void     stbi__pnm_skip_whitespace(stbi__context *s, char *c)
				{
					while (!stbi__at_eof(s) && stbi__pnm_isspace(*c))
						*c = (char)stbi__get8(s);
				}

				int      stbi__pnm_isdigit(char c)
				{
					return c >= '0' && c <= '9';
				}

				int      stbi__pnm_getinteger(stbi__context *s, char *c)
				{
					int value = 0;

					while (!stbi__at_eof(s) && stbi__pnm_isdigit(*c)) {
						value = value * 10 + (*c - '0');
						*c = (char)stbi__get8(s);
					}

					return value;
				}

				int      stbi__pnm_info(stbi__context *s, int *x, int *y, int *comp)
				{
					int maxv;
					char c, p, t;

					stbi__rewind(s);

					// Get identifier
					p = (char)stbi__get8(s);
					t = (char)stbi__get8(s);
					if (p != 'P' || (t != '5' && t != '6')) {
						stbi__rewind(s);
						return 0;
					}

					*comp = (t == '6') ? 3 : 1;  // '5' is 1-component .pgm; '6' is 3-component .ppm

					c = (char)stbi__get8(s);
					stbi__pnm_skip_whitespace(s, &c);

					*x = stbi__pnm_getinteger(s, &c); // read width
					stbi__pnm_skip_whitespace(s, &c);

					*y = stbi__pnm_getinteger(s, &c); // read height
					stbi__pnm_skip_whitespace(s, &c);

					maxv = stbi__pnm_getinteger(s, &c);  // read max value

					if (maxv > 255)
						return stbi__err("max value > 255", "PPM image not 8-bit");
					else
						return 1;
				}
#endif

				int stbi__info_main(stbi__context *s, int *x, int *y, int *comp)
				{
#ifndef STBI_NO_JPEG
					if (stbi__jpeg_info(s, x, y, comp)) return 1;
#endif

#ifndef STBI_NO_PNG
					if (stbi__png_info(s, x, y, comp))  return 1;
#endif

#ifndef STBI_NO_GIF
					if (stbi__gif_info(s, x, y, comp))  return 1;
#endif

#ifndef STBI_NO_BMP
					if (stbi__bmp_info(s, x, y, comp))  return 1;
#endif

#ifndef STBI_NO_PSD
					if (stbi__psd_info(s, x, y, comp))  return 1;
#endif

#ifndef STBI_NO_PIC
					if (stbi__pic_info(s, x, y, comp))  return 1;
#endif

#ifndef STBI_NO_PNM
					if (stbi__pnm_info(s, x, y, comp))  return 1;
#endif

#ifndef STBI_NO_HDR
					if (stbi__hdr_info(s, x, y, comp))  return 1;
#endif

					// test tga last because it's a crappy test!
#ifndef STBI_NO_TGA
					if (stbi__tga_info(s, x, y, comp))
						return 1;
#endif
					return stbi__err("unknown image type", "Image not of any known type, or corrupt");
				}

#ifndef STBI_NO_STDIO
				int stbi_info(char const *filename, int *x, int *y, int *comp)
				{
					FILE *f = stbi__fopen(filename, "rb");
					int result;
					if (!f) return stbi__err("can't fopen", "Unable to open file");
					result = stbi_info_from_file(f, x, y, comp);
					fclose(f);
					return result;
				}

				int stbi_info_from_file(FILE *f, int *x, int *y, int *comp)
				{
					int r;
					stbi__context s;
					long pos = ftell(f);
					stbi__start_file(&s, f);
					r = stbi__info_main(&s, x, y, comp);
					fseek(f, pos, SEEK_SET);
					return r;
				}
#endif // !STBI_NO_STDIO

				int stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp)
				{
					stbi__context s;
					stbi__start_mem(&s, buffer, len);
					return stbi__info_main(&s, x, y, comp);
				}

				int stbi_info_from_callbacks(stbi_io_callbacks const *c, void *user, int *x, int *y, int *comp)
				{
					stbi__context s;
					stbi__start_callbacks(&s, (stbi_io_callbacks *)c, user);
					return stbi__info_main(&s, x, y, comp);
				}
			} // namespace stbi::decode

			namespace encode
			{
				int stbi_write_png(char const *filename, int w, int h, int comp, const void  *data, int stride_in_bytes);
				int stbi_write_bmp(char const *filename, int w, int h, int comp, const void  *data);
				int stbi_write_tga(char const *filename, int w, int h, int comp, const void  *data);
				int stbi_write_hdr(char const *filename, int w, int h, int comp, const float *data);
				typedef void stbi_write_func(void *context, void *data, int size);
				int stbi_write_png_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data, int stride_in_bytes);
				int stbi_write_bmp_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data);
				int stbi_write_tga_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data);
				int stbi_write_hdr_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const float *data);

#if defined(STBIW_MALLOC) && defined(STBIW_FREE) && defined(STBIW_REALLOC)
				// ok
#elif !defined(STBIW_MALLOC) && !defined(STBIW_FREE) && !defined(STBIW_REALLOC)
				// ok
#else
#error "Must define all or none of STBIW_MALLOC, STBIW_FREE, and STBIW_REALLOC."
#endif

#ifndef STBIW_MALLOC
#define STBIW_MALLOC(sz)    malloc(sz)
#define STBIW_REALLOC(p,sz) realloc(p,sz)
#define STBIW_FREE(p)       free(p)
#endif
#ifndef STBIW_MEMMOVE
#define STBIW_MEMMOVE(a,b,sz) memmove(a,b,sz)
#endif


#ifndef STBIW_ASSERT
#define STBIW_ASSERT(x) assert(x)
#endif

				typedef struct
				{
					stbi_write_func *func;
					void *context;
				} stbi__write_context;

				// initialize a callback-based context
				void stbi__start_write_callbacks(stbi__write_context *s, stbi_write_func *c, void *context)
				{
					s->func = c;
					s->context = context;
				}

#ifndef STBI_WRITE_NO_STDIO

				void stbi__stdio_write(void *context, void *data, int size)
				{
					fwrite(data, 1, size, (FILE*)context);
				}

				int stbi__start_write_file(stbi__write_context *s, const char *filename)
				{
					FILE *f = fopen(filename, "wb");
					stbi__start_write_callbacks(s, stbi__stdio_write, (void *)f);
					return f != NULL;
				}

				void stbi__end_write_file(stbi__write_context *s)
				{
					fclose((FILE *)s->context);
				}

#endif // !STBI_WRITE_NO_STDIO

				typedef unsigned int stbiw_uint32;
				typedef int stb_image_write_test[sizeof(stbiw_uint32) == 4 ? 1 : -1];

#ifdef STB_IMAGE_WRITE_
				int stbi_write_tga_with_rle = 1;
#else
				int stbi_write_tga_with_rle = 1;
#endif

				void stbiw__writefv(stbi__write_context *s, const char *fmt, va_list v)
				{
					while (*fmt) {
						switch (*fmt++) {
						case ' ': break;
						case '1': { unsigned char x = (unsigned char)va_arg(v, int);
							s->func(s->context, &x, 1);
							break; }
						case '2': { int x = va_arg(v, int);
							unsigned char b[2];
							b[0] = (unsigned char)x;
							b[1] = (unsigned char)(x >> 8);
							s->func(s->context, b, 2);
							break; }
						case '4': { stbiw_uint32 x = va_arg(v, int);
							unsigned char b[4];
							b[0] = (unsigned char)x;
							b[1] = (unsigned char)(x >> 8);
							b[2] = (unsigned char)(x >> 16);
							b[3] = (unsigned char)(x >> 24);
							s->func(s->context, b, 4);
							break; }
						default:
							STBIW_ASSERT(0);
							return;
						}
					}
				}

				void stbiw__writef(stbi__write_context *s, const char *fmt, ...)
				{
					va_list v;
					va_start(v, fmt);
					stbiw__writefv(s, fmt, v);
					va_end(v);
				}

				void stbiw__write3(stbi__write_context *s, unsigned char a, unsigned char b, unsigned char c)
				{
					unsigned char arr[3];
					arr[0] = a, arr[1] = b, arr[2] = c;
					s->func(s->context, arr, 3);
				}

				void stbiw__write_pixel(stbi__write_context *s, int rgb_dir, int comp, int write_alpha, int expand_mono, unsigned char *d)
				{
					unsigned char bg[3] = { 255, 0, 255 }, px[3];
					int k;

					if (write_alpha < 0)
						s->func(s->context, &d[comp - 1], 1);

					switch (comp) {
					case 1:
						s->func(s->context, d, 1);
						break;
					case 2:
						if (expand_mono)
							stbiw__write3(s, d[0], d[0], d[0]); // monochrome bmp
						else
							s->func(s->context, d, 1);  // monochrome TGA
						break;
					case 4:
						if (!write_alpha) {
							// composite against pink background
							for (k = 0; k < 3; ++k)
								px[k] = bg[k] + ((d[k] - bg[k]) * d[3]) / 255;
							stbiw__write3(s, px[1 - rgb_dir], px[1], px[1 + rgb_dir]);
							break;
						}
						/* FALLTHROUGH */
					case 3:
						stbiw__write3(s, d[1 - rgb_dir], d[1], d[1 + rgb_dir]);
						break;
					}
					if (write_alpha > 0)
						s->func(s->context, &d[comp - 1], 1);
				}

				void stbiw__write_pixels(stbi__write_context *s, int rgb_dir, int vdir, int x, int y, int comp, void *data, int write_alpha, int scanline_pad, int expand_mono)
				{
					stbiw_uint32 zero = 0;
					int i, j, j_end;

					if (y <= 0)
						return;

					if (vdir < 0)
						j_end = -1, j = y - 1;
					else
						j_end = y, j = 0;

					for (; j != j_end; j += vdir) {
						for (i = 0; i < x; ++i) {
							unsigned char *d = (unsigned char *)data + (j*x + i)*comp;
							stbiw__write_pixel(s, rgb_dir, comp, write_alpha, expand_mono, d);
						}
						s->func(s->context, &zero, scanline_pad);
					}
				}

				int stbiw__outfile(stbi__write_context *s, int rgb_dir, int vdir, int x, int y, int comp, int expand_mono, void *data, int alpha, int pad, const char *fmt, ...)
				{
					if (y < 0 || x < 0) {
						return 0;
					}
					else {
						va_list v;
						va_start(v, fmt);
						stbiw__writefv(s, fmt, v);
						va_end(v);
						stbiw__write_pixels(s, rgb_dir, vdir, x, y, comp, data, alpha, pad, expand_mono);
						return 1;
					}
				}

				int stbi_write_bmp_core(stbi__write_context *s, int x, int y, int comp, const void *data)
				{
					int pad = (-x * 3) & 3;
					return stbiw__outfile(s, -1, -1, x, y, comp, 1, (void *)data, 0, pad,
						"11 4 22 4" "4 44 22 444444",
						'B', 'M', 14 + 40 + (x * 3 + pad)*y, 0, 0, 14 + 40,  // file header
						40, x, y, 1, 24, 0, 0, 0, 0, 0, 0);             // bitmap header
				}

				int stbi_write_bmp_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data)
				{
					stbi__write_context s;
					stbi__start_write_callbacks(&s, func, context);
					return stbi_write_bmp_core(&s, x, y, comp, data);
				}

#ifndef STBI_WRITE_NO_STDIO
				int stbi_write_bmp(char const *filename, int x, int y, int comp, const void *data)
				{
					stbi__write_context s;
					if (stbi__start_write_file(&s, filename)) {
						int r = stbi_write_bmp_core(&s, x, y, comp, data);
						stbi__end_write_file(&s);
						return r;
					}
					else
						return 0;
				}
#endif //!STBI_WRITE_NO_STDIO

				int stbi_write_tga_core(stbi__write_context *s, int x, int y, int comp, void *data)
				{
					int has_alpha = (comp == 2 || comp == 4);
					int colorbytes = has_alpha ? comp - 1 : comp;
					int format = colorbytes < 2 ? 3 : 2; // 3 color channels (RGB/RGBA) = 2, 1 color channel (Y/YA) = 3

					if (y < 0 || x < 0)
						return 0;

					if (!stbi_write_tga_with_rle) {
						return stbiw__outfile(s, -1, -1, x, y, comp, 0, (void *)data, has_alpha, 0,
							"111 221 2222 11", 0, 0, format, 0, 0, 0, 0, 0, x, y, (colorbytes + has_alpha) * 8, has_alpha * 8);
					}
					else {
						int i, j, k;

						stbiw__writef(s, "111 221 2222 11", 0, 0, format + 8, 0, 0, 0, 0, 0, x, y, (colorbytes + has_alpha) * 8, has_alpha * 8);

						for (j = y - 1; j >= 0; --j) {
							unsigned char *row = (unsigned char *)data + j * x * comp;
							int len;

							for (i = 0; i < x; i += len) {
								unsigned char *begin = row + i * comp;
								int diff = 1;
								len = 1;

								if (i < x - 1) {
									++len;
									diff = memcmp(begin, row + (i + 1) * comp, comp);
									if (diff) {
										const unsigned char *prev = begin;
										for (k = i + 2; k < x && len < 128; ++k) {
											if (memcmp(prev, row + k * comp, comp)) {
												prev += comp;
												++len;
											}
											else {
												--len;
												break;
											}
										}
									}
									else {
										for (k = i + 2; k < x && len < 128; ++k) {
											if (!memcmp(begin, row + k * comp, comp)) {
												++len;
											}
											else {
												break;
											}
										}
									}
								}

								if (diff) {
									unsigned char header = (unsigned char)(len - 1);
									s->func(s->context, &header, 1);
									for (k = 0; k < len; ++k) {
										stbiw__write_pixel(s, -1, comp, has_alpha, 0, begin + k * comp);
									}
								}
								else {
									unsigned char header = (unsigned char)(len - 129);
									s->func(s->context, &header, 1);
									stbiw__write_pixel(s, -1, comp, has_alpha, 0, begin);
								}
							}
						}
					}
					return 1;
				}

				int stbi_write_tga_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data)
				{
					stbi__write_context s;
					stbi__start_write_callbacks(&s, func, context);
					return stbi_write_tga_core(&s, x, y, comp, (void *)data);
				}

#ifndef STBI_WRITE_NO_STDIO
				int stbi_write_tga(char const *filename, int x, int y, int comp, const void *data)
				{
					stbi__write_context s;
					if (stbi__start_write_file(&s, filename)) {
						int r = stbi_write_tga_core(&s, x, y, comp, (void *)data);
						stbi__end_write_file(&s);
						return r;
					}
					else
						return 0;
				}
#endif

				// *************************************************************************************************
				// Radiance RGBE HDR writer
				// by Baldur Karlsson
#ifndef STBI_WRITE_NO_STDIO

#define stbiw__max(a, b)  ((a) > (b) ? (a) : (b))

				void stbiw__linear_to_rgbe(unsigned char *rgbe, float *linear)
				{
					int exponent;
					float maxcomp = stbiw__max(linear[0], stbiw__max(linear[1], linear[2]));

					if (maxcomp < 1e-32) {
						rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 0;
					}
					else {
						float normalize = (float)frexp(maxcomp, &exponent) * 256.0f / maxcomp;

						rgbe[0] = (unsigned char)(linear[0] * normalize);
						rgbe[1] = (unsigned char)(linear[1] * normalize);
						rgbe[2] = (unsigned char)(linear[2] * normalize);
						rgbe[3] = (unsigned char)(exponent + 128);
					}
				}

				void stbiw__write_run_data(stbi__write_context *s, int length, unsigned char databyte)
				{
					unsigned char lengthbyte = (unsigned char)(length + 128);
					STBIW_ASSERT(length + 128 <= 255);
					s->func(s->context, &lengthbyte, 1);
					s->func(s->context, &databyte, 1);
				}

				void stbiw__write_dump_data(stbi__write_context *s, int length, unsigned char *data)
				{
					unsigned char lengthbyte = (unsigned char)(length & 0xff);
					STBIW_ASSERT(length <= 128); // inconsistent with spec but consistent with official code
					s->func(s->context, &lengthbyte, 1);
					s->func(s->context, data, length);
				}

				void stbiw__write_hdr_scanline(stbi__write_context *s, int width, int ncomp, unsigned char *scratch, float *scanline)
				{
					unsigned char scanlineheader[4] = { 2, 2, 0, 0 };
					unsigned char rgbe[4];
					float linear[3];
					int x;

					scanlineheader[2] = (width & 0xff00) >> 8;
					scanlineheader[3] = (width & 0x00ff);

					/* skip RLE for images too small or large */
					if (width < 8 || width >= 32768) {
						for (x = 0; x < width; x++) {
							switch (ncomp) {
							case 4: /* fallthrough */
							case 3: linear[2] = scanline[x*ncomp + 2];
								linear[1] = scanline[x*ncomp + 1];
								linear[0] = scanline[x*ncomp + 0];
								break;
							default:
								linear[0] = linear[1] = linear[2] = scanline[x*ncomp + 0];
								break;
							}
							stbiw__linear_to_rgbe(rgbe, linear);
							s->func(s->context, rgbe, 4);
						}
					}
					else {
						int c, r;
						/* encode into scratch buffer */
						for (x = 0; x < width; x++) {
							switch (ncomp) {
							case 4: /* fallthrough */
							case 3: linear[2] = scanline[x*ncomp + 2];
								linear[1] = scanline[x*ncomp + 1];
								linear[0] = scanline[x*ncomp + 0];
								break;
							default:
								linear[0] = linear[1] = linear[2] = scanline[x*ncomp + 0];
								break;
							}
							stbiw__linear_to_rgbe(rgbe, linear);
							scratch[x + width * 0] = rgbe[0];
							scratch[x + width * 1] = rgbe[1];
							scratch[x + width * 2] = rgbe[2];
							scratch[x + width * 3] = rgbe[3];
						}

						s->func(s->context, scanlineheader, 4);

						/* RLE each component separately */
						for (c = 0; c < 4; c++) {
							unsigned char *comp = &scratch[width*c];

							x = 0;
							while (x < width) {
								// find first run
								r = x;
								while (r + 2 < width) {
									if (comp[r] == comp[r + 1] && comp[r] == comp[r + 2])
										break;
									++r;
								}
								if (r + 2 >= width)
									r = width;
								// dump up to first run
								while (x < r) {
									int len = r - x;
									if (len > 128) len = 128;
									stbiw__write_dump_data(s, len, &comp[x]);
									x += len;
								}
								// if there's a run, output it
								if (r + 2 < width) { // same test as what we break out of in search loop, so only true if we break'd
									// find next byte after run
									while (r < width && comp[r] == comp[x])
										++r;
									// output run up to r
									while (x < r) {
										int len = r - x;
										if (len > 127) len = 127;
										stbiw__write_run_data(s, len, comp[x]);
										x += len;
									}
								}
							}
						}
					}
				}

				int stbi_write_hdr_core(stbi__write_context *s, int x, int y, int comp, float *data)
				{
					if (y <= 0 || x <= 0 || data == NULL)
						return 0;
					else {
						// Each component is stored separately. Allocate scratch space for full output scanline.
						unsigned char *scratch = (unsigned char *)STBIW_MALLOC(x * 4);
						int i, len;
						char buffer[128];
						char header[] = "#?RADIANCE\n# Written by stb_image_write.h\nFORMAT=32-bit_rle_rgbe\n";
						s->func(s->context, header, sizeof(header)-1);

						len = sprintf(buffer, "EXPOSURE=          1.0000000000000\n\n-Y %d +X %d\n", y, x);
						s->func(s->context, buffer, len);

						for (i = 0; i < y; i++)
							stbiw__write_hdr_scanline(s, x, comp, scratch, data + comp*i*x);
						STBIW_FREE(scratch);
						return 1;
					}
				}

				int stbi_write_hdr_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const float *data)
				{
					stbi__write_context s;
					stbi__start_write_callbacks(&s, func, context);
					return stbi_write_hdr_core(&s, x, y, comp, (float *)data);
				}

				int stbi_write_hdr(char const *filename, int x, int y, int comp, const float *data)
				{
					stbi__write_context s;
					if (stbi__start_write_file(&s, filename)) {
						int r = stbi_write_hdr_core(&s, x, y, comp, (float *)data);
						stbi__end_write_file(&s);
						return r;
					}
					else
						return 0;
				}
#endif // STBI_WRITE_NO_STDIO


				//////////////////////////////////////////////////////////////////////////////
				//
				// PNG writer
				//

				// stretchy buffer; stbiw__sbpush() == vector<>::push_back() -- stbiw__sbcount() == vector<>::size()
#define stbiw__sbraw(a) ((int *) (a) - 2)
#define stbiw__sbm(a)   stbiw__sbraw(a)[0]
#define stbiw__sbn(a)   stbiw__sbraw(a)[1]

#define stbiw__sbneedgrow(a,n)  ((a)==0 || stbiw__sbn(a)+n >= stbiw__sbm(a))
#define stbiw__sbmaybegrow(a,n) (stbiw__sbneedgrow(a,(n)) ? stbiw__sbgrow(a,n) : 0)
#define stbiw__sbgrow(a,n)  stbiw__sbgrowf((void **) &(a), (n), sizeof(*(a)))

#define stbiw__sbpush(a, v)      (stbiw__sbmaybegrow(a,1), (a)[stbiw__sbn(a)++] = (v))
#define stbiw__sbcount(a)        ((a) ? stbiw__sbn(a) : 0)
#define stbiw__sbfree(a)         ((a) ? STBIW_FREE(stbiw__sbraw(a)),0 : 0)

				void *stbiw__sbgrowf(void **arr, int increment, int itemsize)
				{
					int m = *arr ? 2 * stbiw__sbm(*arr) + increment : increment + 1;
					void *p = STBIW_REALLOC(*arr ? stbiw__sbraw(*arr) : 0, itemsize * m + sizeof(int)* 2);
					STBIW_ASSERT(p);
					if (p) {
						if (!*arr) ((int *)p)[1] = 0;
						*arr = (void *)((int *)p + 2);
						stbiw__sbm(*arr) = m;
					}
					return *arr;
				}

				unsigned char *stbiw__zlib_flushf(unsigned char *data, unsigned int *bitbuffer, int *bitcount)
				{
					while (*bitcount >= 8) {
						stbiw__sbpush(data, (unsigned char)*bitbuffer);
						*bitbuffer >>= 8;
						*bitcount -= 8;
					}
					return data;
				}

				int stbiw__zlib_bitrev(int code, int codebits)
				{
					int res = 0;
					while (codebits--) {
						res = (res << 1) | (code & 1);
						code >>= 1;
					}
					return res;
				}

				unsigned int stbiw__zlib_countm(unsigned char *a, unsigned char *b, int limit)
				{
					int i;
					for (i = 0; i < limit && i < 258; ++i)
					if (a[i] != b[i]) break;
					return i;
				}

				unsigned int stbiw__zhash(unsigned char *data)
				{
					stbiw_uint32 hash = data[0] + (data[1] << 8) + (data[2] << 16);
					hash ^= hash << 3;
					hash += hash >> 5;
					hash ^= hash << 4;
					hash += hash >> 17;
					hash ^= hash << 25;
					hash += hash >> 6;
					return hash;
				}

#define stbiw__zlib_flush() (out = stbiw__zlib_flushf(out, &bitbuf, &bitcount))
#define stbiw__zlib_add(code,codebits) \
	(bitbuf |= (code) << bitcount, bitcount += (codebits), stbiw__zlib_flush())
#define stbiw__zlib_huffa(b,c)  stbiw__zlib_add(stbiw__zlib_bitrev(b,c),c)
				// default huffman tables
#define stbiw__zlib_huff1(n)  stbiw__zlib_huffa(0x30 + (n), 8)
#define stbiw__zlib_huff2(n)  stbiw__zlib_huffa(0x190 + (n)-144, 9)
#define stbiw__zlib_huff3(n)  stbiw__zlib_huffa(0 + (n)-256,7)
#define stbiw__zlib_huff4(n)  stbiw__zlib_huffa(0xc0 + (n)-280,8)
#define stbiw__zlib_huff(n)  ((n) <= 143 ? stbiw__zlib_huff1(n) : (n) <= 255 ? stbiw__zlib_huff2(n) : (n) <= 279 ? stbiw__zlib_huff3(n) : stbiw__zlib_huff4(n))
#define stbiw__zlib_huffb(n) ((n) <= 143 ? stbiw__zlib_huff1(n) : stbiw__zlib_huff2(n))

#define stbiw__ZHASH   16384

				unsigned char * stbi_zlib_compress(unsigned char *data, int data_len, int *out_len, int quality)
				{
					unsigned short lengthc[] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 259 };
					unsigned char  lengtheb[] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
					unsigned short distc[] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 32768 };
					unsigned char  disteb[] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
					unsigned int bitbuf = 0;
					int i, j, bitcount = 0;
					unsigned char *out = NULL;
					unsigned char **hash_table[stbiw__ZHASH]; // 64KB on the stack!
					if (quality < 5) quality = 5;

					stbiw__sbpush(out, 0x78);   // DEFLATE 32K window
					stbiw__sbpush(out, 0x5e);   // FLEVEL = 1
					stbiw__zlib_add(1, 1);  // BFINAL = 1
					stbiw__zlib_add(1, 2);  // BTYPE = 1 -- fixed huffman

					for (i = 0; i < stbiw__ZHASH; ++i)
						hash_table[i] = NULL;

					i = 0;
					while (i < data_len - 3) {
						// hash next 3 bytes of data to be compressed
						int h = stbiw__zhash(data + i)&(stbiw__ZHASH - 1), best = 3;
						unsigned char *bestloc = 0;
						unsigned char **hlist = hash_table[h];
						int n = stbiw__sbcount(hlist);
						for (j = 0; j < n; ++j) {
							if (hlist[j] - data > i - 32768) { // if entry lies within window
								int d = stbiw__zlib_countm(hlist[j], data + i, data_len - i);
								if (d >= best) best = d, bestloc = hlist[j];
							}
						}
						// when hash table entry is too long, delete half the entries
						if (hash_table[h] && stbiw__sbn(hash_table[h]) == 2 * quality) {
							STBIW_MEMMOVE(hash_table[h], hash_table[h] + quality, sizeof(hash_table[h][0])*quality);
							stbiw__sbn(hash_table[h]) = quality;
						}
						stbiw__sbpush(hash_table[h], data + i);

						if (bestloc) {
							// "lazy matching" - check match at *next* byte, and if it's better, do cur byte as literal
							h = stbiw__zhash(data + i + 1)&(stbiw__ZHASH - 1);
							hlist = hash_table[h];
							n = stbiw__sbcount(hlist);
							for (j = 0; j < n; ++j) {
								if (hlist[j] - data > i - 32767) {
									int e = stbiw__zlib_countm(hlist[j], data + i + 1, data_len - i - 1);
									if (e > best) { // if next match is better, bail on current match
										bestloc = NULL;
										break;
									}
								}
							}
						}

						if (bestloc) {
							int d = (int)(data + i - bestloc); // distance back
							STBIW_ASSERT(d <= 32767 && best <= 258);
							for (j = 0; best > lengthc[j + 1] - 1; ++j);
							stbiw__zlib_huff(j + 257);
							if (lengtheb[j]) stbiw__zlib_add(best - lengthc[j], lengtheb[j]);
							for (j = 0; d > distc[j + 1] - 1; ++j);
							stbiw__zlib_add(stbiw__zlib_bitrev(j, 5), 5);
							if (disteb[j]) stbiw__zlib_add(d - distc[j], disteb[j]);
							i += best;
						}
						else {
							stbiw__zlib_huffb(data[i]);
							++i;
						}
					}
					// write out final bytes
					for (; i < data_len; ++i)
						stbiw__zlib_huffb(data[i]);
					stbiw__zlib_huff(256); // end of block
					// pad with 0 bits to byte boundary
					while (bitcount)
						stbiw__zlib_add(0, 1);

					for (i = 0; i < stbiw__ZHASH; ++i)
						(void)stbiw__sbfree(hash_table[i]);

					{
						// compute adler32 on input
						unsigned int k = 0, s1 = 1, s2 = 0;
						int blocklen = (int)(data_len % 5552);
						j = 0;
						while (j < data_len) {
							for (i = 0; i < blocklen; ++i) s1 += data[j + i], s2 += s1;
							s1 %= 65521, s2 %= 65521;
							j += blocklen;
							blocklen = 5552;
						}
						stbiw__sbpush(out, (unsigned char)(s2 >> 8));
						stbiw__sbpush(out, (unsigned char)s2);
						stbiw__sbpush(out, (unsigned char)(s1 >> 8));
						stbiw__sbpush(out, (unsigned char)s1);
					}
					*out_len = stbiw__sbn(out);
					// make returned pointer freeable
					STBIW_MEMMOVE(stbiw__sbraw(out), out, *out_len);
					return (unsigned char *)stbiw__sbraw(out);
				}

				unsigned int stbiw__crc32(unsigned char *buffer, int len)
				{
					unsigned int crc_table[256];
					unsigned int crc = ~0u;
					int i, j;
					if (crc_table[1] == 0)
					for (i = 0; i < 256; i++)
					for (crc_table[i] = i, j = 0; j < 8; ++j)
						crc_table[i] = (crc_table[i] >> 1) ^ (crc_table[i] & 1 ? 0xedb88320 : 0);
					for (i = 0; i < len; ++i)
						crc = (crc >> 8) ^ crc_table[buffer[i] ^ (crc & 0xff)];
					return ~crc;
				}

#define stbiw__wpng4(o,a,b,c,d) ((o)[0]=(unsigned char)(a),(o)[1]=(unsigned char)(b),(o)[2]=(unsigned char)(c),(o)[3]=(unsigned char)(d),(o)+=4)
#define stbiw__wp32(data,v) stbiw__wpng4(data, (v)>>24,(v)>>16,(v)>>8,(v));
#define stbiw__wptag(data,s) stbiw__wpng4(data, s[0],s[1],s[2],s[3])

				void stbiw__wpcrc(unsigned char **data, int len)
				{
					unsigned int crc = stbiw__crc32(*data - len - 4, len + 4);
					stbiw__wp32(*data, crc);
				}

				unsigned char stbiw__paeth(int a, int b, int c)
				{
					int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
					if (pa <= pb && pa <= pc) return (unsigned char)a;
					if (pb <= pc) return (unsigned char)b;
					return (unsigned char)c;
				}

				unsigned char *stbi_write_png_to_mem(unsigned char *pixels, int stride_bytes, int x, int y, int n, int *out_len)
				{
					int ctype[5] = { -1, 0, 4, 2, 6 };
					unsigned char sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
					unsigned char *out, *o, *filt, *zlib;
					signed char *line_buffer;
					int i, j, k, p, zlen;

					if (stride_bytes == 0)
						stride_bytes = x * n;

					filt = (unsigned char *)STBIW_MALLOC((x*n + 1) * y); if (!filt) return 0;
					line_buffer = (signed char *)STBIW_MALLOC(x * n); if (!line_buffer) { STBIW_FREE(filt); return 0; }
					for (j = 0; j < y; ++j) {
						int mapping[] = { 0, 1, 2, 3, 4 };
						int firstmap[] = { 0, 1, 0, 5, 6 };
						int *mymap = j ? mapping : firstmap;
						int best = 0, bestval = 0x7fffffff;
						for (p = 0; p < 2; ++p) {
							for (k = p ? best : 0; k < 5; ++k) {
								int type = mymap[k], est = 0;
								unsigned char *z = pixels + stride_bytes*j;
								for (i = 0; i < n; ++i)
									switch (type) {
									case 0: line_buffer[i] = z[i]; break;
									case 1: line_buffer[i] = z[i]; break;
									case 2: line_buffer[i] = z[i] - z[i - stride_bytes]; break;
									case 3: line_buffer[i] = z[i] - (z[i - stride_bytes] >> 1); break;
									case 4: line_buffer[i] = (signed char)(z[i] - stbiw__paeth(0, z[i - stride_bytes], 0)); break;
									case 5: line_buffer[i] = z[i]; break;
									case 6: line_buffer[i] = z[i]; break;
								}
								for (i = n; i < x*n; ++i) {
									switch (type) {
									case 0: line_buffer[i] = z[i]; break;
									case 1: line_buffer[i] = z[i] - z[i - n]; break;
									case 2: line_buffer[i] = z[i] - z[i - stride_bytes]; break;
									case 3: line_buffer[i] = z[i] - ((z[i - n] + z[i - stride_bytes]) >> 1); break;
									case 4: line_buffer[i] = z[i] - stbiw__paeth(z[i - n], z[i - stride_bytes], z[i - stride_bytes - n]); break;
									case 5: line_buffer[i] = z[i] - (z[i - n] >> 1); break;
									case 6: line_buffer[i] = z[i] - stbiw__paeth(z[i - n], 0, 0); break;
									}
								}
								if (p) break;
								for (i = 0; i < x*n; ++i)
									est += abs((signed char)line_buffer[i]);
								if (est < bestval) { bestval = est; best = k; }
							}
						}
						// when we get here, best contains the filter type, and line_buffer contains the data
						filt[j*(x*n + 1)] = (unsigned char)best;
						STBIW_MEMMOVE(filt + j*(x*n + 1) + 1, line_buffer, x*n);
					}
					STBIW_FREE(line_buffer);
					zlib = stbi_zlib_compress(filt, y*(x*n + 1), &zlen, 8); // increase 8 to get smaller but use more memory
					STBIW_FREE(filt);
					if (!zlib) return 0;

					// each tag requires 12 bytes of overhead
					out = (unsigned char *)STBIW_MALLOC(8 + 12 + 13 + 12 + zlen + 12);
					if (!out) return 0;
					*out_len = 8 + 12 + 13 + 12 + zlen + 12;

					o = out;
					STBIW_MEMMOVE(o, sig, 8); o += 8;
					stbiw__wp32(o, 13); // header length
					stbiw__wptag(o, "IHDR");
					stbiw__wp32(o, x);
					stbiw__wp32(o, y);
					*o++ = 8;
					*o++ = (unsigned char)ctype[n];
					*o++ = 0;
					*o++ = 0;
					*o++ = 0;
					stbiw__wpcrc(&o, 13);

					stbiw__wp32(o, zlen);
					stbiw__wptag(o, "IDAT");
					STBIW_MEMMOVE(o, zlib, zlen);
					o += zlen;
					STBIW_FREE(zlib);
					stbiw__wpcrc(&o, zlen);

					stbiw__wp32(o, 0);
					stbiw__wptag(o, "IEND");
					stbiw__wpcrc(&o, 0);

					STBIW_ASSERT(o == out + *out_len);

					return out;
				}

#ifndef STBI_WRITE_NO_STDIO
				int stbi_write_png(char const *filename, int x, int y, int comp, const void *data, int stride_bytes)
				{
					FILE *f;
					int len;
					unsigned char *png = stbi_write_png_to_mem((unsigned char *)data, stride_bytes, x, y, comp, &len);
					if (png == NULL) return 0;
					f = fopen(filename, "wb");
					if (!f) { STBIW_FREE(png); return 0; }
					fwrite(png, 1, len, f);
					fclose(f);
					STBIW_FREE(png);
					return 1;
				}
#endif

				int stbi_write_png_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data, int stride_bytes)
				{
					int len;
					unsigned char *png = stbi_write_png_to_mem((unsigned char *)data, stride_bytes, x, y, comp, &len);
					if (png == NULL) return 0;
					func(context, png, len);
					STBIW_FREE(png);
					return 1;
				}
			} // namespace stbi::encode

			namespace resize
			{
#ifdef _MSC_VER
				typedef unsigned char  stbir_uint8;
				typedef unsigned short stbir_uint16;
				typedef unsigned int   stbir_uint32;
#else
#include <stdint.h>
				typedef uint8_t  stbir_uint8;
				typedef uint16_t stbir_uint16;
				typedef uint32_t stbir_uint32;
#endif

#ifdef STB_IMAGE_RESIZE_STATIC
#define static
#else
#ifdef __cplusplus
#define extern "C"
#else
#define extern
#endif
#endif


				//////////////////////////////////////////////////////////////////////////////
				//
				// Easy-to-use API:
				//
				//     * "input pixels" points to an array of image data with 'num_channels' channels (e.g. RGB=3, RGBA=4)
				//     * input_w is input image width (x-axis), input_h is input image height (y-axis)
				//     * stride is the offset between successive rows of image data in memory, in bytes. you can
				//       specify 0 to mean packed continuously in memory
				//     * alpha channel is treated identically to other channels.
				//     * colorspace is linear or sRGB as specified by function name
				//     * returned result is 1 for success or 0 in case of an error.
				//       #define STBIR_ASSERT() to trigger an assert on parameter validation errors.
				//     * Memory required grows approximately linearly with input and output size, but with
				//       discontinuities at input_w == output_w and input_h == output_h.
				//     * These functions use a "default" resampling filter defined at compile time. To change the filter,
				//       you can change the compile-time defaults by #defining STBIR_DEFAULT_FILTER_UPSAMPLE
				//       and STBIR_DEFAULT_FILTER_DOWNSAMPLE, or you can use the medium-complexity API.

				int stbir_resize_uint8(const unsigned char *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					unsigned char *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					int num_channels);

				int stbir_resize_float(const float *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					float *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					int num_channels);


				// The following functions interpret image data as gamma-corrected sRGB. 
				// Specify STBIR_ALPHA_CHANNEL_NONE if you have no alpha channel,
				// or otherwise provide the index of the alpha channel. Flags value
				// of 0 will probably do the right thing if you're not sure what
				// the flags mean.

#define STBIR_ALPHA_CHANNEL_NONE       -1

				// Set this flag if your texture has premultiplied alpha. Otherwise, stbir will
				// use alpha-weighted resampling (effectively premultiplying, resampling,
				// then unpremultiplying).
#define STBIR_FLAG_ALPHA_PREMULTIPLIED    (1 << 0)
				// The specified alpha channel should be handled as gamma-corrected value even
				// when doing sRGB operations.
#define STBIR_FLAG_ALPHA_USES_COLORSPACE  (1 << 1)

				int stbir_resize_uint8_srgb(const unsigned char *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					unsigned char *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					int num_channels, int alpha_channel, int flags);


				typedef enum
				{
					STBIR_EDGE_CLAMP = 1,
					STBIR_EDGE_REFLECT = 2,
					STBIR_EDGE_WRAP = 3,
					STBIR_EDGE_ZERO = 4,
				} stbir_edge;

				// This function adds the ability to specify how requests to sample off the edge of the image are handled.
				int stbir_resize_uint8_srgb_edgemode(const unsigned char *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					unsigned char *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					int num_channels, int alpha_channel, int flags,
					stbir_edge edge_wrap_mode);

				//////////////////////////////////////////////////////////////////////////////
				//
				// Medium-complexity API
				//
				// This extends the easy-to-use API as follows:
				//
				//     * Alpha-channel can be processed separately
				//       * If alpha_channel is not STBIR_ALPHA_CHANNEL_NONE
				//         * Alpha channel will not be gamma corrected (unless flags&STBIR_FLAG_GAMMA_CORRECT)
				//         * Filters will be weighted by alpha channel (unless flags&STBIR_FLAG_ALPHA_PREMULTIPLIED)
				//     * Filter can be selected explicitly
				//     * uint16 image type
				//     * sRGB colorspace available for all types
				//     * context parameter for passing to STBIR_MALLOC

				typedef enum
				{
					STBIR_FILTER_DEFAULT = 0,  // use same filter type that easy-to-use API chooses
					STBIR_FILTER_BOX = 1,  // A trapezoid w/1-pixel wide ramps, same result as box for integer scale ratios
					STBIR_FILTER_TRIANGLE = 2,  // On upsampling, produces same results as bilinear texture filtering
					STBIR_FILTER_CUBICBSPLINE = 3,  // The cubic b-spline (aka Mitchell-Netrevalli with B=1,C=0), gaussian-esque
					STBIR_FILTER_CATMULLROM = 4,  // An interpolating cubic spline
					STBIR_FILTER_MITCHELL = 5,  // Mitchell-Netrevalli filter with B=1/3, C=1/3
				} stbir_filter;

				typedef enum
				{
					STBIR_COLORSPACE_LINEAR,
					STBIR_COLORSPACE_SRGB,

					STBIR_MAX_COLORSPACES,
				} stbir_colorspace;

				// The following functions are all identical except for the type of the image data

				int stbir_resize_uint8_generic(const unsigned char *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					unsigned char *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					int num_channels, int alpha_channel, int flags,
					stbir_edge edge_wrap_mode, stbir_filter filter, stbir_colorspace space,
					void *alloc_context);

				int stbir_resize_uint16_generic(const stbir_uint16 *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					stbir_uint16 *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					int num_channels, int alpha_channel, int flags,
					stbir_edge edge_wrap_mode, stbir_filter filter, stbir_colorspace space,
					void *alloc_context);

				int stbir_resize_float_generic(const float *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					float *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					int num_channels, int alpha_channel, int flags,
					stbir_edge edge_wrap_mode, stbir_filter filter, stbir_colorspace space,
					void *alloc_context);



				//////////////////////////////////////////////////////////////////////////////
				//
				// Full-complexity API
				//
				// This extends the medium API as follows:
				//
				//       * uint32 image type
				//     * not typesafe
				//     * separate filter types for each axis
				//     * separate edge modes for each axis
				//     * can specify scale explicitly for subpixel correctness
				//     * can specify image source tile using texture coordinates

				typedef enum
				{
					STBIR_TYPE_UINT8,
					STBIR_TYPE_UINT16,
					STBIR_TYPE_UINT32,
					STBIR_TYPE_FLOAT,

					STBIR_MAX_TYPES
				} stbir_datatype;

				int stbir_resize(const void *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					void *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					stbir_datatype datatype,
					int num_channels, int alpha_channel, int flags,
					stbir_edge edge_mode_horizontal, stbir_edge edge_mode_vertical,
					stbir_filter filter_horizontal, stbir_filter filter_vertical,
					stbir_colorspace space, void *alloc_context);

				int stbir_resize_subpixel(const void *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					void *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					stbir_datatype datatype,
					int num_channels, int alpha_channel, int flags,
					stbir_edge edge_mode_horizontal, stbir_edge edge_mode_vertical,
					stbir_filter filter_horizontal, stbir_filter filter_vertical,
					stbir_colorspace space, void *alloc_context,
					float x_scale, float y_scale,
					float x_offset, float y_offset);

				int stbir_resize_region(const void *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					void *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					stbir_datatype datatype,
					int num_channels, int alpha_channel, int flags,
					stbir_edge edge_mode_horizontal, stbir_edge edge_mode_vertical,
					stbir_filter filter_horizontal, stbir_filter filter_vertical,
					stbir_colorspace space, void *alloc_context,
					float s0, float t0, float s1, float t1);
				// (s0, t0) & (s1, t1) are the top-left and bottom right corner (uv addressing style: [0, 1]x[0, 1]) of a region of the input image to use.
				
#ifndef STBIR_ASSERT
//#include <assert.h>
#define STBIR_ASSERT(x) assert(x)
#endif

#ifdef STBIR_DEBUG
#define STBIR__DEBUG_ASSERT STBIR_ASSERT
#else
#define STBIR__DEBUG_ASSERT(x) misc::unused(x)	// fix for clang warning
#endif

				// If you hit this it means I haven't done it yet.
#define STBIR__UNIMPLEMENTED(x) STBIR_ASSERT(!(x))

				// For memset
#include <string.h>

#include <math.h>

#ifndef STBIR_MALLOC
#include <stdlib.h>
#define STBIR_MALLOC(size,c) malloc(size)
#define STBIR_FREE(ptr,c)    free(ptr)
#endif

#ifndef _MSC_VER
#ifdef __cplusplus
#define stbir__inline inline
#else
#define stbir__inline
#endif
#else
#define stbir__inline __forceinline
#endif


				// should produce compiler error if size is wrong
				typedef unsigned char stbir__validate_uint32[sizeof(stbir_uint32) == 4 ? 1 : -1];

#ifdef _MSC_VER
#define STBIR__NOTUSED(v)  (void)(v)
#else
#define STBIR__NOTUSED(v)  (void)sizeof(v)
#endif

#define STBIR__ARRAY_SIZE(a) (sizeof((a))/sizeof((a)[0]))

#ifndef STBIR_DEFAULT_FILTER_UPSAMPLE
#define STBIR_DEFAULT_FILTER_UPSAMPLE    STBIR_FILTER_CATMULLROM
#endif

#ifndef STBIR_DEFAULT_FILTER_DOWNSAMPLE
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE  STBIR_FILTER_MITCHELL
#endif

#ifndef STBIR_PROGRESS_REPORT
#define STBIR_PROGRESS_REPORT(float_0_to_1)
#endif

#ifndef STBIR_MAX_CHANNELS
#define STBIR_MAX_CHANNELS 64
#endif

#if STBIR_MAX_CHANNELS > 65536
#error "Too many channels; STBIR_MAX_CHANNELS must be no more than 65536."
				// because we store the indices in 16-bit variables
#endif

				// This value is added to alpha just before premultiplication to avoid
				// zeroing out color values. It is equivalent to 2^-80. If you don't want
				// that behavior (it may interfere if you have floating point images with
				// very small alpha values) then you can define STBIR_NO_ALPHA_EPSILON to
				// disable it.
#ifndef STBIR_ALPHA_EPSILON
#define STBIR_ALPHA_EPSILON ((float)1 / (1 << 20) / (1 << 20) / (1 << 20) / (1 << 20))
#endif



#ifdef _MSC_VER
#define STBIR__UNUSED_PARAM(v)  (void)(v)
#else
#define STBIR__UNUSED_PARAM(v)  (void)sizeof(v)
#endif

				// must match stbir_datatype
				unsigned char stbir__type_size[] = {
					1, // STBIR_TYPE_UINT8
					2, // STBIR_TYPE_UINT16
					4, // STBIR_TYPE_UINT32
					4, // STBIR_TYPE_FLOAT
				};

				// Kernel function centered at 0
				typedef float (stbir__kernel_fn)(float x, float scale);
				typedef float (stbir__support_fn)(float scale);

				typedef struct
				{
					stbir__kernel_fn* kernel;
					stbir__support_fn* support;
				} stbir__filter_info;

				// When upsampling, the contributors are which source pixels contribute.
				// When downsampling, the contributors are which destination pixels are contributed to.
				typedef struct
				{
					int n0; // First contributing pixel
					int n1; // Last contributing pixel
				} stbir__contributors;

				typedef struct
				{
					const void* input_data;
					int input_w;
					int input_h;
					int input_stride_bytes;

					void* output_data;
					int output_w;
					int output_h;
					int output_stride_bytes;

					float s0, t0, s1, t1;

					float horizontal_shift; // Units: output pixels
					float vertical_shift;   // Units: output pixels
					float horizontal_scale;
					float vertical_scale;

					int channels;
					int alpha_channel;
					stbir_uint32 flags;
					stbir_datatype type;
					stbir_filter horizontal_filter;
					stbir_filter vertical_filter;
					stbir_edge edge_horizontal;
					stbir_edge edge_vertical;
					stbir_colorspace colorspace;

					stbir__contributors* horizontal_contributors;
					float* horizontal_coefficients;

					stbir__contributors* vertical_contributors;
					float* vertical_coefficients;

					int decode_buffer_pixels;
					float* decode_buffer;

					float* horizontal_buffer;

					// cache these because ceil/floor are inexplicably showing up in profile
					int horizontal_coefficient_width;
					int vertical_coefficient_width;
					int horizontal_filter_pixel_width;
					int vertical_filter_pixel_width;
					int horizontal_filter_pixel_margin;
					int vertical_filter_pixel_margin;
					int horizontal_num_contributors;
					int vertical_num_contributors;

					int ring_buffer_length_bytes; // The length of an individual entry in the ring buffer. The total number of ring buffers is stbir__get_filter_pixel_width(filter)
					int ring_buffer_first_scanline;
					int ring_buffer_last_scanline;
					int ring_buffer_begin_index;
					float* ring_buffer;

					float* encode_buffer; // A temporary buffer to store floats so we don't lose precision while we do multiply-adds.

					int horizontal_contributors_size;
					int horizontal_coefficients_size;
					int vertical_contributors_size;
					int vertical_coefficients_size;
					int decode_buffer_size;
					int horizontal_buffer_size;
					int ring_buffer_size;
					int encode_buffer_size;
				} stbir__info;

				stbir__inline int stbir__min(int a, int b)
				{
					return a < b ? a : b;
				}

				stbir__inline int stbir__max(int a, int b)
				{
					return a > b ? a : b;
				}

				stbir__inline float stbir__saturate(float x)
				{
					if (x < 0)
						return 0;

					if (x > 1)
						return 1;

					return x;
				}

#ifdef STBIR_SATURATE_INT
				stbir__inline stbir_uint8 stbir__saturate8(int x)
				{
					if ((unsigned int)x <= 255)
						return x;

					if (x < 0)
						return 0;

					return 255;
				}

				stbir__inline stbir_uint16 stbir__saturate16(int x)
				{
					if ((unsigned int)x <= 65535)
						return x;

					if (x < 0)
						return 0;

					return 65535;
				}
#endif

				float stbir__srgb_uchar_to_linear_float[256] = {
					0.000000f, 0.000304f, 0.000607f, 0.000911f, 0.001214f, 0.001518f, 0.001821f, 0.002125f, 0.002428f, 0.002732f, 0.003035f,
					0.003347f, 0.003677f, 0.004025f, 0.004391f, 0.004777f, 0.005182f, 0.005605f, 0.006049f, 0.006512f, 0.006995f, 0.007499f,
					0.008023f, 0.008568f, 0.009134f, 0.009721f, 0.010330f, 0.010960f, 0.011612f, 0.012286f, 0.012983f, 0.013702f, 0.014444f,
					0.015209f, 0.015996f, 0.016807f, 0.017642f, 0.018500f, 0.019382f, 0.020289f, 0.021219f, 0.022174f, 0.023153f, 0.024158f,
					0.025187f, 0.026241f, 0.027321f, 0.028426f, 0.029557f, 0.030713f, 0.031896f, 0.033105f, 0.034340f, 0.035601f, 0.036889f,
					0.038204f, 0.039546f, 0.040915f, 0.042311f, 0.043735f, 0.045186f, 0.046665f, 0.048172f, 0.049707f, 0.051269f, 0.052861f,
					0.054480f, 0.056128f, 0.057805f, 0.059511f, 0.061246f, 0.063010f, 0.064803f, 0.066626f, 0.068478f, 0.070360f, 0.072272f,
					0.074214f, 0.076185f, 0.078187f, 0.080220f, 0.082283f, 0.084376f, 0.086500f, 0.088656f, 0.090842f, 0.093059f, 0.095307f,
					0.097587f, 0.099899f, 0.102242f, 0.104616f, 0.107023f, 0.109462f, 0.111932f, 0.114435f, 0.116971f, 0.119538f, 0.122139f,
					0.124772f, 0.127438f, 0.130136f, 0.132868f, 0.135633f, 0.138432f, 0.141263f, 0.144128f, 0.147027f, 0.149960f, 0.152926f,
					0.155926f, 0.158961f, 0.162029f, 0.165132f, 0.168269f, 0.171441f, 0.174647f, 0.177888f, 0.181164f, 0.184475f, 0.187821f,
					0.191202f, 0.194618f, 0.198069f, 0.201556f, 0.205079f, 0.208637f, 0.212231f, 0.215861f, 0.219526f, 0.223228f, 0.226966f,
					0.230740f, 0.234551f, 0.238398f, 0.242281f, 0.246201f, 0.250158f, 0.254152f, 0.258183f, 0.262251f, 0.266356f, 0.270498f,
					0.274677f, 0.278894f, 0.283149f, 0.287441f, 0.291771f, 0.296138f, 0.300544f, 0.304987f, 0.309469f, 0.313989f, 0.318547f,
					0.323143f, 0.327778f, 0.332452f, 0.337164f, 0.341914f, 0.346704f, 0.351533f, 0.356400f, 0.361307f, 0.366253f, 0.371238f,
					0.376262f, 0.381326f, 0.386430f, 0.391573f, 0.396755f, 0.401978f, 0.407240f, 0.412543f, 0.417885f, 0.423268f, 0.428691f,
					0.434154f, 0.439657f, 0.445201f, 0.450786f, 0.456411f, 0.462077f, 0.467784f, 0.473532f, 0.479320f, 0.485150f, 0.491021f,
					0.496933f, 0.502887f, 0.508881f, 0.514918f, 0.520996f, 0.527115f, 0.533276f, 0.539480f, 0.545725f, 0.552011f, 0.558340f,
					0.564712f, 0.571125f, 0.577581f, 0.584078f, 0.590619f, 0.597202f, 0.603827f, 0.610496f, 0.617207f, 0.623960f, 0.630757f,
					0.637597f, 0.644480f, 0.651406f, 0.658375f, 0.665387f, 0.672443f, 0.679543f, 0.686685f, 0.693872f, 0.701102f, 0.708376f,
					0.715694f, 0.723055f, 0.730461f, 0.737911f, 0.745404f, 0.752942f, 0.760525f, 0.768151f, 0.775822f, 0.783538f, 0.791298f,
					0.799103f, 0.806952f, 0.814847f, 0.822786f, 0.830770f, 0.838799f, 0.846873f, 0.854993f, 0.863157f, 0.871367f, 0.879622f,
					0.887923f, 0.896269f, 0.904661f, 0.913099f, 0.921582f, 0.930111f, 0.938686f, 0.947307f, 0.955974f, 0.964686f, 0.973445f,
					0.982251f, 0.991102f, 1.0f
				};

				float stbir__srgb_to_linear(float f)
				{
					if (f <= 0.04045f)
						return f / 12.92f;
					else
						return (float)pow((f + 0.055f) / 1.055f, 2.4f);
				}

				float stbir__linear_to_srgb(float f)
				{
					if (f <= 0.0031308f)
						return f * 12.92f;
					else
						return 1.055f * (float)pow(f, 1 / 2.4f) - 0.055f;
				}

#ifndef STBIR_NON_IEEE_FLOAT
				// From https://gist.github.com/rygorous/2203834

				typedef union
				{
					stbir_uint32 u;
					float f;
				} stbir__FP32;

				const stbir_uint32 fp32_to_srgb8_tab4[104] = {
					0x0073000d, 0x007a000d, 0x0080000d, 0x0087000d, 0x008d000d, 0x0094000d, 0x009a000d, 0x00a1000d,
					0x00a7001a, 0x00b4001a, 0x00c1001a, 0x00ce001a, 0x00da001a, 0x00e7001a, 0x00f4001a, 0x0101001a,
					0x010e0033, 0x01280033, 0x01410033, 0x015b0033, 0x01750033, 0x018f0033, 0x01a80033, 0x01c20033,
					0x01dc0067, 0x020f0067, 0x02430067, 0x02760067, 0x02aa0067, 0x02dd0067, 0x03110067, 0x03440067,
					0x037800ce, 0x03df00ce, 0x044600ce, 0x04ad00ce, 0x051400ce, 0x057b00c5, 0x05dd00bc, 0x063b00b5,
					0x06970158, 0x07420142, 0x07e30130, 0x087b0120, 0x090b0112, 0x09940106, 0x0a1700fc, 0x0a9500f2,
					0x0b0f01cb, 0x0bf401ae, 0x0ccb0195, 0x0d950180, 0x0e56016e, 0x0f0d015e, 0x0fbc0150, 0x10630143,
					0x11070264, 0x1238023e, 0x1357021d, 0x14660201, 0x156601e9, 0x165a01d3, 0x174401c0, 0x182401af,
					0x18fe0331, 0x1a9602fe, 0x1c1502d2, 0x1d7e02ad, 0x1ed4028d, 0x201a0270, 0x21520256, 0x227d0240,
					0x239f0443, 0x25c003fe, 0x27bf03c4, 0x29a10392, 0x2b6a0367, 0x2d1d0341, 0x2ebe031f, 0x304d0300,
					0x31d105b0, 0x34a80555, 0x37520507, 0x39d504c5, 0x3c37048b, 0x3e7c0458, 0x40a8042a, 0x42bd0401,
					0x44c20798, 0x488e071e, 0x4c1c06b6, 0x4f76065d, 0x52a50610, 0x55ac05cc, 0x5892058f, 0x5b590559,
					0x5e0c0a23, 0x631c0980, 0x67db08f6, 0x6c55087f, 0x70940818, 0x74a007bd, 0x787d076c, 0x7c330723,
				};

				stbir_uint8 stbir__linear_to_srgb_uchar(float in)
				{
					const stbir__FP32 almostone = { 0x3f7fffff }; // 1-eps
					const stbir__FP32 minval = { (127 - 13) << 23 };
					stbir_uint32 tab, bias, scale, t;
					stbir__FP32 f;

					// Clamp to [2^(-13), 1-eps]; these two values map to 0 and 1, respectively.
					// The tests are carefully written so that NaNs map to 0, same as in the reference
					// implementation.
					if (!(in > minval.f)) // written this way to catch NaNs
						in = minval.f;
					if (in > almostone.f)
						in = almostone.f;

					// Do the table lookup and unpack bias, scale
					f.f = in;
					tab = fp32_to_srgb8_tab4[(f.u - minval.u) >> 20];
					bias = (tab >> 16) << 9;
					scale = tab & 0xffff;

					// Grab next-highest mantissa bits and perform linear interpolation
					t = (f.u >> 12) & 0xff;
					return (unsigned char)((bias + scale*t) >> 16);
				}

#else
				// sRGB transition values, scaled by 1<<28
				int stbir__srgb_offset_to_linear_scaled[256] =
				{
					0, 40738, 122216, 203693, 285170, 366648, 448125, 529603,
					611080, 692557, 774035, 855852, 942009, 1033024, 1128971, 1229926,
					1335959, 1447142, 1563542, 1685229, 1812268, 1944725, 2082664, 2226148,
					2375238, 2529996, 2690481, 2856753, 3028870, 3206888, 3390865, 3580856,
					3776916, 3979100, 4187460, 4402049, 4622919, 4850123, 5083710, 5323731,
					5570236, 5823273, 6082892, 6349140, 6622065, 6901714, 7188133, 7481369,
					7781466, 8088471, 8402427, 8723380, 9051372, 9386448, 9728650, 10078021,
					10434603, 10798439, 11169569, 11548036, 11933879, 12327139, 12727857, 13136073,
					13551826, 13975156, 14406100, 14844697, 15290987, 15745007, 16206795, 16676389,
					17153826, 17639142, 18132374, 18633560, 19142734, 19659934, 20185196, 20718552,
					21260042, 21809696, 22367554, 22933648, 23508010, 24090680, 24681686, 25281066,
					25888850, 26505076, 27129772, 27762974, 28404716, 29055026, 29713942, 30381490,
					31057708, 31742624, 32436272, 33138682, 33849884, 34569912, 35298800, 36036568,
					36783260, 37538896, 38303512, 39077136, 39859796, 40651528, 41452360, 42262316,
					43081432, 43909732, 44747252, 45594016, 46450052, 47315392, 48190064, 49074096,
					49967516, 50870356, 51782636, 52704392, 53635648, 54576432, 55526772, 56486700,
					57456236, 58435408, 59424248, 60422780, 61431036, 62449032, 63476804, 64514376,
					65561776, 66619028, 67686160, 68763192, 69850160, 70947088, 72053992, 73170912,
					74297864, 75434880, 76581976, 77739184, 78906536, 80084040, 81271736, 82469648,
					83677792, 84896192, 86124888, 87363888, 88613232, 89872928, 91143016, 92423512,
					93714432, 95015816, 96327688, 97650056, 98982952, 100326408, 101680440, 103045072,
					104420320, 105806224, 107202800, 108610064, 110028048, 111456776, 112896264, 114346544,
					115807632, 117279552, 118762328, 120255976, 121760536, 123276016, 124802440, 126339832,
					127888216, 129447616, 131018048, 132599544, 134192112, 135795792, 137410592, 139036528,
					140673648, 142321952, 143981456, 145652208, 147334208, 149027488, 150732064, 152447968,
					154175200, 155913792, 157663776, 159425168, 161197984, 162982240, 164777968, 166585184,
					168403904, 170234160, 172075968, 173929344, 175794320, 177670896, 179559120, 181458992,
					183370528, 185293776, 187228736, 189175424, 191133888, 193104112, 195086128, 197079968,
					199085648, 201103184, 203132592, 205173888, 207227120, 209292272, 211369392, 213458480,
					215559568, 217672656, 219797792, 221934976, 224084240, 226245600, 228419056, 230604656,
					232802400, 235012320, 237234432, 239468736, 241715280, 243974080, 246245120, 248528464,
					250824112, 253132064, 255452368, 257785040, 260130080, 262487520, 264857376, 267239664,
				};

				stbir_uint8 stbir__linear_to_srgb_uchar(float f)
				{
					int x = (int)(f * (1 << 28)); // has headroom so you don't need to clamp
					int v = 0;
					int i;

					// Refine the guess with a short binary search.
					i = v + 128; if (x >= stbir__srgb_offset_to_linear_scaled[i]) v = i;
					i = v + 64; if (x >= stbir__srgb_offset_to_linear_scaled[i]) v = i;
					i = v + 32; if (x >= stbir__srgb_offset_to_linear_scaled[i]) v = i;
					i = v + 16; if (x >= stbir__srgb_offset_to_linear_scaled[i]) v = i;
					i = v + 8; if (x >= stbir__srgb_offset_to_linear_scaled[i]) v = i;
					i = v + 4; if (x >= stbir__srgb_offset_to_linear_scaled[i]) v = i;
					i = v + 2; if (x >= stbir__srgb_offset_to_linear_scaled[i]) v = i;
					i = v + 1; if (x >= stbir__srgb_offset_to_linear_scaled[i]) v = i;

					return (stbir_uint8)v;
				}
#endif

				float stbir__filter_trapezoid(float x, float scale)
				{
					float halfscale = scale / 2;
					float t = 0.5f + halfscale;
					STBIR__DEBUG_ASSERT(scale <= 1);

					x = (float)fabs(x);

					if (x >= t)
						return 0;
					else
					{
						float r = 0.5f - halfscale;
						if (x <= r)
							return 1;
						else
							return (t - x) / scale;
					}
				}

				float stbir__support_trapezoid(float scale)
				{
					STBIR__DEBUG_ASSERT(scale <= 1);
					return 0.5f + scale / 2;
				}

				float stbir__filter_triangle(float x, float s)
				{
					STBIR__UNUSED_PARAM(s);

					x = (float)fabs(x);

					if (x <= 1.0f)
						return 1 - x;
					else
						return 0;
				}

				float stbir__filter_cubic(float x, float s)
				{
					STBIR__UNUSED_PARAM(s);

					x = (float)fabs(x);

					if (x < 1.0f)
						return (4 + x*x*(3 * x - 6)) / 6;
					else if (x < 2.0f)
						return (8 + x*(-12 + x*(6 - x))) / 6;

					return (0.0f);
				}

				float stbir__filter_catmullrom(float x, float s)
				{
					STBIR__UNUSED_PARAM(s);

					x = (float)fabs(x);

					if (x < 1.0f)
						return 1 - x*x*(2.5f - 1.5f*x);
					else if (x < 2.0f)
						return 2 - x*(4 + x*(0.5f*x - 2.5f));

					return (0.0f);
				}

				float stbir__filter_mitchell(float x, float s)
				{
					STBIR__UNUSED_PARAM(s);

					x = (float)fabs(x);

					if (x < 1.0f)
						return (16 + x*x*(21 * x - 36)) / 18;
					else if (x < 2.0f)
						return (32 + x*(-60 + x*(36 - 7 * x))) / 18;

					return (0.0f);
				}

				float stbir__support_zero(float s)
				{
					STBIR__UNUSED_PARAM(s);
					return 0;
				}

				float stbir__support_one(float s)
				{
					STBIR__UNUSED_PARAM(s);
					return 1;
				}

				float stbir__support_two(float s)
				{
					STBIR__UNUSED_PARAM(s);
					return 2;
				}

				stbir__filter_info stbir__filter_info_table[] = {
					{ NULL, stbir__support_zero },
					{ stbir__filter_trapezoid, stbir__support_trapezoid },
					{ stbir__filter_triangle, stbir__support_one },
					{ stbir__filter_cubic, stbir__support_two },
					{ stbir__filter_catmullrom, stbir__support_two },
					{ stbir__filter_mitchell, stbir__support_two },
				};

				stbir__inline int stbir__use_upsampling(float ratio)
				{
					return ratio > 1;
				}

				stbir__inline int stbir__use_width_upsampling(stbir__info* stbir_info)
				{
					return stbir__use_upsampling(stbir_info->horizontal_scale);
				}

				stbir__inline int stbir__use_height_upsampling(stbir__info* stbir_info)
				{
					return stbir__use_upsampling(stbir_info->vertical_scale);
				}

				// This is the maximum number of input samples that can affect an output sample
				// with the given filter
				int stbir__get_filter_pixel_width(stbir_filter filter, float scale)
				{
					STBIR_ASSERT(filter != 0);
					STBIR_ASSERT(filter < STBIR__ARRAY_SIZE(stbir__filter_info_table));

					if (stbir__use_upsampling(scale))
						return (int)ceil(stbir__filter_info_table[filter].support(1 / scale) * 2);
					else
						return (int)ceil(stbir__filter_info_table[filter].support(scale) * 2 / scale);
				}

				// This is how much to expand buffers to account for filters seeking outside
				// the image boundaries.
				int stbir__get_filter_pixel_margin(stbir_filter filter, float scale)
				{
					return stbir__get_filter_pixel_width(filter, scale) / 2;
				}

				int stbir__get_coefficient_width(stbir_filter filter, float scale)
				{
					if (stbir__use_upsampling(scale))
						return (int)ceil(stbir__filter_info_table[filter].support(1 / scale) * 2);
					else
						return (int)ceil(stbir__filter_info_table[filter].support(scale) * 2);
				}

				int stbir__get_contributors(float scale, stbir_filter filter, int input_size, int output_size)
				{
					if (stbir__use_upsampling(scale))
						return output_size;
					else
						return (input_size + stbir__get_filter_pixel_margin(filter, scale) * 2);
				}

				int stbir__get_total_horizontal_coefficients(stbir__info* info)
				{
					return info->horizontal_num_contributors
						* stbir__get_coefficient_width(info->horizontal_filter, info->horizontal_scale);
				}

				int stbir__get_total_vertical_coefficients(stbir__info* info)
				{
					return info->vertical_num_contributors
						* stbir__get_coefficient_width(info->vertical_filter, info->vertical_scale);
				}

				stbir__contributors* stbir__get_contributor(stbir__contributors* contributors, int n)
				{
					return &contributors[n];
				}

				// For perf reasons this code is duplicated in stbir__resample_horizontal_upsample/downsample,
				// if you change it here change it there too.
				float* stbir__get_coefficient(float* coefficients, stbir_filter filter, float scale, int n, int c)
				{
					int width = stbir__get_coefficient_width(filter, scale);
					return &coefficients[width*n + c];
				}

				int stbir__edge_wrap_slow(stbir_edge edge, int n, int max)
				{
					switch (edge)
					{
					case STBIR_EDGE_ZERO:
						return 0; // we'll decode the wrong pixel here, and then overwrite with 0s later

					case STBIR_EDGE_CLAMP:
						if (n < 0)
							return 0;

						if (n >= max)
							return max - 1;

						return n; // NOTREACHED

					case STBIR_EDGE_REFLECT:
					{
											   if (n < 0)
											   {
												   if (n < max)
													   return -n;
												   else
													   return max - 1;
											   }

											   if (n >= max)
											   {
												   int max2 = max * 2;
												   if (n >= max2)
													   return 0;
												   else
													   return max2 - n - 1;
											   }

											   return n; // NOTREACHED
					}

					case STBIR_EDGE_WRAP:
						if (n >= 0)
							return (n % max);
						else
						{
							int m = (-n) % max;

							if (m != 0)
								m = max - m;

							return (m);
						}
						return n;  // NOTREACHED

					default:
						STBIR__UNIMPLEMENTED("Unimplemented edge type");
						return 0;
					}
				}

				stbir__inline int stbir__edge_wrap(stbir_edge edge, int n, int max)
				{
					// avoid per-pixel switch
					if (n >= 0 && n < max)
						return n;
					return stbir__edge_wrap_slow(edge, n, max);
				}

				// What input pixels contribute to this output pixel?
				void stbir__calculate_sample_range_upsample(int n, float out_filter_radius, float scale_ratio, float out_shift, int* in_first_pixel, int* in_last_pixel, float* in_center_of_out)
				{
					float out_pixel_center = (float)n + 0.5f;
					float out_pixel_influence_lowerbound = out_pixel_center - out_filter_radius;
					float out_pixel_influence_upperbound = out_pixel_center + out_filter_radius;

					float in_pixel_influence_lowerbound = (out_pixel_influence_lowerbound + out_shift) / scale_ratio;
					float in_pixel_influence_upperbound = (out_pixel_influence_upperbound + out_shift) / scale_ratio;

					*in_center_of_out = (out_pixel_center + out_shift) / scale_ratio;
					*in_first_pixel = (int)(floor(in_pixel_influence_lowerbound + 0.5));
					*in_last_pixel = (int)(floor(in_pixel_influence_upperbound - 0.5));
				}

				// What output pixels does this input pixel contribute to?
				void stbir__calculate_sample_range_downsample(int n, float in_pixels_radius, float scale_ratio, float out_shift, int* out_first_pixel, int* out_last_pixel, float* out_center_of_in)
				{
					float in_pixel_center = (float)n + 0.5f;
					float in_pixel_influence_lowerbound = in_pixel_center - in_pixels_radius;
					float in_pixel_influence_upperbound = in_pixel_center + in_pixels_radius;

					float out_pixel_influence_lowerbound = in_pixel_influence_lowerbound * scale_ratio - out_shift;
					float out_pixel_influence_upperbound = in_pixel_influence_upperbound * scale_ratio - out_shift;

					*out_center_of_in = in_pixel_center * scale_ratio - out_shift;
					*out_first_pixel = (int)(floor(out_pixel_influence_lowerbound + 0.5));
					*out_last_pixel = (int)(floor(out_pixel_influence_upperbound - 0.5));
				}

				void stbir__calculate_coefficients_upsample(stbir__info* stbir_info, stbir_filter filter, float scale, int in_first_pixel, int in_last_pixel, float in_center_of_out, stbir__contributors* contributor, float* coefficient_group)
				{
					int i;
					float total_filter = 0;
					float filter_scale;

					STBIR__DEBUG_ASSERT(in_last_pixel - in_first_pixel <= (int)ceil(stbir__filter_info_table[filter].support(1 / scale) * 2)); // Taken directly from stbir__get_coefficient_width() which we can't call because we don't know if we're horizontal or vertical.

					contributor->n0 = in_first_pixel;
					contributor->n1 = in_last_pixel;

					STBIR__DEBUG_ASSERT(contributor->n1 >= contributor->n0);

					for (i = 0; i <= in_last_pixel - in_first_pixel; i++)
					{
						float in_pixel_center = (float)(i + in_first_pixel) + 0.5f;
						coefficient_group[i] = stbir__filter_info_table[filter].kernel(in_center_of_out - in_pixel_center, 1 / scale);

						// If the coefficient is zero, skip it. (Don't do the <0 check here, we want the influence of those outside pixels.)
						if (i == 0 && !coefficient_group[i])
						{
							contributor->n0 = ++in_first_pixel;
							i--;
							continue;
						}

						total_filter += coefficient_group[i];
					}

					STBIR__DEBUG_ASSERT(stbir__filter_info_table[filter].kernel((float)(in_last_pixel + 1) + 0.5f - in_center_of_out, 1 / scale) == 0);

					STBIR__DEBUG_ASSERT(total_filter > 0.9);
					STBIR__DEBUG_ASSERT(total_filter < 1.1f); // Make sure it's not way off.

					// Make sure the sum of all coefficients is 1.
					filter_scale = 1 / total_filter;

					for (i = 0; i <= in_last_pixel - in_first_pixel; i++)
						coefficient_group[i] *= filter_scale;

					for (i = in_last_pixel - in_first_pixel; i >= 0; i--)
					{
						if (coefficient_group[i])
							break;

						// This line has no weight. We can skip it.
						contributor->n1 = contributor->n0 + i - 1;
					}
				}

				void stbir__calculate_coefficients_downsample(stbir__info* stbir_info, stbir_filter filter, float scale_ratio, int out_first_pixel, int out_last_pixel, float out_center_of_in, stbir__contributors* contributor, float* coefficient_group)
				{
					int i;

					STBIR__DEBUG_ASSERT(out_last_pixel - out_first_pixel <= (int)ceil(stbir__filter_info_table[filter].support(scale_ratio) * 2)); // Taken directly from stbir__get_coefficient_width() which we can't call because we don't know if we're horizontal or vertical.

					contributor->n0 = out_first_pixel;
					contributor->n1 = out_last_pixel;

					STBIR__DEBUG_ASSERT(contributor->n1 >= contributor->n0);

					for (i = 0; i <= out_last_pixel - out_first_pixel; i++)
					{
						float out_pixel_center = (float)(i + out_first_pixel) + 0.5f;
						float x = out_pixel_center - out_center_of_in;
						coefficient_group[i] = stbir__filter_info_table[filter].kernel(x, scale_ratio) * scale_ratio;
					}

					STBIR__DEBUG_ASSERT(stbir__filter_info_table[filter].kernel((float)(out_last_pixel + 1) + 0.5f - out_center_of_in, scale_ratio) == 0);

					for (i = out_last_pixel - out_first_pixel; i >= 0; i--)
					{
						if (coefficient_group[i])
							break;

						// This line has no weight. We can skip it.
						contributor->n1 = contributor->n0 + i - 1;
					}
				}

				void stbir__normalize_downsample_coefficients(stbir__info* stbir_info, stbir__contributors* contributors, float* coefficients, stbir_filter filter, float scale_ratio, float shift, int input_size, int output_size)
				{
					int num_contributors = stbir__get_contributors(scale_ratio, filter, input_size, output_size);
					int num_coefficients = stbir__get_coefficient_width(filter, scale_ratio);
					int i, j;
					int skip;

					for (i = 0; i < output_size; i++)
					{
						float scale;
						float total = 0;

						for (j = 0; j < num_contributors; j++)
						{
							if (i >= contributors[j].n0 && i <= contributors[j].n1)
							{
								float coefficient = *stbir__get_coefficient(coefficients, filter, scale_ratio, j, i - contributors[j].n0);
								total += coefficient;
							}
							else if (i < contributors[j].n0)
								break;
						}

						STBIR__DEBUG_ASSERT(total > 0.9f);
						STBIR__DEBUG_ASSERT(total < 1.1f);

						scale = 1 / total;

						for (j = 0; j < num_contributors; j++)
						{
							if (i >= contributors[j].n0 && i <= contributors[j].n1)
								*stbir__get_coefficient(coefficients, filter, scale_ratio, j, i - contributors[j].n0) *= scale;
							else if (i < contributors[j].n0)
								break;
						}
					}

					// Optimize: Skip zero coefficients and contributions outside of image bounds.
					// Do this after normalizing because normalization depends on the n0/n1 values.
					for (j = 0; j < num_contributors; j++)
					{
						int range, max, width;

						skip = 0;
						while (*stbir__get_coefficient(coefficients, filter, scale_ratio, j, skip) == 0)
							skip++;

						contributors[j].n0 += skip;

						while (contributors[j].n0 < 0)
						{
							contributors[j].n0++;
							skip++;
						}

						range = contributors[j].n1 - contributors[j].n0 + 1;
						max = stbir__min(num_coefficients, range);

						width = stbir__get_coefficient_width(filter, scale_ratio);
						for (i = 0; i < max; i++)
						{
							if (i + skip >= width)
								break;

							*stbir__get_coefficient(coefficients, filter, scale_ratio, j, i) = *stbir__get_coefficient(coefficients, filter, scale_ratio, j, i + skip);
						}

						continue;
					}

					// Using min to avoid writing into invalid pixels.
					for (i = 0; i < num_contributors; i++)
						contributors[i].n1 = stbir__min(contributors[i].n1, output_size - 1);
				}

				// Each scan line uses the same kernel values so we should calculate the kernel
				// values once and then we can use them for every scan line.
				void stbir__calculate_filters(stbir__info* stbir_info, stbir__contributors* contributors, float* coefficients, stbir_filter filter, float scale_ratio, float shift, int input_size, int output_size)
				{
					int n;
					int total_contributors = stbir__get_contributors(scale_ratio, filter, input_size, output_size);

					if (stbir__use_upsampling(scale_ratio))
					{
						float out_pixels_radius = stbir__filter_info_table[filter].support(1 / scale_ratio) * scale_ratio;

						// Looping through out pixels
						for (n = 0; n < total_contributors; n++)
						{
							float in_center_of_out; // Center of the current out pixel in the in pixel space
							int in_first_pixel, in_last_pixel;

							stbir__calculate_sample_range_upsample(n, out_pixels_radius, scale_ratio, shift, &in_first_pixel, &in_last_pixel, &in_center_of_out);

							stbir__calculate_coefficients_upsample(stbir_info, filter, scale_ratio, in_first_pixel, in_last_pixel, in_center_of_out, stbir__get_contributor(contributors, n), stbir__get_coefficient(coefficients, filter, scale_ratio, n, 0));
						}
					}
					else
					{
						float in_pixels_radius = stbir__filter_info_table[filter].support(scale_ratio) / scale_ratio;

						// Looping through in pixels
						for (n = 0; n < total_contributors; n++)
						{
							float out_center_of_in; // Center of the current out pixel in the in pixel space
							int out_first_pixel, out_last_pixel;
							int n_adjusted = n - stbir__get_filter_pixel_margin(filter, scale_ratio);

							stbir__calculate_sample_range_downsample(n_adjusted, in_pixels_radius, scale_ratio, shift, &out_first_pixel, &out_last_pixel, &out_center_of_in);

							stbir__calculate_coefficients_downsample(stbir_info, filter, scale_ratio, out_first_pixel, out_last_pixel, out_center_of_in, stbir__get_contributor(contributors, n), stbir__get_coefficient(coefficients, filter, scale_ratio, n, 0));
						}

						stbir__normalize_downsample_coefficients(stbir_info, contributors, coefficients, filter, scale_ratio, shift, input_size, output_size);
					}
				}

				float* stbir__get_decode_buffer(stbir__info* stbir_info)
				{
					// The 0 index of the decode buffer starts after the margin. This makes
					// it okay to use negative indexes on the decode buffer.
					return &stbir_info->decode_buffer[stbir_info->horizontal_filter_pixel_margin * stbir_info->channels];
				}

#define STBIR__DECODE(type, colorspace) ((type) * (STBIR_MAX_COLORSPACES) + (colorspace))

				void stbir__decode_scanline(stbir__info* stbir_info, int n)
				{
					int c;
					int channels = stbir_info->channels;
					int alpha_channel = stbir_info->alpha_channel;
					int type = stbir_info->type;
					int colorspace = stbir_info->colorspace;
					int input_w = stbir_info->input_w;
					int input_stride_bytes = stbir_info->input_stride_bytes;
					float* decode_buffer = stbir__get_decode_buffer(stbir_info);
					stbir_edge edge_horizontal = stbir_info->edge_horizontal;
					stbir_edge edge_vertical = stbir_info->edge_vertical;
					int in_buffer_row_offset = stbir__edge_wrap(edge_vertical, n, stbir_info->input_h) * input_stride_bytes;
					const void* input_data = (char *)stbir_info->input_data + in_buffer_row_offset;
					int max_x = input_w + stbir_info->horizontal_filter_pixel_margin;
					int decode = STBIR__DECODE(type, colorspace);

					int x = -stbir_info->horizontal_filter_pixel_margin;

					// special handling for STBIR_EDGE_ZERO because it needs to return an item that doesn't appear in the input,
					// and we want to avoid paying overhead on every pixel if not STBIR_EDGE_ZERO
					if (edge_vertical == STBIR_EDGE_ZERO && (n < 0 || n >= stbir_info->input_h))
					{
						for (; x < max_x; x++)
						for (c = 0; c < channels; c++)
							decode_buffer[x*channels + c] = 0;
						return;
					}

					switch (decode)
					{
					case STBIR__DECODE(STBIR_TYPE_UINT8, STBIR_COLORSPACE_LINEAR):
						for (; x < max_x; x++)
						{
							int decode_pixel_index = x * channels;
							int input_pixel_index = stbir__edge_wrap(edge_horizontal, x, input_w) * channels;
							for (c = 0; c < channels; c++)
								decode_buffer[decode_pixel_index + c] = ((float)((const unsigned char*)input_data)[input_pixel_index + c]) / 255;
						}
						break;

					case STBIR__DECODE(STBIR_TYPE_UINT8, STBIR_COLORSPACE_SRGB):
						for (; x < max_x; x++)
						{
							int decode_pixel_index = x * channels;
							int input_pixel_index = stbir__edge_wrap(edge_horizontal, x, input_w) * channels;
							for (c = 0; c < channels; c++)
								decode_buffer[decode_pixel_index + c] = stbir__srgb_uchar_to_linear_float[((const unsigned char*)input_data)[input_pixel_index + c]];

							if (!(stbir_info->flags&STBIR_FLAG_ALPHA_USES_COLORSPACE))
								decode_buffer[decode_pixel_index + alpha_channel] = ((float)((const unsigned char*)input_data)[input_pixel_index + alpha_channel]) / 255;
						}
						break;

					case STBIR__DECODE(STBIR_TYPE_UINT16, STBIR_COLORSPACE_LINEAR):
						for (; x < max_x; x++)
						{
							int decode_pixel_index = x * channels;
							int input_pixel_index = stbir__edge_wrap(edge_horizontal, x, input_w) * channels;
							for (c = 0; c < channels; c++)
								decode_buffer[decode_pixel_index + c] = ((float)((const unsigned short*)input_data)[input_pixel_index + c]) / 65535;
						}
						break;

					case STBIR__DECODE(STBIR_TYPE_UINT16, STBIR_COLORSPACE_SRGB):
						for (; x < max_x; x++)
						{
							int decode_pixel_index = x * channels;
							int input_pixel_index = stbir__edge_wrap(edge_horizontal, x, input_w) * channels;
							for (c = 0; c < channels; c++)
								decode_buffer[decode_pixel_index + c] = stbir__srgb_to_linear(((float)((const unsigned short*)input_data)[input_pixel_index + c]) / 65535);

							if (!(stbir_info->flags&STBIR_FLAG_ALPHA_USES_COLORSPACE))
								decode_buffer[decode_pixel_index + alpha_channel] = ((float)((const unsigned short*)input_data)[input_pixel_index + alpha_channel]) / 65535;
						}
						break;

					case STBIR__DECODE(STBIR_TYPE_UINT32, STBIR_COLORSPACE_LINEAR):
						for (; x < max_x; x++)
						{
							int decode_pixel_index = x * channels;
							int input_pixel_index = stbir__edge_wrap(edge_horizontal, x, input_w) * channels;
							for (c = 0; c < channels; c++)
								decode_buffer[decode_pixel_index + c] = (float)(((double)((const unsigned int*)input_data)[input_pixel_index + c]) / 4294967295);
						}
						break;

					case STBIR__DECODE(STBIR_TYPE_UINT32, STBIR_COLORSPACE_SRGB):
						for (; x < max_x; x++)
						{
							int decode_pixel_index = x * channels;
							int input_pixel_index = stbir__edge_wrap(edge_horizontal, x, input_w) * channels;
							for (c = 0; c < channels; c++)
								decode_buffer[decode_pixel_index + c] = stbir__srgb_to_linear((float)(((double)((const unsigned int*)input_data)[input_pixel_index + c]) / 4294967295));

							if (!(stbir_info->flags&STBIR_FLAG_ALPHA_USES_COLORSPACE))
								decode_buffer[decode_pixel_index + alpha_channel] = (float)(((double)((const unsigned int*)input_data)[input_pixel_index + alpha_channel]) / 4294967295);
						}
						break;

					case STBIR__DECODE(STBIR_TYPE_FLOAT, STBIR_COLORSPACE_LINEAR):
						for (; x < max_x; x++)
						{
							int decode_pixel_index = x * channels;
							int input_pixel_index = stbir__edge_wrap(edge_horizontal, x, input_w) * channels;
							for (c = 0; c < channels; c++)
								decode_buffer[decode_pixel_index + c] = ((const float*)input_data)[input_pixel_index + c];
						}
						break;

					case STBIR__DECODE(STBIR_TYPE_FLOAT, STBIR_COLORSPACE_SRGB):
						for (; x < max_x; x++)
						{
							int decode_pixel_index = x * channels;
							int input_pixel_index = stbir__edge_wrap(edge_horizontal, x, input_w) * channels;
							for (c = 0; c < channels; c++)
								decode_buffer[decode_pixel_index + c] = stbir__srgb_to_linear(((const float*)input_data)[input_pixel_index + c]);

							if (!(stbir_info->flags&STBIR_FLAG_ALPHA_USES_COLORSPACE))
								decode_buffer[decode_pixel_index + alpha_channel] = ((const float*)input_data)[input_pixel_index + alpha_channel];
						}

						break;

					default:
						STBIR__UNIMPLEMENTED("Unknown type/colorspace/channels combination.");
						break;
					}

					if (!(stbir_info->flags & STBIR_FLAG_ALPHA_PREMULTIPLIED))
					{
						for (x = -stbir_info->horizontal_filter_pixel_margin; x < max_x; x++)
						{
							int decode_pixel_index = x * channels;

							// If the alpha value is 0 it will clobber the color values. Make sure it's not.
							float alpha = decode_buffer[decode_pixel_index + alpha_channel];
#ifndef STBIR_NO_ALPHA_EPSILON
							if (stbir_info->type != STBIR_TYPE_FLOAT) {
								alpha += STBIR_ALPHA_EPSILON;
								decode_buffer[decode_pixel_index + alpha_channel] = alpha;
							}
#endif
							for (c = 0; c < channels; c++)
							{
								if (c == alpha_channel)
									continue;

								decode_buffer[decode_pixel_index + c] *= alpha;
							}
						}
					}

					if (edge_horizontal == STBIR_EDGE_ZERO)
					{
						for (x = -stbir_info->horizontal_filter_pixel_margin; x < 0; x++)
						{
							for (c = 0; c < channels; c++)
								decode_buffer[x*channels + c] = 0;
						}
						for (x = input_w; x < max_x; x++)
						{
							for (c = 0; c < channels; c++)
								decode_buffer[x*channels + c] = 0;
						}
					}
				}

				float* stbir__get_ring_buffer_entry(float* ring_buffer, int index, int ring_buffer_length)
				{
					return &ring_buffer[index * ring_buffer_length];
				}

				float* stbir__add_empty_ring_buffer_entry(stbir__info* stbir_info, int n)
				{
					int ring_buffer_index;
					float* ring_buffer;

					if (stbir_info->ring_buffer_begin_index < 0)
					{
						ring_buffer_index = stbir_info->ring_buffer_begin_index = 0;
						stbir_info->ring_buffer_first_scanline = n;
					}
					else
					{
						ring_buffer_index = (stbir_info->ring_buffer_begin_index + (stbir_info->ring_buffer_last_scanline - stbir_info->ring_buffer_first_scanline) + 1) % stbir_info->vertical_filter_pixel_width;
						STBIR__DEBUG_ASSERT(ring_buffer_index != stbir_info->ring_buffer_begin_index);
					}

					ring_buffer = stbir__get_ring_buffer_entry(stbir_info->ring_buffer, ring_buffer_index, stbir_info->ring_buffer_length_bytes / sizeof(float));
					memset(ring_buffer, 0, stbir_info->ring_buffer_length_bytes);

					stbir_info->ring_buffer_last_scanline = n;

					return ring_buffer;
				}


				void stbir__resample_horizontal_upsample(stbir__info* stbir_info, int n, float* output_buffer)
				{
					int x, k;
					int output_w = stbir_info->output_w;
					int kernel_pixel_width = stbir_info->horizontal_filter_pixel_width;
					int channels = stbir_info->channels;
					float* decode_buffer = stbir__get_decode_buffer(stbir_info);
					stbir__contributors* horizontal_contributors = stbir_info->horizontal_contributors;
					float* horizontal_coefficients = stbir_info->horizontal_coefficients;
					int coefficient_width = stbir_info->horizontal_coefficient_width;

					for (x = 0; x < output_w; x++)
					{
						int n0 = horizontal_contributors[x].n0;
						int n1 = horizontal_contributors[x].n1;

						int out_pixel_index = x * channels;
						int coefficient_group = coefficient_width * x;
						int coefficient_counter = 0;

						STBIR__DEBUG_ASSERT(n1 >= n0);
						STBIR__DEBUG_ASSERT(n0 >= -stbir_info->horizontal_filter_pixel_margin);
						STBIR__DEBUG_ASSERT(n1 >= -stbir_info->horizontal_filter_pixel_margin);
						STBIR__DEBUG_ASSERT(n0 < stbir_info->input_w + stbir_info->horizontal_filter_pixel_margin);
						STBIR__DEBUG_ASSERT(n1 < stbir_info->input_w + stbir_info->horizontal_filter_pixel_margin);

						switch (channels) {
						case 1:
							for (k = n0; k <= n1; k++)
							{
								int in_pixel_index = k * 1;
								float coefficient = horizontal_coefficients[coefficient_group + coefficient_counter++];
								STBIR__DEBUG_ASSERT(coefficient != 0);
								output_buffer[out_pixel_index + 0] += decode_buffer[in_pixel_index + 0] * coefficient;
							}
							break;
						case 2:
							for (k = n0; k <= n1; k++)
							{
								int in_pixel_index = k * 2;
								float coefficient = horizontal_coefficients[coefficient_group + coefficient_counter++];
								STBIR__DEBUG_ASSERT(coefficient != 0);
								output_buffer[out_pixel_index + 0] += decode_buffer[in_pixel_index + 0] * coefficient;
								output_buffer[out_pixel_index + 1] += decode_buffer[in_pixel_index + 1] * coefficient;
							}
							break;
						case 3:
							for (k = n0; k <= n1; k++)
							{
								int in_pixel_index = k * 3;
								float coefficient = horizontal_coefficients[coefficient_group + coefficient_counter++];
								STBIR__DEBUG_ASSERT(coefficient != 0);
								output_buffer[out_pixel_index + 0] += decode_buffer[in_pixel_index + 0] * coefficient;
								output_buffer[out_pixel_index + 1] += decode_buffer[in_pixel_index + 1] * coefficient;
								output_buffer[out_pixel_index + 2] += decode_buffer[in_pixel_index + 2] * coefficient;
							}
							break;
						case 4:
							for (k = n0; k <= n1; k++)
							{
								int in_pixel_index = k * 4;
								float coefficient = horizontal_coefficients[coefficient_group + coefficient_counter++];
								STBIR__DEBUG_ASSERT(coefficient != 0);
								output_buffer[out_pixel_index + 0] += decode_buffer[in_pixel_index + 0] * coefficient;
								output_buffer[out_pixel_index + 1] += decode_buffer[in_pixel_index + 1] * coefficient;
								output_buffer[out_pixel_index + 2] += decode_buffer[in_pixel_index + 2] * coefficient;
								output_buffer[out_pixel_index + 3] += decode_buffer[in_pixel_index + 3] * coefficient;
							}
							break;
						default:
							for (k = n0; k <= n1; k++)
							{
								int in_pixel_index = k * channels;
								float coefficient = horizontal_coefficients[coefficient_group + coefficient_counter++];
								int c;
								STBIR__DEBUG_ASSERT(coefficient != 0);
								for (c = 0; c < channels; c++)
									output_buffer[out_pixel_index + c] += decode_buffer[in_pixel_index + c] * coefficient;
							}
							break;
						}
					}
				}

				void stbir__resample_horizontal_downsample(stbir__info* stbir_info, int n, float* output_buffer)
				{
					int x, k;
					int input_w = stbir_info->input_w;
					int output_w = stbir_info->output_w;
					int kernel_pixel_width = stbir_info->horizontal_filter_pixel_width;
					int channels = stbir_info->channels;
					float* decode_buffer = stbir__get_decode_buffer(stbir_info);
					stbir__contributors* horizontal_contributors = stbir_info->horizontal_contributors;
					float* horizontal_coefficients = stbir_info->horizontal_coefficients;
					int coefficient_width = stbir_info->horizontal_coefficient_width;
					int filter_pixel_margin = stbir_info->horizontal_filter_pixel_margin;
					int max_x = input_w + filter_pixel_margin * 2;

					STBIR__DEBUG_ASSERT(!stbir__use_width_upsampling(stbir_info));

					switch (channels) {
					case 1:
						for (x = 0; x < max_x; x++)
						{
							int n0 = horizontal_contributors[x].n0;
							int n1 = horizontal_contributors[x].n1;

							int in_x = x - filter_pixel_margin;
							int in_pixel_index = in_x * 1;
							int max_n = n1;
							int coefficient_group = coefficient_width * x;

							for (k = n0; k <= max_n; k++)
							{
								int out_pixel_index = k * 1;
								float coefficient = horizontal_coefficients[coefficient_group + k - n0];
								STBIR__DEBUG_ASSERT(coefficient != 0);
								output_buffer[out_pixel_index + 0] += decode_buffer[in_pixel_index + 0] * coefficient;
							}
						}
						break;

					case 2:
						for (x = 0; x < max_x; x++)
						{
							int n0 = horizontal_contributors[x].n0;
							int n1 = horizontal_contributors[x].n1;

							int in_x = x - filter_pixel_margin;
							int in_pixel_index = in_x * 2;
							int max_n = n1;
							int coefficient_group = coefficient_width * x;

							for (k = n0; k <= max_n; k++)
							{
								int out_pixel_index = k * 2;
								float coefficient = horizontal_coefficients[coefficient_group + k - n0];
								STBIR__DEBUG_ASSERT(coefficient != 0);
								output_buffer[out_pixel_index + 0] += decode_buffer[in_pixel_index + 0] * coefficient;
								output_buffer[out_pixel_index + 1] += decode_buffer[in_pixel_index + 1] * coefficient;
							}
						}
						break;

					case 3:
						for (x = 0; x < max_x; x++)
						{
							int n0 = horizontal_contributors[x].n0;
							int n1 = horizontal_contributors[x].n1;

							int in_x = x - filter_pixel_margin;
							int in_pixel_index = in_x * 3;
							int max_n = n1;
							int coefficient_group = coefficient_width * x;

							for (k = n0; k <= max_n; k++)
							{
								int out_pixel_index = k * 3;
								float coefficient = horizontal_coefficients[coefficient_group + k - n0];
								STBIR__DEBUG_ASSERT(coefficient != 0);
								output_buffer[out_pixel_index + 0] += decode_buffer[in_pixel_index + 0] * coefficient;
								output_buffer[out_pixel_index + 1] += decode_buffer[in_pixel_index + 1] * coefficient;
								output_buffer[out_pixel_index + 2] += decode_buffer[in_pixel_index + 2] * coefficient;
							}
						}
						break;

					case 4:
						for (x = 0; x < max_x; x++)
						{
							int n0 = horizontal_contributors[x].n0;
							int n1 = horizontal_contributors[x].n1;

							int in_x = x - filter_pixel_margin;
							int in_pixel_index = in_x * 4;
							int max_n = n1;
							int coefficient_group = coefficient_width * x;

							for (k = n0; k <= max_n; k++)
							{
								int out_pixel_index = k * 4;
								float coefficient = horizontal_coefficients[coefficient_group + k - n0];
								STBIR__DEBUG_ASSERT(coefficient != 0);
								output_buffer[out_pixel_index + 0] += decode_buffer[in_pixel_index + 0] * coefficient;
								output_buffer[out_pixel_index + 1] += decode_buffer[in_pixel_index + 1] * coefficient;
								output_buffer[out_pixel_index + 2] += decode_buffer[in_pixel_index + 2] * coefficient;
								output_buffer[out_pixel_index + 3] += decode_buffer[in_pixel_index + 3] * coefficient;
							}
						}
						break;

					default:
						for (x = 0; x < max_x; x++)
						{
							int n0 = horizontal_contributors[x].n0;
							int n1 = horizontal_contributors[x].n1;

							int in_x = x - filter_pixel_margin;
							int in_pixel_index = in_x * channels;
							int max_n = n1;
							int coefficient_group = coefficient_width * x;

							for (k = n0; k <= max_n; k++)
							{
								int c;
								int out_pixel_index = k * channels;
								float coefficient = horizontal_coefficients[coefficient_group + k - n0];
								STBIR__DEBUG_ASSERT(coefficient != 0);
								for (c = 0; c < channels; c++)
									output_buffer[out_pixel_index + c] += decode_buffer[in_pixel_index + c] * coefficient;
							}
						}
						break;
					}
				}

				void stbir__decode_and_resample_upsample(stbir__info* stbir_info, int n)
				{
					// Decode the nth scanline from the source image into the decode buffer.
					stbir__decode_scanline(stbir_info, n);

					// Now resample it into the ring buffer.
					if (stbir__use_width_upsampling(stbir_info))
						stbir__resample_horizontal_upsample(stbir_info, n, stbir__add_empty_ring_buffer_entry(stbir_info, n));
					else
						stbir__resample_horizontal_downsample(stbir_info, n, stbir__add_empty_ring_buffer_entry(stbir_info, n));

					// Now it's sitting in the ring buffer ready to be used as source for the vertical sampling.
				}

				void stbir__decode_and_resample_downsample(stbir__info* stbir_info, int n)
				{
					// Decode the nth scanline from the source image into the decode buffer.
					stbir__decode_scanline(stbir_info, n);

					memset(stbir_info->horizontal_buffer, 0, stbir_info->output_w * stbir_info->channels * sizeof(float));

					// Now resample it into the horizontal buffer.
					if (stbir__use_width_upsampling(stbir_info))
						stbir__resample_horizontal_upsample(stbir_info, n, stbir_info->horizontal_buffer);
					else
						stbir__resample_horizontal_downsample(stbir_info, n, stbir_info->horizontal_buffer);

					// Now it's sitting in the horizontal buffer ready to be distributed into the ring buffers.
				}

				// Get the specified scan line from the ring buffer.
				float* stbir__get_ring_buffer_scanline(int get_scanline, float* ring_buffer, int begin_index, int first_scanline, int ring_buffer_size, int ring_buffer_length)
				{
					int ring_buffer_index = (begin_index + (get_scanline - first_scanline)) % ring_buffer_size;
					return stbir__get_ring_buffer_entry(ring_buffer, ring_buffer_index, ring_buffer_length);
				}


				void stbir__encode_scanline(stbir__info* stbir_info, int num_pixels, void *output_buffer, float *encode_buffer, int channels, int alpha_channel, int decode)
				{
					int x;
					int n;
					int num_nonalpha;
					stbir_uint16 nonalpha[STBIR_MAX_CHANNELS];

					if (!(stbir_info->flags&STBIR_FLAG_ALPHA_PREMULTIPLIED))
					{
						for (x = 0; x < num_pixels; ++x)
						{
							int pixel_index = x*channels;

							float alpha = encode_buffer[pixel_index + alpha_channel];
							float reciprocal_alpha = alpha ? 1.0f / alpha : 0;

							// unrolling this produced a 1% slowdown upscaling a large RGBA linear-space image on my machine - stb
							for (n = 0; n < channels; n++)
							if (n != alpha_channel)
								encode_buffer[pixel_index + n] *= reciprocal_alpha;

							// We added in a small epsilon to prevent the color channel from being deleted with zero alpha.
							// Because we only add it for integer types, it will automatically be discarded on integer
							// conversion, so we don't need to subtract it back out (which would be problematic for
							// numeric precision reasons).
						}
					}

					// build a table of all channels that need colorspace correction, so
					// we don't perform colorspace correction on channels that don't need it.
					for (x = 0, num_nonalpha = 0; x < channels; ++x)
					if (x != alpha_channel || (stbir_info->flags & STBIR_FLAG_ALPHA_USES_COLORSPACE))
						nonalpha[num_nonalpha++] = x;

#define STBIR__ROUND_INT(f)    ((int)          ((f)+0.5))
#define STBIR__ROUND_UINT(f)   ((stbir_uint32) ((f)+0.5))

#ifdef STBIR__SATURATE_INT
#define STBIR__ENCODE_LINEAR8(f)   stbir__saturate8 (STBIR__ROUND_INT((f) * 255  ))
#define STBIR__ENCODE_LINEAR16(f)  stbir__saturate16(STBIR__ROUND_INT((f) * 65535))
#else
#define STBIR__ENCODE_LINEAR8(f)   (unsigned char ) STBIR__ROUND_INT(stbir__saturate(f) * 255  )
#define STBIR__ENCODE_LINEAR16(f)  (unsigned short) STBIR__ROUND_INT(stbir__saturate(f) * 65535)
#endif

					switch (decode)
					{
					case STBIR__DECODE(STBIR_TYPE_UINT8, STBIR_COLORSPACE_LINEAR):
						for (x = 0; x < num_pixels; ++x)
						{
							int pixel_index = x*channels;

							for (n = 0; n < channels; n++)
							{
								int index = pixel_index + n;
								((unsigned char*)output_buffer)[index] = STBIR__ENCODE_LINEAR8(encode_buffer[index]);
							}
						}
						break;

					case STBIR__DECODE(STBIR_TYPE_UINT8, STBIR_COLORSPACE_SRGB):
						for (x = 0; x < num_pixels; ++x)
						{
							int pixel_index = x*channels;

							for (n = 0; n < num_nonalpha; n++)
							{
								int index = pixel_index + nonalpha[n];
								((unsigned char*)output_buffer)[index] = stbir__linear_to_srgb_uchar(encode_buffer[index]);
							}

							if (!(stbir_info->flags & STBIR_FLAG_ALPHA_USES_COLORSPACE))
								((unsigned char *)output_buffer)[pixel_index + alpha_channel] = STBIR__ENCODE_LINEAR8(encode_buffer[pixel_index + alpha_channel]);
						}
						break;

					case STBIR__DECODE(STBIR_TYPE_UINT16, STBIR_COLORSPACE_LINEAR):
						for (x = 0; x < num_pixels; ++x)
						{
							int pixel_index = x*channels;

							for (n = 0; n < channels; n++)
							{
								int index = pixel_index + n;
								((unsigned short*)output_buffer)[index] = STBIR__ENCODE_LINEAR16(encode_buffer[index]);
							}
						}
						break;

					case STBIR__DECODE(STBIR_TYPE_UINT16, STBIR_COLORSPACE_SRGB):
						for (x = 0; x < num_pixels; ++x)
						{
							int pixel_index = x*channels;

							for (n = 0; n < num_nonalpha; n++)
							{
								int index = pixel_index + nonalpha[n];
								((unsigned short*)output_buffer)[index] = (unsigned short)STBIR__ROUND_INT(stbir__linear_to_srgb(stbir__saturate(encode_buffer[index])) * 65535);
							}

							if (!(stbir_info->flags&STBIR_FLAG_ALPHA_USES_COLORSPACE))
								((unsigned short*)output_buffer)[pixel_index + alpha_channel] = STBIR__ENCODE_LINEAR16(encode_buffer[pixel_index + alpha_channel]);
						}

						break;

					case STBIR__DECODE(STBIR_TYPE_UINT32, STBIR_COLORSPACE_LINEAR):
						for (x = 0; x < num_pixels; ++x)
						{
							int pixel_index = x*channels;

							for (n = 0; n < channels; n++)
							{
								int index = pixel_index + n;
								((unsigned int*)output_buffer)[index] = (unsigned int)STBIR__ROUND_UINT(((double)stbir__saturate(encode_buffer[index])) * 4294967295);
							}
						}
						break;

					case STBIR__DECODE(STBIR_TYPE_UINT32, STBIR_COLORSPACE_SRGB):
						for (x = 0; x < num_pixels; ++x)
						{
							int pixel_index = x*channels;

							for (n = 0; n < num_nonalpha; n++)
							{
								int index = pixel_index + nonalpha[n];
								((unsigned int*)output_buffer)[index] = (unsigned int)STBIR__ROUND_UINT(((double)stbir__linear_to_srgb(stbir__saturate(encode_buffer[index]))) * 4294967295);
							}

							if (!(stbir_info->flags&STBIR_FLAG_ALPHA_USES_COLORSPACE))
								((unsigned int*)output_buffer)[pixel_index + alpha_channel] = (unsigned int)STBIR__ROUND_INT(((double)stbir__saturate(encode_buffer[pixel_index + alpha_channel])) * 4294967295);
						}
						break;

					case STBIR__DECODE(STBIR_TYPE_FLOAT, STBIR_COLORSPACE_LINEAR):
						for (x = 0; x < num_pixels; ++x)
						{
							int pixel_index = x*channels;

							for (n = 0; n < channels; n++)
							{
								int index = pixel_index + n;
								((float*)output_buffer)[index] = encode_buffer[index];
							}
						}
						break;

					case STBIR__DECODE(STBIR_TYPE_FLOAT, STBIR_COLORSPACE_SRGB):
						for (x = 0; x < num_pixels; ++x)
						{
							int pixel_index = x*channels;

							for (n = 0; n < num_nonalpha; n++)
							{
								int index = pixel_index + nonalpha[n];
								((float*)output_buffer)[index] = stbir__linear_to_srgb(encode_buffer[index]);
							}

							if (!(stbir_info->flags&STBIR_FLAG_ALPHA_USES_COLORSPACE))
								((float*)output_buffer)[pixel_index + alpha_channel] = encode_buffer[pixel_index + alpha_channel];
						}
						break;

					default:
						STBIR__UNIMPLEMENTED("Unknown type/colorspace/channels combination.");
						break;
					}
				}

				void stbir__resample_vertical_upsample(stbir__info* stbir_info, int n, int in_first_scanline, int in_last_scanline, float in_center_of_out)
				{
					int x, k;
					int output_w = stbir_info->output_w;
					stbir__contributors* vertical_contributors = stbir_info->vertical_contributors;
					float* vertical_coefficients = stbir_info->vertical_coefficients;
					int channels = stbir_info->channels;
					int alpha_channel = stbir_info->alpha_channel;
					int type = stbir_info->type;
					int colorspace = stbir_info->colorspace;
					int kernel_pixel_width = stbir_info->vertical_filter_pixel_width;
					void* output_data = stbir_info->output_data;
					float* encode_buffer = stbir_info->encode_buffer;
					int decode = STBIR__DECODE(type, colorspace);
					int coefficient_width = stbir_info->vertical_coefficient_width;
					int coefficient_counter;
					int contributor = n;

					float* ring_buffer = stbir_info->ring_buffer;
					int ring_buffer_begin_index = stbir_info->ring_buffer_begin_index;
					int ring_buffer_first_scanline = stbir_info->ring_buffer_first_scanline;
					int ring_buffer_last_scanline = stbir_info->ring_buffer_last_scanline;
					int ring_buffer_length = stbir_info->ring_buffer_length_bytes / sizeof(float);

					int n0, n1, output_row_start;
					int coefficient_group = coefficient_width * contributor;

					n0 = vertical_contributors[contributor].n0;
					n1 = vertical_contributors[contributor].n1;

					output_row_start = n * stbir_info->output_stride_bytes;

					STBIR__DEBUG_ASSERT(stbir__use_height_upsampling(stbir_info));

					memset(encode_buffer, 0, output_w * sizeof(float)* channels);

					// I tried reblocking this for better cache usage of encode_buffer
					// (using x_outer, k, x_inner), but it lost speed. -- stb

					coefficient_counter = 0;
					switch (channels) {
					case 1:
						for (k = n0; k <= n1; k++)
						{
							int coefficient_index = coefficient_counter++;
							float* ring_buffer_entry = stbir__get_ring_buffer_scanline(k, ring_buffer, ring_buffer_begin_index, ring_buffer_first_scanline, kernel_pixel_width, ring_buffer_length);
							float coefficient = vertical_coefficients[coefficient_group + coefficient_index];
							for (x = 0; x < output_w; ++x)
							{
								int in_pixel_index = x * 1;
								encode_buffer[in_pixel_index + 0] += ring_buffer_entry[in_pixel_index + 0] * coefficient;
							}
						}
						break;
					case 2:
						for (k = n0; k <= n1; k++)
						{
							int coefficient_index = coefficient_counter++;
							float* ring_buffer_entry = stbir__get_ring_buffer_scanline(k, ring_buffer, ring_buffer_begin_index, ring_buffer_first_scanline, kernel_pixel_width, ring_buffer_length);
							float coefficient = vertical_coefficients[coefficient_group + coefficient_index];
							for (x = 0; x < output_w; ++x)
							{
								int in_pixel_index = x * 2;
								encode_buffer[in_pixel_index + 0] += ring_buffer_entry[in_pixel_index + 0] * coefficient;
								encode_buffer[in_pixel_index + 1] += ring_buffer_entry[in_pixel_index + 1] * coefficient;
							}
						}
						break;
					case 3:
						for (k = n0; k <= n1; k++)
						{
							int coefficient_index = coefficient_counter++;
							float* ring_buffer_entry = stbir__get_ring_buffer_scanline(k, ring_buffer, ring_buffer_begin_index, ring_buffer_first_scanline, kernel_pixel_width, ring_buffer_length);
							float coefficient = vertical_coefficients[coefficient_group + coefficient_index];
							for (x = 0; x < output_w; ++x)
							{
								int in_pixel_index = x * 3;
								encode_buffer[in_pixel_index + 0] += ring_buffer_entry[in_pixel_index + 0] * coefficient;
								encode_buffer[in_pixel_index + 1] += ring_buffer_entry[in_pixel_index + 1] * coefficient;
								encode_buffer[in_pixel_index + 2] += ring_buffer_entry[in_pixel_index + 2] * coefficient;
							}
						}
						break;
					case 4:
						for (k = n0; k <= n1; k++)
						{
							int coefficient_index = coefficient_counter++;
							float* ring_buffer_entry = stbir__get_ring_buffer_scanline(k, ring_buffer, ring_buffer_begin_index, ring_buffer_first_scanline, kernel_pixel_width, ring_buffer_length);
							float coefficient = vertical_coefficients[coefficient_group + coefficient_index];
							for (x = 0; x < output_w; ++x)
							{
								int in_pixel_index = x * 4;
								encode_buffer[in_pixel_index + 0] += ring_buffer_entry[in_pixel_index + 0] * coefficient;
								encode_buffer[in_pixel_index + 1] += ring_buffer_entry[in_pixel_index + 1] * coefficient;
								encode_buffer[in_pixel_index + 2] += ring_buffer_entry[in_pixel_index + 2] * coefficient;
								encode_buffer[in_pixel_index + 3] += ring_buffer_entry[in_pixel_index + 3] * coefficient;
							}
						}
						break;
					default:
						for (k = n0; k <= n1; k++)
						{
							int coefficient_index = coefficient_counter++;
							float* ring_buffer_entry = stbir__get_ring_buffer_scanline(k, ring_buffer, ring_buffer_begin_index, ring_buffer_first_scanline, kernel_pixel_width, ring_buffer_length);
							float coefficient = vertical_coefficients[coefficient_group + coefficient_index];
							for (x = 0; x < output_w; ++x)
							{
								int in_pixel_index = x * channels;
								int c;
								for (c = 0; c < channels; c++)
									encode_buffer[in_pixel_index + c] += ring_buffer_entry[in_pixel_index + c] * coefficient;
							}
						}
						break;
					}
					stbir__encode_scanline(stbir_info, output_w, (char *)output_data + output_row_start, encode_buffer, channels, alpha_channel, decode);
				}

				void stbir__resample_vertical_downsample(stbir__info* stbir_info, int n, int in_first_scanline, int in_last_scanline, float in_center_of_out)
				{
					int x, k;
					int output_w = stbir_info->output_w;
					int output_h = stbir_info->output_h;
					stbir__contributors* vertical_contributors = stbir_info->vertical_contributors;
					float* vertical_coefficients = stbir_info->vertical_coefficients;
					int channels = stbir_info->channels;
					int kernel_pixel_width = stbir_info->vertical_filter_pixel_width;
					void* output_data = stbir_info->output_data;
					float* horizontal_buffer = stbir_info->horizontal_buffer;
					int coefficient_width = stbir_info->vertical_coefficient_width;
					int contributor = n + stbir_info->vertical_filter_pixel_margin;

					float* ring_buffer = stbir_info->ring_buffer;
					int ring_buffer_begin_index = stbir_info->ring_buffer_begin_index;
					int ring_buffer_first_scanline = stbir_info->ring_buffer_first_scanline;
					int ring_buffer_last_scanline = stbir_info->ring_buffer_last_scanline;
					int ring_buffer_length = stbir_info->ring_buffer_length_bytes / sizeof(float);
					int n0, n1;

					n0 = vertical_contributors[contributor].n0;
					n1 = vertical_contributors[contributor].n1;

					STBIR__DEBUG_ASSERT(!stbir__use_height_upsampling(stbir_info));

					for (k = n0; k <= n1; k++)
					{
						int coefficient_index = k - n0;
						int coefficient_group = coefficient_width * contributor;
						float coefficient = vertical_coefficients[coefficient_group + coefficient_index];

						float* ring_buffer_entry = stbir__get_ring_buffer_scanline(k, ring_buffer, ring_buffer_begin_index, ring_buffer_first_scanline, kernel_pixel_width, ring_buffer_length);

						switch (channels) {
						case 1:
							for (x = 0; x < output_w; x++)
							{
								int in_pixel_index = x * 1;
								ring_buffer_entry[in_pixel_index + 0] += horizontal_buffer[in_pixel_index + 0] * coefficient;
							}
							break;
						case 2:
							for (x = 0; x < output_w; x++)
							{
								int in_pixel_index = x * 2;
								ring_buffer_entry[in_pixel_index + 0] += horizontal_buffer[in_pixel_index + 0] * coefficient;
								ring_buffer_entry[in_pixel_index + 1] += horizontal_buffer[in_pixel_index + 1] * coefficient;
							}
							break;
						case 3:
							for (x = 0; x < output_w; x++)
							{
								int in_pixel_index = x * 3;
								ring_buffer_entry[in_pixel_index + 0] += horizontal_buffer[in_pixel_index + 0] * coefficient;
								ring_buffer_entry[in_pixel_index + 1] += horizontal_buffer[in_pixel_index + 1] * coefficient;
								ring_buffer_entry[in_pixel_index + 2] += horizontal_buffer[in_pixel_index + 2] * coefficient;
							}
							break;
						case 4:
							for (x = 0; x < output_w; x++)
							{
								int in_pixel_index = x * 4;
								ring_buffer_entry[in_pixel_index + 0] += horizontal_buffer[in_pixel_index + 0] * coefficient;
								ring_buffer_entry[in_pixel_index + 1] += horizontal_buffer[in_pixel_index + 1] * coefficient;
								ring_buffer_entry[in_pixel_index + 2] += horizontal_buffer[in_pixel_index + 2] * coefficient;
								ring_buffer_entry[in_pixel_index + 3] += horizontal_buffer[in_pixel_index + 3] * coefficient;
							}
							break;
						default:
							for (x = 0; x < output_w; x++)
							{
								int in_pixel_index = x * channels;

								int c;
								for (c = 0; c < channels; c++)
									ring_buffer_entry[in_pixel_index + c] += horizontal_buffer[in_pixel_index + c] * coefficient;
							}
							break;
						}
					}
				}

				void stbir__buffer_loop_upsample(stbir__info* stbir_info)
				{
					int y;
					float scale_ratio = stbir_info->vertical_scale;
					float out_scanlines_radius = stbir__filter_info_table[stbir_info->vertical_filter].support(1 / scale_ratio) * scale_ratio;

					STBIR__DEBUG_ASSERT(stbir__use_height_upsampling(stbir_info));

					for (y = 0; y < stbir_info->output_h; y++)
					{
						float in_center_of_out = 0; // Center of the current out scanline in the in scanline space
						int in_first_scanline = 0, in_last_scanline = 0;

						stbir__calculate_sample_range_upsample(y, out_scanlines_radius, scale_ratio, stbir_info->vertical_shift, &in_first_scanline, &in_last_scanline, &in_center_of_out);

						STBIR__DEBUG_ASSERT(in_last_scanline - in_first_scanline <= stbir_info->vertical_filter_pixel_width);

						if (stbir_info->ring_buffer_begin_index >= 0)
						{
							// Get rid of whatever we don't need anymore.
							while (in_first_scanline > stbir_info->ring_buffer_first_scanline)
							{
								if (stbir_info->ring_buffer_first_scanline == stbir_info->ring_buffer_last_scanline)
								{
									// We just popped the last scanline off the ring buffer.
									// Reset it to the empty state.
									stbir_info->ring_buffer_begin_index = -1;
									stbir_info->ring_buffer_first_scanline = 0;
									stbir_info->ring_buffer_last_scanline = 0;
									break;
								}
								else
								{
									stbir_info->ring_buffer_first_scanline++;
									stbir_info->ring_buffer_begin_index = (stbir_info->ring_buffer_begin_index + 1) % stbir_info->vertical_filter_pixel_width;
								}
							}
						}

						// Load in new ones.
						if (stbir_info->ring_buffer_begin_index < 0)
							stbir__decode_and_resample_upsample(stbir_info, in_first_scanline);

						while (in_last_scanline > stbir_info->ring_buffer_last_scanline)
							stbir__decode_and_resample_upsample(stbir_info, stbir_info->ring_buffer_last_scanline + 1);

						// Now all buffers should be ready to write a row of vertical sampling.
						stbir__resample_vertical_upsample(stbir_info, y, in_first_scanline, in_last_scanline, in_center_of_out);

						STBIR_PROGRESS_REPORT((float)y / stbir_info->output_h);
					}
				}

				void stbir__empty_ring_buffer(stbir__info* stbir_info, int first_necessary_scanline)
				{
					int output_stride_bytes = stbir_info->output_stride_bytes;
					int channels = stbir_info->channels;
					int alpha_channel = stbir_info->alpha_channel;
					int type = stbir_info->type;
					int colorspace = stbir_info->colorspace;
					int output_w = stbir_info->output_w;
					void* output_data = stbir_info->output_data;
					int decode = STBIR__DECODE(type, colorspace);

					float* ring_buffer = stbir_info->ring_buffer;
					int ring_buffer_length = stbir_info->ring_buffer_length_bytes / sizeof(float);

					if (stbir_info->ring_buffer_begin_index >= 0)
					{
						// Get rid of whatever we don't need anymore.
						while (first_necessary_scanline > stbir_info->ring_buffer_first_scanline)
						{
							if (stbir_info->ring_buffer_first_scanline >= 0 && stbir_info->ring_buffer_first_scanline < stbir_info->output_h)
							{
								int output_row_start = stbir_info->ring_buffer_first_scanline * output_stride_bytes;
								float* ring_buffer_entry = stbir__get_ring_buffer_entry(ring_buffer, stbir_info->ring_buffer_begin_index, ring_buffer_length);
								stbir__encode_scanline(stbir_info, output_w, (char *)output_data + output_row_start, ring_buffer_entry, channels, alpha_channel, decode);
								STBIR_PROGRESS_REPORT((float)stbir_info->ring_buffer_first_scanline / stbir_info->output_h);
							}

							if (stbir_info->ring_buffer_first_scanline == stbir_info->ring_buffer_last_scanline)
							{
								// We just popped the last scanline off the ring buffer.
								// Reset it to the empty state.
								stbir_info->ring_buffer_begin_index = -1;
								stbir_info->ring_buffer_first_scanline = 0;
								stbir_info->ring_buffer_last_scanline = 0;
								break;
							}
							else
							{
								stbir_info->ring_buffer_first_scanline++;
								stbir_info->ring_buffer_begin_index = (stbir_info->ring_buffer_begin_index + 1) % stbir_info->vertical_filter_pixel_width;
							}
						}
					}
				}

				void stbir__buffer_loop_downsample(stbir__info* stbir_info)
				{
					int y;
					float scale_ratio = stbir_info->vertical_scale;
					int output_h = stbir_info->output_h;
					float in_pixels_radius = stbir__filter_info_table[stbir_info->vertical_filter].support(scale_ratio) / scale_ratio;
					int pixel_margin = stbir_info->vertical_filter_pixel_margin;
					int max_y = stbir_info->input_h + pixel_margin;

					STBIR__DEBUG_ASSERT(!stbir__use_height_upsampling(stbir_info));

					for (y = -pixel_margin; y < max_y; y++)
					{
						float out_center_of_in; // Center of the current out scanline in the in scanline space
						int out_first_scanline, out_last_scanline;

						stbir__calculate_sample_range_downsample(y, in_pixels_radius, scale_ratio, stbir_info->vertical_shift, &out_first_scanline, &out_last_scanline, &out_center_of_in);

						STBIR__DEBUG_ASSERT(out_last_scanline - out_first_scanline <= stbir_info->vertical_filter_pixel_width);

						if (out_last_scanline < 0 || out_first_scanline >= output_h)
							continue;

						stbir__empty_ring_buffer(stbir_info, out_first_scanline);

						stbir__decode_and_resample_downsample(stbir_info, y);

						// Load in new ones.
						if (stbir_info->ring_buffer_begin_index < 0)
							stbir__add_empty_ring_buffer_entry(stbir_info, out_first_scanline);

						while (out_last_scanline > stbir_info->ring_buffer_last_scanline)
							stbir__add_empty_ring_buffer_entry(stbir_info, stbir_info->ring_buffer_last_scanline + 1);

						// Now the horizontal buffer is ready to write to all ring buffer rows.
						stbir__resample_vertical_downsample(stbir_info, y, out_first_scanline, out_last_scanline, out_center_of_in);
					}

					stbir__empty_ring_buffer(stbir_info, stbir_info->output_h);
				}

				void stbir__setup(stbir__info *info, int input_w, int input_h, int output_w, int output_h, int channels)
				{
					info->input_w = input_w;
					info->input_h = input_h;
					info->output_w = output_w;
					info->output_h = output_h;
					info->channels = channels;
				}

				void stbir__calculate_transform(stbir__info *info, float s0, float t0, float s1, float t1, float *transform)
				{
					info->s0 = s0;
					info->t0 = t0;
					info->s1 = s1;
					info->t1 = t1;

					if (transform)
					{
						info->horizontal_scale = transform[0];
						info->vertical_scale = transform[1];
						info->horizontal_shift = transform[2];
						info->vertical_shift = transform[3];
					}
					else
					{
						info->horizontal_scale = ((float)info->output_w / info->input_w) / (s1 - s0);
						info->vertical_scale = ((float)info->output_h / info->input_h) / (t1 - t0);

						info->horizontal_shift = s0 * info->input_w / (s1 - s0);
						info->vertical_shift = t0 * info->input_h / (t1 - t0);
					}
				}

				void stbir__choose_filter(stbir__info *info, stbir_filter h_filter, stbir_filter v_filter)
				{
					if (h_filter == 0)
						h_filter = stbir__use_upsampling(info->horizontal_scale) ? STBIR_DEFAULT_FILTER_UPSAMPLE : STBIR_DEFAULT_FILTER_DOWNSAMPLE;
					if (v_filter == 0)
						v_filter = stbir__use_upsampling(info->vertical_scale) ? STBIR_DEFAULT_FILTER_UPSAMPLE : STBIR_DEFAULT_FILTER_DOWNSAMPLE;
					info->horizontal_filter = h_filter;
					info->vertical_filter = v_filter;
				}

				stbir_uint32 stbir__calculate_memory(stbir__info *info)
				{
					int pixel_margin = stbir__get_filter_pixel_margin(info->horizontal_filter, info->horizontal_scale);
					int filter_height = stbir__get_filter_pixel_width(info->vertical_filter, info->vertical_scale);

					info->horizontal_num_contributors = stbir__get_contributors(info->horizontal_scale, info->horizontal_filter, info->input_w, info->output_w);
					info->vertical_num_contributors = stbir__get_contributors(info->vertical_scale, info->vertical_filter, info->input_h, info->output_h);

					info->horizontal_contributors_size = info->horizontal_num_contributors * sizeof(stbir__contributors);
					info->horizontal_coefficients_size = stbir__get_total_horizontal_coefficients(info) * sizeof(float);
					info->vertical_contributors_size = info->vertical_num_contributors * sizeof(stbir__contributors);
					info->vertical_coefficients_size = stbir__get_total_vertical_coefficients(info) * sizeof(float);
					info->decode_buffer_size = (info->input_w + pixel_margin * 2) * info->channels * sizeof(float);
					info->horizontal_buffer_size = info->output_w * info->channels * sizeof(float);
					info->ring_buffer_size = info->output_w * info->channels * filter_height * sizeof(float);
					info->encode_buffer_size = info->output_w * info->channels * sizeof(float);

					STBIR_ASSERT(info->horizontal_filter != 0);
					STBIR_ASSERT(info->horizontal_filter < STBIR__ARRAY_SIZE(stbir__filter_info_table)); // this now happens too late
					STBIR_ASSERT(info->vertical_filter != 0);
					STBIR_ASSERT(info->vertical_filter < STBIR__ARRAY_SIZE(stbir__filter_info_table)); // this now happens too late

					if (stbir__use_height_upsampling(info))
						// The horizontal buffer is for when we're downsampling the height and we
						// can't output the result of sampling the decode buffer directly into the
						// ring buffers.
						info->horizontal_buffer_size = 0;
					else
						// The encode buffer is to retain precision in the height upsampling method
						// and isn't used when height downsampling.
						info->encode_buffer_size = 0;

					return info->horizontal_contributors_size + info->horizontal_coefficients_size
						+ info->vertical_contributors_size + info->vertical_coefficients_size
						+ info->decode_buffer_size + info->horizontal_buffer_size
						+ info->ring_buffer_size + info->encode_buffer_size;
				}

				int stbir__resize_allocated(stbir__info *info,
					const void* input_data, int input_stride_in_bytes,
					void* output_data, int output_stride_in_bytes,
					int alpha_channel, stbir_uint32 flags, stbir_datatype type,
					stbir_edge edge_horizontal, stbir_edge edge_vertical, stbir_colorspace colorspace,
					void* tempmem, size_t tempmem_size_in_bytes)
				{
					size_t memory_required = stbir__calculate_memory(info);

					int width_stride_input = input_stride_in_bytes ? input_stride_in_bytes : info->channels * info->input_w * stbir__type_size[type];
					int width_stride_output = output_stride_in_bytes ? output_stride_in_bytes : info->channels * info->output_w * stbir__type_size[type];

#ifdef STBIR_DEBUG_OVERWRITE_TEST
#define OVERWRITE_ARRAY_SIZE 8
					unsigned char overwrite_output_before_pre[OVERWRITE_ARRAY_SIZE];
					unsigned char overwrite_tempmem_before_pre[OVERWRITE_ARRAY_SIZE];
					unsigned char overwrite_output_after_pre[OVERWRITE_ARRAY_SIZE];
					unsigned char overwrite_tempmem_after_pre[OVERWRITE_ARRAY_SIZE];

					size_t begin_forbidden = width_stride_output * (info->output_h - 1) + info->output_w * info->channels * stbir__type_size[type];
					memcpy(overwrite_output_before_pre, &((unsigned char*)output_data)[-OVERWRITE_ARRAY_SIZE], OVERWRITE_ARRAY_SIZE);
					memcpy(overwrite_output_after_pre, &((unsigned char*)output_data)[begin_forbidden], OVERWRITE_ARRAY_SIZE);
					memcpy(overwrite_tempmem_before_pre, &((unsigned char*)tempmem)[-OVERWRITE_ARRAY_SIZE], OVERWRITE_ARRAY_SIZE);
					memcpy(overwrite_tempmem_after_pre, &((unsigned char*)tempmem)[tempmem_size_in_bytes], OVERWRITE_ARRAY_SIZE);
#endif

					STBIR_ASSERT(info->channels >= 0);
					STBIR_ASSERT(info->channels <= STBIR_MAX_CHANNELS);

					if (info->channels < 0 || info->channels > STBIR_MAX_CHANNELS)
						return 0;

					STBIR_ASSERT(info->horizontal_filter < STBIR__ARRAY_SIZE(stbir__filter_info_table));
					STBIR_ASSERT(info->vertical_filter < STBIR__ARRAY_SIZE(stbir__filter_info_table));

					if (info->horizontal_filter >= STBIR__ARRAY_SIZE(stbir__filter_info_table))
						return 0;
					if (info->vertical_filter >= STBIR__ARRAY_SIZE(stbir__filter_info_table))
						return 0;

					if (alpha_channel < 0)
						flags |= STBIR_FLAG_ALPHA_USES_COLORSPACE | STBIR_FLAG_ALPHA_PREMULTIPLIED;

					if (!(flags&STBIR_FLAG_ALPHA_USES_COLORSPACE) || !(flags&STBIR_FLAG_ALPHA_PREMULTIPLIED))
						STBIR_ASSERT(alpha_channel >= 0 && alpha_channel < info->channels);

					if (alpha_channel >= info->channels)
						return 0;

					STBIR_ASSERT(tempmem);

					if (!tempmem)
						return 0;

					STBIR_ASSERT(tempmem_size_in_bytes >= memory_required);

					if (tempmem_size_in_bytes < memory_required)
						return 0;

					memset(tempmem, 0, tempmem_size_in_bytes);

					info->input_data = input_data;
					info->input_stride_bytes = width_stride_input;

					info->output_data = output_data;
					info->output_stride_bytes = width_stride_output;

					info->alpha_channel = alpha_channel;
					info->flags = flags;
					info->type = type;
					info->edge_horizontal = edge_horizontal;
					info->edge_vertical = edge_vertical;
					info->colorspace = colorspace;

					info->horizontal_coefficient_width = stbir__get_coefficient_width(info->horizontal_filter, info->horizontal_scale);
					info->vertical_coefficient_width = stbir__get_coefficient_width(info->vertical_filter, info->vertical_scale);
					info->horizontal_filter_pixel_width = stbir__get_filter_pixel_width(info->horizontal_filter, info->horizontal_scale);
					info->vertical_filter_pixel_width = stbir__get_filter_pixel_width(info->vertical_filter, info->vertical_scale);
					info->horizontal_filter_pixel_margin = stbir__get_filter_pixel_margin(info->horizontal_filter, info->horizontal_scale);
					info->vertical_filter_pixel_margin = stbir__get_filter_pixel_margin(info->vertical_filter, info->vertical_scale);

					info->ring_buffer_length_bytes = info->output_w * info->channels * sizeof(float);
					info->decode_buffer_pixels = info->input_w + info->horizontal_filter_pixel_margin * 2;

#define STBIR__NEXT_MEMPTR(current, newtype) (newtype*)(((unsigned char*)current) + current##_size)

					info->horizontal_contributors = (stbir__contributors *)tempmem;
					info->horizontal_coefficients = STBIR__NEXT_MEMPTR(info->horizontal_contributors, float);
					info->vertical_contributors = STBIR__NEXT_MEMPTR(info->horizontal_coefficients, stbir__contributors);
					info->vertical_coefficients = STBIR__NEXT_MEMPTR(info->vertical_contributors, float);
					info->decode_buffer = STBIR__NEXT_MEMPTR(info->vertical_coefficients, float);

					if (stbir__use_height_upsampling(info))
					{
						info->horizontal_buffer = NULL;
						info->ring_buffer = STBIR__NEXT_MEMPTR(info->decode_buffer, float);
						info->encode_buffer = STBIR__NEXT_MEMPTR(info->ring_buffer, float);

						STBIR__DEBUG_ASSERT((size_t)STBIR__NEXT_MEMPTR(info->encode_buffer, unsigned char) == (size_t)tempmem + tempmem_size_in_bytes);
					}
					else
					{
						info->horizontal_buffer = STBIR__NEXT_MEMPTR(info->decode_buffer, float);
						info->ring_buffer = STBIR__NEXT_MEMPTR(info->horizontal_buffer, float);
						info->encode_buffer = NULL;

						STBIR__DEBUG_ASSERT((size_t)STBIR__NEXT_MEMPTR(info->ring_buffer, unsigned char) == (size_t)tempmem + tempmem_size_in_bytes);
					}

#undef STBIR__NEXT_MEMPTR

					// This signals that the ring buffer is empty
					info->ring_buffer_begin_index = -1;

					stbir__calculate_filters(info, info->horizontal_contributors, info->horizontal_coefficients, info->horizontal_filter, info->horizontal_scale, info->horizontal_shift, info->input_w, info->output_w);
					stbir__calculate_filters(info, info->vertical_contributors, info->vertical_coefficients, info->vertical_filter, info->vertical_scale, info->vertical_shift, info->input_h, info->output_h);

					STBIR_PROGRESS_REPORT(0);

					if (stbir__use_height_upsampling(info))
						stbir__buffer_loop_upsample(info);
					else
						stbir__buffer_loop_downsample(info);

					STBIR_PROGRESS_REPORT(1);

#ifdef STBIR_DEBUG_OVERWRITE_TEST
					STBIR__DEBUG_ASSERT(memcmp(overwrite_output_before_pre, &((unsigned char*)output_data)[-OVERWRITE_ARRAY_SIZE], OVERWRITE_ARRAY_SIZE) == 0);
					STBIR__DEBUG_ASSERT(memcmp(overwrite_output_after_pre, &((unsigned char*)output_data)[begin_forbidden], OVERWRITE_ARRAY_SIZE) == 0);
					STBIR__DEBUG_ASSERT(memcmp(overwrite_tempmem_before_pre, &((unsigned char*)tempmem)[-OVERWRITE_ARRAY_SIZE], OVERWRITE_ARRAY_SIZE) == 0);
					STBIR__DEBUG_ASSERT(memcmp(overwrite_tempmem_after_pre, &((unsigned char*)tempmem)[tempmem_size_in_bytes], OVERWRITE_ARRAY_SIZE) == 0);
#endif

					return 1;
				}


				int stbir__resize_arbitrary(
					void *alloc_context,
					const void* input_data, int input_w, int input_h, int input_stride_in_bytes,
					void* output_data, int output_w, int output_h, int output_stride_in_bytes,
					float s0, float t0, float s1, float t1, float *transform,
					int channels, int alpha_channel, stbir_uint32 flags, stbir_datatype type,
					stbir_filter h_filter, stbir_filter v_filter,
					stbir_edge edge_horizontal, stbir_edge edge_vertical, stbir_colorspace colorspace)
				{
					stbir__info info;
					int result;
					size_t memory_required;
					void* extra_memory;

					stbir__setup(&info, input_w, input_h, output_w, output_h, channels);
					stbir__calculate_transform(&info, s0, t0, s1, t1, transform);
					stbir__choose_filter(&info, h_filter, v_filter);
					memory_required = stbir__calculate_memory(&info);
					extra_memory = STBIR_MALLOC(memory_required, alloc_context);

					if (!extra_memory)
						return 0;

					result = stbir__resize_allocated(&info, input_data, input_stride_in_bytes,
						output_data, output_stride_in_bytes,
						alpha_channel, flags, type,
						edge_horizontal, edge_vertical,
						colorspace, extra_memory, memory_required);

					STBIR_FREE(extra_memory, alloc_context);

					return result;
				}

				int stbir_resize_uint8(const unsigned char *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					unsigned char *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					int num_channels)
				{
					return stbir__resize_arbitrary(NULL, input_pixels, input_w, input_h, input_stride_in_bytes,
						output_pixels, output_w, output_h, output_stride_in_bytes,
						0, 0, 1, 1, NULL, num_channels, -1, 0, STBIR_TYPE_UINT8, STBIR_FILTER_DEFAULT, STBIR_FILTER_DEFAULT,
						STBIR_EDGE_CLAMP, STBIR_EDGE_CLAMP, STBIR_COLORSPACE_LINEAR);
				}

				int stbir_resize_float(const float *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					float *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					int num_channels)
				{
					return stbir__resize_arbitrary(NULL, input_pixels, input_w, input_h, input_stride_in_bytes,
						output_pixels, output_w, output_h, output_stride_in_bytes,
						0, 0, 1, 1, NULL, num_channels, -1, 0, STBIR_TYPE_FLOAT, STBIR_FILTER_DEFAULT, STBIR_FILTER_DEFAULT,
						STBIR_EDGE_CLAMP, STBIR_EDGE_CLAMP, STBIR_COLORSPACE_LINEAR);
				}

				int stbir_resize_uint8_srgb(const unsigned char *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					unsigned char *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					int num_channels, int alpha_channel, int flags)
				{
					return stbir__resize_arbitrary(NULL, input_pixels, input_w, input_h, input_stride_in_bytes,
						output_pixels, output_w, output_h, output_stride_in_bytes,
						0, 0, 1, 1, NULL, num_channels, alpha_channel, flags, STBIR_TYPE_UINT8, STBIR_FILTER_DEFAULT, STBIR_FILTER_DEFAULT,
						STBIR_EDGE_CLAMP, STBIR_EDGE_CLAMP, STBIR_COLORSPACE_SRGB);
				}

				int stbir_resize_uint8_srgb_edgemode(const unsigned char *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					unsigned char *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					int num_channels, int alpha_channel, int flags,
					stbir_edge edge_wrap_mode)
				{
					return stbir__resize_arbitrary(NULL, input_pixels, input_w, input_h, input_stride_in_bytes,
						output_pixels, output_w, output_h, output_stride_in_bytes,
						0, 0, 1, 1, NULL, num_channels, alpha_channel, flags, STBIR_TYPE_UINT8, STBIR_FILTER_DEFAULT, STBIR_FILTER_DEFAULT,
						edge_wrap_mode, edge_wrap_mode, STBIR_COLORSPACE_SRGB);
				}

				int stbir_resize_uint8_generic(const unsigned char *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					unsigned char *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					int num_channels, int alpha_channel, int flags,
					stbir_edge edge_wrap_mode, stbir_filter filter, stbir_colorspace space,
					void *alloc_context)
				{
					return stbir__resize_arbitrary(alloc_context, input_pixels, input_w, input_h, input_stride_in_bytes,
						output_pixels, output_w, output_h, output_stride_in_bytes,
						0, 0, 1, 1, NULL, num_channels, alpha_channel, flags, STBIR_TYPE_UINT8, filter, filter,
						edge_wrap_mode, edge_wrap_mode, space);
				}

				int stbir_resize_uint16_generic(const stbir_uint16 *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					stbir_uint16 *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					int num_channels, int alpha_channel, int flags,
					stbir_edge edge_wrap_mode, stbir_filter filter, stbir_colorspace space,
					void *alloc_context)
				{
					return stbir__resize_arbitrary(alloc_context, input_pixels, input_w, input_h, input_stride_in_bytes,
						output_pixels, output_w, output_h, output_stride_in_bytes,
						0, 0, 1, 1, NULL, num_channels, alpha_channel, flags, STBIR_TYPE_UINT16, filter, filter,
						edge_wrap_mode, edge_wrap_mode, space);
				}


				int stbir_resize_float_generic(const float *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					float *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					int num_channels, int alpha_channel, int flags,
					stbir_edge edge_wrap_mode, stbir_filter filter, stbir_colorspace space,
					void *alloc_context)
				{
					return stbir__resize_arbitrary(alloc_context, input_pixels, input_w, input_h, input_stride_in_bytes,
						output_pixels, output_w, output_h, output_stride_in_bytes,
						0, 0, 1, 1, NULL, num_channels, alpha_channel, flags, STBIR_TYPE_FLOAT, filter, filter,
						edge_wrap_mode, edge_wrap_mode, space);
				}


				int stbir_resize(const void *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					void *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					stbir_datatype datatype,
					int num_channels, int alpha_channel, int flags,
					stbir_edge edge_mode_horizontal, stbir_edge edge_mode_vertical,
					stbir_filter filter_horizontal, stbir_filter filter_vertical,
					stbir_colorspace space, void *alloc_context)
				{
					return stbir__resize_arbitrary(alloc_context, input_pixels, input_w, input_h, input_stride_in_bytes,
						output_pixels, output_w, output_h, output_stride_in_bytes,
						0, 0, 1, 1, NULL, num_channels, alpha_channel, flags, datatype, filter_horizontal, filter_vertical,
						edge_mode_horizontal, edge_mode_vertical, space);
				}


				int stbir_resize_subpixel(const void *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					void *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					stbir_datatype datatype,
					int num_channels, int alpha_channel, int flags,
					stbir_edge edge_mode_horizontal, stbir_edge edge_mode_vertical,
					stbir_filter filter_horizontal, stbir_filter filter_vertical,
					stbir_colorspace space, void *alloc_context,
					float x_scale, float y_scale,
					float x_offset, float y_offset)
				{
					float transform[4];
					transform[0] = x_scale;
					transform[1] = y_scale;
					transform[2] = x_offset;
					transform[3] = y_offset;
					return stbir__resize_arbitrary(alloc_context, input_pixels, input_w, input_h, input_stride_in_bytes,
						output_pixels, output_w, output_h, output_stride_in_bytes,
						0, 0, 1, 1, transform, num_channels, alpha_channel, flags, datatype, filter_horizontal, filter_vertical,
						edge_mode_horizontal, edge_mode_vertical, space);
				}

				int stbir_resize_region(const void *input_pixels, int input_w, int input_h, int input_stride_in_bytes,
					void *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
					stbir_datatype datatype,
					int num_channels, int alpha_channel, int flags,
					stbir_edge edge_mode_horizontal, stbir_edge edge_mode_vertical,
					stbir_filter filter_horizontal, stbir_filter filter_vertical,
					stbir_colorspace space, void *alloc_context,
					float s0, float t0, float s1, float t1)
				{
					return stbir__resize_arbitrary(alloc_context, input_pixels, input_w, input_h, input_stride_in_bytes,
						output_pixels, output_w, output_h, output_stride_in_bytes,
						s0, t0, s1, t1, NULL, num_channels, alpha_channel, flags, datatype, filter_horizontal, filter_vertical,
						edge_mode_horizontal, edge_mode_vertical, space);
				}
			} // namespace stbi::resize

		} // namespace stbi

		namespace jo
		{
			const unsigned char s_jo_ZigZag[] = { 0, 1, 5, 6, 14, 15, 27, 28, 2, 4, 7, 13, 16, 26, 29, 42, 3, 8, 12, 17, 25, 30, 41, 43, 9, 11, 18, 24, 31, 40, 44, 53, 10, 19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38, 46, 51, 55, 60, 21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48, 49, 57, 58, 62, 63 };

			void jo_writeBits(FILE *fp, int &bitBuf, int &bitCnt, const unsigned short *bs) {
				bitCnt += bs[1];
				bitBuf |= bs[0] << (24 - bitCnt);
				while (bitCnt >= 8) {
					unsigned char c = (bitBuf >> 16) & 255;
					putc(c, fp);
					if (c == 255) {
						putc(0, fp);
					}
					bitBuf <<= 8;
					bitCnt -= 8;
				}
			}

			void jo_DCT(float &d0, float &d1, float &d2, float &d3, float &d4, float &d5, float &d6, float &d7) {
				float tmp0 = d0 + d7;
				float tmp7 = d0 - d7;
				float tmp1 = d1 + d6;
				float tmp6 = d1 - d6;
				float tmp2 = d2 + d5;
				float tmp5 = d2 - d5;
				float tmp3 = d3 + d4;
				float tmp4 = d3 - d4;

				// Even part
				float tmp10 = tmp0 + tmp3;	// phase 2
				float tmp13 = tmp0 - tmp3;
				float tmp11 = tmp1 + tmp2;
				float tmp12 = tmp1 - tmp2;

				d0 = tmp10 + tmp11; 		// phase 3
				d4 = tmp10 - tmp11;

				float z1 = (tmp12 + tmp13) * 0.707106781f; // c4
				d2 = tmp13 + z1; 		// phase 5
				d6 = tmp13 - z1;

				// Odd part
				tmp10 = tmp4 + tmp5; 		// phase 2
				tmp11 = tmp5 + tmp6;
				tmp12 = tmp6 + tmp7;

				// The rotator is modified from fig 4-8 to avoid extra negations.
				float z5 = (tmp10 - tmp12) * 0.382683433f; // c6
				float z2 = tmp10 * 0.541196100f + z5; // c2-c6
				float z4 = tmp12 * 1.306562965f + z5; // c2+c6
				float z3 = tmp11 * 0.707106781f; // c4

				float z11 = tmp7 + z3;		// phase 5
				float z13 = tmp7 - z3;

				d5 = z13 + z2;			// phase 6
				d3 = z13 - z2;
				d1 = z11 + z4;
				d7 = z11 - z4;
			}

			void jo_calcBits(int val, unsigned short bits[2]) {
				int tmp1 = val < 0 ? -val : val;
				val = val < 0 ? val - 1 : val;
				bits[1] = 1;
				while (tmp1 >>= 1) {
					++bits[1];
				}
				bits[0] = val & ((1 << bits[1]) - 1);
			}

			int jo_processDU(FILE *fp, int &bitBuf, int &bitCnt, float *CDU, float *fdtbl, int DC, const unsigned short HTDC[256][2], const unsigned short HTAC[256][2]) {
				const unsigned short EOB[2] = { HTAC[0x00][0], HTAC[0x00][1] };
				const unsigned short M16zeroes[2] = { HTAC[0xF0][0], HTAC[0xF0][1] };

				// DCT rows
				for (int dataOff = 0; dataOff<64; dataOff += 8) {
					jo_DCT(CDU[dataOff], CDU[dataOff + 1], CDU[dataOff + 2], CDU[dataOff + 3], CDU[dataOff + 4], CDU[dataOff + 5], CDU[dataOff + 6], CDU[dataOff + 7]);
				}
				// DCT columns
				for (int dataOff = 0; dataOff<8; ++dataOff) {
					jo_DCT(CDU[dataOff], CDU[dataOff + 8], CDU[dataOff + 16], CDU[dataOff + 24], CDU[dataOff + 32], CDU[dataOff + 40], CDU[dataOff + 48], CDU[dataOff + 56]);
				}
				// Quantize/descale/zigzag the coefficients
				int DU[64];
				for (int i = 0; i<64; ++i) {
					float v = CDU[i] * fdtbl[i];
					DU[s_jo_ZigZag[i]] = (int)(v < 0 ? ceilf(v - 0.5f) : floorf(v + 0.5f));
				}

				// Encode DC
				int diff = DU[0] - DC;
				if (diff == 0) {
					jo_writeBits(fp, bitBuf, bitCnt, HTDC[0]);
				}
				else {
					unsigned short bits[2];
					jo_calcBits(diff, bits);
					jo_writeBits(fp, bitBuf, bitCnt, HTDC[bits[1]]);
					jo_writeBits(fp, bitBuf, bitCnt, bits);
				}
				// Encode ACs
				int end0pos = 63;
				for (; (end0pos>0) && (DU[end0pos] == 0); --end0pos) {
				}
				// end0pos = first element in reverse order !=0
				if (end0pos == 0) {
					jo_writeBits(fp, bitBuf, bitCnt, EOB);
					return DU[0];
				}
				for (int i = 1; i <= end0pos; ++i) {
					int startpos = i;
					for (; DU[i] == 0 && i <= end0pos; ++i) {
					}
					int nrzeroes = i - startpos;
					if (nrzeroes >= 16) {
						int lng = nrzeroes >> 4;
						for (int nrmarker = 1; nrmarker <= lng; ++nrmarker)
							jo_writeBits(fp, bitBuf, bitCnt, M16zeroes);
						nrzeroes &= 15;
					}
					unsigned short bits[2];
					jo_calcBits(DU[i], bits);
					jo_writeBits(fp, bitBuf, bitCnt, HTAC[(nrzeroes << 4) + bits[1]]);
					jo_writeBits(fp, bitBuf, bitCnt, bits);
				}
				if (end0pos != 63) {
					jo_writeBits(fp, bitBuf, bitCnt, EOB);
				}
				return DU[0];
			}

			bool jo_write_jpg(const char *filename, const void *data, int width, int height, int comp, int quality) {
				// Constants that don't pollute global namespace
				const unsigned char std_dc_luminance_nrcodes[] = { 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
				const unsigned char std_dc_luminance_values[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
				const unsigned char std_ac_luminance_nrcodes[] = { 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
				const unsigned char std_ac_luminance_values[] = {
					0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
					0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
					0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
					0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
					0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
					0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
					0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa
				};
				const unsigned char std_dc_chrominance_nrcodes[] = { 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
				const unsigned char std_dc_chrominance_values[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
				const unsigned char std_ac_chrominance_nrcodes[] = { 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
				const unsigned char std_ac_chrominance_values[] = {
					0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
					0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
					0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
					0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
					0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
					0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
					0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa
				};
				// Huffman tables
				const unsigned short YDC_HT[256][2] = { { 0, 2 }, { 2, 3 }, { 3, 3 }, { 4, 3 }, { 5, 3 }, { 6, 3 }, { 14, 4 }, { 30, 5 }, { 62, 6 }, { 126, 7 }, { 254, 8 }, { 510, 9 } };
				const unsigned short UVDC_HT[256][2] = { { 0, 2 }, { 1, 2 }, { 2, 2 }, { 6, 3 }, { 14, 4 }, { 30, 5 }, { 62, 6 }, { 126, 7 }, { 254, 8 }, { 510, 9 }, { 1022, 10 }, { 2046, 11 } };
				const unsigned short YAC_HT[256][2] = {
					{ 10, 4 }, { 0, 2 }, { 1, 2 }, { 4, 3 }, { 11, 4 }, { 26, 5 }, { 120, 7 }, { 248, 8 }, { 1014, 10 }, { 65410, 16 }, { 65411, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 12, 4 }, { 27, 5 }, { 121, 7 }, { 502, 9 }, { 2038, 11 }, { 65412, 16 }, { 65413, 16 }, { 65414, 16 }, { 65415, 16 }, { 65416, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 28, 5 }, { 249, 8 }, { 1015, 10 }, { 4084, 12 }, { 65417, 16 }, { 65418, 16 }, { 65419, 16 }, { 65420, 16 }, { 65421, 16 }, { 65422, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 58, 6 }, { 503, 9 }, { 4085, 12 }, { 65423, 16 }, { 65424, 16 }, { 65425, 16 }, { 65426, 16 }, { 65427, 16 }, { 65428, 16 }, { 65429, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 59, 6 }, { 1016, 10 }, { 65430, 16 }, { 65431, 16 }, { 65432, 16 }, { 65433, 16 }, { 65434, 16 }, { 65435, 16 }, { 65436, 16 }, { 65437, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 122, 7 }, { 2039, 11 }, { 65438, 16 }, { 65439, 16 }, { 65440, 16 }, { 65441, 16 }, { 65442, 16 }, { 65443, 16 }, { 65444, 16 }, { 65445, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 123, 7 }, { 4086, 12 }, { 65446, 16 }, { 65447, 16 }, { 65448, 16 }, { 65449, 16 }, { 65450, 16 }, { 65451, 16 }, { 65452, 16 }, { 65453, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 250, 8 }, { 4087, 12 }, { 65454, 16 }, { 65455, 16 }, { 65456, 16 }, { 65457, 16 }, { 65458, 16 }, { 65459, 16 }, { 65460, 16 }, { 65461, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 504, 9 }, { 32704, 15 }, { 65462, 16 }, { 65463, 16 }, { 65464, 16 }, { 65465, 16 }, { 65466, 16 }, { 65467, 16 }, { 65468, 16 }, { 65469, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 505, 9 }, { 65470, 16 }, { 65471, 16 }, { 65472, 16 }, { 65473, 16 }, { 65474, 16 }, { 65475, 16 }, { 65476, 16 }, { 65477, 16 }, { 65478, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 506, 9 }, { 65479, 16 }, { 65480, 16 }, { 65481, 16 }, { 65482, 16 }, { 65483, 16 }, { 65484, 16 }, { 65485, 16 }, { 65486, 16 }, { 65487, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 1017, 10 }, { 65488, 16 }, { 65489, 16 }, { 65490, 16 }, { 65491, 16 }, { 65492, 16 }, { 65493, 16 }, { 65494, 16 }, { 65495, 16 }, { 65496, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 1018, 10 }, { 65497, 16 }, { 65498, 16 }, { 65499, 16 }, { 65500, 16 }, { 65501, 16 }, { 65502, 16 }, { 65503, 16 }, { 65504, 16 }, { 65505, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 2040, 11 }, { 65506, 16 }, { 65507, 16 }, { 65508, 16 }, { 65509, 16 }, { 65510, 16 }, { 65511, 16 }, { 65512, 16 }, { 65513, 16 }, { 65514, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 65515, 16 }, { 65516, 16 }, { 65517, 16 }, { 65518, 16 }, { 65519, 16 }, { 65520, 16 }, { 65521, 16 }, { 65522, 16 }, { 65523, 16 }, { 65524, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 2041, 11 }, { 65525, 16 }, { 65526, 16 }, { 65527, 16 }, { 65528, 16 }, { 65529, 16 }, { 65530, 16 }, { 65531, 16 }, { 65532, 16 }, { 65533, 16 }, { 65534, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
				};
				const unsigned short UVAC_HT[256][2] = {
					{ 0, 2 }, { 1, 2 }, { 4, 3 }, { 10, 4 }, { 24, 5 }, { 25, 5 }, { 56, 6 }, { 120, 7 }, { 500, 9 }, { 1014, 10 }, { 4084, 12 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 11, 4 }, { 57, 6 }, { 246, 8 }, { 501, 9 }, { 2038, 11 }, { 4085, 12 }, { 65416, 16 }, { 65417, 16 }, { 65418, 16 }, { 65419, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 26, 5 }, { 247, 8 }, { 1015, 10 }, { 4086, 12 }, { 32706, 15 }, { 65420, 16 }, { 65421, 16 }, { 65422, 16 }, { 65423, 16 }, { 65424, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 27, 5 }, { 248, 8 }, { 1016, 10 }, { 4087, 12 }, { 65425, 16 }, { 65426, 16 }, { 65427, 16 }, { 65428, 16 }, { 65429, 16 }, { 65430, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 58, 6 }, { 502, 9 }, { 65431, 16 }, { 65432, 16 }, { 65433, 16 }, { 65434, 16 }, { 65435, 16 }, { 65436, 16 }, { 65437, 16 }, { 65438, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 59, 6 }, { 1017, 10 }, { 65439, 16 }, { 65440, 16 }, { 65441, 16 }, { 65442, 16 }, { 65443, 16 }, { 65444, 16 }, { 65445, 16 }, { 65446, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 121, 7 }, { 2039, 11 }, { 65447, 16 }, { 65448, 16 }, { 65449, 16 }, { 65450, 16 }, { 65451, 16 }, { 65452, 16 }, { 65453, 16 }, { 65454, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 122, 7 }, { 2040, 11 }, { 65455, 16 }, { 65456, 16 }, { 65457, 16 }, { 65458, 16 }, { 65459, 16 }, { 65460, 16 }, { 65461, 16 }, { 65462, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 249, 8 }, { 65463, 16 }, { 65464, 16 }, { 65465, 16 }, { 65466, 16 }, { 65467, 16 }, { 65468, 16 }, { 65469, 16 }, { 65470, 16 }, { 65471, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 503, 9 }, { 65472, 16 }, { 65473, 16 }, { 65474, 16 }, { 65475, 16 }, { 65476, 16 }, { 65477, 16 }, { 65478, 16 }, { 65479, 16 }, { 65480, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 504, 9 }, { 65481, 16 }, { 65482, 16 }, { 65483, 16 }, { 65484, 16 }, { 65485, 16 }, { 65486, 16 }, { 65487, 16 }, { 65488, 16 }, { 65489, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 505, 9 }, { 65490, 16 }, { 65491, 16 }, { 65492, 16 }, { 65493, 16 }, { 65494, 16 }, { 65495, 16 }, { 65496, 16 }, { 65497, 16 }, { 65498, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 506, 9 }, { 65499, 16 }, { 65500, 16 }, { 65501, 16 }, { 65502, 16 }, { 65503, 16 }, { 65504, 16 }, { 65505, 16 }, { 65506, 16 }, { 65507, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 2041, 11 }, { 65508, 16 }, { 65509, 16 }, { 65510, 16 }, { 65511, 16 }, { 65512, 16 }, { 65513, 16 }, { 65514, 16 }, { 65515, 16 }, { 65516, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 16352, 14 }, { 65517, 16 }, { 65518, 16 }, { 65519, 16 }, { 65520, 16 }, { 65521, 16 }, { 65522, 16 }, { 65523, 16 }, { 65524, 16 }, { 65525, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
					{ 1018, 10 }, { 32707, 15 }, { 65526, 16 }, { 65527, 16 }, { 65528, 16 }, { 65529, 16 }, { 65530, 16 }, { 65531, 16 }, { 65532, 16 }, { 65533, 16 }, { 65534, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
				};
				const int YQT[] = { 16, 11, 10, 16, 24, 40, 51, 61, 12, 12, 14, 19, 26, 58, 60, 55, 14, 13, 16, 24, 40, 57, 69, 56, 14, 17, 22, 29, 51, 87, 80, 62, 18, 22, 37, 56, 68, 109, 103, 77, 24, 35, 55, 64, 81, 104, 113, 92, 49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99 };
				const int UVQT[] = { 17, 18, 24, 47, 99, 99, 99, 99, 18, 21, 26, 66, 99, 99, 99, 99, 24, 26, 56, 99, 99, 99, 99, 99, 47, 66, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99 };
				const float aasf[] = { 1.0f * 2.828427125f, 1.387039845f * 2.828427125f, 1.306562965f * 2.828427125f, 1.175875602f * 2.828427125f, 1.0f * 2.828427125f, 0.785694958f * 2.828427125f, 0.541196100f * 2.828427125f, 0.275899379f * 2.828427125f };

				if (!data || !filename || !width || !height || comp > 4 || comp < 1 || comp == 2) {
					return false;
				}

				FILE *fp = fopen(filename, "wb");
				if (!fp) {
					return false;
				}

				quality = quality ? quality : 90;
				quality = quality < 1 ? 1 : quality > 100 ? 100 : quality;
				quality = quality < 50 ? 5000 / quality : 200 - quality * 2;

				unsigned char YTable[64], UVTable[64];
				for (int i = 0; i < 64; ++i) {
					int yti = (YQT[i] * quality + 50) / 100;
					YTable[s_jo_ZigZag[i]] = yti < 1 ? 1 : yti > 255 ? 255 : yti;
					int uvti = (UVQT[i] * quality + 50) / 100;
					UVTable[s_jo_ZigZag[i]] = uvti < 1 ? 1 : uvti > 255 ? 255 : uvti;
				}

				float fdtbl_Y[64], fdtbl_UV[64];
				for (int row = 0, k = 0; row < 8; ++row) {
					for (int col = 0; col < 8; ++col, ++k) {
						fdtbl_Y[k] = 1 / (YTable[s_jo_ZigZag[k]] * aasf[row] * aasf[col]);
						fdtbl_UV[k] = 1 / (UVTable[s_jo_ZigZag[k]] * aasf[row] * aasf[col]);
					}
				}

				// Write Headers
				const unsigned char head0[] = { 0xFF, 0xD8, 0xFF, 0xE0, 0, 0x10, 'J', 'F', 'I', 'F', 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0xFF, 0xDB, 0, 0x84, 0 };
				fwrite(head0, sizeof(head0), 1, fp);
				fwrite(YTable, sizeof(YTable), 1, fp);
				putc(1, fp);
				fwrite(UVTable, sizeof(UVTable), 1, fp);
				const unsigned char head1[] = { 0xFF, 0xC0, 0, 0x11, 8, (unsigned char)(height >> 8), (unsigned char)(height & 0xFF), (unsigned char)(width >> 8), (unsigned char)(width & 0xFF), 3, 1, 0x11, 0, 2, 0x11, 1, 3, 0x11, 1, 0xFF, 0xC4, 0x01, 0xA2, 0 };
				fwrite(head1, sizeof(head1), 1, fp);
				fwrite(std_dc_luminance_nrcodes + 1, sizeof(std_dc_luminance_nrcodes)-1, 1, fp);
				fwrite(std_dc_luminance_values, sizeof(std_dc_luminance_values), 1, fp);
				putc(0x10, fp); // HTYACinfo
				fwrite(std_ac_luminance_nrcodes + 1, sizeof(std_ac_luminance_nrcodes)-1, 1, fp);
				fwrite(std_ac_luminance_values, sizeof(std_ac_luminance_values), 1, fp);
				putc(1, fp); // HTUDCinfo
				fwrite(std_dc_chrominance_nrcodes + 1, sizeof(std_dc_chrominance_nrcodes)-1, 1, fp);
				fwrite(std_dc_chrominance_values, sizeof(std_dc_chrominance_values), 1, fp);
				putc(0x11, fp); // HTUACinfo
				fwrite(std_ac_chrominance_nrcodes + 1, sizeof(std_ac_chrominance_nrcodes)-1, 1, fp);
				fwrite(std_ac_chrominance_values, sizeof(std_ac_chrominance_values), 1, fp);
				const unsigned char head2[] = { 0xFF, 0xDA, 0, 0xC, 3, 1, 0, 2, 0x11, 3, 0x11, 0, 0x3F, 0 };
				fwrite(head2, sizeof(head2), 1, fp);

				// Encode 8x8 macroblocks
				const unsigned char *imageData = (const unsigned char *)data;
				int DCY = 0, DCU = 0, DCV = 0;
				int bitBuf = 0, bitCnt = 0;
				int ofsG = comp > 1 ? 1 : 0, ofsB = comp > 1 ? 2 : 0;
				for (int y = 0; y < height; y += 8) {
					for (int x = 0; x < width; x += 8) {
						float YDU[64], UDU[64], VDU[64];
						for (int row = y, pos = 0; row < y + 8; ++row) {
							for (int col = x; col < x + 8; ++col, ++pos) {
								int p = row*width*comp + col*comp;
								if (row >= height) {
									p -= width*comp*(row + 1 - height);
								}
								if (col >= width) {
									p -= comp*(col + 1 - width);
								}

								float r = imageData[p + 0], g = imageData[p + ofsG], b = imageData[p + ofsB];
								YDU[pos] = +0.29900f*r + 0.58700f*g + 0.11400f*b - 128;
								UDU[pos] = -0.16874f*r - 0.33126f*g + 0.50000f*b;
								VDU[pos] = +0.50000f*r - 0.41869f*g - 0.08131f*b;
							}
						}

						DCY = jo_processDU(fp, bitBuf, bitCnt, YDU, fdtbl_Y, DCY, YDC_HT, YAC_HT);
						DCU = jo_processDU(fp, bitBuf, bitCnt, UDU, fdtbl_UV, DCU, UVDC_HT, UVAC_HT);
						DCV = jo_processDU(fp, bitBuf, bitCnt, VDU, fdtbl_UV, DCV, UVDC_HT, UVAC_HT);
					}
				}

				// Do the bit alignment of the EOI marker
				const unsigned short fillBits[] = { 0x7F, 7 };
				jo_writeBits(fp, bitBuf, bitCnt, fillBits);

				// EOI
				putc(0xFF, fp);
				putc(0xD9, fp);

				fclose(fp);
				return true;
			}
		} // namespace jo
	}
	// \endcond 

	namespace fmt
	{
		namespace consts
		{
			static const char			kFormatSpecifierEscapeChar = '%';
			static const std::string	kZeroPaddingStr = std::string("0");
		}

		bool is_digit(char c)
		{
			return c >= '0' && c <= '9';
		}

		bool wild_card_match(const char* str, const char* pattern)
		{
			while (*pattern)
			{
				switch (*pattern)
				{
				case '?':
					if (!*str) return false;
					++str;
					++pattern;
					break;
				case '*':
					if (wild_card_match(str, pattern + 1)) return true;
					if (*str && wild_card_match(str + 1, pattern)) return true;
					return false;
					break;
				default:
					if (*str++ != *pattern++) return false;
					break;
				}
			}
			return !*str && !*pattern;
		}

		std::string ltrim(std::string str)
		{
			str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(&std::isspace))));
			return str;
		}

		std::string rtrim(std::string str)
		{
			str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(&std::isspace))).base(), str.end());
			return str;
		}

		std::string trim(std::string str)
		{
			return ltrim(rtrim(str));
		}

		std::string lstrip(std::string str, std::string what)
		{
			auto pos = str.find(what);
			if (0 == pos)
			{
				str.erase(pos, what.length());
			}
			return str;
		}

		std::string rstrip(std::string str, std::string what)
		{
			auto pos = str.rfind(what);
			if (str.length() - what.length() == pos)
			{
				str.erase(pos, what.length());
			}
			return str;
		}

		std::string lskip(std::string str, std::string delim)
		{
			auto pos = str.find(delim);
			if (pos == std::string::npos)
			{
				str = std::string();
			}
			else
			{
				str = str.substr(pos + 1);
			}
			return str;
		}

		std::string rskip(std::string str, std::string delim)
		{
			auto pos = str.rfind(delim);
			if (pos == 0)
			{
				str = std::string();
			}
			else if (std::string::npos != pos)
			{
				str = str.substr(0, pos);
			}
			return str;
		}

		std::string rskip_all(std::string str, std::string delim)
		{
			auto pos = str.find(delim);
			if (pos == std::string::npos)
			{
				return str;
			}
			else
			{
				return str.substr(0, pos);
			}
		}

		std::vector<std::string> split(const std::string s, char delim)
		{
			std::vector<std::string> elems;
			std::istringstream ss(s);
			std::string item;
			while (std::getline(ss, item, delim))
			{
				if (!item.empty()) elems.push_back(item);
			}
			return elems;
		}

		std::vector<std::string> split(const std::string s, std::string delim)
		{
			std::vector<std::string> elems;
			std::string ss(s);
			std::string item;
			size_t pos = 0;
			while ((pos = ss.find(delim)) != std::string::npos) {
				item = ss.substr(0, pos);
				if (!item.empty()) elems.push_back(item);
				ss.erase(0, pos + delim.length());
			}
			if (!ss.empty()) elems.push_back(ss);
			return elems;
		}

		std::vector<std::string> split_multi_delims(const std::string s, std::string delims)
		{
			std::vector<std::string> elems;
			std::string ss(s);
			std::string item;
			size_t pos = 0;
			while ((pos = ss.find_first_of(delims)) != std::string::npos) {
				item = ss.substr(0, pos);
				if (!item.empty()) elems.push_back(item);
				ss.erase(0, pos + 1);
			}
			if (!ss.empty()) elems.push_back(ss);
			return elems;
		}

		std::vector<std::string> split_whitespace(const std::string s)
		{
			auto list = split_multi_delims(s, " \t\n");
			std::vector<std::string> ret;
			for (auto elem : list)
			{
				auto rest = fmt::trim(elem);
				if (!rest.empty()) ret.push_back(rest);
			}
			return ret;
		}

		std::pair<std::string, std::string> split_first_occurance(const std::string s, char delim)
		{
			auto pos = s.find_first_of(delim);
			std::string first = s.substr(0, pos);
			std::string second = (pos != std::string::npos ? s.substr(pos + 1) : std::string());
			return std::make_pair(first, second);
		}

		std::vector<std::string>& erase_empty(std::vector<std::string> &vec)
		{
			for (auto it = vec.begin(); it != vec.end();)
			{
				if (it->empty())
				{
					it = vec.erase(it);
				}
				else
				{
					++it;
				}
			}
			return vec;
		}

		std::string join(std::vector<std::string> elems, char delim)
		{
			std::string str;
			elems = erase_empty(elems);
			if (elems.empty()) return str;
			str = elems[0];
			for (std::size_t i = 1; i < elems.size(); ++i)
			{
				if (elems[i].empty()) continue;
				str += delim + elems[i];
			}
			return str;
		}

		bool starts_with(const std::string& str, const std::string& start)
		{
			return (str.length() >= start.length()) && (str.compare(0, start.length(), start) == 0);
		}

		bool ends_with(const std::string& str, const std::string& end)
		{
			return (str.length() >= end.length()) && (str.compare(str.length() - end.length(), end.length(), end) == 0);
		}

		std::string& replace_all(std::string& str, char replaceWhat, char replaceWith)
		{
			std::replace(str.begin(), str.end(), replaceWhat, replaceWith);
			return str;
		}

		std::string& replace_all(std::string& str, const std::string& replaceWhat, const std::string& replaceWith)
		{
			if (replaceWhat == replaceWith) return str;
			std::size_t foundAt = std::string::npos;
			while ((foundAt = str.find(replaceWhat, foundAt + 1)) != std::string::npos)
			{
				str.replace(foundAt, replaceWhat.length(), replaceWith);
			}
			return str;
		}

		void replace_first_with_escape(std::string &str, const std::string &replaceWhat, const std::string &replaceWith)
		{
			std::size_t foundAt = std::string::npos;
			while ((foundAt = str.find(replaceWhat, foundAt + 1)) != std::string::npos)
			{
				if (foundAt > 0 && str[foundAt - 1] == consts::kFormatSpecifierEscapeChar)
				{
					str.erase(foundAt > 0 ? foundAt - 1 : 0, 1);
					++foundAt;
				}
				else
				{
					str.replace(foundAt, replaceWhat.length(), replaceWith);
					return;
				}
			}
		}

		void replace_all_with_escape(std::string &str, const std::string &replaceWhat, const std::string &replaceWith)
		{
			std::size_t foundAt = std::string::npos;
			while ((foundAt = str.find(replaceWhat, foundAt + 1)) != std::string::npos)
			{
				if (foundAt > 0 && str[foundAt - 1] == consts::kFormatSpecifierEscapeChar)
				{
					str.erase(foundAt > 0 ? foundAt - 1 : 0, 1);
					++foundAt;
				}
				else
				{
					str.replace(foundAt, replaceWhat.length(), replaceWith);
					foundAt += replaceWith.length();
				}
			}
		}

		void replace_sequential_with_escape(std::string &str, const std::string &replaceWhat, const std::vector<std::string> &replaceWith)
		{
			std::size_t foundAt = std::string::npos;
			std::size_t candidatePos = 0;
			while ((foundAt = str.find(replaceWhat, foundAt + 1)) != std::string::npos && replaceWith.size() > candidatePos)
			{
				if (foundAt > 0 && str[foundAt - 1] == consts::kFormatSpecifierEscapeChar)
				{
					str.erase(foundAt > 0 ? foundAt - 1 : 0, 1);
					++foundAt;
				}
				else
				{
					str.replace(foundAt, replaceWhat.length(), replaceWith[candidatePos]);
					foundAt += replaceWith[candidatePos].length();
					++candidatePos;
				}
			}
		}

		bool str_equals(const char* s1, const char* s2)
		{
			if (s1 == nullptr && s2 == nullptr) return true;
			if (s1 == nullptr || s2 == nullptr) return false;
			return std::string(s1) == std::string(s2);	// this is safe, not with strcmp
		}

		//std::string& left_zero_padding(std::string &str, int width)
		//{
		//	int toPad = width - static_cast<int>(str.length());
		//	while (toPad > 0)
		//	{
		//		str = consts::kZeroPaddingStr + str;
		//		--toPad;
		//	}
		//	return str;
		//}

		std::string to_lower_ascii(std::string mixed)
		{
			std::transform(mixed.begin(), mixed.end(), mixed.begin(), ::tolower);
			return mixed;
		}

		std::string to_upper_ascii(std::string mixed)
		{
			std::transform(mixed.begin(), mixed.end(), mixed.begin(), ::toupper);
			return mixed;
		}

		std::u16string utf8_to_utf16(std::string u8str)
		{
			try
			{
				std::u16string ret;
				thirdparty::utf8::utf8to16(u8str.begin(), u8str.end(), std::back_inserter(ret));
				return ret;
			}
			catch (...)
			{
				throw RuntimeException("Invalid UTF-8 string.");
			}
		}

		std::string utf16_to_utf8(std::u16string u16str)
		{
			try
			{
				std::vector<unsigned char> u8vec;
				thirdparty::utf8::utf16to8(u16str.begin(), u16str.end(), std::back_inserter(u8vec));
				auto ptr = reinterpret_cast<char*>(u8vec.data());
				return std::string(ptr, ptr + u8vec.size());
			}
			catch (...)
			{
				throw RuntimeException("Invalid UTF-16 string.");
			}
		}

		std::u32string utf8_to_utf32(std::string u8str)
		{
			try
			{
				std::u32string ret;
				thirdparty::utf8::utf8to32(u8str.begin(), u8str.end(), std::back_inserter(ret));
				return ret;
			}
			catch (...)
			{
				throw RuntimeException("Invalid UTF-8 string.");
			}
		}

		std::string utf32_to_utf8(std::u32string u32str)
		{
			try
			{
				std::vector<unsigned char> u8vec;
				thirdparty::utf8::utf32to8(u32str.begin(), u32str.end(), std::back_inserter(u8vec));
				auto ptr = reinterpret_cast<char*>(u8vec.data());
				return std::string(ptr, ptr + u8vec.size());
			}
			catch (...)
			{
				throw RuntimeException("Invalid UTF-32 string.");
			}
		}

	} // namespace fmt

	namespace fs
	{
		Path::Path(std::string path, bool isAbsolute)
		{
			if (isAbsolute) abspath_ = path;
			else abspath_ = os::absolute_path(path);
		}

		bool Path::empty() const
		{
			return abspath_.empty();
		}

		bool Path::exist() const
		{
			if (empty()) return false;
			return os::path_exists(abspath_, true);
		}

		bool Path::is_file() const
		{
			return os::is_file(abspath_);
		}

		bool Path::is_dir() const
		{
			return os::is_directory(abspath_);
		}

		std::string Path::abs_path() const
		{
			return abspath_;
		}

		std::string Path::relative_path() const
		{
			std::string cwd = os::current_working_directory() + os::path_delim();
			if (fmt::starts_with(abspath_, cwd))
			{
				return fmt::lstrip(abspath_, cwd);
			}
			return relative_path(cwd);
		}

		std::string Path::relative_path(std::string root) const
		{
			if (!fmt::ends_with(root, os::path_delim())) root += os::path_delim();
			if (fmt::starts_with(abspath_, root))
			{
				return fmt::lstrip(abspath_, root);
			}

			auto rootParts = os::path_split(root);
			auto thisParts = os::path_split(abspath_);
			std::size_t commonParts = 0;
			std::size_t size = (std::min)(rootParts.size(), thisParts.size());
			for (std::size_t i = 0; i < size; ++i)
			{
				if (!os::path_identical(rootParts[i], thisParts[i])) break;
				++commonParts;
			}

			if (commonParts == 0)
			{
				log::detail::zupply_internal_warn("Unable to resolve relative path to: " + root + "! Return absolute path.");
				return abspath_;
			}

			std::vector<std::string> tmp;
			// traverse back from root, add ../ to path
			for (std::size_t pos = rootParts.size(); pos > commonParts; --pos)
			{
				tmp.push_back("..");
			}
			// forward add parts of this path
			for (std::size_t pos = commonParts; pos < thisParts.size(); ++pos)
			{
				tmp.push_back(thisParts[pos]);
			}
			return os::path_join(tmp);
		}

		std::string Path::filename() const
		{
			if (is_file()) return os::path_split_filename(abspath_);
			return std::string();
		}

		Directory::Directory(std::string root, bool recursive)
			:root_(root), recursive_(recursive)
		{
			resolve();
		}

                Directory::Directory(std::string root, const std::string pattern, bool recursive)
			: root_(root), recursive_(recursive)
		{
			resolve();
			filter(pattern);
		}

                Directory::Directory(std::string root, const std::vector<std::string> patterns, bool recursive)
                        : root_(root), recursive_(recursive)
                {
                        resolve();
                        filter(patterns);
                }

                Directory::Directory(std::string root, const std::vector<const char*> patterns, bool recursive)
                        : root_(root), recursive_(recursive)
                {
                        resolve();
                        filter(patterns);
                }

		bool Directory::is_recursive() const
		{
			return recursive_;
		}

		std::string Directory::root() const
		{
			return root_.abs_path();
		}

		std::vector<Path> Directory::to_list() const
		{
			return paths_;
		}

		void Directory::resolve()
		{
			std::deque<std::string> workList;
			if (root_.is_dir())
			{
				workList.push_back(root_.abs_path());
			}

			while (!workList.empty())
			{
				std::vector<std::string> list = os::list_directory(workList.front());
				workList.pop_front();
				for (auto i = list.begin(); i != list.end(); ++i)
				{
					if (os::is_file(*i))
					{
						paths_.push_back(Path(*i));
					}
					else if (os::is_directory(*i))
					{
						paths_.push_back(Path(*i));
						if (recursive_) workList.push_back(*i);	// add subdir work list
					}
					else
					{
						// this should not happen unless file/dir modified during runtime
						// ignore the error
					}
				}
			}
		}

                void Directory::filter(const std::string pattern)
		{
			std::vector<Path> filtered;
			for (auto entry : paths_)
			{
				if (entry.is_file())
				{
					std::string filename = os::path_split_filename(entry.abs_path());
					if (fmt::wild_card_match(filename.c_str(), pattern.c_str()))
					{
						filtered.push_back(entry);
					}
				}
			}
			paths_ = filtered;	// replace all
		}

                void Directory::filter(const std::vector<std::string> patterns)
                {
                    std::vector<Path> filtered;
                    for (auto entry : paths_)
                    {
                            if (entry.is_file())
                            {
                                    std::string filename = os::path_split_filename(entry.abs_path());
                                    for (auto pattern : patterns)
                                    {
                                        if (fmt::wild_card_match(filename.c_str(), pattern.c_str()))
                                        {
                                                filtered.push_back(entry);
                                        }
                                    }
                            }
                    }
                    paths_ = filtered;	// replace all
                }

                void Directory::filter(const std::vector<const char*> patterns)
                {
                    std::vector<Path> filtered;
                    for (auto entry : paths_)
                    {
                            if (entry.is_file())
                            {
                                    std::string filename = os::path_split_filename(entry.abs_path());
                                    for (auto pattern : patterns)
                                    {
                                        if (fmt::wild_card_match(filename.c_str(), pattern))
                                        {
                                                filtered.push_back(entry);
                                        }
                                    }
                            }
                    }
                    paths_ = filtered;	// replace all
                }

		void Directory::reset()
		{
			resolve();
		}

		FileEditor::FileEditor(std::string filename, bool truncateOrNot, int retryTimes, int retryInterval)
		{
			// use absolute path
			filename_ = os::absolute_path(filename);
			// try open
			this->try_open(retryTimes, retryInterval, truncateOrNot);
		};

		//FileEditor::FileEditor(FileEditor&& other) : filename_(std::move(other.filename_)),
		//	stream_(std::move(other.stream_)),
		//	readPos_(std::move(other.readPos_)),
		//	writePos_(std::move(other.writePos_))
		//{
		//	other.filename_ = std::string();
		//};

		bool FileEditor::open(bool truncateOrNot)
		{
			if (this->is_open()) return true;
			std::ios::openmode mode = std::ios::in | std::ios::out | std::ios::binary;
			if (truncateOrNot)
			{
				mode |= std::ios::trunc;
			}
			else
			{
				mode |= std::ios::app;
			}
			os::fstream_open(stream_, filename_, mode);
			if (this->is_open()) return true;
			return false;
		}

		bool FileEditor::open(const char* filename, bool truncateOrNot, int retryTimes, int retryInterval)
		{
			this->close();
			// use absolute path
			filename_ = os::absolute_path(filename);
			// try open
			return this->try_open(retryTimes, retryInterval, truncateOrNot);
		}

		bool FileEditor::open(std::string filename, bool truncateOrNot, int retryTimes, int retryInterval)
		{
			this->close();
			// use absolute path
			filename_ = os::absolute_path(filename);
			// try open
			return this->try_open(retryTimes, retryInterval, truncateOrNot);
		}

		bool FileEditor::reopen(bool truncateOrNot)
		{
			this->close();
			return this->open(truncateOrNot);
		}

		void FileEditor::close()
		{
			stream_.close();
			stream_.clear();
			// unregister this file
			//detail::FileEditorRegistry::instance().erase(filename_);
		}

		bool FileEditor::try_open(int retryTime, int retryInterval, bool truncateOrNot)
		{
			while (retryTime > 0 && (!this->open(truncateOrNot)))
			{
				time::sleep(retryInterval);
				--retryTime;
			}
			return this->is_open();
		}

		FileReader::FileReader(std::string filename, int retryTimes, int retryInterval)
		{
			// use absolute path
			filename_ = os::absolute_path(filename);
			// try open
			this->try_open(retryTimes, retryInterval);
		}

		//FileReader::FileReader(FileReader&& other) : filename_(std::move(other.filename_)), istream_(std::move(other.istream_))
		//{
		//	other.filename_ = std::string();
		//};

		bool FileReader::open()
		{
			if (this->is_open()) return true;
			this->check_valid();
			os::ifstream_open(istream_, filename_, std::ios::in | std::ios::binary);
			if (this->is_open()) return true;
			return false;
		}

		bool FileReader::try_open(int retryTime, int retryInterval)
		{
			while (retryTime > 0 && (!this->open()))
			{
				time::sleep(retryInterval);
				--retryTime;
			}
			return this->is_open();
		}

		std::size_t FileReader::file_size()
		{
			if (is_open())
			{
				auto curPos = istream_.tellg();
				istream_.seekg(0, istream_.end);
				std::size_t size = static_cast<std::size_t>(istream_.tellg());
				istream_.seekg(curPos);
				return size;
			}
			return 0;
		}

		std::size_t FileReader::count_lines()
		{
			// store current read location
			std::streampos readPtrBackup = istream_.tellg();
			istream_.seekg(std::ios_base::beg);

			const int bufSize = 1024 * 1024;	// using 1MB buffer
			std::vector<char> buf(bufSize);

			std::size_t ct = 0;
			std::streamsize nbuf = 0;
			char last = 0;
			do
			{
				istream_.read(&buf.front(), bufSize);
				nbuf = istream_.gcount();
				for (auto i = 0; i < nbuf; i++)
				{
					last = buf[i];
					if (last == '\n')
					{
						++ct;
					}
				}
			} while (nbuf > 0);

			if (last != '\n') ++ct;

			// restore read position
			istream_.clear();
			istream_.seekg(readPtrBackup);

			return ct;
		}

		std::string FileReader::next_line(bool trimWhitespaces)
		{
			std::string line;

			if (!istream_.good() || !istream_.is_open())
			{
				return line;
			}

			if (!istream_.eof())
			{
				std::getline(istream_, line);
				if (trimWhitespaces) return fmt::trim(line);
				if (line.back() == '\r')
				{
					// LFCR \r\n problem
					line.pop_back();
				}
			}
			return line;
		}

		int FileReader::goto_line(int n)
		{
			if (!istream_.good() || !istream_.is_open())
				return -1;

			istream_.seekg(std::ios::beg);

			if (n < 0)
			{
				throw ArgException("Jumping to a negtive position!");
			}

			if (n == 0)
			{
				return 0;
			}

			int i = 0;
			for (i = 0; i < n - 1; ++i)
			{

				istream_.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

				if (istream_.eof())
				{
					log::detail::zupply_internal_warn("Reached end of file, line: " + std::to_string(i + 1));
					break;
				}
			}

			return i + 1;
		}

		std::size_t get_file_size(std::string filename)
		{
			std::ifstream fs;
			os::ifstream_open(fs, filename, std::ios::in | std::ios::ate);
			std::size_t size = static_cast<std::size_t>(fs.tellg());
			return size;
		}

	} //namespace fs



	namespace time
	{
		namespace consts
		{
			static const unsigned		kMaxDateTimeLength = 2048;
			static const char			*kDateFractionSpecifier = "%frac";
			static const unsigned	    kDateFractionWidth = 3;
			static const char			*kTimerPrecisionSecSpecifier = "%sec";
			static const char			*kTimerPrecisionMsSpecifier = "%ms";
			static const char			*kTimerPrecisionUsSpecifier = "%us";
			static const char			*kTimerPrecisionNsSpecifier = "%ns";
			static const char			*kDateTimeSpecifier = "%datetime";
		}

		DateTime::DateTime()
		{
			auto now = std::chrono::system_clock::now();
			timeStamp_ = std::chrono::system_clock::to_time_t(now);
			calendar_ = os::localtime(timeStamp_);
			fraction_ = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
				% math::Pow<10, consts::kDateFractionWidth>::result;
			fractionStr_ = fmt::int_to_zero_pad_str(fraction_, consts::kDateFractionWidth);
		}

		void DateTime::to_local_time()
		{
			calendar_ = os::localtime(timeStamp_);
		}

		void DateTime::to_utc_time()
		{
			calendar_ = os::gmtime(timeStamp_);
		}

		std::string DateTime::to_string(const char *format)
		{
			std::string fmt(format);
			fmt::replace_all_with_escape(fmt, consts::kDateFractionSpecifier, fractionStr_);
			std::vector<char> mbuf(fmt.length() + 100);
			std::size_t size = strftime(mbuf.data(), mbuf.size(), fmt.c_str(), &calendar_);
			while (size == 0)
			{
				if (mbuf.size() > consts::kMaxDateTimeLength)
				{
					return std::string("String size exceed limit!");
				}
				mbuf.resize(mbuf.size() * 2);
				size = strftime(mbuf.data(), mbuf.size(), fmt.c_str(), &calendar_);
			}
			return std::string(mbuf.begin(), mbuf.begin() + size);
		}

		DateTime DateTime::local_time()
		{
			DateTime date;
			return date;
		}

		DateTime DateTime::utc_time()
		{
			DateTime date;
			date.to_utc_time();
			return date;
		}

		Timer::Timer()
		{
			this->reset();
		};

		void Timer::reset()
		{
			timeStamp_ = std::chrono::steady_clock::now();
			elapsed_ = 0;
			paused_ = false;
		}

		void Timer::pause()
		{
			if (paused_) return;
			elapsed_ += elapsed_ns();

			paused_ = true;
		}

		void Timer::resume()
		{
			if (!paused_) return;
			timeStamp_ = std::chrono::steady_clock::now();
			paused_ = false;
		}

		std::size_t	Timer::elapsed_ns()
		{
			if (paused_) return elapsed_;
			return static_cast<std::size_t>(std::chrono::duration_cast<std::chrono::nanoseconds>
				(std::chrono::steady_clock::now() - timeStamp_).count()) + elapsed_;
		}

		std::string Timer::elapsed_ns_str()
		{
			return std::to_string(elapsed_ns());
		}

		std::size_t Timer::elapsed_us()
		{
			return elapsed_ns() / 1000;
		}

		std::string Timer::elapsed_us_str()
		{
			return std::to_string(elapsed_us());
		}

		std::size_t Timer::elapsed_ms()
		{
			return elapsed_ns() / 1000000;
		}

		std::string Timer::elapsed_ms_str()
		{
			return std::to_string(elapsed_ms());
		}

		std::size_t Timer::elapsed_sec()
		{
			return elapsed_ns() / 1000000000;
		}

		std::string Timer::elapsed_sec_str()
		{
			return std::to_string(elapsed_sec());
		}

		double Timer::elapsed_sec_double()
		{
			return static_cast<double>(elapsed_ns()) / 1000000000;
		}

		std::string Timer::to_string(const char *format)
		{
			std::string str(format);
			fmt::replace_all_with_escape(str, consts::kTimerPrecisionSecSpecifier, elapsed_sec_str());
			fmt::replace_all_with_escape(str, consts::kTimerPrecisionMsSpecifier, elapsed_ms_str());
			fmt::replace_all_with_escape(str, consts::kTimerPrecisionUsSpecifier, elapsed_us_str());
			fmt::replace_all_with_escape(str, consts::kTimerPrecisionNsSpecifier, elapsed_ns_str());
			return str;
		}


	} // namespace time



	namespace os
	{
		namespace consts
		{
			static const std::string kEndLineCRLF = std::string("\r\n");
			static const std::string kEndLineLF = std::string("\n");
			static const std::string kNativePathDelimWindows = "\\";
			static const std::string kNativePathDelimPosix = "/";
		}

		int system(const char *const command, const char *const moduleName)
		{
			misc::unused(moduleName);
#if ZUPPLY_OS_WINDOWS
			PROCESS_INFORMATION pi;
			STARTUPINFO si;
			std::memset(&pi, 0, sizeof(PROCESS_INFORMATION));
			std::memset(&si, 0, sizeof(STARTUPINFO));
			GetStartupInfoA(&si);
			si.cb = sizeof(si);
			si.wShowWindow = SW_HIDE;
			si.dwFlags |= SW_HIDE | STARTF_USESHOWWINDOW;
			const BOOL res = CreateProcessA((LPCTSTR)moduleName, (LPTSTR)command, 0, 0, FALSE, 0, 0, 0, &si, &pi);
			if (res) {
				WaitForSingleObject(pi.hProcess, INFINITE);
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				return 0;
			}
			else return std::system(command);
#elif ZUPPLY_OS_UNIX
			const unsigned int l = std::strlen(command);
			if (l) {
				char *const ncommand = new char[l + 16];
				std::strncpy(ncommand, command, l);
				std::strcpy(ncommand + l, " 2> /dev/null"); // Make command silent.
				const int out_val = std::system(ncommand);
				delete[] ncommand;
				return out_val;
			}
			else return -1;
#else
			misc::unused(command);
			return -1;
#endif
		}

		std::size_t thread_id()
		{
#if ZUPPLY_OS_WINDOWS
			// It exists because the std::this_thread::get_id() is much slower(espcially under VS 2013)
			return  static_cast<size_t>(::GetCurrentThreadId());
#elif __linux__
			return  static_cast<size_t>(syscall(SYS_gettid));
#else //Default to standard C++11 (OSX and other Unix)
			static std::mutex threadIdMutex;
			static std::map<std::thread::id, std::size_t> threadIdHashmap;
			std::lock_guard<std::mutex> lock(threadIdMutex);
			auto id = std::this_thread::get_id();
			if (threadIdHashmap.count(id) < 1)
			{
				threadIdHashmap[id] = threadIdHashmap.size() + 1;
			}
			return threadIdHashmap[id];
			//return static_cast<size_t>(std::hash<std::thread::id>()(std::this_thread::get_id()));
#endif

		}

		int is_atty()
		{
#if ZUPPLY_OS_WINDOWS
			return _isatty(_fileno(stdout));
#elif ZUPPLY_OS_UNIX
			return isatty(fileno(stdout));
#endif
			return 0;
		}

		Size console_size()
		{
			Size ret(-1, -1);
#if ZUPPLY_OS_WINDOWS
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
			ret.width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
			ret.height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#elif ZUPPLY_OS_UNIX
			struct winsize w;
			ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
			ret.width = w.ws_col;
			ret.height = w.ws_row;
#endif
			return ret;
		}

		std::tm localtime(std::time_t t)
		{
			std::tm temp;
#if ZUPPLY_OS_WINDOWS
			localtime_s(&temp, &t);
			return temp;
#elif ZUPPLY_OS_UNIX
			// POSIX SUSv2 thread safe localtime_r
			return *localtime_r(&t, &temp);
#else
			return temp; // return default tm struct, no idea what it is
#endif
		}

		std::tm gmtime(std::time_t t)
		{
			std::tm temp;
#if ZUPPLY_OS_WINDOWS
			gmtime_s(&temp, &t);
			return temp;
#elif ZUPPLY_OS_UNIX
			// POSIX SUSv2 thread safe gmtime_r
			return *gmtime_r(&t, &temp);
#else
			return temp; // return default tm struct, no idea what it is
#endif 
		}

		std::wstring utf8_to_wstring(std::string &u8str)
		{
#if ZUPPLY_OS_WINDOWS
			// windows use 16 bit wstring 
			std::u16string u16str = fmt::utf8_to_utf16(u8str);
			return std::wstring(u16str.begin(), u16str.end());
#else
			// otherwise use 32 bit wstring
			std::u32string u32str = fmt::utf8_to_utf32(u8str);
			return std::wstring(u32str.begin(), u32str.end());
#endif
		}

		std::string wstring_to_utf8(std::wstring &wstr)
		{
#if ZUPPLY_OS_WINDOWS
			// windows use 16 bit wstring 
			std::u16string u16str(wstr.begin(), wstr.end());
			return fmt::utf16_to_utf8(u16str);
#else
			// otherwise use 32 bit wstring
			std::u32string u32str(wstr.begin(), wstr.end());
			return fmt::utf32_to_utf8(u32str);
#endif
		}

		std::vector<std::string> path_split(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			std::replace(path.begin(), path.end(), '\\', '/');
			return fmt::split(path, '/');
#else
			return fmt::split(path, '/');
#endif
		}

		std::string path_join(std::vector<std::string> elems)
		{
#if ZUPPLY_OS_WINDOWS
			return fmt::join(elems, '\\');
#else
			return fmt::join(elems, '/');
#endif
		}

		std::string path_split_filename(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			std::string::size_type pos = fmt::trim(path).find_last_of("/\\");
#else
			std::string::size_type pos = fmt::trim(path).find_last_of("/");
#endif
			if (pos == std::string::npos) return path;
			if (pos != path.length())
			{
				return path.substr(pos + 1);
			}
			return std::string();
		}

		std::string path_split_directory(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			std::string::size_type pos = fmt::trim(path).find_last_of("/\\");
#else
			std::string::size_type pos = fmt::trim(path).find_last_of("/");
#endif
			if (pos != std::string::npos)
			{
				return path.substr(0, pos);
			}
			return std::string();
		}

		std::string path_split_extension(std::string path)
		{
			std::string filename = path_split_filename(path);
			auto list = fmt::split(filename, '.');
			if (list.size() > 1)
			{
				return list.back();
			}
			return std::string();
		}

		std::string path_split_basename(std::string path)
		{
			std::string filename = path_split_filename(path);
			auto list = fmt::split(filename, '.');
			if (list.size() == 1)
			{
				return list[0];
			}
			else if (list.size() > 1)
			{
				return fmt::join({ list.begin(), list.end() - 1 }, '.');
			}
			return std::string();
		}

		std::string path_append_basename(std::string origPath, std::string whatToAppend)
		{
			std::string newPath = path_join({ path_split_directory(origPath), path_split_basename(origPath) })
				+ whatToAppend;
			std::string ext = path_split_extension(origPath);
			if (!ext.empty())
			{
				newPath += "." + ext;
			}
			return newPath;
		}


		void fstream_open(std::fstream &stream, std::string filename, std::ios::openmode openmode)
		{
			// make sure directory exists for the target file
			create_directory_recursive(path_split_directory(filename));
#if ZUPPLY_OS_WINDOWS
			stream.open(utf8_to_wstring(filename), openmode);
#else
			stream.open(filename, openmode);
#endif
		}

		void ifstream_open(std::ifstream &stream, std::string filename, std::ios::openmode openmode)
		{
#if ZUPPLY_OS_WINDOWS
			stream.open(utf8_to_wstring(filename), openmode);
#else
			stream.open(filename, openmode);
#endif
		}

		bool rename(std::string oldName, std::string newName)
		{
#if ZUPPLY_OS_WINDOWS
			return (!_wrename(utf8_to_wstring(oldName).c_str(), utf8_to_wstring(newName).c_str()));
#else
			return (!::rename(oldName.c_str(), newName.c_str()));
#endif
		}

		bool copyfile(std::string src, std::string dst, bool replaceDst)
		{
			if (!replaceDst)
			{
				if (path_exists(dst, true)) return false;
			}
			remove_all(dst);
			std::ifstream  srcf;
			std::fstream  dstf;
			ifstream_open(srcf, src, std::ios::binary);
			fstream_open(dstf, dst, std::ios::binary | std::ios::trunc);
			dstf << srcf.rdbuf();
			return true;
		}

		bool movefile(std::string src, std::string dst, bool replaceDst)
		{
			if (!replaceDst)
			{
				if (path_exists(dst, true)) return false;
			}
			return os::rename(src, dst);
		}

		bool remove_all(std::string path)
		{
			if (!os::path_exists(path, true)) return true;
			if (os::is_directory(path))
			{
				return remove_dir(path);
			}
			return remove_file(path);
		}

#if ZUPPLY_OS_UNIX 
		// for nftw tree walk directory operations, so for unix only
		int nftw_remove(const char *path, const struct stat *sb, int flag, struct FTW *ftwbuf)
		{
			misc::unused(sb);
			misc::unused(flag);
			misc::unused(ftwbuf);
			return ::remove(path);
		}
#endif

		bool remove_dir(std::string path, bool recursive)
		{
			// as long as dir does not exist, return true
			if (!is_directory(path)) return true;
#if ZUPPLY_OS_WINDOWS
			std::wstring root = utf8_to_wstring(path);
			bool            bSubdirectory = false;       // Flag, indicating whether subdirectories have been found
			HANDLE          hFile;                       // Handle to directory
			std::wstring     strFilePath;                 // Filepath
			std::wstring     strPattern;                  // Pattern
			WIN32_FIND_DATAW FileInformation;             // File information


			strPattern = root + L"\\*.*";
			hFile = ::FindFirstFileW(strPattern.c_str(), &FileInformation);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (FileInformation.cFileName[0] != '.')
					{
						strFilePath.erase();
						strFilePath = root + L"\\" + FileInformation.cFileName;

						if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{
							if (recursive)
							{
								// Delete subdirectory
								bool iRC = remove_dir(wstring_to_utf8(strFilePath), recursive);
								if (!iRC) return false;
							}
							else
								bSubdirectory = true;
						}
						else
						{
							// Set file attributes
							if (::SetFileAttributesW(strFilePath.c_str(),
								FILE_ATTRIBUTE_NORMAL) == FALSE)
								return false;

							// Delete file
							if (::DeleteFileW(strFilePath.c_str()) == FALSE)
								return false;
						}
					}
				} while (::FindNextFileW(hFile, &FileInformation) == TRUE);

				// Close handle
				::FindClose(hFile);

				DWORD dwError = ::GetLastError();
				if (dwError != ERROR_NO_MORE_FILES)
					return false;
				else
				{
					if (!bSubdirectory)
					{
						// Set directory attributes
						if (::SetFileAttributesW(root.c_str(),
							FILE_ATTRIBUTE_NORMAL) == FALSE)
							return false;

						// Delete directory
						if (::RemoveDirectoryW(root.c_str()) == FALSE)
							return false;
					}
				}
			}

			return true;
#else
			if (recursive) ::nftw(path.c_str(), nftw_remove, 20, FTW_DEPTH);
			else ::remove(path.c_str());
			return (is_directory(path) == false);
#endif
		}

		bool remove_file(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			_wremove(utf8_to_wstring(path).c_str());
#else
			unlink(path.c_str());
#endif
			return (is_file(path) == false);
		}

		std::string last_error()
		{
#if ZUPPLY_OS_WINDOWS
			DWORD error = GetLastError();
			if (error)
			{
				LPVOID lpMsgBuf;
				DWORD bufLen = FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					error,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR)&lpMsgBuf,
					0, NULL);
				if (bufLen)
				{
					LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
					std::string result(lpMsgStr, lpMsgStr + bufLen);

					LocalFree(lpMsgBuf);

					return result;
				}
			}
#else
			char* errStr = strerror(errno);
			if (errStr) return std::string(errStr);
#endif
			return std::string();
		}

		std::string endl()
		{
#if ZUPPLY_OS_WINDOWS
			return consts::kEndLineCRLF;
#else // *nix, OSX, and almost everything else(OS 9 or ealier use CR only, but they are antiques now)
			return consts::kEndLineLF;
#endif
		}

		std::string path_delim()
		{
#if ZUPPLY_OS_WINDOWS
			return consts::kNativePathDelimWindows;
#else // posix
			return consts::kNativePathDelimPosix;
#endif
		}

		std::string current_working_directory()
		{
#if ZUPPLY_OS_WINDOWS
			wchar_t *buffer = nullptr;
			if ((buffer = _wgetcwd(nullptr, 0)) == nullptr)
			{
				// failed
				log::detail::zupply_internal_warn("Unable to get current working directory!");
				return std::string(".");
			}
			else
			{
				std::wstring ret(buffer);
				free(buffer);
				return wstring_to_utf8(ret);
			}
#elif _GNU_SOURCE
			char *buffer = get_current_dir_name();
			if (buffer == nullptr)
			{
				// failed
				log::detail::zupply_internal_warn("Unable to get current working directory!");
				return std::string(".");
			}
			else
			{
				// success
				std::string ret(buffer);
				free(buffer);
				return ret;
			}
#else
			char *buffer = getcwd(nullptr, 0);
			if (buffer == nullptr)
			{
				// failed
				log::detail::zupply_internal_warn("Unable to get current working directory!");
				return std::string(".");
			}
			else
			{
				// success
				std::string ret(buffer);
				free(buffer);
				return ret;
			}
#endif
		}

		bool path_exists(std::string path, bool considerFile)
		{
#if ZUPPLY_OS_WINDOWS
			DWORD fileType = GetFileAttributesW(utf8_to_wstring(path).c_str());
			if (fileType == INVALID_FILE_ATTRIBUTES) {
				return false;
			}
			return considerFile ? true : ((fileType & FILE_ATTRIBUTE_DIRECTORY) == 0 ? false : true);
#elif ZUPPLY_OS_UNIX
			struct stat st;
			int ret = stat(path.c_str(), &st);
			return considerFile ? (ret == 0) : S_ISDIR(st.st_mode);			
#endif
			return false;
		}

		bool is_file(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			DWORD fileType = GetFileAttributesW(utf8_to_wstring(path).c_str());
			if (fileType == INVALID_FILE_ATTRIBUTES || ((fileType & FILE_ATTRIBUTE_DIRECTORY) != 0))
			{
				return false;
			}
			return true;
#elif ZUPPLY_OS_UNIX
			struct stat st;
			if (stat(path.c_str(), &st) != 0) return false;
			if (S_ISDIR(st.st_mode)) return false;
			return true;
#endif
			return false;
		}

		bool is_directory(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			DWORD fileType = GetFileAttributesW(utf8_to_wstring(path).c_str());
			if (fileType == INVALID_FILE_ATTRIBUTES) return false;
			if ((fileType & FILE_ATTRIBUTE_DIRECTORY) != 0) return true;
			return false;
#elif ZUPPLY_OS_UNIX
			struct stat st;
			if (stat(path.c_str(), &st) != 0) return false;
			if (S_ISDIR(st.st_mode)) return true;
			return false;
#endif
			return false;
		}

		bool path_identical(std::string first, std::string second, bool forceCaseSensitve)
		{
#if ZUPPLY_OS_WINDOWS
			if (!forceCaseSensitve)
			{
				return fmt::to_lower_ascii(first) == fmt::to_lower_ascii(second);
			}
			return first == second;
#else
			return first == second;
#endif
		}

		std::string absolute_path(std::string reletivePath)
		{
#if ZUPPLY_OS_WINDOWS
			wchar_t *buffer = nullptr;
			std::wstring widePath = utf8_to_wstring(reletivePath);
			buffer = _wfullpath(buffer, widePath.c_str(), _MAX_PATH);
			if (buffer == nullptr)
			{
				// failed
				log::detail::zupply_internal_warn("Unable to get absolute path for: " + reletivePath + "! Return original.");
				return reletivePath;
			}
			else
			{
				std::wstring ret(buffer);
				free(buffer);
				return wstring_to_utf8(ret);
			}
#elif ZUPPLY_OS_UNIX
			char *buffer = realpath(reletivePath.c_str(), nullptr);
			if (buffer == nullptr)
			{
				// failed
				if (ENOENT == errno)
				{
					// try recover manually
					std::string dirtyPath;
					if (fmt::starts_with(reletivePath, "/"))
					{
						// already an absolute path
						dirtyPath = reletivePath;
					}
					else
					{
						dirtyPath = path_join({ current_working_directory(), reletivePath });
					}
					std::vector<std::string> parts = path_split(dirtyPath);
					std::vector<std::string> ret;
					for (auto i = parts.begin(); i != parts.end(); ++i)
					{
						if (*i == ".") continue;
						if (*i == "..")
						{
							if (ret.size() < 1) throw RuntimeException("Invalid path: " + dirtyPath);
							ret.pop_back();
						}
						else
						{
							ret.push_back(*i);
						}
					}
					std::string tmp = path_join(ret);
					if (!fmt::starts_with(tmp, "/")) tmp = "/" + tmp;
					return tmp;
				}
				//still failed
				log::detail::zupply_internal_warn("Unable to get absolute path for: " + reletivePath + "! Return original.");
				return reletivePath;
			}
			else
			{
				std::string ret(buffer);
				free(buffer);
				return ret;
			}
#endif
			return std::string();
		}

		bool create_directory(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			std::wstring widePath = utf8_to_wstring(path);
			int ret = _wmkdir(widePath.c_str());
			if (0 == ret) return true;	// success
			if (EEXIST == ret) return true; // already exists
			return false;
#elif ZUPPLY_OS_UNIX
			// read/write/search permissions for owner and group
			// and with read/search permissions for others
			int status = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			if (0 == status) return true; // success
			if (EEXIST == errno) return true; // already exists
			return false;
#endif
			return false;
		}

		bool create_directory_recursive(std::string path)
		{
			std::string tmp = absolute_path(path);
			std::string target = tmp;

			while (!is_directory(tmp))
			{
				if (tmp.empty()) return false;	// invalid path
				tmp = absolute_path(tmp + "/../");
			}

			// tmp is the root from where to build
			auto list = path_split(fmt::lstrip(target, tmp));
			for (auto sub : list)
			{
				tmp = path_join({ tmp, sub });
				if (!create_directory(tmp)) break;
			}
			return is_directory(path);
		}

		std::vector<std::string> list_directory(std::string root)
		{
			std::vector<std::string> ret;
			root = os::absolute_path(root);
			if (os::is_file(root))
			{
				// it's a file, not dir
				log::detail::zupply_internal_warn(root + " is a file, not a directory!");
				return ret;
			}
			if (!os::is_directory(root)) return ret;	// not a dir

#if ZUPPLY_OS_WINDOWS
			std::wstring wroot = os::utf8_to_wstring(root);
			wchar_t dirPath[1024];
			wsprintfW(dirPath, L"%s\\*", wroot.c_str());
			WIN32_FIND_DATAW f;
			HANDLE h = FindFirstFileW(dirPath, &f);
			if (h == INVALID_HANDLE_VALUE) { return ret; }

			do
			{
				const wchar_t *name = f.cFileName;
				if (lstrcmpW(name, L".") == 0 || lstrcmpW(name, L"..") == 0) { continue; }
				std::wstring path = wroot + L"\\" + name;
				ret.push_back(os::wstring_to_utf8(path));
			} while (FindNextFileW(h, &f));
			FindClose(h);
			return ret;
#else
			DIR *dir;
			struct dirent *entry;

			if (!(dir = opendir(root.c_str())))
				return ret;
			if (!(entry = readdir(dir)))
				return ret;

			do {
				if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
					continue;
				std::string path = root + "/" + entry->d_name;
				ret.push_back(path);
			} while ((entry = readdir(dir)));
			closedir(dir);
			return ret;
#endif
			return ret;
		}

	}// namespace os

	namespace cds
	{
		bool RWLockable::Counters::is_waiting_for_write() const {
			return writeClaim_ != writeDone_;
		}

		bool RWLockable::Counters::is_waiting_for_read() const {
			return read_ != 0;
		}

		bool RWLockable::Counters::is_my_turn_to_write(Counters const & claim) const {
			return writeDone_ == claim.writeClaim_ - 1;
		}

		bool RWLockable::Counters::want_to_read(RWLockable::Counters * buf) const {
			if (read_ == UINT16_MAX) {
				return false;
			}
			*buf = *this;
			buf->read_ += 1;
			return true;
		}

		bool RWLockable::Counters::want_to_write(RWLockable::Counters * buf) const {
			if (writeClaim_ == UINT8_MAX) {
				return false;
			}
			*buf = *this;
			buf->writeClaim_ += 1;
			return true;
		}

		RWLockable::Counters RWLockable::Counters::done_reading() const {
			Counters c = *this;
			c.read_ -= 1;
			return c;
		}

		RWLockable::Counters RWLockable::Counters::done_writing() const {
			Counters c = *this;
			c.writeDone_ += 1;
			if (c.writeDone_ == UINT8_MAX) {
				c.writeClaim_ = c.writeDone_ = 0;
			}
			return c;
		}

		RWLock RWLockable::lock_for_read() {
			bool written = false;
			do {
				Counters exp = counters_.load(std::memory_order_acquire);
				do {
					if (exp.is_waiting_for_write()) {
						break;
					}
					Counters claim;
					if (!exp.want_to_read(&claim)) {
						break;
					}
					written = counters_.compare_exchange_weak(
						exp, claim,
						std::memory_order_release,
						std::memory_order_acquire
						);
				} while (!written);
				// todo: if (!written) progressive backoff
			} while (!written);
			return RWLock(this, false);
		}

		RWLock RWLockable::lock_for_write() {
			Counters exp = counters_.load(std::memory_order_acquire);
			Counters claim;
			do {
				while (!exp.want_to_write(&claim)) {
					// todo: progressive backoff
					exp = counters_.load(std::memory_order_acquire);
				}
			} while (!counters_.compare_exchange_weak(
				exp, claim,
				std::memory_order_release,
				std::memory_order_acquire
				));
			while (exp.is_waiting_for_read() || !exp.is_my_turn_to_write(claim)) {
				// todo: progressive backoff
				exp = counters_.load(std::memory_order_acquire);
			}
			return RWLock(this, true);
		}

		void RWLockable::unlock_read() {
			Counters exp = counters_.load(std::memory_order_consume);
			Counters des;
			do {
				des = exp.done_reading();
			} while (!counters_.compare_exchange_weak(
				exp, des,
				std::memory_order_release,
				std::memory_order_consume
				));
		}

		void RWLockable::unlock_write() {
			Counters exp = counters_.load(std::memory_order_consume);
			Counters des;
			do {
				des = exp.done_writing();
			} while (!counters_.compare_exchange_weak(
				exp, des,
				std::memory_order_release,
				std::memory_order_consume
				));
		}

		bool RWLockable::is_lock_free() const {
			return counters_.is_lock_free();
		}

		RWLock::RWLock(RWLockable * const lockable, bool const exclusive)
			: lockable_(lockable)
			, lockType_(exclusive ? LockType::write : LockType::read)
		{}

		RWLock::RWLock()
			: lockable_(nullptr)
			, lockType_(LockType::none)
		{}


		RWLock::RWLock(RWLock&& rhs) {
			lockable_ = rhs.lockable_;
			lockType_ = rhs.lockType_;
			rhs.lockable_ = nullptr;
			rhs.lockType_ = LockType::none;
		}

		RWLock& RWLock::operator =(RWLock&& rhs) {
			std::swap(lockable_, rhs.lockable_);
			std::swap(lockType_, rhs.lockType_);
			return *this;
		}


		RWLock::~RWLock() {
			if (lockable_ == nullptr) {
				return;
			}
			switch (lockType_) {
			case LockType::read:
				lockable_->unlock_read();
				break;
			case LockType::write:
				lockable_->unlock_write();
				break;
			default:
				// do nothing
				break;
			}
		}

		void RWLock::unlock() {
			(*this) = RWLock();
			// emptyLock's dtor will now activate
		}

		RWLock::LockType RWLock::get_lock_type() const {
			return lockType_;
		}
	} // namespace cds

	namespace cfg
	{

		CfgParser::CfgParser(std::string filename) : ln_(0)
		{
			os::ifstream_open(stream_, filename, std::ios::in | std::ios::binary);
			if (!stream_.is_open())
			{
				throw IOException("Failed to open config file: " + filename);
			}
			pstream_ = &stream_;
			parse(root_);
		}

		void CfgParser::error_handler(std::string msg)
		{
			throw RuntimeException("Parser failed at line<" + std::to_string(ln_) + "> " + line_ + "\n" + msg);
		}

		CfgLevel* CfgParser::parse_section(std::string sline, CfgLevel* lvl)
		{
			if (sline.empty()) return lvl;
			if (sline.back() == '.') error_handler("section ends with .");
			auto pos = sline.find('.');
			std::string currentSection;
			std::string nextSection;
			if (std::string::npos == pos)
			{
				currentSection = sline;
			}
			else
			{
				currentSection = sline.substr(0, pos);
				nextSection = sline.substr(pos + 1);
			}
			CfgLevel *l = &lvl->sections[currentSection];
			l->depth = lvl->depth + 1;
			l->parent = lvl;
			l->prefix = lvl->prefix + currentSection + ".";
			return parse_section(nextSection, l);
		}

		CfgLevel* CfgParser::parse_key_section(std::string& vline, CfgLevel *lvl)
		{
			if (vline.back() == '.') error_handler("key ends with .");
			auto pos = vline.find('.');
			if (std::string::npos == pos) return lvl;
			std::string current = vline.substr(0, pos);
			vline = vline.substr(pos + 1);
			CfgLevel *l = &lvl->sections[current];
			l->depth = lvl->depth + 1;
			l->parent = lvl;
			l->prefix = lvl->prefix + current + ".";
			return parse_key_section(vline, l);
		}

		std::string CfgParser::split_key_value(std::string& line)
		{
			auto pos1 = line.find_first_of(':');
			auto pos2 = line.find_first_of('=');
			if (std::string::npos == pos1 && std::string::npos == pos2)
			{
				throw RuntimeException("':' or '=' expected in key-value pair, none found.");
			}
			auto pos = (std::min)(pos1, pos2);
			std::string key = fmt::trim(std::string(line.substr(0, pos)));
			line = fmt::trim(std::string(line.substr(pos + 1)));
			return key;
		}

		void CfgParser::parse(CfgLevel& lvl)
		{
			CfgLevel *ls = &lvl;
			while (std::getline(*pstream_, line_))
			{
				++ln_;
				line_ = fmt::rskip_all(line_, "#");
				line_ = fmt::rskip_all(line_, ";");
				line_ = fmt::trim(line_);
				if (line_.empty()) continue;
				if (line_[0] == '[')
				{
					// start a section
					if (line_.back() != ']')
					{
						error_handler("missing right ]");
					}
					ls = &lvl;
					ls = parse_section(std::string(line_.substr(1, line_.length() - 2)), ls);
				}
				else
				{
					// parse section inline with key
					std::string key = split_key_value(line_);
					CfgLevel *lv = ls;
					lv = parse_key_section(key, lv);
					if (lv->values.find(key) != lv->values.end())
					{
						error_handler("duplicate key");
					}
					lv->values[key] = line_;
				}
			}
		}

		ArgOption::ArgOption(char shortKey, std::string longKey) :
			shortKey_(shortKey), longKey_(longKey),
			type_(""), default_(""), required_(false),
			min_(0), max_(1), once_(false), count_(0),
			val_(""), size_(0)
		{}

		ArgOption::ArgOption(char shortKey) : ArgOption(shortKey, "")
		{}

		ArgOption::ArgOption(std::string longKey) : ArgOption(-1, longKey)
		{}

		ArgOption& ArgOption::call(std::function<void()> todo)
		{
			callback_.push_back(todo);
			return *this;
		}

		ArgOption& ArgOption::call(std::function<void()> todo, std::function<void()> otherwise)
		{
			callback_.push_back(todo);
			othercall_.push_back(otherwise);
			return *this;
		}

		ArgOption& ArgOption::set_help(std::string helpInfo)
		{
			help_ = helpInfo;
			return *this;
		}

		ArgOption& ArgOption::require(bool require)
		{
			required_ = require;
			return *this;
		}

		ArgOption& ArgOption::set_once(bool onlyOnce)
		{
			once_ = onlyOnce;
			return *this;
		}

		ArgOption& ArgOption::set_type(std::string type)
		{
			type_ = type;
			return *this;
		}

		ArgOption& ArgOption::set_min(int minCount)
		{
			min_ = minCount > 0 ? minCount : 0;
			return *this;
		}

		ArgOption& ArgOption::set_max(int maxCount)
		{
			max_ = maxCount;
			if (max_ < 0) max_ = -1;
			return *this;
		}

		Value ArgOption::get_value()
		{
			return val_;
		}

		int ArgOption::get_count()
		{
			return count_;
		}

		std::string ArgOption::get_help()
		{
			// target: '-i, --input=FILE		this is an example input(default: abc.txt)'
			const std::size_t alignment = 26;
			std::string ret;

			// place holders
			if (shortKey_ == -1 && longKey_.empty() && min_ > 0)
			{
				ret = "<" + type_ + ">";
			}
			else
			{
				if (shortKey_ != -1)
				{
					ret += "-";
					ret.push_back(shortKey_);
				}
				if (!longKey_.empty())
				{
					if (!ret.empty()) ret += ", ";
					ret += "--";
					ret += longKey_;
				}
				if (!type_.empty())
				{
					ret.push_back('=');
					ret += type_;
				}
			}

			if (ret.size() < alignment)
			{
				while (ret.size() < alignment)
					ret.push_back(' ');
			}
			else
			{
				// super long option, then create a new line
				ret.push_back('\n');
				std::string tmp;
				tmp.resize(alignment);
				ret += tmp;
			}
			ret += help_;
			if (!default_.empty())
			{
				ret += "(default: ";
				ret += default_;
				ret += ")";
			}
			return ret;
		}

		std::string ArgOption::get_short_help()
		{
			// [-a=<DOUBLE>] [--long=<INT> [<INT>]]...
			std::string ret = "-";
			if (shortKey_ != -1)
			{
				ret.push_back(shortKey_);
			}
			else if (!longKey_.empty())
			{
				ret += "-" + longKey_;
			}
			else
			{
				ret = "<" + type_ + ">";
				return ret;
			}

			// type info
			std::string st = "<" + type_ + ">";
			if (min_ > 0)
			{
				if (required_) ret.push_back('=');
				else ret.push_back(' ');
				ret += st;
				for (int i = 2; i < min_; ++i)
				{
					ret.push_back(' ');
					ret += st;
				}
				if (max_ == -1 || max_ > min_)
				{
					ret += " {" + st + "}...";
				}
			}

			if (!required_)
			{
				ret = "[" + ret + "]";
			}
			return ret;
		}

		ArgParser::ArgParser() : info_({ "program", "?" })
		{
			add_opt_internal(-1, "").set_max(0);	// reserve for help
			add_opt_internal(-1, "").set_max(0);	// reserve for version
		}

		void ArgParser::add_info(std::string info)
		{
			info_.push_back(info);
		}

		std::vector<Value> ArgParser::arguments() const
		{
			return args_;
		}

		void ArgParser::register_keys(char shortKey, std::string longKey, std::size_t pos)
		{
			if (shortKey != -1)
			{
				if (shortKey < 32 || shortKey > 126)
				{
					// unsupported ASCII characters
					throw ArgException("Unsupported ASCII character: " + std::string(1, shortKey));
				}
				auto opt = shortKeys_.find(shortKey);
				if (opt != shortKeys_.end())
				{
					std::string tmp("Short key: ");
					tmp.push_back(shortKey);
					throw ArgException(tmp + " already occupied -> " + options_[opt->second].get_help());
				}
				// register short key
				shortKeys_[shortKey] = pos;
			}

			if (!longKey.empty())
			{
				auto opt = longKeys_.find(longKey);
				if (opt != longKeys_.end())
				{
					throw ArgException("Long key: " + longKey + " already occupied -> " + options_[opt->second].get_help());
				}
				// register long key
				longKeys_[longKey] = pos;
			}
		}

		ArgOption& ArgParser::add_opt_internal(char shortKey, std::string longKey)
		{
			options_.push_back(ArgOption(shortKey, longKey));
			if (shortKey != -1 || (!longKey.empty()))
			{
				register_keys(shortKey, longKey, options_.size() - 1);
			}
			return options_.back();
		}

		ArgOption& ArgParser::add_opt(char shortKey, std::string longKey)
		{
			return add_opt_internal(shortKey, longKey);
		}

		ArgOption& ArgParser::add_opt(char shortKey)
		{
			return add_opt_internal(shortKey, "");
		}

		ArgOption& ArgParser::add_opt(std::string longKey)
		{
			return add_opt_internal(-1, longKey);
		}

		void ArgParser::add_opt_help(char shortKey, std::string longKey, std::string help)
		{
			auto& opt = options_[0];	// options_[0] reserved for help
			register_keys(shortKey, longKey, 0);
			opt.shortKey_ = shortKey;
			opt.longKey_ = longKey;
			opt.set_help(help)
				.call([this]{std::cout << "\n" << this->get_help() << std::endl; std::exit(0); })
				.set_min(0).set_max(0);
		}

		void ArgParser::add_opt_version(char shortKey, std::string longKey, std::string version, std::string help)
		{
			info_[1] = version;			// info_[1] reserved for version string
			auto& opt = options_[1];	// options_[1] reserved for version option
			register_keys(shortKey, longKey, 1);
			opt.shortKey_ = shortKey;
			opt.longKey_ = longKey;
			opt.set_help(help)
				.call([this]{std::cout << this->version() << std::endl; std::exit(0); })
				.set_min(0).set_max(0);
		}

		ArgOption& ArgParser::add_opt_flag(char shortKey, std::string longKey, std::string help, bool* dst)
		{
			auto& opt = add_opt(shortKey, longKey).set_help(help).set_min(0).set_max(0);
			if (nullptr != dst)
			{
				opt.call([dst]{*dst = true; }, [dst]{*dst = false; });
			}
			return opt;
		}

		ArgParser::Type ArgParser::check_type(std::string opt)
		{
			if (opt.length() == 1 && opt[0] == '-') return Type::INVALID;
			if (opt.length() == 2 && opt == "--") return Type::INVALID;
			if (opt[0] == '-' && opt[1] == '-')
			{
				if (opt.length() < 3) return Type::INVALID;
				return Type::LONG_KEY;
			}
			else if (opt[0] == '-')
			{
				return Type::SHORT_KEY;
			}
			else
			{
				return Type::ARGUMENT;
			}
		}

		ArgParser::ArgQueue ArgParser::pretty_arguments(int argc, char** argv)
		{
			ArgQueue queue;
			for (int p = 1; p < argc; ++p)
			{
				std::string opt(argv[p]);
				Type type = check_type(opt);
				if (Type::SHORT_KEY == type)
				{
					// parse options like -abc=somevalue
					auto lr = fmt::split_first_occurance(opt, '=');
					auto first = lr.first; // -abc
					for (std::size_t i = 1; i < first.length(); ++i)
					{
						// put a b c into queue
						queue.push_back(std::make_pair("-" + first.substr(i, 1), Type::SHORT_KEY));
					}
					if (!lr.second.empty())
					{
						// put somevalue into queue
						queue.push_back(std::make_pair(lr.second, Type::ARGUMENT));
					}
				}
				else if (Type::LONG_KEY == type)
				{
					// parse long option like: --long=somevalue
					auto lr = fmt::split_first_occurance(opt, '=');
					queue.push_back(std::make_pair(lr.first, Type::LONG_KEY));
					if (!lr.second.empty())
					{
						queue.push_back(std::make_pair(lr.second, Type::ARGUMENT));
					}
				}
				else if (Type::ARGUMENT == type)
				{
					if (opt.length() > 2)
					{
						if (opt.front() == '\'' && opt.back() == '\'')
						{
							opt = opt.substr(1, opt.length() - 2);
						}
					}
					queue.push_back(std::make_pair(opt, Type::ARGUMENT));
				}
				else
				{
					queue.push_back(std::make_pair(opt, Type::INVALID));
				}
			}
			return queue;
		}

		void ArgParser::error_option(std::string opt, std::string msg)
		{
			errors_.push_back("Error parsing option: " + opt + " " + msg);
		}

		void ArgParser::parse_option(ArgOption* ptr)
		{
			++ptr->count_;
		}

		void ArgParser::parse_value(ArgOption* ptr, const std::string& value)
		{
			if (ptr == nullptr) args_.push_back(value);
			else
			{
				if (ptr->max_ < 0 || ptr->size_ < ptr->max_)
				{
					// boundless or not yet reached maximum
					ptr->val_ = ptr->val_.str() + value + " ";
					++ptr->size_;
				}
				else
				{
					args_.push_back(value);
					ptr = nullptr;
				}
			}
		}

		void ArgParser::parse(int argc, char** argv, bool ignoreUnknown)
		{
			if (argc < 1)
			{
				errors_.push_back("Argc < 1");
				return;
			}

			// 0. prog name
			info_[0] = os::path_split_filename(std::string(argv[0]));

			// 1. make prettier argument queue
			auto queue = pretty_arguments(argc, argv);

			// 2. parse one by one
			ArgOption *opt = nullptr;
			for (auto q : queue)
			{
				if (Type::SHORT_KEY == q.second)
				{
					auto iter = shortKeys_.find(q.first[1]);
					if (iter == shortKeys_.end())
					{
						if (!ignoreUnknown) error_option(q.first, "unknown option.");
						continue;
					}
					opt = &options_[iter->second];
					parse_option(opt);
				}
				else if (Type::LONG_KEY == q.second)
				{
					auto iter = longKeys_.find(q.first.substr(2));
					if (iter == longKeys_.end())
					{
						if (!ignoreUnknown) error_option(q.first, "unknown option.");
						continue;
					}
					opt = &options_[iter->second];
					parse_option(opt);
				}
				else if (Type::ARGUMENT == q.second)
				{
					parse_value(opt, q.first);
				}
				else
				{
					error_option(q.first, "invalid option.");
				}
			}

			// 3. placeholders
			for (auto o = options_.begin(); o != options_.end(); ++o)
			{
				if (o->shortKey_ == -1 && o->longKey_.empty())
				{
					int n = o->max_;
					while (n > 0 && args_.size() > 0)
					{
						o->val_ = o->val_.str() + " " + args_[0].str();
						++o->count_;
						--n;
						args_.erase(args_.begin());
					}
					o->val_ = fmt::trim(o->val_.str());
					if (args_.empty()) break;
				}
			}

			// 4. callbacks
			for (auto o : options_)
			{
				if (o.count_ > 0)
				{
					// callback functions
					for (auto call : o.callback_)
					{
						call();
					}
				}
				else
				{
					// not found, call the other functions
					for (auto othercall : o.othercall_)
					{
						othercall();
					}
				}
			}


			// 5. check errors
			for (auto o : options_)
			{
				if (o.required_ && (o.count_ < 1))
				{
					errors_.push_back("[ArgParser Error]: Required option not found: " + o.get_short_help());
				}
				if (o.count_ > 1 && o.min_ > o.size_)
				{
					errors_.push_back("[ArgParser Error]:" + o.get_short_help() + " need at least " + std::to_string(o.min_) + " arguments.");
				}
				if (o.count_ > 1 && o.once_)
				{
					errors_.push_back("[ArgParser Error]:" + o.get_short_help() + " limited to be called only once.");
				}
			}
		}

		std::string ArgParser::get_help()
		{
			std::string ret("Usage: ");
			ret += info_[0];
			std::string usageLine;

			// single line options list
			for (auto opt : options_)
			{
				if (!opt.required_ && opt.max_ == 0 && opt.shortKey_ != -1)
				{
					// put optional short flags together [-abcdefg...]
					usageLine.push_back(opt.shortKey_);
				}
			}
			if (!usageLine.empty())
			{
				usageLine = " [-" + usageLine + "]";
			}

			// options that take arguments
			for (auto opt : options_)
			{
				if (!opt.required_ && opt.max_ == 0) continue;
				std::string tmp = " " + opt.get_short_help();
				if (!tmp.empty()) usageLine += tmp;
			}

			ret += " " + usageLine + "\n";

			for (std::size_t i = 2; i < info_.size(); ++i)
			{
				ret += "  " + info_[i] + "\n";
			}

			// required options first
			ret += "\n  Required options:\n";
			for (auto opt : options_)
			{
				if (opt.required_)
				{
					std::string tmpLine = "  " + opt.get_help() + "\n";
					if (fmt::trim(tmpLine).empty()) continue;
					ret += tmpLine;
				}
			}
			// optional options
			ret += "\n  Optional options:\n";
			for (auto opt : options_)
			{
				if (!opt.required_)
				{
					std::string tmpLine = "  " + opt.get_help() + "\n";
					if (fmt::trim(tmpLine).empty()) continue;
					ret += tmpLine;
				}
			}
			return ret;
		}

		std::string ArgParser::get_error()
		{
			std::string ret;
			for (auto e : errors_)
			{
				ret += e;
				ret.push_back('\n');
			}
			return ret;
		}

		int	ArgParser::count(char shortKey)
		{
			auto iter = shortKeys_.find(shortKey);
			return iter == shortKeys_.end() ? 0 : options_[iter->second].count_;
		}

		int ArgParser::count(std::string longKey)
		{
			auto iter = longKeys_.find(longKey);
			return iter == longKeys_.end() ? 0 : options_[iter->second].count_;
		}

		Value ArgParser::operator[](const std::string& longKey)
		{
			auto iter = longKeys_.find(longKey);
			if (iter == longKeys_.end())
			{
				return Value();
			}
			else
			{
				return options_[iter->second].val_;
			}
		}

		Value ArgParser::operator[](const char shortKey)
		{
			auto iter = shortKeys_.find(shortKey);
			if (iter == shortKeys_.end())
			{
				return Value();
			}
			else
			{
				return options_[iter->second].val_;
			}
		}

	} // namespace cfg

	namespace log
	{
		ProgBar::ProgBar(unsigned range, std::string info)
			: ss_(nullptr), info_(info)
		{
			range_ = range;
			pos_ = 0;
			running_ = false;
			timer_.reset();
			sprintf(rate_, "%.1f/sec", 0.0);
			start();
		}

		ProgBar::~ProgBar()
		{
			stop();
		}

		void ProgBar::step(unsigned size)
		{
			pos_ += size;
			if (pos_ >= range_)
			{
				stop();
			}
			calc_rate(size);
			timer_.reset();
		}

		void ProgBar::calc_rate(unsigned size)
		{
			double interval = timer_.elapsed_sec_double();
			if (interval < 1e-12) interval = 1e-12;
			double rate = size / interval;
			if (rate > 1073741824)
			{
				sprintf(rate_, "%.1e/s", rate);
			}
			else if (rate > 1048576)
			{
				sprintf(rate_, "%.1fM/sec", rate / 1048576);
			}
			else if (rate > 1024)
			{
				sprintf(rate_, "%.1fK/sec", rate / 1024);
			}
			else if (rate > 0.1)
			{
				sprintf(rate_, "%.1f/sec", rate);
			}
			else if (rate * 60 > 1)
			{
				sprintf(rate_, "%.1f/min", rate * 60);
			}
			else if (rate * 3600 > 1)
			{
				sprintf(rate_, "%.1f/sec", rate * 3600);
			}
			else
			{
				sprintf(rate_, "%.1e/s", rate);
			}
		}

		void ProgBar::start()
		{
			if (os::is_atty())
			{
				oldCout_ = std::cout.rdbuf(buffer_.rdbuf());
				oldCerr_ = std::cerr.rdbuf(buffer_.rdbuf());
				ss_.rdbuf(oldCout_);
				//ss_ << std::endl;
				running_ = true;
				worker_ = std::thread([this]{ this->bg_work(); });
			}
		}

		void ProgBar::stop()
		{
			if (running_)
			{
				running_ = false;
				worker_.join();	// join worker thread
				draw();
				std::cout.rdbuf(oldCout_);
				std::cerr.rdbuf(oldCerr_);
				std::cout << std::endl;
			}
		}

		void ProgBar::draw()
		{
			Size sz = os::console_size();
			// clear line
			ss_ << "\r";
			for (int i = 1; i < sz.width; ++i)
			{
				ss_ << " ";
			}
			ss_ << "\r";
			// flush stored messages
			std::string buf = buffer_.str();
			if (!buf.empty())
			{
				ss_ << buf;
				if (buf.back() != '\n')
				{
					ss_ << "\n";
				}
				try
				{
					buffer_.str(std::string());
				}
				catch (...)
				{
					// suppress the exception as a hack
				}
			}

			const int reserved = 21;	// leave some space for info
			int available = sz.width - reserved - static_cast<int>(info_.size()) - 2;
			if (available > 10)
			{
				int cnt = 0;
				int len = static_cast<int>(pos_ / static_cast<double>(range_)* available) - 1;
				ss_ << info_ << "[";
				for (int i = 0; i < len; ++i)
				{
					ss_ << "=";
					++cnt;
				}
				if (len >= 0)
				{
					ss_ << ">";
					++cnt;
				}
				for (int i = cnt; i < available; ++i)
				{
					ss_ << " ";
				}
				ss_ << "] ";
				float perc = 100.f * pos_ / range_;
				ss_ << std::fixed << std::setprecision(1) << perc;
				ss_ << "% (";
				ss_ << std::string(rate_);
				ss_ << ")";
				ss_.flush();
			}

		}

		void ProgBar::bg_work()
		{
			while (running_)
			{
				draw();
				time::sleep(66);
			}
		}

		namespace detail
		{
			LoggerRegistry& LoggerRegistry::instance()
			{
				static LoggerRegistry sInstance;
				return sInstance;
			}

			LoggerPtr LoggerRegistry::create(const std::string &name)
			{
				auto ptr = new_registry(name);
				if (!ptr)
				{
					throw RuntimeException("Logger with name: " + name + " already existed.");
				}
				return ptr;
			}

			LoggerPtr LoggerRegistry::ensure_get(std::string &name)
			{
				LoggerPtr ptr;
				if (loggers_.get(name, ptr))
				{
					return ptr;
				}

				do
				{
					ptr = new_registry(name);
				} while (ptr == nullptr);

				return ptr;
			}

			LoggerPtr LoggerRegistry::get(std::string &name)
			{
				LoggerPtr ptr;
				if (loggers_.get(name, ptr)) return ptr;
				return nullptr;
			}

			std::vector<LoggerPtr> LoggerRegistry::get_all()
			{
				std::vector<LoggerPtr> list;
				auto loggers = loggers_.snapshot();
				for (auto logger : loggers)
				{
					list.push_back(logger.second);
				}
				return list;
			}

			void LoggerRegistry::drop(const std::string &name)
			{
				loggers_.erase(name);
			}

			void LoggerRegistry::drop_all()
			{
				loggers_.clear();
			}

			void LoggerRegistry::lock()
			{
				lock_ = true;
			}

			void LoggerRegistry::unlock()
			{
				lock_ = false;
			}

			bool LoggerRegistry::is_locked() const
			{
				return lock_;
			}

			LoggerPtr LoggerRegistry::new_registry(const std::string &name)
			{
				LoggerPtr newLogger = std::make_shared<Logger>(name);
				auto defaultSinkList = LogConfig::instance().sink_list();
				for (auto sinkname : defaultSinkList)
				{
					SinkPtr psink = nullptr;
					if (sinkname == consts::kStdoutSinkName)
					{
						psink = new_stdout_sink();
					}
					else if (sinkname == consts::kStderrSinkName)
					{
						psink = new_stderr_sink();
					}
					else
					{
						psink = get_sink(sinkname);
					}
					if (psink) newLogger->attach_sink(psink);
				}
				if (loggers_.insert(name, newLogger))
				{
					return newLogger;
				}
				return nullptr;
			}

			RotateFileSink::RotateFileSink(const std::string filename, std::size_t maxSizeInByte, bool backup)
				:maxSizeInByte_(maxSizeInByte), backup_(backup)
			{
				if (backup_)
				{
					back_up(filename);
				}
				fileEditor_.open(filename, true);
				currentSize_ = 0;
			}

			void RotateFileSink::back_up(std::string oldFile)
			{
				std::string backupName = os::path_append_basename(oldFile,
					time::DateTime::local_time().to_string("_%y-%m-%d_%H-%M-%S-%frac"));
				os::rename(oldFile, backupName);
			}

			void RotateFileSink::rotate()
			{
				std::lock_guard<std::mutex> lock(mutex_);
				// check again in case other thread 
				// just waited for this operation
				if (currentSize_ > maxSizeInByte_)
				{
					if (backup_)
					{
						fileEditor_.close();
						back_up(fileEditor_.filename());
						fileEditor_.open(true);
					}
					else
					{
						fileEditor_.reopen(true);
					}
					currentSize_ = 0;
				}
			}

			void sink_list_revise(std::vector<std::string> &list, std::map<std::string, std::string> &map)
			{
				for (auto m : map)
				{
					for (auto l = list.begin(); l != list.end(); ++l)
					{
						if (m.first == *l)
						{
							l = list.erase(l);
							l = list.insert(l, m.second);
						}
					}
				}
			}

			void config_loggers_from_section(cfg::CfgLevel::section_map_t &section, std::map<std::string, std::string> &map)
			{
				for (auto loggerSec : section)
				{
					for (auto value : loggerSec.second.values)
					{
						LoggerPtr logger = nullptr;
						if (consts::kConfigLevelsSpecifier == value.first)
						{
							int mask = level_mask_from_string(value.second.str());
							if (!logger) logger = get_logger(loggerSec.first, true);
							logger->set_level_mask(mask);
						}
						else if (consts::kConfigSinkListSpecifier == value.first)
						{
							auto list = fmt::split_whitespace(value.second.str());
							if (list.empty()) continue;
							sink_list_revise(list, map);
							if (!logger) logger = get_logger(loggerSec.first, true);
							logger->detach_all_sinks();
							logger->attach_sink_list(list);
						}
						else
						{
							zupply_internal_warn("Unrecognized configuration key: " + value.first);
						}
					}
				}
			}

			LoggerPtr get_hidden_logger()
			{
				auto hlogger = get_logger("hidden", false);
				if (!hlogger)
				{
					hlogger = get_logger("hidden", true);
					hlogger->set_level_mask(0);
				}
				return hlogger;
			}

			std::map<std::string, std::string> config_sinks_from_section(cfg::CfgLevel::section_map_t &section)
			{
				std::map<std::string, std::string> sinkMap;
				for (auto sinkSec : section)
				{
					std::string type;
					std::string filename;
					std::string fmt;
					std::string levelStr;
					SinkPtr sink = nullptr;

					for (auto value : sinkSec.second.values)
					{
						// entries
						if (consts::kConfigSinkTypeSpecifier == value.first)
						{
							type = value.second.str();
						}
						else if (consts::kConfigSinkFilenameSpecifier == value.first)
						{
							filename = value.second.str();
						}
						else if (consts::kConfigFormatSpecifier == value.first)
						{
							fmt = value.second.str();
						}
						else if (consts::kConfigLevelsSpecifier == value.first)
						{
							levelStr = value.second.str();
						}
						else
						{
							zupply_internal_warn("Unrecognized config key entry: " + value.first);
						}
					}

					// sink
					if (type.empty()) throw RuntimeException("No suitable type specified for sink: " + sinkSec.first);
					if (type == consts::kStdoutSinkName)
					{
						sink = new_stdout_sink();
					}
					else if (type == consts::kStderrSinkName)
					{
						sink = new_stderr_sink();
					}
					else
					{
						if (filename.empty()) throw RuntimeException("No name specified for sink: " + sinkSec.first);
						if (type == consts::kOstreamSinkType)
						{
							zupply_internal_warn("Currently do not support init ostream logger from config file.");
						}
						else if (type == consts::kSimplefileSinkType)
						{
							sink = new_simple_file_sink(filename);
							get_hidden_logger()->attach_sink(sink);
						}
						else if (type == consts::kRotatefileSinkType)
						{
							sink = new_rotate_file_sink(filename);
							get_hidden_logger()->attach_sink(sink);
						}
						else
						{
							zupply_internal_warn("Unrecognized sink type: " + type);
						}
					}
					if (sink)
					{
						if (!fmt.empty()) sink->set_format(fmt);
						if (!levelStr.empty())
						{
							int mask = level_mask_from_string(levelStr);
							sink->set_level_mask(mask);
						}
						if (!get_hidden_logger()->get_sink(sink->name()))
						{
							get_hidden_logger()->attach_sink(sink);
						}
						sinkMap[sinkSec.first] = sink->name();
					}
				}
				return sinkMap;
			}

			void zupply_internal_warn(std::string msg)
			{
				auto zlog = get_logger(consts::kZupplyInternalLoggerName, true);
				zlog->warn() << msg;
			}

			void zupply_internal_error(std::string msg)
			{
				auto zlog = get_logger(consts::kZupplyInternalLoggerName, true);
				zlog->error() << msg;
			}
		} // namespace log::detail

		LogConfig::LogConfig()
		{
			// Default configurations
			sinkList_.set({ std::string(consts::kStdoutSinkName), std::string(consts::kStderrSinkName) });	//!< attach console by default
#ifdef NDEBUG
			logLevelMask_ = 0x3C;	//!< 0x3C->b111100: no debug, no trace
#else
			logLevelMask_ = 0x3E;	//!< 0x3E->b111110: debug, no trace
#endif
			format_.set(std::string(consts::kDefaultLoggerFormat));
			datetimeFormat_.set(std::string(consts::kDefaultLoggerDatetimeFormat));
		}

		LogConfig& LogConfig::instance()
		{
			static LogConfig instance_;
			return instance_;
		}

		void LogConfig::set_default_format(std::string format)
		{
			LogConfig::instance().set_format(format);
		}

		void LogConfig::set_default_datetime_format(std::string dateFormat)
		{
			LogConfig::instance().set_datetime_format(dateFormat);
		}

		void LogConfig::set_default_sink_list(std::vector<std::string> list)
		{
			LogConfig::instance().set_sink_list(list);
		}

		void LogConfig::set_default_level_mask(int levelMask)
		{
			LogConfig::instance().set_log_level_mask(levelMask);
		}

		std::vector<std::string> LogConfig::sink_list()
		{
			return sinkList_.get();
		}

		void LogConfig::set_sink_list(std::vector<std::string> &list)
		{
			sinkList_.set(list);
		}

		int LogConfig::log_level_mask()
		{
			return logLevelMask_;
		}

		void LogConfig::set_log_level_mask(int newMask)
		{
			logLevelMask_ = newMask;
		}

		std::string LogConfig::format()
		{
			return format_.get();
		}

		void LogConfig::set_format(std::string newFormat)
		{
			format_.set(newFormat);
		}

		std::string LogConfig::datetime_format()
		{
			return datetimeFormat_.get();
		}

		void LogConfig::set_datetime_format(std::string newDatetimeFormat)
		{
			datetimeFormat_.set(newDatetimeFormat);
		}

		// logger.info() << ".." call  style
		detail::LineLogger Logger::trace()
		{
			return log_if_enabled(LogLevels::trace);
		}
		detail::LineLogger Logger::debug()
		{
			return log_if_enabled(LogLevels::debug);
		}
		detail::LineLogger Logger::info()
		{
			return log_if_enabled(LogLevels::info);
		}
		detail::LineLogger Logger::warn()
		{
			return log_if_enabled(LogLevels::warn);
		}
		detail::LineLogger Logger::error()
		{
			return log_if_enabled(LogLevels::error);
		}
		detail::LineLogger Logger::fatal()
		{
			return log_if_enabled(LogLevels::fatal);
		}

		SinkPtr Logger::get_sink(std::string name)
		{
			SinkPtr ptr;
			if (sinks_.get(name, ptr))
			{
				return ptr;
			}
			return nullptr;
		}

		void Logger::attach_sink(SinkPtr sink)
		{
			if (!sinks_.insert(sink->name(), sink))
			{
				throw RuntimeException("Sink with name: " + sink->name() + " already attached to logger: " + name_);
			}
		}

		void Logger::detach_sink(SinkPtr sink)
		{
			sinks_.erase(sink->name());
		}

		void Logger::detach_all_sinks()
		{
			sinks_.clear();
		}

		void Logger::log_msg(detail::LogMessage msg)
		{
			auto sinks = sinks_.snapshot();
			for (auto s : sinks)
			{
				s.second->log(msg);
			}
		}

		std::string Logger::to_string()
		{
			std::string str(name() + ": " + level_mask_to_string(levelMask_));
			str += "\n{\n";
			auto sinkmap = sinks_.snapshot();
			for (auto sink : sinkmap)
			{
				str += sink.second->to_string() + "\n";
			}
			str += "}";
			return str;
		}

		std::string level_mask_to_string(int levelMask)
		{
			std::string str("<|");
			for (int i = 0; i < LogLevels::off; ++i)
			{
				if (level_should_log(levelMask, static_cast<LogLevels>(i)))
				{
					str += consts::kLevelNames[i];
					str += "|";
				}
			}
			return str + ">";
		}

		LogLevels level_from_str(std::string level)
		{
			std::string upperLevel = fmt::to_upper_ascii(level);
			for (int i = 0; i < LogLevels::off; ++i)
			{
				if (upperLevel == consts::kLevelNames[i])
				{
					return static_cast<LogLevels>(i);
				}
			}
			return LogLevels::off;
		}

		int level_mask_from_string(std::string levels)
		{
			int mask = 0;
			auto levelList = fmt::split_whitespace(levels);
			for (auto lvl : levelList)
			{
				auto l = level_from_str(lvl);
				mask |= 1 << static_cast<int>(l);
			}
			return mask & LogLevels::sentinel;
		}

		LoggerPtr get_logger(std::string name, bool createIfNotExists)
		{
			if (createIfNotExists)
			{
				return detail::LoggerRegistry::instance().ensure_get(name);
			}
			else
			{
				return detail::LoggerRegistry::instance().get(name);
			}
		}

		SinkPtr new_stdout_sink()
		{
			return detail::StdoutSink::instance();
		}

		SinkPtr new_stderr_sink()
		{
			return detail::StderrSink::instance();
		}

		void Logger::attach_console()
		{
			sinks_.insert(std::string(consts::kStdoutSinkName), new_stdout_sink());
			sinks_.insert(std::string(consts::kStderrSinkName), new_stderr_sink());
		}

		void Logger::detach_console()
		{
			sinks_.erase(std::string(consts::kStdoutSinkName));
			sinks_.erase(std::string(consts::kStderrSinkName));
		}

		SinkPtr get_sink(std::string name)
		{
			SinkPtr psink = nullptr;
			auto loggers = detail::LoggerRegistry::instance().get_all();
			for (auto logger : loggers)
			{
				psink = logger->get_sink(name);
				if (psink)
				{
					return psink;
				}
			}
			return nullptr;
		}

		void dump_loggers(std::ostream &out)
		{
			auto loggers = detail::LoggerRegistry::instance().get_all();
			out << "{\n";
			for (auto logger : loggers)
			{
				out << logger->to_string() << "\n";
			}
			out << "}" << std::endl;
		}

		SinkPtr new_ostream_sink(std::ostream &stream, std::string name, bool forceFlush)
		{
			auto sinkptr = get_sink(name);
			if (sinkptr)
			{
				throw RuntimeException(name + " already holded by another sink\n" + sinkptr->to_string());
			}
			return std::make_shared<detail::OStreamSink>(stream, name.c_str(), forceFlush);
		}

		SinkPtr new_simple_file_sink(std::string filename, bool truncate)
		{
			auto sinkptr = get_sink(os::absolute_path(filename));
			if (sinkptr)
			{
				throw RuntimeException("File: " + filename + " already holded by another sink!\n" + sinkptr->to_string());
			}
			return std::make_shared<detail::SimpleFileSink>(filename, truncate);
		}

		SinkPtr new_rotate_file_sink(std::string filename, std::size_t maxSizeInByte, bool backupOld)
		{
			auto sinkptr = get_sink(os::absolute_path(filename));
			if (sinkptr)
			{
				throw RuntimeException("File: " + filename + " already holded by another sink!\n" + sinkptr->to_string());
			}
			return std::make_shared<detail::RotateFileSink>(filename, maxSizeInByte, backupOld);
		}

		void Logger::attach_sink_list(std::vector<std::string> &sinkList)
		{
			for (auto sinkname : sinkList)
			{
				SinkPtr psink = nullptr;
				if (sinkname == consts::kStdoutSinkName)
				{
					psink = new_stdout_sink();
				}
				else if (sinkname == consts::kStderrSinkName)
				{
					psink = new_stderr_sink();
				}
				else
				{
					psink = log::get_sink(sinkname);
				}
				if (psink && (!this->get_sink(psink->name()))) attach_sink(psink);
			}
		}

		void lock_loggers()
		{
			detail::LoggerRegistry::instance().lock();
		}

		void unlock_loggers()
		{
			detail::LoggerRegistry::instance().unlock();
		}

		void drop_logger(std::string name)
		{
			detail::LoggerRegistry::instance().drop(name);
		}

		void drop_all_loggers()
		{
			detail::LoggerRegistry::instance().drop_all();
		}

		void drop_sink(std::string name)
		{
			SinkPtr psink = get_sink(name);
			if (nullptr == psink) return;
			auto loggers = detail::LoggerRegistry::instance().get_all();
			for (auto logger : loggers)
			{
				logger->detach_sink(psink);
			}
		}

		void detail::config_from_parser(cfg::CfgParser& parser)
		{
			// config for specific sinks
			auto sinkSection = parser(consts::KConfigSinkSectionSpecifier).sections;
			auto sinkMap = detail::config_sinks_from_section(sinkSection);

			// global format
			std::string format = parser(consts::KConfigGlobalSectionSpecifier)[consts::kConfigFormatSpecifier].str();
			if (!format.empty()) LogConfig::set_default_format(format);
			// global datetime format
			std::string datefmt = parser(consts::KConfigGlobalSectionSpecifier)[consts::kConfigDateTimeFormatSpecifier].str();
			if (!datefmt.empty()) LogConfig::set_default_datetime_format(datefmt);
			// global log levels
			auto v = parser(consts::KConfigGlobalSectionSpecifier)[consts::kConfigLevelsSpecifier];
			if (!v.str().empty()) LogConfig::set_default_level_mask(level_mask_from_string(v.str()));
			// global sink list
			v = parser(consts::KConfigGlobalSectionSpecifier)[consts::kConfigSinkListSpecifier];
			if (!v.str().empty())
			{
				auto list = fmt::split_whitespace(v.str());
				detail::sink_list_revise(list, sinkMap);
				LogConfig::set_default_sink_list(list);
			}

			// config for specific loggers
			auto loggerSection = parser(consts::KConfigLoggerSectionSpecifier).sections;
			detail::config_loggers_from_section(loggerSection, sinkMap);
		}

		void config_from_file(std::string cfgFilename)
		{
			cfg::CfgParser parser(cfgFilename);
			detail::config_from_parser(parser);
		}

		void config_from_stringstream(std::stringstream& ss)
		{
			cfg::CfgParser parser(ss);
			detail::config_from_parser(parser);
		}

	} // namespace log

	Image::Image(const char* filename)
	{
		load(filename);
	}

	void Image::load(const char* filename)
	{
		int x;
		int y;
		int comp;
		Image::value_type *buffer = nullptr;
		buffer = thirdparty::stbi::decode::stbi_load(filename, &x, &y, &comp, 0);
		if (!buffer)
		{
			std::string msg = "Failed to load from " + std::string(filename) + ": ";
			msg += thirdparty::stbi::decode::stbi_failure_reason();
			throw RuntimeException(msg);
		};
		import(buffer, y, x, comp);
		thirdparty::stbi::decode::stbi_image_free(buffer);
	}

	void Image::save(const char* filename, int quality) const
	{
		std::string ext = fmt::to_lower_ascii(os::path_split_extension(filename));
		if (ext == "jpg" || ext == "jpeg")
		{
			thirdparty::jo::jo_write_jpg(filename, (*data_).data(), cols_, rows_, channels_, quality);
		}
		else if (ext == "png")
		{
			thirdparty::stbi::encode::stbi_write_png(filename, cols_, rows_, channels_, (*data_).data(), cols_ * channels_);
		}
		else if (ext == "bmp")
		{
			thirdparty::stbi::encode::stbi_write_bmp(filename, cols_, rows_, channels_, (*data_).data());
		}
		else if (ext == "tga")
		{
			thirdparty::stbi::encode::stbi_write_tga(filename, cols_, rows_, channels_, (*data_).data());
		}
		else
		{
			throw ArgException("Not supported format: " + ext);
		}
	}

	void Image::resize(int width, int height)
	{
		assert(height > 0 && "height must > 0!");
		assert(width > 0 && "width must > 0!");
		range_check(0);
		detach();
		std::shared_ptr<std::vector<Image::value_type>> buf = std::make_shared<std::vector<Image::value_type>> (height * width * channels_);
		thirdparty::stbi::resize::stbir_resize_uint8(&(*data_).front(), cols_, rows_, 0, &(*buf).front(), width, height, 0, channels_);
		data_ = buf;
		rows_ = height;
		cols_ = width;
	}

	void Image::resize(double ratio)
	{
		assert(ratio > 0 && "resize ratio must > 0!");
		int width = static_cast<int>(cols_ * ratio);
		int height = static_cast<int>(rows_ * ratio);
		resize(height, width);
	}

	void Image::resize(Size sz)
	{
		resize(sz.height, sz.width);
	}

	ImageHdr::ImageHdr(const char* filename)
	{
		load(filename);
	}

	ImageHdr::ImageHdr(const Image& from, float range)
	{
		from_normal(from, range);
	}

	void ImageHdr::load(const char* filename)
	{
		int x;
		int y;
		int comp;
		ImageHdr::value_type *buffer = nullptr;
		buffer = thirdparty::stbi::decode::stbi_loadf(filename, &x, &y, &comp, 0);
		if (!buffer)
		{
			std::string msg = "Failed to load from " + std::string(filename) + ": ";
			msg += thirdparty::stbi::decode::stbi_failure_reason();
			throw RuntimeException(msg);
		};
		import(buffer, y, x, comp);
		thirdparty::stbi::decode::stbi_image_free(buffer);
	}

	void ImageHdr::save_hdr(const char* filename) const
	{
		thirdparty::stbi::encode::stbi_write_hdr(filename, cols_, rows_, channels_, (*data_).data());
	}

	Image ImageHdr::to_normal(float range) const
	{
		Image tmp(rows_, cols_, channels_);
		auto p = tmp.ptr();
		for (auto iter = (*data_).cbegin(); iter != (*data_).cend(); ++iter)
		{
			*p = saturate_cast<Image::value_type>(*iter / range * 255);
			++p;
		}
		return tmp;
	}

	void ImageHdr::from_normal(const Image& from, float range)
	{
		create(from.rows(), from.cols(), from.channels());
		auto p = from.ptr();
		for (auto iter = (*data_).begin(); iter != (*data_).end(); ++iter)
		{
			*iter = saturate_cast<value_type>(*p / 255.0f * range);
			++p;
		}
	}

	void ImageHdr::resize(int width, int height)
	{
		assert(height > 0 && "height must > 0!");
		assert(width > 0 && "width must > 0!");
		range_check(0);
		detach();
		std::shared_ptr<std::vector<ImageHdr::value_type>> buf = std::make_shared<std::vector<ImageHdr::value_type>>(height * width * channels_);
		thirdparty::stbi::resize::stbir_resize_float(&(*data_).front(), cols_, rows_, 0, &(*buf).front(), width, height, 0, channels_);
		data_ = buf;
		rows_ = height;
		cols_ = width;
	}

	void ImageHdr::resize(double ratio)
	{
		assert(ratio > 0 && "resize ratio must > 0!");
		int width = static_cast<int>(cols_ * ratio);
		int height = static_cast<int>(rows_ * ratio);
		resize(height, width);
	}

	void ImageHdr::resize(Size sz)
	{
		resize(sz.height, sz.width);
	}

} // end namesapce zz
