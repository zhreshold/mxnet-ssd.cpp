/* Doxygen main page */
/*! \mainpage Zupply
###   A light-weight portable C++ 11 library for researches and demos

*   Author: Joshua Zhang
*   Date since: June-2015
*
*   Copyright (c) <2015> <JOSHUA Z. ZHANG>
*
*   Open source according to MIT License.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
*   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
*   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
*   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*
*
#### Project on Github: [link](https://github.com/ZhreShold/zupply)
***************************************************************************/




#ifndef _ZUPPLY_ZUPPLY_HPP_
#define _ZUPPLY_ZUPPLY_HPP_

///////////////////////////////////////////////////////////////
// Require C++ 11 features
///////////////////////////////////////////////////////////////
#if ((defined(_MSC_VER) && _MSC_VER >= 1800) || __cplusplus >= 201103L)
// VC++ defined an older version of __cplusplus, but it should work later than vc12
#else
#error C++11 required, add -std=c++11 to CFLAG.
#endif

#ifdef _MSC_VER

// Disable silly security warnings
//#ifndef _CRT_SECURE_NO_WARNINGS
//#define _CRT_SECURE_NO_WARNINGS
//#endif

// Define __func__ macro
#ifndef __func__
#define __func__ __FUNCTION__
#endif 

// Define NDEBUG in Release Mode, ensure assert() disabled.
#if (!defined(_DEBUG) && !defined(NDEBUG))
#define NDEBUG
#endif

#if _MSC_VER < 1900
#define ZUPPLY_NOEXCEPT throw()
#endif

#endif

#ifndef ZUPPLY_NOEXCEPT
#define ZUPPLY_NOEXCEPT noexcept
#endif

// Optional header only mode, not done.
#ifdef ZUPPLY_HEADER_ONLY
#define ZUPPLY_EXPORT inline
#else
#define ZUPPLY_EXPORT
#endif

#include <string>
#include <exception>
#include <fstream>
#include <utility>
#include <iostream>
#include <vector>
#include <locale>
#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <unordered_map>
#include <memory>
#include <type_traits>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <climits>
#include <cassert>
#include <cstring>


/*!
 * \namespace zz
 * \brief Namespace for zupply
 */
namespace zz
{
	/*!
		* \namespace	zz::consts
		*
		* \brief	Namespace for parameters.
		*/
	namespace consts
	{
		static const char* kExceptionPrefixGeneral = "[Zupply Exception] ";
		static const char* kExceptionPrefixLogic = "[Zupply Exception->Logic] ";
		static const char* kExceptionPrefixArgument = "[Zupply Exception->Logic->Argument] ";
		static const char* kExceptionPrefixRuntime = "[Zupply Exception->Runtime] ";
		static const char* kExceptionPrefixIO = "[Zupply Exception->Runtime->IO] ";
		static const char* kExceptionPrefixCast = "[Zupply Exception->Runtime->Cast] ";
		static const char* kExceptionPrefixMemory = "[Zupply Exception->Runtime->Memory] ";
		static const char* kExceptionPrefixStrictWarn = "[Zupply Exception->StrictWarn] ";
	}

	/*!
	* \class	UnCopyable
	*
	* \brief	A not copyable base class, should be inheritated privately.
	*/
	class UnCopyable
	{
	public:
		UnCopyable() {};
		// no copy constructor
		UnCopyable(const UnCopyable&) = delete;
		// no assignment
		UnCopyable& operator=(const UnCopyable&) = delete;
	};

	/*!
	* \class	UnMovable
	*
	* \brief	A not movable/copyable base class, should be inheritated privately.
	*/
	class UnMovable
	{
	public:
		UnMovable() {};
		// no copy constructor
		UnMovable(const UnMovable&) = delete;
		// no copy operator
		UnMovable& operator=(const UnMovable&) = delete;
		// no move constructor
		UnMovable(UnMovable&&) = delete;
		// no move operator
		UnMovable& operator=(UnMovable&&) = delete;
	};

	/*!
	* \class	Exception
	*
	* \brief	An exception with customized prefix information.
	*/
	class Exception : public std::exception
	{
	public:
		explicit Exception(const char* message, const char* prefix = consts::kExceptionPrefixGeneral)
		{
			message_ = std::string(prefix) + message;
		};
		explicit Exception(const std::string message, const char* prefix = consts::kExceptionPrefixGeneral)
		{
			message_ = std::string(prefix) + message;
		};
		virtual ~Exception() ZUPPLY_NOEXCEPT{};

		const char* what() const ZUPPLY_NOEXCEPT{ return message_.c_str(); };
	private:
		std::string message_;
	};

	/*!
	* \class	LogicException
	*
	* \brief	Exception for signalling logic errors.
	*/
	class LogicException : public Exception
	{
	public:
		explicit LogicException(const char *message) : Exception(message, consts::kExceptionPrefixLogic){};
		explicit LogicException(const std::string &message) : Exception(message, consts::kExceptionPrefixLogic){};
	};

	/*!
	* \class	ArgException
	*
	* \brief	Exception for signalling argument errors.
	*/
	class ArgException : public Exception
	{
	public:
		explicit ArgException(const char *message) : Exception(message, consts::kExceptionPrefixArgument){};
		explicit ArgException(const std::string &message) : Exception(message, consts::kExceptionPrefixArgument){};
	};

	/*!
	* \class	RuntimeException
	*
	* \brief	Exception for signalling unexpected runtime errors.
	*/
	class RuntimeException : public Exception
	{
	public:
		explicit RuntimeException(const char *message) : Exception(message, consts::kExceptionPrefixRuntime){};
		explicit RuntimeException(const std::string &message) : Exception(message, consts::kExceptionPrefixRuntime){};
	};

	/*!
	* \class	CastException
	*
	* \brief	Exception for signalling unsuccessful cast operations.
	*/
	class CastException : public Exception
	{
	public:
		explicit CastException(const char *message) : Exception(message, consts::kExceptionPrefixCast){};
		explicit CastException(const std::string &message) : Exception(message, consts::kExceptionPrefixCast){};
	};

	/*!
	* \class	IOException
	*
	* \brief	Exception for signalling unexpected IO errors.
	*/
	class IOException : public Exception
	{
	public:
		explicit IOException(const char *message) : Exception(message, consts::kExceptionPrefixIO){};
		explicit IOException(const std::string &message) : Exception(message, consts::kExceptionPrefixIO){};
	};

	/*!
	* \class	MemException
	*
	* \brief	Exception for signalling memory errors.
	*/
	class MemException : public Exception
	{
	public:
		explicit MemException(const char *message) : Exception(message, consts::kExceptionPrefixMemory){};
		explicit MemException(const std::string &message) : Exception(message, consts::kExceptionPrefixMemory){};
	};

	/*!
	* \class	WarnException
	*
	* \brief	Exception for signalling warning errors when strict warning is enabled.
	*/
	class WarnException : public Exception
	{
	public:
		explicit WarnException(const char *message) : Exception(message, consts::kExceptionPrefixStrictWarn){};
		explicit WarnException(const std::string &message) : Exception(message, consts::kExceptionPrefixStrictWarn){};
	};

	/*!
	 * \brief The 2D size class
	 * The class represents the size of a 2D rectangle, image size, matrix size etc.
	 * Normally, Size ~ Size_<int> is used.
	 */
	template<typename _Tp> class Size_
	{
	public:
		typedef _Tp value_type;

		/*!
		 * \brief Default ctor
		 */
		Size_();

		/*!
		 * \brief Ctor with width and height
		 * \param width
		 * \param height
		 */
		Size_(_Tp width, _Tp height);

		/*!
		 * \brief Copy ctor
		 * \param sz
		 */
		Size_(const Size_& sz);

		/*!
		 * \brief Copy operator
		 * \param sz
		 */
		Size_& operator = (const Size_& sz);

		/*!
		 * \brief Compare operator
		 * \param other
		 * \return True if size identical
		 */
		bool operator==(const Size_& other);

		/*!
		 * \brief Compare operator
		 * \param other
		 * \return True if size NOT identical
		 */
		bool operator!=(const Size_& other);

		/*!
		 * \brief area Calculate area
		 * \return Area
		 */
		_Tp area() const;

		/*!
		 * \brief Covnert to another type
		 */
		template<typename _Tp2> operator Size_<_Tp2>() const;

		_Tp width, height; // the width and the height
	};
	/*!
	 * \brief Size2i Int version
	 */
	using Size2i = Size_<int>;

	/*!
	 * \brief Size2f Float version
	 */
	using Size2f = Size_<float>;

	/*!
	 * \brief Size2d Float version
	 */
	using Size2d = Size_<double>;

	/*!
	 * \brief Size By default using Size2i, int version
	 */
	using Size = Size_<int>;

	template<typename _Tp> class Rect_;
	/*! 
	 * \brief template 2D point class.
	 * The class defines a point in 2D space. Data type of the point coordinates is specified
	 * as a template parameter. There are a few shorter aliases available for user convenience.
	 * See zz::Point, zz::Point2i, zz::Point2f and zz::Point2d.
	*/
	template<typename _Tp> class Point_
	{
	public:
		typedef _Tp value_type;

		/*!
		 * \brief Default constructor
		 */
		Point_();

		/*!
		 * \brief Constructor with x and y coordinate
		 * \param _x
		 * \param _y
		 */
		Point_(_Tp _x, _Tp _y);

		/*!
		 * \brief Copy constructor
		 * \param pt
		 */
		Point_(const Point_& pt);

		/*!
		 * \brief Copy operator
		 * \param pt
		 * \return
		 */
		Point_& operator = (const Point_& pt);

		/*!
		 * \brief conversion to another data type
		 */
		template<typename _Tp2> operator Point_<_Tp2>() const;

		/*!
		 * \brief dot product
		 * \param pt
		 * \return
		 */
		_Tp dot(const Point_& pt) const;

		/*!
		 * \brief dot product computed in double-precision arithmetics
		 * \param pt
		 * \return
		 */
		double ddot(const Point_& pt) const;
		/*!
		 * \brief cross-product
		 * \param pt
		 * \return
		 */
		double cross(const Point_& pt) const;
		/*!
		 * \brief checks whether the point is inside the specified rectangle
		 * \param r
		 * \return
		 */
		bool inside(const Rect_<_Tp>& r) const;

		_Tp x, y; //!< the point coordinates
	};

	/*!
	 * \brief Point2i Int version
	 */
	typedef Point_<int> Point2i;

	/*!
	 * \brief Point2f Float version
	 */
	typedef Point_<float> Point2f;

	/*!
	 * \brief Point2d Double version
	 */
	typedef Point_<double> Point2d;

	/*!
	 * \brief Point By default using Point2i
	 */
	typedef Point2i Point;

	/*!
	 * \brief Vecpts 1-D vector of points
	 */
	typedef std::vector<Point> Vecpts;

	/*! 
	 * \brief The 2D up-right rectangle class
	 * The class represents a 2D rectangle with coordinates of the specified data type.
	 * Normally, zz::Rect ~ zz::Rect_<int> is used.
	*/
	template<typename _Tp> class Rect_
	{
	public:
		typedef _Tp value_type;

		/*!
		 * \brief Default constructor
		 */
		Rect_();

		/*!
		 * \brief Constructor with x, y, width and height
		 * \param _x
		 * \param _y
		 * \param _width
		 * \param _height
		 */
		Rect_(_Tp _x, _Tp _y, _Tp _width, _Tp _height);

		/*!
		 * \brief Copy constructor
		 * \param r
		 */
		Rect_(const Rect_& r);

		/*!
		 * \brief Constructor with top-left point and its size(width, height)
		 * \param org
		 * \param sz
		 */
		Rect_(const Point_<_Tp>& org, const Size_<_Tp>& sz);

		/*!
		 * \brief Constructor with top-left point and right-bottom point
		 * \param pt1
		 * \param pt2
		 */
		Rect_(const Point_<_Tp>& pt1, const Point_<_Tp>& pt2);

		/*!
		 * \brief Copy operator
		 * \param r
		 * \return
		 */
		Rect_& operator = (const Rect_& r);

		/*!
		 * \brief Get top left corner point
		 * \return Top-left corner
		 */
		Point_<_Tp> tl() const;

		/*!
		 * \brief Get bottom right corner point
		 * \return Bottom-right corner
		 */
		Point_<_Tp> br() const;

		/*!
		 * \brief Get size of rectangle
		 * \return Size(width, height) of the rectangle
		 */
		Size_<_Tp> size() const;

		/*!
		 * \brief Get area of rectagle
		 * \return Area
		 */
		_Tp area() const;

		/*!
		 * \brief Convert to another type
		 */
		template<typename _Tp2> operator Rect_<_Tp2>() const;

		/*!
		 * \brief Checks whether the rectangle contains the point
		 * \param pt
		 * \return True if contains, false otherwise
		 */
		bool contains(const Point_<_Tp>& pt) const;

		_Tp x, y, width, height; //!< the top-left corner, as well as width and height of the rectangle
	};

	/*!
	 * \brief Rect2i Int version
	 */
	typedef Rect_<int> Rect2i;

	/*!
	 * \brief Rect2f Float version
	 */
	typedef Rect_<float> Rect2f;

	/*!
	 * \brief Rect2d Double version
	 */
	typedef Rect_<double> Rect2d;

	/*!
	 * \brief Rect By default use Rect2i, int version
	 */
	typedef Rect2i Rect;

	namespace detail
	{
		/*!
		 * \brief Base image storage class
		 * This defines the storage and pixel-wise access to a image like 3-D matrix
		 */
		template<typename _Tp> class ImageBase
		{
		public:
			typedef _Tp value_type;

			/*!
			 * \brief ImageBase Default(empty) constructor
			 */
			ImageBase();

			/*!
			 * \brief ImageBase Constructor with size info
			 * \param rows
			 * \param cols
			 * \param channels
			 */
			ImageBase(int rows, int cols, int channels);

			/*!
			 * \brief ImageBase Copy constructor(shallow copy)
			 * \note ImageBase use Copy-on-write technique, so you don't need to worry about the deep/shallow copy issue.
			 * \param other
			 */
			ImageBase(const ImageBase& other);

			/*!
			 * \brief ImageBase Move constructor
			 * \param other
			 */
			ImageBase(ImageBase&& other);


			virtual ~ImageBase();

			/*!
			 * \brief create Create storage with specified size
			 * \param rows
			 * \param cols
			 * \param channels
			 */
			void create(int rows, int cols, int channels);

			/*!
			 * \brief release Destroy memory storage
			 */
			void release();

			/*!
			 * \brief operator = Copy operator, again, shallow copy
			 * \param other
			 * \return
			 */
			ImageBase& operator= (const ImageBase& other);

			/*!
			 * \brief operator = Move operator
			 * \param other
			 * \return
			 */
			ImageBase& operator= (ImageBase&& other);

			/*!
			 * \brief operator () Access pixel element
			 * \param row
			 * \param col
			 * \param channel
			 * \return
			 */
			_Tp& operator() (int row, int col, int channel = 0);

			/*!
			 * \brief operator () Access pixel element, immutable version
			 * \param row
			 * \param col
			 * \param channel
			 * \return
			 */
			const _Tp& operator() (int row, int col, int channel = 0) const;

			/*!
			 * Conver to another data type, using saturate_cast<>
			 */
			template <typename _Tp2> operator ImageBase<_Tp2>() const;

			/*!
			 * \brief empty Check empty or not
			 * \return True if empty
			 */
			bool empty() const;

			/*!
			 * \brief rows Get number of rows(height, y...)
			 * \return Number of rows(height, y...)
			 */
			int rows() const;

			/*!
			 * \brief cols Get number of columns(width, x...)
			 * \return Number of columns(width, x...)
			 */
			int cols() const;

			/*!
			 * \brief channels Get number of channels
			 * \return Number of channels
			 */
			int channels() const;

			/*!
			 * \brief at Access pixel, immutable version.
			 * This is guanranteed to be faster than () operator if you don't need to modify data.
			 * \param row
			 * \param col
			 * \param channel
			 * \return Pixel value
			 */
			_Tp at(int row, int col, int channel = 0) const;

			/*!
			 * \brief ptr Data pointer given specifed position.
			 * Use with cautious. This is provided for performance consideration.
			 * Will not trigger copy-on-write method, so if you change the data, the previous copied images will be affected.
			 * \param offset
			 * \return Raw pointer to specific data point
			 */
			_Tp* ptr(int offset = 0) const;

			/*!
			 * \brief ptr Data pointer given specifed position.
			 * Use with cautious. This is provided for performance consideration.
			 * Will not trigger copy-on-write method, so if you change the data, the previous copied images will be affected.
			 * \param row
			 * \param col
			 * \param channel
			 * \return
			 */
			_Tp* ptr(int row, int col, int channel = 0) const;

			/*!
			 * \brief import Import data from raw pointer array.
			 * Please make sure the length of array satisfies the size provided.
			 * \param data
			 * \param rows
			 * \param cols
			 * \param channels
			 */
			void import(_Tp* data, int rows, int cols, int channels);

			/*!
			 * \brief import Import data from vector.
			 * Please make sure the length of vector satisfies the size provided.
			 * \param data
			 * \param rows
			 * \param cols
			 * \param channels
			 */
			void import(std::vector<_Tp> data, int rows, int cols, int channels);

			/*!
			 * \brief export_raw Export data to vector in original order(interleaved, rgb order).
			 * \example Data output will be r1g1b1r2g2b2r3g3b3...
			 * \return Vector of data
			 */
			std::vector<_Tp> export_raw() const;

			/*!
			 * \brief export data to provided vector container.
			 */
			template <typename _Tp2> std::vector<_Tp2>& export_raw(std::vector<_Tp2>& out) const;

			/*!
			 * \brief crop Crop image given coordinates.
			 * \param r0
			 * \param c0
			 * \param r1
			 * \param c1
			 */
			void crop(int r0, int c0, int r1, int c1);

			/*!
			 * \brief crop Crop image given two points
			 * \param p0
			 * \param p1
			 */
			void crop(Point p0, Point p1);

			/*!
			 * \brief crop Crop image given a rectangle area.
			 * \param rect
			 */
			void crop(Rect rect);

		protected:
			void range_check(long long pos) const;
			void range_check(int row, int col, int channel) const;
			void detach();

			int rows_;
			int cols_;
			int channels_;
			std::shared_ptr<std::vector<_Tp>> data_;

		};
	} // namespace zz::detail

	/*!
	 * \brief The Image class.
	 * Image container for 8-bit image manipulation including read/write.
	 * Image is based on detail::ImageBase.
	 */
	class Image : public detail::ImageBase<unsigned char>
	{
	public:
		/*!
		 * \brief Image Default(empty) constructor
		 */
		Image() : ImageBase() {};

		/*!
		 * \brief Image Constructor with specified size
		 * \param rows
		 * \param cols
		 * \param channels
		 */
		Image(int rows, int cols, int channels) : ImageBase(rows, cols, channels) {};

		/*!
		 * \brief Image Constructor from disk image file.
		 * \param filename
		 */
		Image(const char* filename);

		/*!
		 * \brief load Load image from file.
		 * \param filename
		 */
		void load(const char* filename);

		/*!
		 * \brief save Save image to file.
		 * \param filename
		 * \param quality Save quality(0-100), only applied to JPEG image format.
		 */
		void save(const char* filename, int quality = 80) const;

		/*!
		* \brief resize Resize image given new size
		* \param sz
		*/
		void resize(Size sz);

		/*!
		* \brief resize Resize image given new height and width
		* \param height
		* \param width
		*/
		void resize(int width, int height);

		/*!
		* \brief resize Resize image given ratio to the old size
		* \param ratio
		*/
		void resize(double ratio);
	};

	/*!
	 * \brief The ImageHdr class.
	 * Image container specifically good for HDR images which uses 32bit float precison.
	 */
	class ImageHdr : public detail::ImageBase<float>
	{
	public:
		/*!
		 * \brief ImageHdr Default(empty) constructor
		 */
		ImageHdr() : ImageBase() {};

		/*!
		 * \brief ImageHdr Constructor given specific size
		 * \param rows
		 * \param cols
		 * \param channels
		 */
		ImageHdr(int rows, int cols, int channels) : ImageBase(rows, cols, channels) {};

		/*!
		 * \brief ImageHdr Constructor from disk image file
		 * \param filename
		 */
		ImageHdr(const char* filename);

		/*!
		 * \brief ImageHdr Constructor from 8-bit image
		 * \param from 8-bit image
		 * \param range The range of data stored, normally 1.0 is used. In this case, [0-255] data will be normalized to [0.0-1.0]
		 */
		ImageHdr(const Image& from, float range = 1.0f);

		/*!
		 * \brief load Load image from disk file, HDR image(*.hdr) supported.
		 * \param filename
		 */
		void load(const char* filename);

		/*!
		 * \brief save_hdr Save to HDR image
		 * \param filename
		 */
		void save_hdr(const char* filename) const;

		/*!
		 * \brief to_normal Convert to 8-bit image(lose precision)
		 * \param range The range of stored data, normally 1.0 is used.
		 * \return An 8-bit image
		 */
		Image to_normal(float range = 1.0f) const;

		/*!
		 * \brief from_normal Convert from an 8-bit image.
		 * \param from
		 * \param range The range of new stored data, normally 1.0 is used.
		 */
		void from_normal(const Image& from, float range = 1.0f);
		
		/*!
		* \brief resize Resize image given new size
		* \param sz
		*/
		void resize(Size sz);

		/*!
		* \brief resize Resize image given new height and width
		* \param height
		* \param width
		*/
		void resize(int width, int height);

		/*!
		* \brief resize Resize image given ratio to the old size
		* \param ratio
		*/
		void resize(double ratio);
	};


	/*!
	 * \namespace  zz::math
	 * \brief Namespace for math operations
	 */
	namespace math
	{
		using std::min;
		using std::max;
		using std::abs;

		/*!
		 * \fn template<class T> inline const T clip(const T& value, const T& low, const T& high)
		 * \brief Clip values in-between low and high values
		 * \param value The value to be clipped
		 * \param low The lower bound
		 * \param high The higher bound
		 * \return Value if value in[low, high] or low if (value < low) or high if (value > high)
		 * \note Foolproof if low/high are not set correctly
		 */
		template<class T> inline const T clip(const T& value, const T& low, const T& high)
		{
			// fool proof max/min values
			T h = (std::max)(low, high);
			T l = (std::min)(low, high);
			return (std::max)((std::min)(value, h), l);
		}

		/*!
		 * \brief Template meta programming for pow(a, b) where a, b must be natural numbers
		 * Use Pow<a, b>::result = a^b, which is computed in compilation rather than runtime.
		 */
		template <unsigned long B, unsigned long E>
		struct Pow
		{
			static const unsigned long result = B * Pow<B, E - 1>::result;
		};

		template <unsigned long B>
		struct Pow<B, 0>
		{
			static const unsigned long result = 1;
		};


		/*!
		 * \fn int round(double value)
		 * \brief fast round utilizing hardware acceleration features
		 * \param value
		 * \return Rounded int
		 */
		inline int round(double value)
		{
#if ((defined _MSC_VER && defined _M_X64) || (defined __GNUC__ && defined __x86_64__ && defined __SSE2__ && !defined __APPLE__)) && !defined(__CUDACC__) && 0
			__m128d t = _mm_set_sd(value);
			return _mm_cvtsd_si32(t);
#elif defined _MSC_VER && defined _M_IX86
			int t;
			__asm
			{
				fld value;
				fistp t;
			}
			return t;
#elif defined _MSC_VER && defined _M_ARM && defined HAVE_TEGRA_OPTIMIZATION
			TEGRA_ROUND(value);
#elif defined CV_ICC || defined __GNUC__
#  ifdef HAVE_TEGRA_OPTIMIZATION
			TEGRA_ROUND(value);
#  else
			return (int)lrint(value);
#  endif
#else
			double intpart, fractpart;
			fractpart = modf(value, &intpart);
			if ((fabs(fractpart) != 0.5) || ((((int)intpart) % 2) != 0))
				return (int)(value + (value >= 0 ? 0.5 : -0.5));
			else
				return (int)intpart;
#endif
		}

	} // namespace math

	/*!
	 * \namespace zz::misc
	 * \brief Namespace for miscellaneous utility functions
	 */
	namespace misc
	{
		/*!
		 * \fn inline void unused(const T&)
		 * \brief Suppress warning for unused variables, do nothing actually
		 */
		template<typename T>
		inline void unused(const T&) {}

		namespace detail
		{
			// To allow ADL with custom begin/end
			using std::begin;
			using std::end;

			template <typename T>
			auto is_iterable_impl(int)
				-> decltype (
				begin(std::declval<T&>()) != end(std::declval<T&>()), // begin/end and operator !=
				++std::declval<decltype(begin(std::declval<T&>()))&>(), // operator ++
				*begin(std::declval<T&>()), // operator*
				std::true_type{});

			template <typename T>
			std::false_type is_iterable_impl(...);

		}

		template <typename T>
		using is_iterable = decltype(detail::is_iterable_impl<T>(0));

		/*!
		 * \brief The general functor Callback class
		 */
		class Callback
		{
		public:
			Callback(std::function<void()> f) : f_(f) { f_(); }
		private:
			std::function<void()> f_;
		};
	} // namespace misc

	/*!
	 * \namespace zz::cds
	 * \brief Namespace for concurrent data structures
	 */
	namespace cds
	{
		inline void nop_pause()
		{
#if (defined _MSC_VER) && (defined _M_IX86 || defined _M_X64) && 0
			__asm volatile("pause" ::: "memory");
#endif
		}

		/*!
		* \class	NullMutex
		*
		* \brief	A null mutex, no cost.
		*/
		class NullMutex
		{
		public:
			void lock() {}
			void unlock() {}
			bool try_lock() { return true; }
		};

		/*!
		* \class	SpinLock
		*
		* \brief	A simple spin lock utilizing c++11 atomic_flag
		*/
		class SpinLock : UnMovable
		{
		public:
			SpinLock(){ flag_.clear(); }
			inline void lock()
			{
				while (flag_.test_and_set(std::memory_order_acquire))
				{
					nop_pause();	// no op or use architecture pause
				}
			}

			inline void unlock()
			{
				flag_.clear(std::memory_order_release);
			}

			inline bool try_lock()
			{
				return !flag_.test_and_set(std::memory_order_acquire);
			}
		private:
			std::atomic_flag flag_;
		};

		class RWLockable;

		class RWLock {
			friend class RWLockable;

		public:
			enum class LockType {
				none,
				read,
				write
			};

		private:
			RWLockable * lockable_;
			LockType lockType_;

			RWLock(RWLockable * const lockable, bool const exclusive);
			RWLock();

		public:
			RWLock(RWLock&& rhs);
			RWLock& operator =(RWLock&& rhs);
			~RWLock();

			void unlock();
			LockType get_lock_type() const;
		};

		class RWLockable {
			friend RWLock::~RWLock();

		private:
			class Counters {
			private:
				uint16_t read_;
				uint8_t writeClaim_;
				uint8_t writeDone_;
			public:
				bool is_waiting_for_write() const;
				bool is_waiting_for_read() const;
				bool is_my_turn_to_write(Counters const & claim) const;

				bool want_to_read(Counters * buf) const;
				bool want_to_write(Counters * buf) const;
				Counters done_reading() const;
				Counters done_writing() const;
			};

			std::atomic<Counters> counters_;

			void unlock_read();
			void unlock_write();

		public:
			RWLock lock_for_read();
			RWLock lock_for_write();
			bool is_lock_free() const;
		};

		namespace lockfree
		{
			template <typename Key, typename Value> class UnorderedMap : public UnMovable
			{
			public:
				using MapType = std::unordered_map<Key, Value>;
				using MapPairType = std::pair<Key, Value>;

				UnorderedMap() {}

				bool is_lock_free() const
				{
					return lockable_.is_lock_free();
				}

				bool contains(const Key& key)
				{
					auto lock = lockable_.lock_for_read();
					return map_.count(key) > 0;
				}

				bool get(const Key& key, Value& dst)
				{
					auto lock = lockable_.lock_for_read();
					if (map_.count(key) > 0)
					{
						dst = map_[key];
						return true;
					}
					return false;
				}

				MapType snapshot()
				{
					auto lock = lockable_.lock_for_read();
					return map_;
				}

				bool insert(const Key& key, const Value& value)
				{
					if (contains(key)) return false;
					auto lock = lockable_.lock_for_write();
					map_[key] = value;
					return true;
				}

				bool insert(const MapPairType& pair)
				{
					return insert(pair.first, pair.second);
				}

				void replace(const Key& key, const Value& value)
				{
					auto lock = lockable_.lock_for_write();
					map_[key] = value;
				}

				void replace(const MapPairType& pair)
				{
					replace(pair.first, pair.second);
				}

				void erase(const Key& key)
				{
					auto lock = lockable_.lock_for_write();
					map_.erase(key);
				}

				void clear()
				{
					auto lock = lockable_.lock_for_write();
					map_.clear();
				}

				//template <typename Function>
				//void for_each(std::function<Function> functor)
				//{
				//	auto lock = lockable_.lock_for_read();
				//	std::for_each(map_.begin(), map_.end(), functor);
				//}

			private:

				RWLockable	lockable_;
				MapType	map_;
			};

			template <typename T> class NonTrivialContainer : public UnMovable
			{
			public:
				NonTrivialContainer() {}

				bool is_lock_free() const
				{
					return lockable_.is_lock_free();
				}

				T get()
				{
					auto lock = lockable_.lock_for_read();
					return obj_;
				}

				void set(const T& t)
				{
					//std::lock_guard<std::mutex> lock(mutex_);
					auto lock = lockable_.lock_for_write();
					obj_ = t;
				}

			private:
				T	obj_;
				RWLockable lockable_;

			};
		}

		namespace lockbased
		{
			template <typename Key, typename Value> class UnorderedMap : public UnMovable
			{
			public:
				using MapType = std::unordered_map<Key, Value>;
				using MapPairType = std::pair<Key, Value>;

				UnorderedMap() {}

				bool is_lock_free() const
				{
					return false;
				}

				bool contains(const Key& key)
				{
					std::lock_guard<std::mutex> lock(mutex_);
					return map_.count(key) > 0;
				}

				bool get(const Key& key, Value& dst)
				{
					std::lock_guard<std::mutex> lock(mutex_);
					if (map_.count(key) > 0)
					{
						dst = map_[key];
						return true;
					}
					return false;
				}

				MapType snapshot()
				{
					std::lock_guard<std::mutex> lock(mutex_);
					return map_;
				}

				bool insert(const Key& key, const Value& value)
				{
					if (contains(key)) return false;
					std::lock_guard<std::mutex> lock(mutex_);
					map_[key] = value;
					return true;
				}

				bool insert(const MapPairType& pair)
				{
					return insert(pair.first, pair.second);
				}

				void replace(const Key& key, const Value& value)
				{
					std::lock_guard<std::mutex> lock(mutex_);
					map_[key] = value;
				}

				void replace(const MapPairType& pair)
				{
					replace(pair.first, pair.second);
				}

				void erase(const Key& key)
				{
					std::lock_guard<std::mutex> lock(mutex_);
					map_.erase(key);
				}

				void clear()
				{
					std::lock_guard<std::mutex> lock(mutex_);
					map_.clear();
				}

				//template <typename Function>
				//void for_each(std::function<Function> functor)
				//{
				//	auto lock = lockable_.lock_for_read();
				//	std::for_each(map_.begin(), map_.end(), functor);
				//}

			private:

				std::mutex mutex_;
				MapType	map_;
			};

			template <typename T> class NonTrivialContainer : public UnMovable
			{
			public:
				NonTrivialContainer() {}

				bool is_lock_free() const
				{
					return false;
				}

				T get()
				{
					std::lock_guard<std::mutex> lock(mutex_);
					return obj_;
				}

				void set(const T& t)
				{
					std::lock_guard<std::mutex> lock(mutex_);
					obj_ = t;
				}

			private:
				T	obj_;
				std::mutex mutex_;

			};
		}

		///*!
		// * \brief AtomicUnorderedMap Template atomic unordered_map<>
		// * AtomicUnorderedMap is lock-free, however, modification will create copies.
		// * Thus this structure is good for read-many write-rare purposes.
		// */
		//template <typename Key, typename Value> class AtomicUnorderedMap
		//{
		//	using MapType = std::unordered_map<Key, Value>;
		//	using MapPtr = std::shared_ptr<MapType>;
		//public:
		//	AtomicUnorderedMap()
		//	{
		//		mapPtr_ = std::make_shared<MapType>();
		//	}

		//	/*!
		//	 * \brief Get shared_ptr of unorderd_map instance
		//	 * \return Shared_ptr of unordered_map
		//	 */
		//	MapPtr get()
		//	{
		//		return std::atomic_load(&mapPtr_);
		//	}

		//	/*!
		//	 * \brief Insert key-value pair to the map
		//	 * \param key
		//	 * \param value
		//	 * \return True on success
		//	 */
		//	bool insert(const Key& key, const Value& value)
		//	{
		//		MapPtr p = std::atomic_load(&mapPtr_);
		//		MapPtr copy;
		//		do
		//		{
		//			if ((*p).count(key) > 0) return false;
		//			copy = std::make_shared<MapType>(*p);
		//			(*copy).insert({ key, value });
		//		} while (!std::atomic_compare_exchange_weak(&mapPtr_, &p, std::move(copy)));
		//		return true;
		//	}

		//	/*!
		//	 * \brief Erase according to key
		//	 * \param key
		//	 * \return True on success
		//	 */
		//	bool erase(const Key& key)
		//	{
		//		MapPtr p = std::atomic_load(&mapPtr_);
		//		MapPtr copy;
		//		do
		//		{
		//			if ((*p).count(key) <= 0) return false;
		//			copy = std::make_shared<MapType>(*p);
		//			(*copy).erase(key);
		//		} while (!std::atomic_compare_exchange_weak(&mapPtr_, &p, std::move(copy)));
		//		return true;
		//	}

		//	/*!
		//	 * \brief Clear all
		//	 */
		//	void clear()
		//	{
		//		MapPtr p = atomic_load(&mapPtr_);
		//		auto copy = std::make_shared<MapType>();
		//		do
		//		{
		//			;	// do clear when possible does not require old status
		//		} while (!std::atomic_compare_exchange_weak(&mapPtr_, &p, std::move(copy)));
		//	}

		//private:
		//	std::shared_ptr<MapType>	mapPtr_;
		//};

		///*!
		// * \brief AtomicNonTrivial Template lock-free class
		// * AtomicNonTrivial is lock-free, however, modification will create copies.
		// * Thus this structure is good for read-many write-rare purposes.
		// */
		//template <typename T> class AtomicNonTrivial
		//{
		//public:
		//	AtomicNonTrivial()
		//	{
		//		ptr_ = std::make_shared<T>();
		//	}

		//	/*!
		//	 * \brief Get shared_ptr to instance
		//	 * \return Shared_ptr to instance
		//	 */
		//	std::shared_ptr<T> get()
		//	{
		//		return std::atomic_load(&ptr_);
		//	}

		//	/*!
		//	 * \brief Set to new value
		//	 * \param val
		//	 * This operation will make a copy which is only visible for future get()
		//	 */
		//	void set(const T& val)
		//	{
		//		std::shared_ptr<T> copy = std::make_shared<T>(val);
		//		std::atomic_store(&ptr_, copy);
		//	}

		//private:
		//	std::shared_ptr<T>	ptr_;
		//};


		//		namespace gc
		//		{
		//			class HPRecord
		//			{
		//			public:
		//				void*	pHazard_;	//!< can be used by the thread that acquire it
		//				HPRecord*			pNext_;
		//				static HPRecord* head() { return pHead_; }
		//
		//				static HPRecord* acquire()
		//				{
		//					// try to reuse a retired hp record
		//					HPRecord* p = pHead_;
		//					bool inactive(false);
		//
		//					for (; p; p = p->pNext_)
		//					{
		//						if (p->active_ || (!p->active_.compare_exchange_weak(inactive, true)))
		//						{
		//							continue;
		//						}
		//						return p; // got one!
		//					}
		//					// increment the list length by adding one
		//					int oldLen;
		//					do
		//					{
		//						oldLen = listLen_;
		//					} while (!listLen_.compare_exchange_weak(oldLen, oldLen + 1));
		//					// allocate a new one
		//					p = new HPRecord;
		//					p->active_.store(true);
		//					p->pHazard_ = nullptr;
		//					// push it to the front
		//					HPRecord* old = nullptr;
		//					do
		//					{
		//						old = pHead_;
		//						p->pNext_ = old;
		//					} while (!pHead_.compare_exchange_weak(old, p));
		//
		//					return p; // return acquired pointer
		//				}
		//
		//				static void release(HPRecord* p)
		//				{
		//					p->pHazard_ = nullptr;
		//					p->active_.store(false);
		//				}
		//
		//			private:
		//				std::atomic<bool>	active_;
		//				static std::atomic<HPRecord*>	pHead_;	//!< global header of the hazard pointer list
		//				static std::atomic<int>			listLen_; //!< length of the list
		//			};
		//
		//			template <typename T> void garbage_collection(std::vector<T*> rlist)
		//			{
		//				// stage 1: scan hazard ponter list
		//				// collecting all non-null pointers
		//				std::vector<void*> hp;
		//				HPRecord* head = HPRecord::head();
		//				while (head)
		//				{
		//					void *p = head->pHazard_;
		//					if (p) hp.push_back(p);
		//					head = head->pNext_;
		//				}
		//				// stage 2: sort the hazard pointers
		//				std::sort(hp.begin(), hp.end(), std::less<void*>());
		//				// stage 3: search for null
		//				std::vector<T*>::iterator iter = rlist.begin();
		//				while (iter != rlist.end())
		//				{
		//					if (!std::binary_search(hp.begin(), hp.end(), *iter))
		//					{
		//						delete *iter;	// safely reclaim this memory
		//						if (&*iter != rlist.back())
		//						{
		//							*iter = rlist.back();
		//						}
		//						rlist.pop_back();
		//					}
		//					else
		//					{
		//						++iter;
		//					}
		//				}
		//			}
		//
		//		} // namespace gc
		//
		//		namespace consts
		//		{
		//			static const unsigned kGarbageCollectionThreshold = 4;
		//		}
		//
		//		template <typename K, typename V> class WRRMUMap
		//		{
		//		public:
		//
		//		private:
		//			using MapType = std::unordered_map<K, V>;
		//
		//			static void retire(MapType* old)
		//			{
		//				// put old map into retire list
		//				rlist_.push_back(old);
		//				if (rlist_.size() >= consts::kGarbageCollectionThreshold)
		//				{
		//					gc::garbage_collection(rlist_);
		//				}
		//			}
		//#if _MSC_VER
		//			static __declspec(thread)  std::vector<MapType*> rlist_;
		//#else
		//			static thread_local std::vector<MapType*> rlist_;
		//#endif
		//			MapType* pMap_;
		//		};


	} // namespace cds


	/*!
	 * \namespace zz::os
	 * \brief Namespace for OS specific implementations
	 */
	namespace os
	{
		/*!
		 * \fn int system(const char *const command, const char *const module_name = 0)
		 * \brief Execute sub-process using system call
		 * \param command
		 * \param module_name
		 * \return Return code of sub-process
		 */
		int system(const char *const command, const char *const module_name = 0);

		/*!
		 * \fn std::size_t thread_id()
		 * \brief Get thread id
		 * \return Current thread id
		 */
		std::size_t thread_id();

		/*!
		 * \brief Check if stdout is associated with console
		 * \return None zero value if stdout is not redirected to file
		 */
		int is_atty();

		/*!
		 * \fn std::tm localtime(std::time_t t)
		 * \brief Thread-safe version of localtime
		 * \param t std::time_t
		 * \return std::tm format localtime
		 */
		std::tm localtime(std::time_t t);

		/*!
		 * \fn std::tm gmtime(std::time_t t)
		 * \brief Thread-safe version of gmtime
		 * \param t std::time_t
		 * \return std::tm format UTC time
		 */
		std::tm gmtime(std::time_t t);

		/*!
		 * \brief Convert UTF-8 string to wstring
		 * \param u8str
		 * \return Converted wstring
		 */
		std::wstring utf8_to_wstring(std::string &u8str);

		/*!
		 * \brief Convert wstring to UTF-8
		 * \param wstr
		 * \return Converted UTF-8 string
		 */
		std::string wstring_to_utf8(std::wstring &wstr);

		/*!
		 * \brief Check if path exist in filesystem
		 * \param path
		 * \param considerFile Consider file as well?
		 * \return true if path exists
		 */
		bool path_exists(std::string path, bool considerFile = true);

		/*!
		 * \brief Check if path is file and exists
		 * \param path
		 * \return true if file exists, not directory
		 */
		bool is_file(std::string path);

		/*!
		 * \brief Check if path is directory and exists
		 * \param path
		 * \return true if directory exists, not file
		 */
		bool is_directory(std::string path);

		/*!
		 * \brief Open fstream using UTF-8 string
		 * This is a wrapper function because Windows will require wstring to process unicode filename/path.
		 * \param stream
		 * \param filename
		 * \param openmode
		 */
		void fstream_open(std::fstream &stream, std::string filename, std::ios::openmode openmode);

		/*!
		 * \brief Open ifstream using UTF-8 string
		 * This is a wrapper function because Windows will require wstring to process unicode filename/path.
		 * \param stream
		 * \param filename
		 * \param openmode
		 */
		void ifstream_open(std::ifstream &stream, std::string filename, std::ios::openmode openmode);

		/*!
		 * \brief Rename file, support unicode filename/path.
		 * \param oldName
		 * \param newName
		 * \return true on success
		 */
		bool rename(std::string oldName, std::string newName);

		/*!
		 * \brief Copy file, support unicode filename/path.
		 * \param oldName
		 * \param newName
		 * \param replaceDst If true, existing dst file will be replaced
		 * \return true on success
		 */
		bool copyfile(std::string src, std::string dst, bool replaceDst = false);

		/*!
		 * \brief Move file, support unicode filename/path.
		 * \param oldName
		 * \param newName
		 * \param replaceDst If true, existing dst file will be replaced
		 * \return true on success
		 */
		bool movefile(std::string src, std::string dst, bool replaceDst = false);

		/*!
		 * \brief Remove path, whatever file or directory.
		 * Dangerous! Cannot revert.
		 * \param path
		 * \return true as long as path does not exists anymore
		 */
		bool remove_all(std::string path);

		/*!
		 * \brief Remove directory and all sub-directories and files if set recursive to true.
		 * If recursive set to false, will try to delete an empty one only. Dangerous! Cannot revert.
		 * \param root
		 * \param recursive Delete all contents inside of it.
		 * \return true as long as directory does not exists anymore(file with same name may exist)
		 */
		bool remove_dir(std::string root, bool recursive = true);

		/*!
		 * \brief Remove file.
		 * Dangerous! Cannot revert.
		 * \param path
		 * \return true as long as file does not exists anymore
		 */
		bool remove_file(std::string path);

		/*!
		 * \brief Retrieve the last error in errno
		 * \return Human readable error string
		 */
		std::string last_error();

		/*!
		 * \brief	Gets the OS dependent line end characters.
		 * \return	A std::string.
		 */
		std::string endl();

		/*!
		 * \brief	Gets the OS dependent path delim.
		 * \return	A std::string.
		 */
		std::string path_delim();

		/*!
		 * \brief Get current working directory
		 * \return Current working directory
		 */
		std::string current_working_directory();

		/*!
		 * \brief Convert reletive path to absolute path
		 * \param reletivePath
		 * \return Absolute path
		 */
		std::string absolute_path(std::string reletivePath);

		/*!
		 * \brief Split path into hierachical sub-folders
		 * \param path
		 * \return std::vector<std::string> of sub-folders
		 * path_split("/usr/local/bin/xxx/")= {"usr","local","bin","xxx"}
		 */
		std::vector<std::string> path_split(std::string path);

		/*!
		 * \brief Compare identical path according to OS.
		 * By default, windows paths are case-INsensitive.
		 * \param first
		 * \param second
		 * \param forceCaseSensitve Force compare using case sensitivity.
		 * \return True if paths are indentical
		 */
		bool path_identical(std::string first, std::string second, bool forceCaseSensitve = false);

		/*!
		 * \brief Join path from sub-folders
		 * This function will handle system dependent path formats
		 * such as '/' for unix like OS, and '\\' for windows
		 * \param elems
		 * \return Long path
		 * path_join({"/home/abc/","def"} = "/home/abc/def"
		 */
		std::string path_join(std::vector<std::string> elems);

		/*!
		 * \brief Split filename from path
		 * \param path
		 * \return Full filename with extension
		 */
		std::string path_split_filename(std::string path);

		/*!
		 * \brief Split the deepest directory
		 * \param path
		 * \return Deepest directory name. E.g. "abc/def/ghi/xxx.xx"->"ghi"
		 */
		std::string path_split_directory(std::string path);

		/*!
		 * \brief Split basename
		 * \param path
		 * \return Basename without extension
		 */
		std::string path_split_basename(std::string path);

		/*!
		 * \brief Split extension if any
		 * \param path
		 * \return Extension or "" if no extension exists
		 */
		std::string path_split_extension(std::string path);

		/*!
		 * \brief Append string to basename directly, rather than append to extension
		 * This is more practically useful because nobody want to change extension in most situations.
		 * Appending string to basename in order to change filename happens.
		 * \param origPath
		 * \param whatToAppend
		 * \return New filename with appended string.
		 * path_append_basename("/home/test/abc.jpg", "_01") = "/home/test/abc_01.jpg"
		 */
		std::string path_append_basename(std::string origPath, std::string whatToAppend);

		/*!
		 * \brief Create directory if not exist
		 * \param path
		 * \return True if directory created/already exist
		 * \note Will not create recursively, fail if parent directory inexist. Use create_directory_recursive instead.
		 */
		bool create_directory(std::string path);

		/*!
		 * \brief Create directory recursively if not exist
		 * \param path
		 * \return True if directory created/already exist, false when failed to create the final directory
		 * Function create_directory_recursive will create directory recursively.
		 * For example, create_directory_recursive("/a/b/c") will create /a->/a/b->/a/b/c recursively.
		 */
		bool create_directory_recursive(std::string path);

		/*!
		 * \brief List directory contents
		 * \param root Root of the directory
		 * \return A std::vecotr<std::string>, vector of absolute paths of files and sub-directories
		 */
		std::vector<std::string> list_directory(std::string root);

		/*!
		 * \brief Return the Size of console window
		 * \return Size, which contains width and height
		 */
		Size console_size();

	} // namespace os

	/*!
	 * \namespace zz::time
	 * \brief Namespace for time related stuff
	 */
	namespace time
	{
		/*!
		* \class	DateTime
		*
		* \brief	A calendar date class.
		*/
		class DateTime
		{
		public:
			DateTime();
			virtual ~DateTime() = default;

			/*!
			 * \brief Convert to local time zone
			 */
			void to_local_time();

			/*!
			 * \brief Convert to UTC time zone
			 */
			void to_utc_time();

			/*!
			 * \brief Convert date to user friendly string.
			 * Support various formats.
			 * \param format
			 * \return Readable string
			 */
			std::string to_string(const char *format = "%y-%m-%d %H:%M:%S.%frac");

			/*!
			 * \brief Static function to return in local_time
			 * \return DateTime instance
			 */
			static DateTime local_time();

			/*!
			 * \brief Static function to return in utc_time
			 * \return DateTime instance
			 */
			static DateTime utc_time();

		private:
			std::time_t		timeStamp_;
			int				fraction_;
			std::string		fractionStr_;
			struct std::tm	calendar_;

		};

		/*!
		* \class	Timer
		*
		* \brief	A timer class.
		*/
		class Timer
		{
		public:
			Timer();
			/*!
			 * \brief Reset timer to record new process
			 */
			void reset();

			/*!
			* \brief Pause recording timelapse
			*/
			void pause();

			/*!
			* \brief Resume timer
			*/
			void resume();

			/*!
			 * \brief Return elapsed time quantized in nanosecond
			 * \return Nanosecond elapsed
			 */
			std::size_t	elapsed_ns();

			/*!
			 * \brief Return string of elapsed time quantized in nanosecond
			 * \return Nanosecond elapsed in string
			 */
			std::string elapsed_ns_str();

			/*!
			 * \brief Return elapsed time quantized in microsecond
			 * \return Microsecond elapsed
			 */
			std::size_t elapsed_us();

			/*!
			 * \brief Return string of elapsed time quantized in microsecond
			 * \return Microsecond elapsed in string
			 */
			std::string elapsed_us_str();

			/*!
			 * \brief Return elapsed time quantized in millisecond
			 * \return Millisecond elapsed
			 */
			std::size_t elapsed_ms();

			/*!
			 * \brief Return string of elapsed time quantized in millisecond
			 * \return Millisecond elapsed in string
			 */
			std::string elapsed_ms_str();

			/*!
			 * \brief Return elapsed time quantized in second
			 * \return Second elapsed
			 */
			std::size_t elapsed_sec();

			/*!
			 * \brief Return string of elapsed time quantized in second
			 * \return Second elapsed in string
			 */
			std::string elapsed_sec_str();

			/*!
			 * \brief Return elapsed time in second, no quantization
			 * \return Second elapsed in double
			 */
			double elapsed_sec_double();

			/*!
			 * \brief Convert timer to user friendly string.
			 * Support various formats
			 * \param format
			 * \return Formatted string
			 */
			std::string to_string(const char *format = "[%ms ms]");

		private:
			std::chrono::steady_clock::time_point timeStamp_;
			std::size_t	elapsed_;
			bool		paused_;
		};

		/*!
		* \fn	void sleep(int milliseconds);
		*
		* \brief	Sleep for specified milliseconds
		*
		* \param	milliseconds	The milliseconds.
		*/
		inline void sleep(int milliseconds)
		{
			if (milliseconds > 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
			}
		}

	} // namespace time

	/*!
	 * \namespace zz::fs
	 * \brief Namespace for classes adpated to filesystems
	 */
	namespace fs
	{
		namespace consts
		{
			static const int kDefaultFileOpenRetryTimes = 5;
			static const int kDefaultFileOpenRetryInterval = 10;
		}

		/*!
		* \brief The Path class to resolve filesystem path
		*/
		class Path
		{
		public:
			Path(std::string path, bool isAbsolute = false);

			/*!
			 * \brief Check if path is empty
			 * \return True if empty
			 */
			bool empty() const;

			/*!
			 * \brief Check path existance, whatever file/dir/device...
			 * \return True if path exist
			 */
			bool exist() const;

			/*!
			 * \brief Check if path is a file and exist
			 * \return True if file exist
			 */
			bool is_file() const;

			/*!
			 * \brief Check if path is a directory and exist
			 * \return True if dir exist
			 */
			bool is_dir() const;

			/*!
			 * \brief Return Absolute path
			 * \return A std::string of absolute path
			 */
			std::string abs_path() const;

			/*!
			 * \brief Return relative path to current working directory
			 * \return A std::string of relative path to cwd
			 */
			std::string relative_path() const;

			/*!
			 * \brief Return relative path to specified root.
			 * \param root Specified root path
			 * \return A std::string of relative path to root
			 */
			std::string relative_path(std::string root) const;

			/*!
			 * \brief Return filename if is file and exist.
			 * Will return empty string if path is not a file
			 * \return A std::string of filename
			 */
			std::string filename() const;

		private:
			std::string abspath_;
		};

		/*!
		* \brief The Directory class for filesystem directory operations
		*/
		class Directory
		{
		public:
			using iterator = std::vector<Path>::iterator;
			using const_iterator = std::vector<Path>::const_iterator;

			/*!
			 * \brief Directory constructor
			 * \param root Root of directory
			 * \param recursive Recursive search or not
			 */
			Directory(std::string root, bool recursive = false);

			/*!
			 * \brief Directory constructor with filter pattern
			 * \param root Root of directory
			 * \param pattern Filter pattern
			 * \param recursive Recursive search or not
			 */
			Directory(std::string root, const std::string pattern, bool recursive = false);

			/*!
			 * \brief Directory constructor with filter pattern
			 * \param root Root of directory
			 * \param patternList Vector of filter patterns
			 * \param recursive Recursive search or not
			 */
			Directory(std::string root, const std::vector<std::string> patternList, bool recursive);

			/*!
			 * \brief Directory constructor with filter pattern
			 * \param root Root of directory
			 * \param patternList Vector of filter patterns
			 * \param recursive Recursive search or not
			 */
			Directory(std::string root, const std::vector<const char*> patternList, bool recursive);

			/*!
			 * \brief Return begin iterator
			 * \return Iterator to begin
			 */
			iterator begin() { return paths_.begin(); }

			/*!
			 * \brief Return end iterator
			 * \return Iterator to end
			 */
			iterator end() { return paths_.end(); }

			/*!
			 * \brief Return const begin iterator
			 * \return Iterator to cbegin
			 */
			const_iterator cbegin() const { return paths_.cbegin(); }

			/*!
			 * \brief Return const end iterator
			 * \return Iterator to cend
			 */
			const_iterator cend() const { return paths_.cend(); }

			/*!
			 * \brief Return number of contained directories or files
			 * \return Size
			 */
			std::size_t size() const { return paths_.size(); }

			/*!
			* \brief Check if directory is resursively searched
			* \return True if recursive
			*/
			bool is_recursive() const;

			/*!
			 * \brief Return root of directory
			 * \return A std::string of absolute root path
			 */
			std::string root() const;

			/*!
			 * \brief Filter directory with specified pattern
			 * \param Wild card matching pattern
			 */
			void filter(const std::string pattern);

			/*!
			 * \brief Filter directory with multiple patterns
			 * \param patternList Vector of wild card matching patterns
			 */
			void filter(const std::vector<const char*> patternList);

			/*!
			 * \brief Filter directory with multiple patterns
			 * \param patternList Vector of wild card matching patterns
			 */
			void filter(const std::vector<std::string> patternList);

			/*!
			 * \brief Clear filter pattern, redo search in directory
			 */
			void reset();

			/*!
			 * \brief Return the entire list
			 * \return A std::vector<Path> containing current list
			 */
			std::vector<Path> to_list() const;

		private:
			void resolve();

			bool recursive_;
			Path root_;
			std::vector<Path> paths_;
		};

		/*!
		 * \brief The FileEditor class to modify file
		 * This class is derived from UnCopyable, so no copy operation.
		 * Move operation is allowed by std::move();
		 */
		class FileEditor : private UnCopyable
		{
		public:
			FileEditor() = default;

			/*!
			 * \brief FileEditor constructor
			 * \param filename
			 * \param truncateOrNot Whether open in truncate mode or not
			 * \param retryTimes Retry open file if not success
			 * \param retryInterval Retry interval in ms
			 */
			FileEditor(std::string filename, bool truncateOrNot = false,
				int retryTimes = consts::kDefaultFileOpenRetryTimes,
				int retryInterval = consts::kDefaultFileOpenRetryInterval);

			/*!
			* \fn	File::File(File&& from)
			*
			* \brief	Move constructor.
			*
			* \param [in,out]	other	Source for the instance.
			*/
			//FileEditor(FileEditor&& other);

			virtual ~FileEditor() { this->close(); }

			// No move operator
			FileEditor& operator=(FileEditor&&) = delete;

			/*!
			 * \brief Overload << operator just like a stream
			 */
			template <typename T>
			FileEditor& operator<<(T what) { stream_ << what; return *this; }

			/*!
			 * \brief Return filename
			 * \return Filename of this object
			 */
			std::string filename() const { return filename_; }

			/*!
			 * \brief Open file
			 * \param filename
			 * \param truncateOrNot Whether open in truncate mode or not
			 * \param retryTimes Retry open file if not success
			 * \param retryInterval Retry interval in ms
			 * \return True if success
			 */
			bool open(std::string filename, bool truncateOrNot = false,
				int retryTimes = consts::kDefaultFileOpenRetryTimes,
				int retryInterval = consts::kDefaultFileOpenRetryInterval);

			/*!
			 * \brief Open file
			 * \param filename
			 * \param truncateOrNot Whether open in truncate mode or not
			 * \param retryTimes Retry open file if not success
			 * \param retryInterval Retry interval in ms
			 * \return True if success
			 */
			bool open(const char* filename, bool truncateOrNot = false,
				int retryTimes = consts::kDefaultFileOpenRetryTimes,
				int retryInterval = consts::kDefaultFileOpenRetryInterval);

			/*!
			 * \brief Open file
			 * \param truncateOrNot Whether open in truncate mode or not
			 * \return True if success
			 */
			bool open(bool truncateOrNot = false);

			/*!
			 * \brief Reopen current file
			 * \param truncateOrNot
			 * \return True if success
			 */
			bool reopen(bool truncateOrNot = true);

			/*!
			 * \brief Close current file
			 */
			void close();

			/*!
			 * \brief Check whether current file handler is set
			 * \return True if valid file
			 */
			bool is_valid() const { return !filename_.empty(); }

			/*!
			 * \brief Check if file is opened
			 * \return True if opened
			 */
			bool is_open() const { return stream_.is_open(); }

			/*!
			 * \brief Flush file stream
			 */
			void flush() { stream_.flush(); }
		private:

			bool try_open(int retryTime, int retryInterval, bool truncateOrNot = false);
			void check_valid() { if (!this->is_valid()) throw RuntimeException("Invalid File Editor!"); }

			std::string		filename_;
			std::fstream	stream_;
			std::streampos	readPos_;
			std::streampos	writePos_;
		};

		/*!
		 * \brief The FileReader class for read-only operations.
		 * This class is derived from UnCopyable, so no copy operation.
		 * Move operation is allowed by std::move();
		 */
		class FileReader : private UnCopyable
		{
		public:
			FileReader() = delete;

			/*!
			 * \brief FileReader constructor
			 * \param filename
			 * \param retryTimes Retry open times
			 * \param retryInterval Retry interval in ms
			 */
			FileReader(std::string filename, int retryTimes = consts::kDefaultFileOpenRetryTimes,
				int retryInterval = consts::kDefaultFileOpenRetryInterval);

			/*!
			 * \brief FileReader move constructor
			 * \param other
			 */
			//FileReader(FileReader&& other);

			/*!
			 * \brief Return filename
			 * \return Filename
			 */
			std::string filename() const { return filename_; }

			/*!
			 * \brief Check if is opened
			 * \return True if opened
			 */
			bool is_open() const { return istream_.is_open(); }

			/*!
			 * \brief Check if valid filename is set
			 * \return True if valid
			 */
			bool is_valid() const { return !filename_.empty(); }

			/*!
			 * \brief Close file handler
			 */
			void close() { istream_.close(); }

			/*!
			 * \brief Get file size in byte, member function
			 * \return File size in byte
			 */
			std::size_t	file_size();

			/*!
			 * \brief Count number of lines in text file
			 * \return Number of lines
			 */
			std::size_t count_lines();

			/*!
			 * \brief Get next line
			 * \param trimWhitespaces Whether or not trim whitespaces
			 * \return A std::string of the line
			 */
			std::string next_line(bool trimWhiteSpaces = false);

			/*!
			 * \brief Count number of lines in text file
			 * If reached end of file, will return the number of lines
			 * \param n Line # to jump to
			 * \return -1 if failed, otherwise return the position actually jumped to
			 */
			int goto_line(int n);

		private:
			bool open();
			bool try_open(int retryTime, int retryInterval);
			void check_valid(){ if (!this->is_valid()) throw RuntimeException("Invalid File Reader!"); }
			std::string		filename_;
			std::ifstream	istream_;
		};

		/*!
		 * \brief Get file size in byte
		 * \param filename
		 * \return File size in byte, 0 if empty or any error occurred.
		 */
		std::size_t get_file_size(std::string filename);

	} //namespace fs


	/*!
	 * \namespace zz::fmt
	 * \brief Namespace for formatting functions
	 */
	namespace fmt
	{
		namespace consts
		{
			static const std::string kFormatSpecifierPlaceHolder = std::string("{}");
		}

		namespace detail
		{
			template<class Facet>
			struct DeletableFacet : Facet
			{
				template<class ...Args>
				DeletableFacet(Args&& ...args) : Facet(std::forward<Args>(args)...) {}
				~DeletableFacet() {}
			};
		} // namespace detail

		/*!
		 * \brief Convert int to left zero padded string
		 * \param num Integer number
		 * \param length String length
		 * \return Zero-padded string
		 */
		inline std::string int_to_zero_pad_str(int num, int length)
		{
			std::ostringstream ss;
			ss << std::setw(length) << std::setfill('0') << num;
			return ss.str();
		}

		/*!
		 * \brief Check if is digit
		 * \param c
		 * \return
		 */
		bool is_digit(char c);

		/*!
		 * \brief Match string with wildcard.
		 * Match string with wildcard pattern, '*' and '?' supported.
		 * \param str
		 * \param pattern
		 * \return True for success match
		 */
		bool wild_card_match(const char* str, const char* pattern);

		/*!
		 * \brief Check if string starts with specified sub-string
		 * \param str
		 * \param start
		 * \return True if success
		 */
		bool starts_with(const std::string& str, const std::string& start);

		/*!
		 * \brief Check if string ends with specified sub-string
		 * \param str
		 * \param end
		 * \return True if success
		 */
		bool ends_with(const std::string& str, const std::string& end);

		/*!
		 * \brief Replace all from one to another sub-string
		 * \param str
		 * \param replaceWhat
		 * \param replaceWith
		 * \return Reference to modified string
		 */
		std::string& replace_all(std::string& str, const std::string& replaceWhat, const std::string& replaceWith);

		/*!
		 * \brief Replace all from one to another sub-string, char version
		 * \param str
		 * \param replaceWhat
		 * \param replaceWith
		 * \return Reference to modified string
		 */
		std::string& replace_all(std::string& str, char replaceWhat, char replaceWith);

		/*!
		 * \brief Compare c style raw string
		 * Will take care of string not ends with '\0'(ignore it),
		 * which is unsafe using strcmp().
		 * \param s1
		 * \param s2
		 * \return True if same
		 */
		bool str_equals(const char* s1, const char* s2);

		/*!
		 * \brief Left trim whitespace
		 * \param str
		 * \return Trimed string
		 */
		std::string ltrim(std::string str);

		/*!
		 * \brief Right trim whitespace
		 * \param str
		 * \return Trimed string
		 */
		std::string rtrim(std::string str);

		/*!
		 * \brief Left and right trim whitespace
		 * \param str
		 * \return Trimed string
		 */
		std::string trim(std::string str);

		/*!
		 * \brief Strip specified sub-string from left.
		 * The strip will do strict check from left, even whitespace.
		 * \param str Original string
		 * \param what What to be stripped
		 * \return Stripped string
		 */
		std::string lstrip(std::string str, std::string what);

		/*!
		 * \brief Strip specified sub-string from right.
		 * The strip will do strict check from right, even whitespace.
		 * \param str Original string
		 * \param what What to be stripped
		 * \return Stripped string
		 */
		std::string rstrip(std::string str, std::string what);

		/*!
		* \brief Skip from left until delimiter string found.
		* \param str
		* \param delim
		* \return Skipped string
		*/
		std::string lskip(std::string str, std::string delim);

		/*!
		 * \brief Skip from right until delimiter string found.
		 * \param str
		 * \param delim
		 * \return Skipped string
		 */
		std::string rskip(std::string str, std::string delim);

		/*!
		 * \brief Skip from right, remove all stuff right to left-most delim
		 * \param str
		 * \param delim
		 * \return Skipped string
		 */
		std::string rskip_all(std::string str, std::string delim);

		/*!
		 * \brief Split string into parts with specified single char delimiter
		 * \param s
		 * \param delim
		 * \return A std::vector of parts in std::string
		 */
		std::vector<std::string> split(const std::string s, char delim = ' ');

		/*!
		 * \brief Split string into parts with specified string delimiter.
		 * The entire delimiter must be matched exactly
		 * \param s
		 * \param delim
		 * \return A std::vector of parts in std::string
		 */
		std::vector<std::string> split(const std::string s, std::string delim);

		/*!
		 * \brief Split string into parts with multiple single char delimiter
		 * \param s
		 * \param delim A std::string contains multiple single char delimiter, e.g.(' \t\n')
		 * \return A std::vector of parts in std::string
		 */
		std::vector<std::string> split_multi_delims(const std::string s, std::string delims);

		/*!
		 * \brief Special case to split_multi_delims(), split all whitespace.
		 * \param s
		 * \return A std::vector of parts in std::string
		 */
		std::vector<std::string> split_whitespace(const std::string s);

		/*!
		 * \brief Split string in two parts with specified delim
		 * \param s
		 * \param delim
		 * \return A std::pair of std::string, ret.first = first part, ret.second = second part
		 */
		std::pair<std::string, std::string> split_first_occurance(const std::string s, char delim);

		/*!
		 * \brief Concatenates a std::vector of strings into a string with delimiters
		 * \param elems
		 * \param delim
		 * \return Concatenated string
		 */
		std::string join(std::vector<std::string> elems, char delim);

		/*!
		 * \brief Go through vector and erase empty ones.
		 * Erase in-place in vector.
		 * \param vec
		 * \return Clean vector with no empty elements.
		 */
		std::vector<std::string>& erase_empty(std::vector<std::string>& vec);

		/*!
		 * \brief Replace first occurance of one string with specified another string.
		 * Replace in-place.
		 * \param str
		 * \param replaceWhat What substring to be replaced.
		 * \param replaceWith What string to replace.
		 */
		void replace_first_with_escape(std::string &str, const std::string &replaceWhat, const std::string &replaceWith);

		/*!
		 * \brief Replace every occurance of one string with specified another string.
		 * Replace in-place.
		 * \param str
		 * \param replaceWhat What substring to be replaced.
		 * \param replaceWith What string to replace.
		 */
		void replace_all_with_escape(std::string &str, const std::string &replaceWhat, const std::string &replaceWith);

		/*!
		* \brief Replace every occurance of one string with specified list of strings sequentially.
		* Replace in-place.
		* \param str
		* \param replaceWhat What substring to be replaced.
		* \param replaceWith Vector of what strings to replace one by one.
		*/
		void replace_sequential_with_escape(std::string &str, const std::string &replaceWhat, const std::vector<std::string> &replaceWith);

		/*!
		 * \brief Convert string to lower case.
		 * Support ASCII characters only. Unicode string will trigger undefined behavior.
		 * \param mixed Mixed case string
		 * \return Lower case string.
		 */
		std::string to_lower_ascii(std::string mixed);

		/*!
		 * \brief Convert string to upper case.
		 * Support ASCII characters only. Unicode string will trigger undefined behavior.
		 * \param mixed Mixed case string
		 * \return Upper case string.
		 */
		std::string to_upper_ascii(std::string mixed);

		/*!
		 * \brief C++ 11 UTF-8 string to UTF-16 string
		 * \param u8str UTF-8 string
		 * \return UTF-16 string
		 */
		std::u16string utf8_to_utf16(std::string u8str);

		/*!
		 * \brief C++ 11 UTF-16 string to UTF-8 string
		 * \param u16str UTF-16 string
		 * \return UTF-8 string
		 */
		std::string utf16_to_utf8(std::u16string u16str);

		/*!
		 * \brief C++ 11 UTF-8 string to UTF-32 string
		 * \param u8str UTF-8 string
		 * \return UTF-32 string
		 */
		std::u32string utf8_to_utf32(std::string u8str);

		/*!
		 * \brief C++ 11 UTF-32 string to UTF-8 string
		 * \param u32str UTF-32 string
		 * \return UTF-8 string
		 */
		std::string utf32_to_utf8(std::u32string u32str);

		/*!
		 * \fn template<typename Arg> inline void format_string(std::string &fmt, const Arg &last)
		 * \brief Format function to replace each {} with templated type variable
		 * \param fmt Original string with place-holder {}
		 * \param last The last variable in variadic template function
		 */
		template<typename Arg>
		inline void format_string(std::string &fmt, const Arg &last)
		{
			std::stringstream ss;
			ss << last;
			replace_first_with_escape(fmt, consts::kFormatSpecifierPlaceHolder, ss.str());
		}

		/*!
		 * \fn template<typename Arg> inline void format_string(std::string &fmt, const Arg& current, const Args&... more)
		 * \brief Vairadic variable version of format function to replace each {} with templated type variable
		 * \param fmt Original string with place-holder {}
		 * \param current The current variable to be converted
		 * \param ... more Variadic variables to be templated.
		 */
		template<typename Arg, typename... Args>
		inline void format_string(std::string &fmt, const Arg& current, const Args&... more)
		{
			std::stringstream ss;
			ss << current;
			replace_first_with_escape(fmt, consts::kFormatSpecifierPlaceHolder, ss.str());
			format_string(fmt, more...);
		}

	} // namespace fmt

	/*!
	 * \namespace zz::cfg
	 * \brief Namespace for configuration related classes and functions
	 */
	namespace cfg
	{
		/*!
		 * \brief The Value class for store/load various type to/from string
		 */
		class Value
		{
		public:
			/*!
			 * \brief Value default constructor
			 */
			Value(){}

			/*!
			 * \brief Value constructor from raw string
			 * \param cstr
			 */
			Value(const char* cstr) : str_(cstr) {}

			/*!
			 * \brief Value constrctor from string
			 * \param str
			 */
			Value(std::string str) : str_(str) {}

			/*!
			 * \brief Value copy constructor
			 * \param other
			 */
			Value(const Value& other) : str_(other.str_) {}

			/*!
			 * \brief Return stored string
			 * \return String
			 */
			std::string str() const { return str_; }

			/*!
			 * \brief Check if stored string is empty
			 * \return True if empty
			 */
			bool empty() const { return str_.empty(); }

			/*!
			 * \brief Clear stored string
			 */
			void clear() { str_.clear(); }

			/*!
			 * \brief Overloaded operator == for comparison
			 * \param other
			 * \return True if stored strings are same
			 */
			bool operator== (const Value& other) { return str_ == other.str_; }

			/*!
			 * \brief Overloaded operator = for copy
			 * \param other
			 * \return
			 */
			Value& operator= (const Value& other) { str_ = other.str_; return *this; }

			/*!
			 * \fn template <typename T> std::string store(T t);
			 * \brief Store value from template type T
			 * Support type that can << to a stringstream
			 */
			template <typename T> std::string store(T t);

			/*!
			 * \fn template <typename T> std::string store(std::vector<T> t);
			 * \brief Template specification of store for vector of type T
			 * Support type that can << to a stringstream
			 */
			template <typename T> std::string store(std::vector<T> t);

			/*!
			 * \fn template <typename T> T load(T& t);
			 * \brief Load value to template type T
			 * Support type that can >> to a stringstream
			 */
			template <typename T> T load(T& t);

			/*!
			 * \fn template <typename T> std::string load(std::vector<T> t);
			 * \brief Template specification of load for vector of type T
			 * Support type that can >> to a stringstream
			 */
			template <typename T> std::vector<T> load(std::vector<T>& t);

			/*!
			 * \fn template <> bool load(bool& b);
			 * \brief Template specification of load for bool type
			 * Support "1", "0", or alpha version of boolean value,
			 * such as "True", "False", case-insensitive.
			 */
			//template <> bool load(bool& b);

			/*!
			 * \fn template <typename T> T load() { T t; return load(t); }
			 * \brief Template for load function with no input.
			 */
			template <typename T> T load() { T t; return load(t); }

		private:
			std::string str_;
		};

		/*!
		 * \brief The CfgLevel struct, internal struct for cfgParser.
		 * Tree structure for config sections
		 */
		struct CfgLevel
		{
			CfgLevel() : parent(nullptr), depth(0) {}
			CfgLevel(CfgLevel* p, std::size_t d) : parent(p), depth(d) {}

			using value_map_t = std::map<std::string, Value>;
			using section_map_t = std::map<std::string, CfgLevel>;

			value_map_t values;
			section_map_t sections;
			std::string prefix;
			CfgLevel* parent;
			size_t depth;

			Value operator[](const std::string& name) { return values[name]; }
			CfgLevel& operator()(const std::string& name) { return sections[name]; }
			std::string to_string()
			{
				std::string ret;
				for (auto v : values)
				{
					ret += prefix + v.first + " = " + v.second.str() + "\n";
				}
				for (auto s : sections)
				{
					ret += s.second.to_string();
				}
				return ret;
			}
		};

		/*!
		 * \brief The CfgParser class for INI/CFG file parsing
		 */
		class CfgParser
		{
		public:
			/*!
			 * \brief CfgParser constructor from filename
			 * \param filename
			 */
			CfgParser(std::string filename);

			/*!
			 * \brief CfgParser constructor from stream
			 * \param s
			 */
			CfgParser(std::istream& s) : pstream_(&s), ln_(0) { parse(root_); }

			/*!
			 * \brief Get root section of configuration
			 * \return Root level of config file
			 */
			CfgLevel& root() { return root_; }

			/*!
			 * \brief Overloaded operator [] for config values
			 * \param name Config value entry
			 * \return Value
			 */
			Value operator[](const std::string& name) { return root_.values[name]; }

			/*!
			 * \brief Overloaded operator () for config sections
			 * \param name Section entry.
			 * \return Section from given name.
			 */
			CfgLevel& operator()(const std::string& name) { return root_.sections[name]; }

		private:
			void parse(CfgLevel& lvl);
			CfgLevel* parse_section(std::string sline, CfgLevel *lvl);
			CfgLevel* parse_key_section(std::string& vline, CfgLevel *lvl);
			std::string split_key_value(std::string& line);
			void error_handler(std::string msg);

			CfgLevel		root_;
			std::ifstream	stream_;
			std::istream	*pstream_;
			std::string		line_;
			std::size_t		ln_;
		};

		/*!
		 * \brief The ArgOption class for ArgParser
		 */
		class ArgOption
		{
			friend class ArgParser;
		public:

			/*!
			 * \brief ArgOption constructor with short key and long key
			 * \param shortKey A char, use 'a'
			 * \param longKey A std::string "long"
			 */
			ArgOption(char shortKey, std::string longKey);

			/*!
			 * \brief ArgOption constructor with short key only
			 * \param shortKey A char, use 'a'
			 */
			ArgOption(char shortKey);

			/*!
			 * \brief ArgOption constructor with long key only
			 * \param longKey A std::string "long"
			 */
			ArgOption(std::string longKey);

			/*!
			 * \brief Set callback function when this option is triggered.
			 * One can use lambda function as callback
			 * \param todo Function callback
			 * \return Reference to this option
			 */
			ArgOption& call(std::function<void()> todo);

			/*!
			 * \brief Set callback functions.
			 * Both TODO function and Default function required.
			 * If ArgParser detect this option, the TODO function will be called.
			 * Otherwise, the default callback function will be executed.
			 * \param todo Callback function when triggered
			 * \param otherwise Default function if this option not found
			 * \return Reference to this option
			 */
			ArgOption& call(std::function<void()> todo, std::function<void()> otherwise);

			/*!
			 * \brief Set help info for this option.
			 * Use this to add description for this option.
			 * \param helpInfo
			 * \return Reference to this option
			 */
			ArgOption& set_help(std::string helpInfo);

			/*!
			 * \brief Set this option to required or not.
			 * After parsing arguments, if this set to required but not found,
			 * ArgParser will generate an error information.
			 * \param require Require this option if set to true
			 * \return Reference to this option
			 */
			ArgOption& require(bool require = true);

			/*!
			 * \brief Set this option to allow option be called only once.
			 * This is set to disable accidentally set a variable multiply times.
			 * \param onlyOnce Allow this option be used only once
			 * \return Reference to this option
			 */
			ArgOption& set_once(bool onlyOnce = true);

			/*!
			 * \brief Set option variable type.
			 * Optional. Let user know what type this option take, INT, FLOAT, STRING...
			 * \param type Type name in string, just a reminder.
			 * \return Reference to this option
			 */
			ArgOption& set_type(std::string type);

			/*!
			 * \brief Set minimum number of argument this option take.
			 * If minimum number not satisfied, ArgParser will generate an error.
			 * \param minCount
			 * \return Reference to this option
			 */
			ArgOption& set_min(int minCount);

			/*!
			 * \brief Set maximum number of argument this option take.
			 * If maximum number not satisfied, ArgParser will generate an error.
			 * \param maxCount
			 * \return Reference to this option
			 */
			ArgOption& set_max(int maxCount);

			/*!
			 * \brief Return the stored value
			 * \return Stored value
			 */
			Value get_value();

			/*!
			 * \brief Return the count
			 * \return Reference count
			 */
			int get_count();

		private:
			std::string get_help();			//!< get help string
			std::string get_short_help();	//!< get short help string

			char			shortKey_;	//!< short key eg. -h
			std::string		longKey_;	//!< long key eg. --help
			std::string		type_;		//!< INT, DOUBLE, STRING, FILE, whatever
			std::string		help_;		//!< detailed help info
			std::string		default_;	//!< default value
			bool			required_;	//!< not optional
			int				min_;		//!< min arguments for this option
			int				max_;		//!< max arguments for this option
			int				size_;		//!< stored argument size
			bool			once_;		//!< should not access multiply times
			int				count_;		//!< reference count
			Value			val_;		//!< stored values
			std::vector<std::function<void()>> callback_;	//!< call these when option found
			std::vector<std::function<void()>> othercall_;	//!< call these when option not found	
		};

		/*!
		 * \brief The ArgParser class.
		 * For parsing command line arguments.
		 */
		class ArgParser
		{
		public:
			/*!
			 * \brief ArgParser default constructor
			 */
			ArgParser();

			/*!
			 * \brief Add info to parser
			 * \param info Info to add
			 */
			void add_info(std::string info);

			/*!
			 * \brief Add option.
			 * Use -1 for shortkey if you don't want to specify short key.
			 * Use "" for longkey if you don't want to specify long key.
			 * \param shortKey A char
			 * \param longKey A std::string
			 * \return Reference to the added option
			 */
			ArgOption& add_opt(char shortKey, std::string longKey);

			/*!
			* \brief Add option, short version only.
			* \param shortKey A char
			* \return Reference to the added option
			*/
			ArgOption& add_opt(char shortKey);

			/*!
			* \brief Add option, long version only.
			* \param longKey A std::string
			* \return Reference to the added option
			*/
			ArgOption& add_opt(std::string longKey);

			/*!
			 * \fn template <typename T> ArgOption& add_opt_value(char shortKey, std::string longKey,
			 T& dst, T defaultValue, std::string help = "", std::string type = "");
			 * \brief Template function for an option take a value.
			 * This will store the value from argument to dst, otherwise dst = defaultValue.
			 * \param shortKey A char
			 * \param longKey A std::string
			 * \param dst Reference to the variable to be set
			 * \param defaultValue Default value if this option not found
			 * \param help Help description to this option
			 * \param type Help describe the type of this option
			 * \param min Minimum count of argument it takes
			 * \param max Maximum count of argument it takes, use -1 to allow unlimited arguments
			 * \return Reference to the added option
			 */
			template <typename T> ArgOption& add_opt_value(char shortKey, std::string longKey,
				T& dst, T defaultValue, std::string help = "", std::string type = "", int min = 1, int max = 1);

			/*!
			 * \brief Add an toggle option.
			 * When this option found, dst = true, otherwise dst = false.
			 * \param shortKey A char
			 * \param longKey A std::string
			 * \param help Help description.
			 * \param dst Pointer to a bool variable to be set accroding to this option.
			 * \return Reference to the added option
			 */
			ArgOption& add_opt_flag(char shortKey, std::string longKey, std::string help = "", bool* dst = nullptr);

			/*!
			 * \brief Add a special option used to display help information for argument parser.
			 * When triggered, program will print help info and exit.
			 * \param shortKey A char
			 * \param longKey A std::string
			 * \param help Option description
			 */
			void add_opt_help(char shortKey, std::string longKey, std::string help = "print this help and exit");

			/*!
			 * \brief Add a special option used to display version information for argument parser.
			 * When triggered, program will print version info and exit.
			 * \param shortKey A char
			 * \param longKey A std::string
			 * \param help Option description
			 */
			void add_opt_version(char shortKey, std::string longKey, std::string version, std::string help = "print version and exit");

			/*!
			 * \brief Return version info.
			 * \return Version info in string
			 */
			std::string version() const { return info_[1]; }

			/*!
			 * \brief Start parsing arguments
			 * \param argc
			 * \param argv
			 * \param ignoreUnknown Whether or not ignore unknown option keys
			 */
			void parse(int argc, char** argv, bool ignoreUnknown = false);

			/*!
			 * \brief Get error count generated during parsing
			 * \return Number of errors generated.
			 */
			std::size_t count_error() { return errors_.size(); }

			/*!
			 * \brief Count the occurance of the option by short key
			 * \param shortKey
			 * \return Occurance count
			 */
			int	count(char shortKey);

			/*!
			 * \brief Count the occurance of the option by long key
			 * \param longKey
			 * \return Occurance count
			 */
			int count(std::string longKey);

			/*!
			 * \brief Get all errors generated during parsing.
			 * \return Errors in string
			 */
			std::string get_error();

			/*!
			 * \brief Get help information of entire parser.
			 * \return All help information
			 */
			std::string get_help();

			/*!
			 * \brief Overloaded operator [] to retrieve the value by long key
			 * \param longKey
			 * \return Value type where the argument was stored
			 */
			Value operator[](const std::string& longKey);

			/*!
			 * \brief Overloaded operator [] to retrieve the value by short key
			 * \param shortKey
			 * \return Value type where the argument was stored
			 */
			Value operator[](const char shortKey);

			/*!
			* \brief Return all input arguments that does not belong to any option.
			* \return Arguments in vector
			*/
			std::vector<Value> arguments() const;

		private:
			//using ArgOptIter = std::vector<ArgOption>::iterator;
			enum class Type { SHORT_KEY, LONG_KEY, ARGUMENT, INVALID };
			using ArgQueue = std::vector<std::pair<std::string, Type>>;

			ArgOption& add_opt_internal(char shortKey, std::string longKey);
			void register_keys(char shortKey, std::string longKey, std::size_t pos);
			Type check_type(std::string str);
			ArgQueue pretty_arguments(int argc, char** argv);
			void error_option(std::string opt, std::string msg = "");
			void parse_option(ArgOption* ptr);
			void parse_value(ArgOption* ptr, const std::string& value);

			std::vector<ArgOption> options_;	//!< hooked options
			std::unordered_map<char, std::size_t> shortKeys_;		//!< short keys -a -b 
			std::unordered_map<std::string, std::size_t> longKeys_;	//!< long keys --long --version
			std::vector<Value>	args_;	//!< anything not belong to option will be stored here
			std::vector<std::string> errors_;	//!< store parsing errors
			std::vector<std::string> info_;	//!< program name[0] from argv[0], other infos from user
		};


		// implementations in cfg:: namespace

		template <typename T> inline std::string Value::store(T t)
		{
			std::ostringstream oss;
			oss << t;
			str_ = oss.str();
			return str_;
		}

		template <typename T> inline std::string Value::store(std::vector<T> t)
		{
			std::ostringstream oss;
			for (auto e : t)
			{
				oss << e << " ";
			}
			str_ = oss.str();
			return str_;
		}

		template <typename T> inline T Value::load(T& t)
		{
			std::istringstream iss(str_);
			iss >> t;
			return t;
		}

		template <> inline bool Value::load(bool& b)
		{
			b = false;
			std::string lowered = fmt::to_lower_ascii(str_);
			std::istringstream iss(lowered);
			iss >> std::boolalpha >> b;
			return b;
		}

		template <typename T> inline std::vector<T> Value::load(std::vector<T>& t)
		{
			std::istringstream iss(str_);
			t.clear();
			T val;
			std::string dummy;
			while (iss >> val || !iss.eof())
			{
				if (iss.fail())
				{
					iss.clear();
					iss >> dummy;
					continue;
				}
				t.push_back(val);
			}
			return t;
		}

		template <typename T> inline ArgOption& ArgParser::add_opt_value(char shortKey, std::string longKey,
			T& dst, T defaultValue, std::string help, std::string type, int min, int max)
		{
			dst = defaultValue;
			Value dfltValue;
			dfltValue.store(defaultValue);
			auto pos = options_.size();
			auto& opt = add_opt(shortKey, longKey).call([&, pos] {options_[pos].get_value().load(dst); });
			opt.default_ = dfltValue.str();
			return opt.set_help(help).set_type(type).set_min(min).set_max(max);
		}

	} // namespace cfg

	/*!
	 * \namespace zz::log
	 * \brief Namespace for logging and message stuffs
	 */
	namespace log
	{
		// \cond

		namespace detail
		{
			void zupply_internal_warn(std::string msg);
			void zupply_internal_error(std::string msg);

			class ScopedRedirect
			{
			public:
				ScopedRedirect(std::ostream &orig) : backup_(orig)
				{
					old = orig.rdbuf(buffer_.rdbuf());
				}

				~ScopedRedirect()
				{
					backup_.rdbuf(old);
					backup_ << buffer_.str();
				}

			private:
				ScopedRedirect(const ScopedRedirect&);
				ScopedRedirect& operator=(const ScopedRedirect&);

				std::ostream &backup_;
				std::streambuf *old;
				std::ostringstream buffer_;
			};
		}
		
		// \endcond

		/*!
		 * \brief The Progress bar class.
		 * For simple progress bar.
		 */
		class ProgBar
		{
		public:
			/*!
			 * \brief Constructor of ProgBar
			 * \param range Number of tasks to finish
			 * \param info Info message to show
			 */
			ProgBar(unsigned range = 100, std::string info = "");

			/*!
			 * \brief Destructor will stop background thread
			 */
			~ProgBar();

			/*!
			 * \brief Step forward for the progress
			 * \param size Step size, indicate how much work done in this step
			 */
			void step(unsigned size = 1);

			/*!
			 * \brief Stop the progress bar.
			 * Don't call this manully unless you know what u r doing.
			 * This will automatically be triggered when progress reach 100%.
			 */
			void stop();

		private:
			void start();
			void bg_work();
			void draw();
			void calc_rate(unsigned size);

			std::string				info_;
			unsigned				range_;
			std::atomic<unsigned>	pos_;
			std::thread				worker_;
			std::atomic_bool		running_;
			std::streambuf			*oldCout_;
			std::streambuf			*oldCerr_;
			std::ostream			ss_;
			std::ostringstream		buffer_;
			char					rate_[10];
			time::Timer				timer_;
		};

		// \cond
		typedef enum LogLevelEnum
		{
			trace = 0,
			debug = 1,
			info = 2,
			warn = 3,
			error = 4,
			fatal = 5,
			off = 6,
			sentinel = 63
		}LogLevels;

		namespace consts
		{
			static const char	*kLevelNames[] { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "OFF"};
			static const char	*kShortLevelNames[] { "T", "D", "I", "W", "E", "F", "O"};
			static const char	*kZupplyInternalLoggerName = "zupply";
			static const int	kAsyncQueueSize = 4096;	//!< asynchrous queue size, must be power of 2

			static const char	*kSinkDatetimeSpecifier = "%datetime";
			static const char	*kSinkLoggerNameSpecifier = "%logger";
			static const char	*kSinkThreadSpecifier = "%thread";
			static const char	*kSinkLevelSpecifier = "%level";
			static const char	*kSinkLevelShortSpecifier = "%lvl";
			static const char	*kSinkMessageSpecifier = "%msg";
			static const char	*kStdoutSinkName = "stdout";
			static const char	*kStderrSinkName = "stderr";
			static const char	*kSimplefileSinkType = "simplefile";
			static const char	*kRotatefileSinkType = "rotatefile";
			static const char	*kOstreamSinkType = "ostream";
			static const char	*kDefaultLoggerFormat = "[%datetime][T%thread][%logger][%level] %msg";
			static const char	*kDefaultLoggerDatetimeFormat = "%y-%m-%d %H:%M:%S.%frac";

			// config file formats
			static const char	*KConfigGlobalSectionSpecifier = "global";
			static const char	*KConfigLoggerSectionSpecifier = "loggers";
			static const char	*KConfigSinkSectionSpecifier = "sinks";
			static const char	*kConfigFormatSpecifier = "format";
			static const char	*kConfigLevelsSpecifier = "levels";
			static const char	*kConfigDateTimeFormatSpecifier = "datetime_format";
			static const char	*kConfigSinkListSpecifier = "sink_list";
			static const char	*kConfigSinkTypeSpecifier = "type";
			static const char	*kConfigSinkFilenameSpecifier = "filename";
		}

		// forward declaration
		namespace detail
		{
			struct LogMessage;
			class LineLogger;
			class SinkInterface;
		} // namespace detail
		typedef std::shared_ptr<detail::SinkInterface> SinkPtr;

		inline bool level_should_log(int levelMask, LogLevels lvl)
		{
			return (LogLevels::sentinel & levelMask & (1 << lvl)) > 0;
		}

		std::string level_mask_to_string(int levelMask);

		LogLevels level_from_str(std::string level);

		int level_mask_from_string(std::string levels);

		// \endcond

		/*!
		 * \brief The LogConfig class.
		 * For get/set logging configurations.
		 */
		class LogConfig
		{
		public:
			/*!
			 * \brief Get instance of LogConfig class.
			 * LogConfig is a singleton class, so use this to get the instance.
			 * \return Reference to instance.
			 */
			static LogConfig& instance();

			/*!
			 * \brief Set default format for all future loggers.
			 * \param format
			 */
			static void set_default_format(std::string format);

			/*!
			 * \brief Set default datetime format for all future loggers.
			 * \param dateFormat
			 */
			static void set_default_datetime_format(std::string dateFormat);

			/*!
			 * \brief Set default sink list for all future loggers.
			 * \param list A vector of strings storing the name of sinks
			 */
			static void set_default_sink_list(std::vector<std::string> list);

			/*!
			 * \brief Set default level mask for all future loggers.
			 * Level mask is an int.
			 * Each bit control if corresponding level should be logged or not.
			 * \param levelMask A bit mask.
			 */
			static void set_default_level_mask(int levelMask);

			/*!
			 * \brief Get default sink list
			 * \return std::vector of sink names
			 */
			std::vector<std::string> sink_list();

			/*!
			 * \brief Set default sink list
			 * \param list Vector of names of the sinks to be set
			 */
			void set_sink_list(std::vector<std::string> &list);

			/*!
			 * \brief Get default log level mask
			 * \return Integer representing levels using bit mask
			 */
			int log_level_mask();

			/*!
			 * \brief Set default log level mask
			 * \param newMask
			 */
			void set_log_level_mask(int newMask);

			/*!
			 * \brief Get default logger format
			 * \return Default format
			 */
			std::string format();

			/*!
			 * \brief Set default logger format
			 * \param newFormat
			 */
			void set_format(std::string newFormat);

			/*!
			 * \brief Get default datetime format
			 * \return Default datetime format
			 */
			std::string datetime_format();

			/*!
			 * \brief Set default datetime format
			 * \param newDatetimeFormat
			 */
			void set_datetime_format(std::string newDatetimeFormat);

		private:
			LogConfig();

			cds::lockbased::NonTrivialContainer<std::vector<std::string>> sinkList_;
			std::atomic_int logLevelMask_;
			cds::lockbased::NonTrivialContainer<std::string> format_;
			cds::lockbased::NonTrivialContainer<std::string> datetimeFormat_;
		};

		/*!
		 * \brief The Logger class
		 * Logger is the object to be called to log some message.
		 * Each logger may links to several Sinks as logging destinations.
		 */
		class Logger : UnMovable
		{
		public:
			/*!
			 * \brief Logger default constructor
			 * \param name Logger's name, need a name to retrieve the logger
			 */
			Logger(std::string name) : name_(name)
			{
				levelMask_ = LogConfig::instance().log_level_mask();
			}

			/*!
			 * \brief Logger constructor with name and mask
			 * \param name
			 * \param levelMask Specify which levels should be logged
			 */
			Logger(std::string name, int levelMask) : name_(name)
			{
				levelMask_ = levelMask;
			}

			// logger.info(format string, arg1, arg2, arg3, ...) call style
			/*!
			 * \fn template <typename... Args> detail::LineLogger trace(const char* fmt, const Args&... args)
			 * \brief Variadic argument style to log some message.
			 * Logger.trace(format string, arg1, arg2, arg3, ...) call style
			 */
			template <typename... Args> detail::LineLogger trace(const char* fmt, const Args&... args);

			/*!
			 * \fn template <typename... Args> detail::LineLogger debug(const char* fmt, const Args&... args)
			 * \brief Variadic argument style to log some message.
			 * Logger.debug(format string, arg1, arg2, arg3, ...) call style
			 */
			template <typename... Args> detail::LineLogger debug(const char* fmt, const Args&... args);

			/*!
			 * \fn template <typename... Args> detail::LineLogger info(const char* fmt, const Args&... args)
			 * \brief Variadic argument style to log some message.
			 * Logger.info(format string, arg1, arg2, arg3, ...) call style
			 */
			template <typename... Args> detail::LineLogger info(const char* fmt, const Args&... args);

			/*!
			 * \fn template <typename... Args> detail::LineLogger warn(const char* fmt, const Args&... args)
			 * \brief Variadic argument style to log some message.
			 * Logger.warn(format string, arg1, arg2, arg3, ...) call style
			 */
			template <typename... Args> detail::LineLogger warn(const char* fmt, const Args&... args);

			/*!
			 * \fn template <typename... Args> detail::LineLogger error(const char* fmt, const Args&... args)
			 * \brief Variadic argument style to log some message.
			 * Logger.error(format string, arg1, arg2, arg3, ...) call style
			 */
			template <typename... Args> detail::LineLogger error(const char* fmt, const Args&... args);

			/*!
			 * \fn template <typename... Args> detail::LineLogger fatal(const char* fmt, const Args&... args)
			 * \brief Variadic argument style to log some message.
			 * Logger.fatal(format string, arg1, arg2, arg3, ...) call style
			 */
			template <typename... Args> detail::LineLogger fatal(const char* fmt, const Args&... args);


			// logger.info(msg) << ".." call style
			/*!
			 * \fn template <typename T> detail::LineLogger trace(const T& msg)
			 * \brief Log message and overload with more stream style message.
			 * Logger.trace(msg) << ".." call style
			 */
			template <typename T> detail::LineLogger trace(const T& msg);

			/*!
			 * \fn template <typename T> detail::LineLogger debug(const T& msg)
			 * \brief Log message and overload with more stream style message.
			 * Logger.debug(msg) << ".." call style
			 */
			template <typename T> detail::LineLogger debug(const T& msg);

			/*!
			 * \fn template <typename T> detail::LineLogger info(const T& msg)
			 * \brief Log message and overload with more stream style message.
			 * Logger.info(msg) << ".." call style
			 */
			template <typename T> detail::LineLogger info(const T& msg);

			/*!
			 * \fn template <typename T> detail::LineLogger warn(const T& msg)
			 * \brief Log message and overload with more stream style message.
			 * Logger.warn(msg) << ".." call style
			 */
			template <typename T> detail::LineLogger warn(const T& msg);

			/*!
			 * \fn template <typename T> detail::LineLogger error(const T& msg)
			 * \brief Log message and overload with more stream style message.
			 * Logger.error(msg) << ".." call style
			 */
			template <typename T> detail::LineLogger error(const T& msg);

			/*!
			 * \fn template <typename T> detail::LineLogger fatal(const T& msg)
			 * \brief Log message and overload with more stream style message.
			 * Logger.fatal(msg) << ".." call style
			 */
			template <typename T> detail::LineLogger fatal(const T& msg);


			// logger.info() << ".." call  style

			/*!
			 * \brief Trace level overloaded with stream style message.
			 * \return LineLogger
			 */
			detail::LineLogger trace();

			/*!
			 * \brief Debug level overloaded with stream style message.
			 * \return LineLogger
			 */
			detail::LineLogger debug();

			/*!
			 * \brief Info level overloaded with stream style message.
			 * \return LineLogger
			 */
			detail::LineLogger info();

			/*!
			 * \brief Warn level overloaded with stream style message.
			 * \return LineLogger
			 */
			detail::LineLogger warn();

			/*!
			 * \brief Error level overloaded with stream style message.
			 * \return LineLogger
			 */
			detail::LineLogger error();

			/*!
			 * \brief Fatal level overloaded with stream style message.
			 * \return LineLogger
			 */
			detail::LineLogger fatal();

			//const LogLevels level() const
			//{
			//	return static_cast<LogLevels>(level_.load(std::memory_order_relaxed));
			//}

			/*!
			 * \brief Set level mask to this logger
			 * \param levelMask
			 */
			void set_level_mask(int levelMask)
			{
				levelMask_ = levelMask & LogLevels::sentinel;
			}

			/*!
			 * \brief Check if a specific level should be logged in this logger
			 * \param msgLevel
			 * \return True if the level should be logged
			 */
			bool should_log(LogLevels msgLevel) const
			{
				return level_should_log(levelMask_, msgLevel);
			}

			/*!
			 * \brief Get name of the logger
			 * \return Name of the logger
			 */
			std::string name() const { return name_; };

			/*!
			 * \brief Get user friendly information about this logger.
			 * Get informations such as log levels, sink list, etc...
			 * \return User friendly string info.
			 */
			std::string to_string();

			/*!
			 * \brief Get pointer to a sink by name.
			 * \param name Name of the sink
			 * \return Shared pointer to the sink, return nullptr if not found.
			 */
			SinkPtr get_sink(std::string name);

			/*!
			 * \brief Attach a sink to this logger.
			 * \param sink
			 */
			void attach_sink(SinkPtr sink);

			/*!
			 * \brief Detach a sink from this logger.
			 * \param sink
			 */
			void detach_sink(SinkPtr sink);

			/*!
			* \brief Detach all sinks from this logger.
			*/
			void detach_all_sinks();

			/*!
			 * \brief Attach the entire vector of sinks to the logger.
			 * \param sinkList
			 */
			void attach_sink_list(std::vector<std::string> &sinkList);

			/*!
			 * \brief Attach stdout and stderr to the logger.
			 * Special case of attach_sink()
			 */
			void attach_console();

			/*!
			 * \brief Detach stdout and stderr from the logger if exist.
			 * Special case of detach_sink()
			 */
			void detach_console();

		private:

			friend detail::LineLogger;

			detail::LineLogger log_if_enabled(LogLevels lvl);

			template <typename... Args>
			detail::LineLogger log_if_enabled(LogLevels lvl, const char* fmt, const Args&... args);

			template<typename T>
			detail::LineLogger log_if_enabled(LogLevels lvl, const T& msg);

			void log_msg(detail::LogMessage msg);

			std::string				name_;
			std::atomic_int			levelMask_;
			cds::lockbased::UnorderedMap<std::string, SinkPtr> sinks_;
		};
		typedef std::shared_ptr<Logger> LoggerPtr;


		// \cond
		namespace detail
		{
			template<typename T>
			class mpmc_bounded_queue
			{
			public:

				using item_type = T;
				mpmc_bounded_queue(size_t buffer_size)
					: buffer_(new cell_t[buffer_size]),
					buffer_mask_(buffer_size - 1)
				{
					//queue size must be power of two
					if (!((buffer_size >= 2) && ((buffer_size & (buffer_size - 1)) == 0)))
						throw ArgException("async logger queue size must be power of two");

					for (size_t i = 0; i != buffer_size; i += 1)
						buffer_[i].sequence_.store(i, std::memory_order_relaxed);
					enqueue_pos_.store(0, std::memory_order_relaxed);
					dequeue_pos_.store(0, std::memory_order_relaxed);
				}

				~mpmc_bounded_queue()
				{
					delete[] buffer_;
				}


				bool enqueue(T&& data)
				{
					cell_t* cell;
					size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
					for (;;)
					{
						cell = &buffer_[pos & buffer_mask_];
						size_t seq = cell->sequence_.load(std::memory_order_acquire);
						intptr_t dif = (intptr_t)seq - (intptr_t)pos;
						if (dif == 0)
						{
							if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
								break;
						}
						else if (dif < 0)
						{
							return false;
						}
						else
						{
							pos = enqueue_pos_.load(std::memory_order_relaxed);
						}
					}
					cell->data_ = std::move(data);
					cell->sequence_.store(pos + 1, std::memory_order_release);
					return true;
				}

				bool dequeue(T& data)
				{
					cell_t* cell;
					size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
					for (;;)
					{
						cell = &buffer_[pos & buffer_mask_];
						size_t seq =
							cell->sequence_.load(std::memory_order_acquire);
						intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
						if (dif == 0)
						{
							if (dequeue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
								break;
						}
						else if (dif < 0)
							return false;
						else
							pos = dequeue_pos_.load(std::memory_order_relaxed);
					}
					data = std::move(cell->data_);
					cell->sequence_.store(pos + buffer_mask_ + 1, std::memory_order_release);
					return true;
				}

			private:
				struct cell_t
				{
					std::atomic<size_t>   sequence_;
					T                     data_;
				};

				static size_t const     cacheline_size = 64;
				typedef char            cacheline_pad_t[cacheline_size];

				cacheline_pad_t         pad0_;
				cell_t* const           buffer_;
				size_t const            buffer_mask_;
				cacheline_pad_t         pad1_;
				std::atomic<size_t>     enqueue_pos_;
				cacheline_pad_t         pad2_;
				std::atomic<size_t>     dequeue_pos_;
				cacheline_pad_t         pad3_;

				mpmc_bounded_queue(mpmc_bounded_queue const&);
				void operator = (mpmc_bounded_queue const&);
			};

			struct LogMessage
			{
				std::string			loggerName_;
				LogLevels			level_;
				time::DateTime			dateTime_;
				size_t				threadId_;
				std::string			buffer_;
			};

			class LineLogger : private UnCopyable
			{
			public:
				LineLogger(Logger* callbacker, LogLevels lvl, bool enable) :callbackLogger_(callbacker), enabled_(enable)
				{
					msg_.level_ = lvl;
				}

				LineLogger(LineLogger&& other) :
					callbackLogger_(other.callbackLogger_),
					msg_(std::move(other.msg_)),
					enabled_(other.enabled_)
				{
					other.disable();
				}

				~LineLogger()
				{
					if (enabled_)
					{
						msg_.loggerName_ = callbackLogger_->name();
						msg_.dateTime_ = time::DateTime::local_time();
						msg_.threadId_ = os::thread_id();
						callbackLogger_->log_msg(msg_);
					}
				}

				LineLogger& operator=(LineLogger&&) = delete;

				void write(const char* what)
				{
					if (enabled_)
						msg_.buffer_ += what;
				}

				template <typename... Args>
				void write(const char* fmt, const Args&... args)
				{
					if (!enabled_) return;
					std::string buf(fmt);
					fmt::format_string(buf, args...);
					msg_.buffer_ += buf;
				}


				LineLogger& operator<<(const char* what)
				{
					if (enabled_) msg_.buffer_ += what;
					return *this;
				}

				LineLogger& operator<<(const std::string& what)
				{
					if (enabled_) msg_.buffer_ += what;
					return *this;
				}

				template<typename T>
				LineLogger& operator<<(const T& what)
				{
					if (enabled_)
					{
						std::ostringstream oss;
						oss << std::dec << what;
						msg_.buffer_ += oss.str();
					}
					return *this;
				}

				void disable()
				{
					enabled_ = false;
				}

				bool is_enabled() const
				{
					return enabled_;
				}
			private:
				Logger			*callbackLogger_;
				LogMessage		msg_;
				bool			enabled_;
			};

			class SinkInterface
			{
			public:
				virtual ~SinkInterface() {};
				virtual void log(const LogMessage& msg) = 0;
				virtual void flush() = 0;
				virtual std::string name() const = 0;
				virtual std::string to_string() const = 0;
				virtual void set_level_mask(int levelMask) = 0;
				virtual void set_format(const std::string &fmt) = 0;
			};

			// Due to a bug in VC12, thread join in static object dtor
			// will cause deadlock. TODO: implement async sink when
			// VS2015 ready.
			class AsyncSink : public SinkInterface, private UnCopyable
			{

			};

			class Sink : public SinkInterface, private UnCopyable
			{
			public:
				Sink()
				{
					levelMask_ = 0x3F & LogConfig::instance().log_level_mask();
					set_format(LogConfig::instance().format());
				};

				virtual ~Sink()  {};

				void log(const LogMessage& msg) override
				{
					if (!level_should_log(levelMask_, msg.level_)) return;
					std::string finalMessage = format_message(msg);
					// mutex for multi-thread race, actually vc++ and gnu++ are ok without lock
					// but this behavior is not guanranteed, and libc++ will have corrupt output
					std::lock_guard<std::mutex> lock(mutex_);
					sink_it(finalMessage);
				}

				void set_level_mask(int levelMask) override
				{
					levelMask_ = levelMask & LogLevels::sentinel;
				}

				int level_mask() const
				{
					return levelMask_;
				}

				void set_format(const std::string &format) override
				{
					format_.set(format);
				}

				virtual void sink_it(const std::string &finalMsg) = 0;

			private:
				std::string format_message(const LogMessage &msg)
				{
					std::string ret(format_.get());
					auto dt = msg.dateTime_;
					fmt::replace_all_with_escape(ret, consts::kSinkDatetimeSpecifier, dt.to_string(LogConfig::instance().datetime_format().c_str()));
					fmt::replace_all_with_escape(ret, consts::kSinkLoggerNameSpecifier, msg.loggerName_);
					fmt::replace_all_with_escape(ret, consts::kSinkThreadSpecifier, std::to_string(msg.threadId_));
					fmt::replace_all_with_escape(ret, consts::kSinkLevelSpecifier, consts::kLevelNames[msg.level_]);
					fmt::replace_all_with_escape(ret, consts::kSinkLevelShortSpecifier, consts::kShortLevelNames[msg.level_]);
					fmt::replace_all_with_escape(ret, consts::kSinkMessageSpecifier, msg.buffer_);
					// make sure new line
					if (!fmt::ends_with(ret, "\n")) ret += os::endl();
					return ret;
				}

				std::atomic_int		levelMask_;
				std::mutex			mutex_;
				cds::lockbased::NonTrivialContainer<std::string>		format_;
			};

			class SimpleFileSink : public Sink
			{
			public:
				SimpleFileSink(const std::string filename, bool truncate) :fileEditor_(filename, truncate)
				{
				}

				void flush() override
				{
					fileEditor_.flush();
				}

				void sink_it(const std::string &finalMsg) override
				{
					fileEditor_ << finalMsg;
				}

				std::string name() const override
				{
					return fileEditor_.filename();
				}

				std::string to_string() const override
				{
					return "SimpleFileSink->" + name() + " " + level_mask_to_string(level_mask());
				}

			private:
				fs::FileEditor fileEditor_;
			};

			class RotateFileSink : public Sink
			{
			public:
				RotateFileSink(const std::string filename, std::size_t maxSizeInByte, bool backup);

				void flush() override
				{
					fileEditor_.flush();
				}

				void sink_it(const std::string &finalMsg) override
				{
					currentSize_ += finalMsg.length();
					if (currentSize_ > maxSizeInByte_)
					{
						rotate();
					}
					fileEditor_ << finalMsg;
				}

				std::string name() const override
				{
					return fileEditor_.filename();
				}

				std::string to_string() const override
				{
					return "RotateFileSink->" + name() + " " + level_mask_to_string(level_mask());
				}

			private:
				void back_up(std::string oldFile);
				void rotate();

				fs::FileEditor				fileEditor_;
				std::mutex					mutex_;
				std::size_t					maxSizeInByte_;
				std::atomic<std::size_t>	currentSize_;
				bool						backup_;
			};

			class OStreamSink : public Sink
			{
			public:
				explicit OStreamSink(std::ostream& os, const char *name, bool forceFlush = false)
					:ostream_(os), name_(name), forceFlush_(forceFlush)
				{
				}

				std::string name() const override
				{
					return name_;
				}

				std::string to_string() const override
				{
					return "OStreamSink->" + name() + " " + level_mask_to_string(level_mask());
				}

			private:
				void sink_it(const std::string &finalMsg) override
				{
					ostream_ << finalMsg;
					if (forceFlush_) ostream_.flush();
				}

				void flush() override
				{
					ostream_.flush();
				}

				std::ostream&	ostream_;
				std::string		name_;
				bool			forceFlush_;
			};

			class StdoutSink : public OStreamSink
			{
			public:
				StdoutSink() : OStreamSink(std::cout, consts::kStdoutSinkName, true)
				{
					// stdout by design should not receive warning and error message
					// so add cout and cerr together and filter by level is a better idea
					set_level_mask(0x07 & LogConfig::instance().log_level_mask());
				}
				static std::shared_ptr<StdoutSink> instance()
				{
					static std::shared_ptr<StdoutSink> instance = std::make_shared<StdoutSink>();
					return instance;
				}

				std::string to_string() const override
				{
					return "StdoutSink->" + name() + " " + level_mask_to_string(level_mask());
				}
			};

			class StderrSink : public OStreamSink
			{
			public:
				StderrSink() : OStreamSink(std::cerr, consts::kStderrSinkName, true)
				{
					// stderr by design should only log error/warning msg
					set_level_mask(0x38 & LogConfig::instance().log_level_mask());
				}
				static std::shared_ptr<StderrSink> instance()
				{
					static std::shared_ptr<StderrSink> instance = std::make_shared<StderrSink>();
					return instance;
				}

				std::string to_string() const override
				{
					return "StderrSink->" + name() + " " + level_mask_to_string(level_mask());
				}
			};

			class LoggerRegistry : UnMovable
			{
			public:
				static LoggerRegistry& instance();
				LoggerPtr create(const std::string &name);
				LoggerPtr ensure_get(std::string &name);
				LoggerPtr get(std::string &name);
				std::vector<LoggerPtr> get_all();
				void drop(const std::string &name);
				void drop_all();
				void lock();
				void unlock();
				bool is_locked() const;

			private:
				LoggerRegistry(){ lock_ = false; }

				LoggerPtr new_registry(const std::string &name);

				cds::lockbased::UnorderedMap<std::string, LoggerPtr> loggers_;
				std::atomic_bool	lock_;
			};
		} // namespace detail

		inline detail::LineLogger Logger::log_if_enabled(LogLevels lvl)
		{
			detail::LineLogger l(this, lvl, should_log(lvl));
			return l;
		}

		template <typename... Args>
		inline detail::LineLogger Logger::log_if_enabled(LogLevels lvl, const char* fmt, const Args&... args)
		{
			detail::LineLogger l(this, lvl, should_log(lvl));
			l.write(fmt, args...);
			return l;
		}

		template<typename T>
		inline detail::LineLogger Logger::log_if_enabled(LogLevels lvl, const T& msg)
		{
			detail::LineLogger l(this, lvl, should_log(lvl));
			l.write(msg);
			return l;
		}

		// logger.info(format string, arg1, arg2, arg3, ...) call style
		template <typename... Args>
		inline detail::LineLogger Logger::trace(const char* fmt, const Args&... args)
		{
			return log_if_enabled(LogLevels::trace, fmt, args...);
		}
		template <typename... Args>
		inline detail::LineLogger Logger::debug(const char* fmt, const Args&... args)
		{
			return log_if_enabled(LogLevels::debug, fmt, args...);
		}
		template <typename... Args>
		inline detail::LineLogger Logger::info(const char* fmt, const Args&... args)
		{
			return log_if_enabled(LogLevels::info, fmt, args...);
		}
		template <typename... Args>
		inline detail::LineLogger Logger::warn(const char* fmt, const Args&... args)
		{
			return log_if_enabled(LogLevels::warn, fmt, args...);
		}
		template <typename... Args>
		inline detail::LineLogger Logger::error(const char* fmt, const Args&... args)
		{
			return log_if_enabled(LogLevels::error, fmt, args...);
		}
		template <typename... Args>
		inline detail::LineLogger Logger::fatal(const char* fmt, const Args&... args)
		{
			return log_if_enabled(LogLevels::fatal, fmt, args...);
		}


		// logger.info(msg) << ".." call style
		template <typename T>
		inline detail::LineLogger Logger::trace(const T& msg)
		{
			return log_if_enabled(LogLevels::trace, msg);
		}
		template <typename T>
		inline detail::LineLogger Logger::debug(const T& msg)
		{
			return log_if_enabled(LogLevels::debug, msg);
		}
		template <typename T>
		inline detail::LineLogger Logger::info(const T& msg)
		{
			return log_if_enabled(LogLevels::info, msg);
		}
		template <typename T>
		inline detail::LineLogger Logger::warn(const T& msg)
		{
			return log_if_enabled(LogLevels::warn, msg);
		}
		template <typename T>
		inline detail::LineLogger Logger::error(const T& msg)
		{
			return log_if_enabled(LogLevels::error, msg);
		}
		template <typename T>
		inline detail::LineLogger Logger::fatal(const T& msg)
		{
			return log_if_enabled(LogLevels::fatal, msg);
		}

		// \endcond

		/*!
		 * \brief Get logger by name
		 * \param name
		 * \param createIfNotExists Create new logger with the name provided if not exist.
		 * \return Shared pointer to the logger.
		 */
		LoggerPtr get_logger(std::string name, bool createIfNotExists = true);

		/*!
		 * \brief Get the sink pointer to stdout
		 * \return Shared pointer to stdout sink.
		 */
		SinkPtr new_stdout_sink();

		/*!
		 * \brief Get the sink pointer to stderr
		 * \return Shared pointer to stderr sink.
		 */
		SinkPtr new_stderr_sink();

		/*!
		 * \brief Get the sink pointer from name provided.
		 * Return nullptr if sink does not exist.
		 * \return Shared pointer to sink.
		 */
		SinkPtr get_sink(std::string name);

		/*!
		 * \brief Dump all loggers info.
		 * \param out The stream where you want the info direct to, default std::cout
		 */
		void dump_loggers(std::ostream &out = std::cout);

		/*!
		 * \brief Create new ostream sink from existing ostream.
		 * \param stream
		 * \param name
		 * \param forceFlush
		 * \return Shared pointer to the new sink.
		 */
		SinkPtr new_ostream_sink(std::ostream &stream, std::string name, bool forceFlush = false);

		/*!
		 * \brief Create new simple file sink withe filename provided.
		 * \param filename
		 * \param truncate Open the file in truncate mode?
		 * \return Shared pointer to the new sink.
		 */
		SinkPtr new_simple_file_sink(std::string filename, bool truncate = false);

		/*!
		 * \brief Create new rotate file sink.
		 * \param filename
		 * \param maxSizeInByte Maximum size in byte, sink will truncate the file and rewrite new content if exceed this size.
		 * \param backupOld Whether keep backups of the old files.
		 * \return Shared pointer to the new sink
		 */
		SinkPtr new_rotate_file_sink(std::string filename, std::size_t maxSizeInByte = 4194304, bool backupOld = false);

		/*!
		 * \brief Lock all loggers.
		 * When locked, loggers cannot be modified.
		 */
		void lock_loggers();

		/*!
		 * \brief Unlock all loggers.
		 * When locked, loggers cannot be modified.
		 */
		void unlock_loggers();

		/*!
		 * \brief Delete a logger with the name given.
		 * \param name
		 */
		void drop_logger(std::string name);

		/*!
		 * \brief Delete all loggers.
		 */
		void drop_all_loggers();

		/*!
		 * \brief Delete the sink, detach it from every logger.
		 * \param name
		 */
		void drop_sink(std::string name);

		// \cond
		namespace detail
		{
			void sink_list_revise(std::vector<std::string> &list, std::map<std::string, std::string> &map);

			void config_loggers_from_section(cfg::CfgLevel::section_map_t &section, std::map<std::string, std::string> &map);

			LoggerPtr get_hidden_logger();

			std::map<std::string, std::string> config_sinks_from_section(cfg::CfgLevel::section_map_t &section);


			void zupply_internal_warn(std::string msg);

			void zupply_internal_error(std::string msg);

			void config_from_parser(cfg::CfgParser& parser);
		} // namespace detail
		// \endcond

		/*!
		 * \brief Config loggers from a config file
		 * \param cfgFilename
		 */
		void config_from_file(std::string cfgFilename);

		/*!
		* \brief Config loggers from a stringstream
		* \param ss Stringstream where configs stored
		*/
		void config_from_stringstream(std::stringstream& ss);
	} // namespace log

	// \cond
	/////////////// implementations ////////////////
	/////////////// saturate_cast (used in image & signal processing) ///////////////////

	template<typename _Tp> static inline _Tp saturate_cast(unsigned char v)    { return _Tp(v); }
	template<typename _Tp> static inline _Tp saturate_cast(char v)    { return _Tp(v); }
	template<typename _Tp> static inline _Tp saturate_cast(unsigned short v)   { return _Tp(v); }
	template<typename _Tp> static inline _Tp saturate_cast(short v)    { return _Tp(v); }
	template<typename _Tp> static inline _Tp saturate_cast(unsigned v) { return _Tp(v); }
	template<typename _Tp> static inline _Tp saturate_cast(int v)      { return _Tp(v); }
	template<typename _Tp> static inline _Tp saturate_cast(float v)    { return _Tp(v); }
	template<typename _Tp> static inline _Tp saturate_cast(double v)   { return _Tp(v); }

	template<> inline unsigned char saturate_cast<unsigned char>(char v)        { return (unsigned char)(std::max)((int)v, 0); }
	template<> inline unsigned char saturate_cast<unsigned char>(unsigned char v)       { return (unsigned char)(std::min)((unsigned)v, (unsigned)UCHAR_MAX); }
	template<> inline unsigned char saturate_cast<unsigned char>(int v)          { return (unsigned char)((unsigned)v <= UCHAR_MAX ? v : v > 0 ? UCHAR_MAX : 0); }
	template<> inline unsigned char saturate_cast<unsigned char>(short v)        { return saturate_cast<unsigned char>((int)v); }
	template<> inline unsigned char saturate_cast<unsigned char>(unsigned v)     { return (unsigned char)(std::min)(v, (unsigned)UCHAR_MAX); }
	template<> inline unsigned char saturate_cast<unsigned char>(float v)        { int iv = math::round(v); return saturate_cast<unsigned char>(iv); }
	template<> inline unsigned char saturate_cast<unsigned char>(double v)       { int iv = math::round(v); return saturate_cast<unsigned char>(iv); }

	template<> inline char saturate_cast<char>(unsigned char v)        { return (char)(std::min)((int)v, SCHAR_MAX); }
	template<> inline char saturate_cast<char>(unsigned short v)       { return (char)(std::min)((unsigned)v, (unsigned)SCHAR_MAX); }
	template<> inline char saturate_cast<char>(int v)          { return (char)((unsigned)(v - SCHAR_MIN) <= (unsigned)UCHAR_MAX ? v : v > 0 ? SCHAR_MAX : SCHAR_MIN); }
	template<> inline char saturate_cast<char>(short v)        { return saturate_cast<char>((int)v); }
	template<> inline char saturate_cast<char>(unsigned int v)     { return (char)(std::min)(v, (unsigned)SCHAR_MAX); }
	template<> inline char saturate_cast<char>(float v)        { int iv = math::round(v); return saturate_cast<char>(iv); }
	template<> inline char saturate_cast<char>(double v)       { int iv = math::round(v); return saturate_cast<char>(iv); }

	template<> inline unsigned short saturate_cast<unsigned short>(char v)      { return (unsigned short)(std::max)((int)v, 0); }
	template<> inline unsigned short saturate_cast<unsigned short>(short v)      { return (unsigned short)(std::max)((int)v, 0); }
	template<> inline unsigned short saturate_cast<unsigned short>(int v)        { return (unsigned short)((unsigned)v <= (unsigned)USHRT_MAX ? v : v > 0 ? USHRT_MAX : 0); }
	template<> inline unsigned short saturate_cast<unsigned short>(unsigned v)   { return (unsigned short)(std::min)(v, (unsigned)USHRT_MAX); }
	template<> inline unsigned short saturate_cast<unsigned short>(float v)      { int iv = math::round(v); return saturate_cast<unsigned short>(iv); }
	template<> inline unsigned short saturate_cast<unsigned short>(double v)     { int iv = math::round(v); return saturate_cast<unsigned short>(iv); }

	template<> inline short saturate_cast<short>(unsigned short v)       { return (short)(std::min)((int)v, SHRT_MAX); }
	template<> inline short saturate_cast<short>(int v)          { return (short)((unsigned)(v - SHRT_MIN) <= (unsigned)USHRT_MAX ? v : v > 0 ? SHRT_MAX : SHRT_MIN); }
	template<> inline short saturate_cast<short>(unsigned v)     { return (short)(std::min)(v, (unsigned)SHRT_MAX); }
	template<> inline short saturate_cast<short>(float v)        { int iv = math::round(v); return saturate_cast<short>(iv); }
	template<> inline short saturate_cast<short>(double v)       { int iv = math::round(v); return saturate_cast<short>(iv); }

	template<> inline int saturate_cast<int>(float v)            { return math::round(v); }
	template<> inline int saturate_cast<int>(double v)           { return math::round(v); }

	// we intentionally do not clip negative numbers, to make -1 become 0xffffffff etc.
	template<> inline unsigned saturate_cast<unsigned>(float v)  { return (unsigned)math::round(v); }
	template<> inline unsigned saturate_cast<unsigned>(double v) { return (unsigned)math::round(v); }

	////////////////////////////////// Size /////////////////////////////////
	template<typename _Tp> inline
		Size_<_Tp>::Size_()
		: width(0), height(0) {}

	template<typename _Tp> inline
		Size_<_Tp>::Size_(_Tp _width, _Tp _height)
		: width(_width), height(_height) {}

	template<typename _Tp> inline
		Size_<_Tp>::Size_(const Size_& sz)
		: width(sz.width), height(sz.height) {}

	template<typename _Tp> template<typename _Tp2> inline
		Size_<_Tp>::operator Size_<_Tp2>() const
	{
			return Size_<_Tp2>(saturate_cast<_Tp2>(width), saturate_cast<_Tp2>(height));
		}

	template<typename _Tp> inline
		Size_<_Tp>& Size_<_Tp>::operator = (const Size_<_Tp>& sz)
	{
			width = sz.width; height = sz.height;
			return *this;
		}

	template<typename _Tp> inline
		bool Size_<_Tp>::operator == (const Size_<_Tp>& sz)
	{
			return (width == sz.width) && (height == sz.height);
		}

	template<typename _Tp> inline
		bool Size_<_Tp>::operator != (const Size_<_Tp>& sz)
	{
			return (width != sz.width) || (height != sz.height);
		}

	template<typename _Tp> inline
		_Tp Size_<_Tp>::area() const
	{
			return width * height;
		}

	template<typename _Tp> static inline
		Size_<_Tp>& operator *= (Size_<_Tp>& a, _Tp b)
	{
			a.width *= b;
			a.height *= b;
			return a;
		}

	template<typename _Tp> static inline
		Size_<_Tp> operator * (const Size_<_Tp>& a, _Tp b)
	{
			Size_<_Tp> tmp(a);
			tmp *= b;
			return tmp;
		}

	template<typename _Tp> static inline
		Size_<_Tp>& operator /= (Size_<_Tp>& a, _Tp b)
	{
			a.width /= b;
			a.height /= b;
			return a;
		}

	template<typename _Tp> static inline
		Size_<_Tp> operator / (const Size_<_Tp>& a, _Tp b)
	{
			Size_<_Tp> tmp(a);
			tmp /= b;
			return tmp;
		}

	template<typename _Tp> static inline
		Size_<_Tp>& operator += (Size_<_Tp>& a, const Size_<_Tp>& b)
	{
			a.width += b.width;
			a.height += b.height;
			return a;
		}

	template<typename _Tp> static inline
		Size_<_Tp> operator + (const Size_<_Tp>& a, const Size_<_Tp>& b)
	{
			Size_<_Tp> tmp(a);
			tmp += b;
			return tmp;
		}

	template<typename _Tp> static inline
		Size_<_Tp>& operator -= (Size_<_Tp>& a, const Size_<_Tp>& b)
	{
			a.width -= b.width;
			a.height -= b.height;
			return a;
		}

	template<typename _Tp> static inline
		Size_<_Tp> operator - (const Size_<_Tp>& a, const Size_<_Tp>& b)
	{
			Size_<_Tp> tmp(a);
			tmp -= b;
			return tmp;
		}

	template<typename _Tp> static inline
		bool operator == (const Size_<_Tp>& a, const Size_<_Tp>& b)
	{
			return a.width == b.width && a.height == b.height;
		}

	template<typename _Tp> static inline
		bool operator != (const Size_<_Tp>& a, const Size_<_Tp>& b)
	{
			return !(a == b);
		}

	//////////////////////////////// 2D Point ///////////////////////////////
	template<typename _Tp> inline
		Point_<_Tp>::Point_()
		: x(0), y(0) {}

	template<typename _Tp> inline
		Point_<_Tp>::Point_(_Tp _x, _Tp _y)
		: x(_x), y(_y) {}

	template<typename _Tp> inline
		Point_<_Tp>::Point_(const Point_& pt)
		: x(pt.x), y(pt.y) {}

	/// operator override
	template<typename _Tp> inline
		Point_<_Tp>& Point_<_Tp>::operator = (const Point_& pt)
	{
			x = pt.x; y = pt.y;
			return *this;
		}

	template<typename _Tp> template<typename _Tp2> inline
		Point_<_Tp>::operator Point_<_Tp2>() const
	{
			return Point_<_Tp2>(saturate_cast<_Tp2>(x), saturate_cast<_Tp2>(y));
		}

	template<typename _Tp> static inline
		Point_<_Tp>& operator += (Point_<_Tp>& a, const Point_<_Tp>& b)
	{
			a.x += b.x;
			a.y += b.y;
			return a;
		}

	template<typename _Tp> static inline
		Point_<_Tp>& operator -= (Point_<_Tp>& a, const Point_<_Tp>& b)
	{
			a.x -= b.x;
			a.y -= b.y;
			return a;
		}

	template<typename _Tp> static inline
		Point_<_Tp>& operator *= (Point_<_Tp>& a, int b)
	{
			a.x = saturate_cast<_Tp>(a.x * b);
			a.y = saturate_cast<_Tp>(a.y * b);
			return a;
		}

	template<typename _Tp> static inline
		Point_<_Tp>& operator *= (Point_<_Tp>& a, float b)
	{
			a.x = saturate_cast<_Tp>(a.x * b);
			a.y = saturate_cast<_Tp>(a.y * b);
			return a;
		}

	template<typename _Tp> static inline
		Point_<_Tp>& operator *= (Point_<_Tp>& a, double b)
	{
			a.x = saturate_cast<_Tp>(a.x * b);
			a.y = saturate_cast<_Tp>(a.y * b);
			return a;
		}

	template<typename _Tp> static inline
		Point_<_Tp>& operator /= (Point_<_Tp>& a, int b)
	{
			a.x = saturate_cast<_Tp>(a.x / b);
			a.y = saturate_cast<_Tp>(a.y / b);
			return a;
		}

	template<typename _Tp> static inline
		Point_<_Tp>& operator /= (Point_<_Tp>& a, float b)
	{
			a.x = saturate_cast<_Tp>(a.x / b);
			a.y = saturate_cast<_Tp>(a.y / b);
			return a;
		}

	template<typename _Tp> static inline
		Point_<_Tp>& operator /= (Point_<_Tp>& a, double b)
	{
			a.x = saturate_cast<_Tp>(a.x / b);
			a.y = saturate_cast<_Tp>(a.y / b);
			return a;
		}

	template<typename _Tp> static inline
		bool operator == (const Point_<_Tp>& a, const Point_<_Tp>& b)
	{
			return a.x == b.x && a.y == b.y;
		}

	template<typename _Tp> static inline
		bool operator != (const Point_<_Tp>& a, const Point_<_Tp>& b)
	{
			return a.x != b.x || a.y != b.y;
		}

	template<typename _Tp> static inline
		Point_<_Tp> operator + (const Point_<_Tp>& a, const Point_<_Tp>& b)
	{
			return Point_<_Tp>(saturate_cast<_Tp>(a.x + b.x), saturate_cast<_Tp>(a.y + b.y));
		}

	template<typename _Tp> static inline
		Point_<_Tp> operator - (const Point_<_Tp>& a, const Point_<_Tp>& b)
	{
			return Point_<_Tp>(saturate_cast<_Tp>(a.x - b.x), saturate_cast<_Tp>(a.y - b.y));
		}

	template<typename _Tp> static inline
		Point_<_Tp> operator - (const Point_<_Tp>& a)
	{
			return Point_<_Tp>(saturate_cast<_Tp>(-a.x), saturate_cast<_Tp>(-a.y));
		}

	template<typename _Tp> static inline
		Point_<_Tp> operator * (const Point_<_Tp>& a, int b)
	{
			return Point_<_Tp>(saturate_cast<_Tp>(a.x*b), saturate_cast<_Tp>(a.y*b));
		}

	template<typename _Tp> static inline
		Point_<_Tp> operator * (int a, const Point_<_Tp>& b)
	{
			return Point_<_Tp>(saturate_cast<_Tp>(b.x*a), saturate_cast<_Tp>(b.y*a));
		}

	template<typename _Tp> static inline
		Point_<_Tp> operator * (const Point_<_Tp>& a, float b)
	{
			return Point_<_Tp>(saturate_cast<_Tp>(a.x*b), saturate_cast<_Tp>(a.y*b));
		}

	template<typename _Tp> static inline
		Point_<_Tp> operator * (float a, const Point_<_Tp>& b)
	{
			return Point_<_Tp>(saturate_cast<_Tp>(b.x*a), saturate_cast<_Tp>(b.y*a));
		}

	template<typename _Tp> static inline
		Point_<_Tp> operator * (const Point_<_Tp>& a, double b)
	{
			return Point_<_Tp>(saturate_cast<_Tp>(a.x*b), saturate_cast<_Tp>(a.y*b));
		}

	template<typename _Tp> static inline
		Point_<_Tp> operator * (double a, const Point_<_Tp>& b)
	{
			return Point_<_Tp>(saturate_cast<_Tp>(b.x*a), saturate_cast<_Tp>(b.y*a));
		}


	template<typename _Tp> static inline
		Point_<_Tp> operator / (const Point_<_Tp>& a, int b)
	{
			Point_<_Tp> tmp(a);
			tmp /= b;
			return tmp;
		}

	template<typename _Tp> static inline
		Point_<_Tp> operator / (const Point_<_Tp>& a, float b)
	{
			Point_<_Tp> tmp(a);
			tmp /= b;
			return tmp;
		}

	template<typename _Tp> static inline
		Point_<_Tp> operator / (const Point_<_Tp>& a, double b)
	{
			Point_<_Tp> tmp(a);
			tmp /= b;
			return tmp;
		}

	template<typename _Tp> inline
		_Tp Point_<_Tp>::dot(const Point_& pt) const
	{
			return saturate_cast<_Tp>(x*pt.x + y*pt.y);
		}

	template<typename _Tp> inline
		double Point_<_Tp>::ddot(const Point_& pt) const
	{
			return (double)x*pt.x + (double)y*pt.y;
		}

	template<typename _Tp> inline
		double Point_<_Tp>::cross(const Point_& pt) const
	{
			return (double)x*pt.y - (double)y*pt.x;
		}

	template<typename _Tp> inline bool
		Point_<_Tp>::inside(const Rect_<_Tp>& r) const
	{
			return r.contains(*this);
		}

	////////////////////////////////// Rect /////////////////////////////////

	template<typename _Tp> inline
		Rect_<_Tp>::Rect_()
		: x(0), y(0), width(0), height(0) {}

	template<typename _Tp> inline
		Rect_<_Tp>::Rect_(_Tp _x, _Tp _y, _Tp _width, _Tp _height)
		: x(_x), y(_y), width(_width), height(_height) {}

	template<typename _Tp> inline
		Rect_<_Tp>::Rect_(const Rect_<_Tp>& r)
		: x(r.x), y(r.y), width(r.width), height(r.height) {}

	template<typename _Tp> inline
		Rect_<_Tp>::Rect_(const Point_<_Tp>& org, const Size_<_Tp>& sz)
		: x(org.x), y(org.y), width(sz.width), height(sz.height) {}

	template<typename _Tp> inline
		Rect_<_Tp>::Rect_(const Point_<_Tp>& pt1, const Point_<_Tp>& pt2)
	{
			x = (std::min)(pt1.x, pt2.x);
			y = (std::min)(pt1.y, pt2.y);
			width = (std::max)(pt1.x, pt2.x) - x;
			height = (std::max)(pt1.y, pt2.y) - y;
		}

	template<typename _Tp> inline
		Rect_<_Tp>& Rect_<_Tp>::operator = (const Rect_<_Tp>& r)
	{
			x = r.x;
			y = r.y;
			width = r.width;
			height = r.height;
			return *this;
		}

	template<typename _Tp> inline
		Point_<_Tp> Rect_<_Tp>::tl() const
	{
			return Point_<_Tp>(x, y);
		}

	template<typename _Tp> inline
		Point_<_Tp> Rect_<_Tp>::br() const
	{
			return Point_<_Tp>(x + width, y + height);
		}

	template<typename _Tp> inline
		Size_<_Tp> Rect_<_Tp>::size() const
	{
			return Size_<_Tp>(width, height);
		}

	template<typename _Tp> inline
		_Tp Rect_<_Tp>::area() const
	{
			return width * height;
		}

	template<typename _Tp> template<typename _Tp2> inline
		Rect_<_Tp>::operator Rect_<_Tp2>() const
	{
			return Rect_<_Tp2>(saturate_cast<_Tp2>(x), saturate_cast<_Tp2>(y), saturate_cast<_Tp2>(width), saturate_cast<_Tp2>(height));
		}

	template<typename _Tp> inline
		bool Rect_<_Tp>::contains(const Point_<_Tp>& pt) const
	{
			return x <= pt.x && pt.x < x + width && y <= pt.y && pt.y < y + height;
		}


	template<typename _Tp> static inline
		Rect_<_Tp>& operator += (Rect_<_Tp>& a, const Point_<_Tp>& b)
	{
			a.x += b.x;
			a.y += b.y;
			return a;
		}

	template<typename _Tp> static inline
		Rect_<_Tp>& operator -= (Rect_<_Tp>& a, const Point_<_Tp>& b)
	{
			a.x -= b.x;
			a.y -= b.y;
			return a;
		}

	template<typename _Tp> static inline
		Rect_<_Tp>& operator += (Rect_<_Tp>& a, const Size_<_Tp>& b)
	{
			a.width += b.width;
			a.height += b.height;
			return a;
		}

	template<typename _Tp> static inline
		Rect_<_Tp>& operator -= (Rect_<_Tp>& a, const Size_<_Tp>& b)
	{
			a.width -= b.width;
			a.height -= b.height;
			return a;
		}

	template<typename _Tp> static inline
		Rect_<_Tp>& operator &= (Rect_<_Tp>& a, const Rect_<_Tp>& b)
	{
			_Tp x1 = (std::max)(a.x, b.x);
			_Tp y1 = (std::max)(a.y, b.y);
			a.width = (std::min)(a.x + a.width, b.x + b.width) - x1;
			a.height = (std::min)(a.y + a.height, b.y + b.height) - y1;
			a.x = x1;
			a.y = y1;
			if (a.width <= 0 || a.height <= 0)
				a = Rect();
			return a;
		}

	template<typename _Tp> static inline
		Rect_<_Tp>& operator |= (Rect_<_Tp>& a, const Rect_<_Tp>& b)
	{
			_Tp x1 = (std::min)(a.x, b.x);
			_Tp y1 = (std::min)(a.y, b.y);
			a.width = (std::max)(a.x + a.width, b.x + b.width) - x1;
			a.height = (std::max)(a.y + a.height, b.y + b.height) - y1;
			a.x = x1;
			a.y = y1;
			return a;
		}

	template<typename _Tp> static inline
		bool operator == (const Rect_<_Tp>& a, const Rect_<_Tp>& b)
	{
			return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
		}

	template<typename _Tp> static inline
		bool operator != (const Rect_<_Tp>& a, const Rect_<_Tp>& b)
	{
			return a.x != b.x || a.y != b.y || a.width != b.width || a.height != b.height;
		}

	template<typename _Tp> static inline
		Rect_<_Tp> operator + (const Rect_<_Tp>& a, const Point_<_Tp>& b)
	{
			return Rect_<_Tp>(a.x + b.x, a.y + b.y, a.width, a.height);
		}

	template<typename _Tp> static inline
		Rect_<_Tp> operator - (const Rect_<_Tp>& a, const Point_<_Tp>& b)
	{
			return Rect_<_Tp>(a.x - b.x, a.y - b.y, a.width, a.height);
		}

	template<typename _Tp> static inline
		Rect_<_Tp> operator + (const Rect_<_Tp>& a, const Size_<_Tp>& b)
	{
			return Rect_<_Tp>(a.x, a.y, a.width + b.width, a.height + b.height);
		}

	template<typename _Tp> static inline
		Rect_<_Tp> operator & (const Rect_<_Tp>& a, const Rect_<_Tp>& b)
	{
			Rect_<_Tp> c = a;
			return c &= b;
		}

	template<typename _Tp> static inline
		Rect_<_Tp> operator | (const Rect_<_Tp>& a, const Rect_<_Tp>& b)
	{
			Rect_<_Tp> c = a;
			return c |= b;
		}

	namespace detail
	{
		////////////////////////////////// ImageBase /////////////////////////////////
		template<typename _Tp> inline
			ImageBase<_Tp>::ImageBase()
			:rows_(0), cols_(0), channels_(0), data_(nullptr)
		{
			}

		template<typename _Tp> inline
			ImageBase<_Tp>::ImageBase(int rows, int cols, int channels)
		{
				create(rows, cols, channels);
			}

		template<typename _Tp> inline
			ImageBase<_Tp>::ImageBase(const ImageBase& other)
		{
				rows_ = other.rows_;
				cols_ = other.cols_;
				channels_ = other.channels_;
				data_ = other.data_;	// shallow copy
			}

		template<typename _Tp> template<typename _Tp2> inline
			ImageBase<_Tp>::operator ImageBase<_Tp2>() const
		{
				ImageBase<_Tp2> tmp(rows_, cols_, channels_);
				_Tp2* p = tmp.ptr(0);
				_Tp* p_ = ptr(0);
				for (long i = 0; i < rows_ * cols_ * channels_; ++i)
				{
					*p = saturate_cast<_Tp>(*p_);
					++p;
					++p_;
				}
				return tmp;
			}

		template<typename _Tp> inline
			ImageBase<_Tp>::ImageBase(ImageBase&& other)
		{
				rows_ = other.rows_;
				cols_ = other.cols_;
				channels_ = other.channels_;
				data_ = other.data_;	// shallow copy
				other.release();
			}

		template<typename _Tp> inline
			ImageBase<_Tp>::~ImageBase()
		{
				release();
			}

		template<typename _Tp> inline
			void ImageBase<_Tp>::create(int rows, int cols, int channels)
		{
				assert(rows > 0 && cols > 0 && channels > 0);
				rows_ = rows;
				cols_ = cols;
				channels_ = channels;
				data_ = std::make_shared<std::vector<_Tp>>(rows * cols * channels);
			}

		template<typename _Tp> inline
			void ImageBase<_Tp>::release()
		{
				rows_ = 0;
				cols_ = 0;
				channels_ = 0;
				data_ = nullptr;
			}

		template<typename _Tp> inline
			ImageBase<_Tp>& ImageBase<_Tp>::operator= (const ImageBase<_Tp>& other)
		{
				rows_ = other.rows_;
				cols_ = other.cols_;
				channels_ = other.channels_;
				data_ = other.data_;	// shallow copy
				return *this;
			}

		template<typename _Tp> inline
			ImageBase<_Tp>& ImageBase<_Tp>::operator= (ImageBase<_Tp>&& other)
		{
				rows_ = other.rows_;
				cols_ = other.cols_;
				channels_ = other.channels_;
				data_ = other.data_;	// shallow copy
				other.release();
				return *this;
			}

		template<typename _Tp> inline
			_Tp& ImageBase<_Tp>::operator() (int row, int col, int channel)
		{
				detach();
				long pos = row * cols_ * channels_ + col * channels_ + channel;
				range_check(row, col, channel);
				return (*data_)[pos];
			}

		template<typename _Tp> inline
			const _Tp& ImageBase<_Tp>::operator() (int row, int col, int channel) const
		{
				detach();
				long pos = row * cols_ * channels_ + col * channels_ + channel;
				range_check(row, col, channel);
				return (*data_)[pos];
			}

		template<typename _Tp> inline
			bool ImageBase<_Tp>::empty() const
		{
				return (rows_ < 1 || cols_ < 1 || channels_ < 1 || (!data_));
			}

		template<typename _Tp> inline
			int ImageBase<_Tp>::rows() const
		{
				return rows_;
			}

		template<typename _Tp> inline
			int ImageBase<_Tp>::cols() const
		{
				return cols_;
			}

		template<typename _Tp> inline
			int ImageBase<_Tp>::channels() const
		{
				return channels_;
			}

		template<typename _Tp> inline
			void ImageBase<_Tp>::range_check(long long pos) const
		{
				assert(pos >= 0);
				if (empty()) throw RuntimeException("Accessing emtpy image!");
				if (pos >= rows_ * cols_ * channels_) throw RuntimeException("Access out of range!");
			}

		template<typename _Tp> inline
			void ImageBase<_Tp>::range_check(int row, int col, int channel) const
		{
				assert(row >= 0 && col >= 0 && channel >= 0);
				if (empty()) throw RuntimeException("Accessing emtpy image!");
				if (row >= rows_ || col >= cols_ || channel >= channels_) throw RuntimeException("Access out of range!");
			}

		template<typename _Tp> inline
			_Tp ImageBase<_Tp>::at(int row, int col, int channel) const
		{
				range_check(row, col, channel);
				long pos = row * cols_ * channels_ + col * channels_ + channel;
				return (*data_)[pos];
			}

		template<typename _Tp> inline
			_Tp* ImageBase<_Tp>::ptr(int offset) const
		{
				range_check(offset);
				return (*data_).data() + offset;
			}

		template<typename _Tp> inline
			_Tp* ImageBase<_Tp>::ptr(int row, int col, int channel) const
		{
				range_check(row, col, channel);
				long pos = row * cols_ * channels_ + col * channels_ + channel;
				return (*data_).data() + pos;
			}

		template<typename _Tp> inline
			void ImageBase<_Tp>::import(_Tp* data, int rows, int cols, int channels)
		{
				assert(rows > 0 && cols > 0 && channels > 0 && "import size should be positive");
				create(rows, cols, channels);
				std::memcpy((*data_).data(), data, sizeof(_Tp)* rows * cols * channels);
			}

		template<typename _Tp> inline
			void ImageBase<_Tp>::import(std::vector<_Tp> data, int rows, int cols, int channels)
		{
				assert(rows > 0 && cols > 0 && channels > 0 && data.size() >= rows * cols * channels);
				create(rows, cols, channels);
				std::memcpy((*data_).data(), data.data(), sizeof(_Tp)* rows * cols * channels);
			}

		template<typename _Tp> inline
			std::vector<_Tp> ImageBase<_Tp>::export_raw() const
		{
				return (*data_);
			}

		template<typename _Tp> template<typename _Tp2> inline
			std::vector<_Tp2>& ImageBase<_Tp>::export_raw(std::vector<_Tp2>& out) const
		{
				out.resize(rows_ * cols_ * channels_);
				auto p = out.begin();
				for (auto i = (*data_).begin(); i != (*data_).end(); ++i, ++p)
				{
					*p = saturate_cast<_Tp2>(*i);
				}
				return out;
			}

		template<typename _Tp> inline
			void ImageBase<_Tp>::crop(int r0, int c0, int r1, int c1)
		{
				assert(r0 >= 0 && c0 >= 0 && r1 >= 0 && c1 >= 0 && "crop region should be positive!");
				assert(r0 < rows_ && r1 < rows_ && c0 < cols_ && c1 < cols_ && "crop point should in image!");
				assert(r0 != r1 && (c0 != c1) && "need a rectangle region!");

				int width = std::abs(c0 - c1);
				int height = std::abs(r0 - r1);
				int i0 = (std::min)(r0, r1);
				int i1 = (std::max)(r0, r1);
				int j0 = (std::min)(c0, c1);
				int j1 = (std::max)(c0, c1);
				detach();
				ImageBase<_Tp> tmp(height, width, channels_);
				
				// temporary solution, not optimized
				_Tp* pOld = ptr(i0, j0, 0);
				_Tp* pNew = tmp.ptr(0);
				int step = cols_ * channels_;
				int bulkSize = width * channels_;
				for (auto r = 0; r < height; ++r)
				{
					// copy entire row
					std::memcpy(pNew, pOld, sizeof(_Tp)* bulkSize);
					pOld += step;
					pNew += bulkSize;
				}
				std::swap(*this, tmp);
			}

		template<typename _Tp> inline
			void ImageBase<_Tp>::crop(Point p0, Point p1)
		{
				crop(p0.y, p0.x, p1.y, p1.x);
			}

		template<typename _Tp> inline
			void ImageBase<_Tp>::crop(Rect rect)
		{
				crop(rect.y, rect.x, rect.y + rect.height, rect.y + rect.width);
			}

		template<typename _Tp> inline
			void ImageBase<_Tp>::detach()
		{
				if (data_.use_count() < 2) return;
				// detach the current resource from shared
				std::shared_ptr<std::vector<_Tp>> tmp = std::make_shared<std::vector<_Tp>>();
				*tmp = *data_; // deep copy
				data_ = tmp;
			}
	} // namespace zz::detail

	// \endcond

} // namespace zz

#endif //END _ZUPPLY_ZUPPLY_HPP_


