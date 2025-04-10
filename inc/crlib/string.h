//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <string.h>
#include <ctype.h>

#include <crlib/basic.h>
#include <crlib/memory.h>
#include <crlib/movable.h>
#include <crlib/uniqueptr.h>
#include <crlib/array.h>

CR_NAMESPACE_BEGIN

// helper for null-termination
static const char kNullChar = '\0';

// overloaded version of snprintf to take String and StringRef as char arrays
class SNPrintfWrap : public Singleton <SNPrintfWrap> {
public:
   explicit SNPrintfWrap () = default;
   ~SNPrintfWrap () = default;

private:
   template <typename U> U cast (U value) {
      return value;
   }

   const char *cast (const class String &value);
   const char *cast (const class StringRef &value);

public:
   template <typename ...Args> int32_t exec (char *buffer, const size_t maxSize, const char *fmt, Args &&...args) {
      if (buffer) {
         buffer[0] = kNullChar;
      }
      return snprintf (buffer, maxSize, fmt, cast (cr::forward <Args> (args))...);
   }
};

// expose global format wrap
CR_EXPOSE_GLOBAL_SINGLETON (SNPrintfWrap, fmtwrap);


// simple non-owning string class like std::string_view
class StringRef {
private:
   static constexpr size_t lenstr (const char *str) noexcept {
      auto count = static_cast <size_t> (0);

      for (; *str != kNullChar; ++str) {
         ++count;
      }
      return count;
   }

public:
   static constexpr uint32_t fnv1a32 (const char *str) noexcept {
      constexpr uint32_t prime = 0x1000193;
      constexpr uint32_t basis = 0x811c9dc5;

      auto hash = basis;

      for (; *str != kNullChar; ++str) {
         hash = (hash ^ static_cast <uint32_t> (static_cast <uint8_t> (*str))) * prime;
      }
      return hash;
   }

public:
   enum : size_t {
      InvalidIndex = static_cast <size_t> (-1)
   };

private:
   const char *chars_ = "";
   size_t length_ {};

public:
   constexpr StringRef () noexcept = default;

   constexpr StringRef (const char *chars) : chars_ (chars), length_ (chars ? StringRef::lenstr (chars) : 0)
   { }

   constexpr StringRef (const char *chars, size_t length) : chars_ (chars), length_ (length)
   { }

   constexpr StringRef (nullptr_t) : chars_ (""), length_ (0)
   { }

   constexpr StringRef (nullptr_t, size_t) : chars_ (""), length_ (0)
   { }

public:
   StringRef (const String &str);

public:
   constexpr StringRef (const StringRef &) = default;
   constexpr  StringRef &operator = (const StringRef &) = default;

   constexpr StringRef (StringRef &&) = default;
   constexpr StringRef &operator = (StringRef &&) = default;

public:
   constexpr bool operator == (const StringRef &rhs) const {
      return length_ == rhs.length_ && memcmp (chars_, rhs.chars_, length_) == 0;
   }

   bool operator == (const char *rhs) const {
      return strcmp (chars_, rhs) == 0;
   }

   constexpr bool operator != (const StringRef &rhs) const {
      return !(*this == rhs);
   }

   bool operator != (const char *rhs) const {
      return !(*this == rhs);
   }

   constexpr char operator [] (size_t index) const {
      return chars_[index];
   }

public:
   constexpr bool empty () const {
      return length_ == 0;
   }

   constexpr size_t length () const {
      return length_;
   }

   constexpr const char *chars () const {
      return chars_;
   }

   constexpr bool equals (const StringRef &rhs) const {
      return *this == rhs;
   }

   constexpr uint32_t hash () const {
      return fnv1a32 (chars ());
   }

public:
   template <typename U> constexpr U as () const {
      if constexpr (cr::is_same <U, float>::value) {
         return static_cast <float> (atof (chars ()));
      }
      else if constexpr (cr::is_same <U, int>::value) {
         return atoi (chars ());
      }
   }

   bool startsWith (StringRef prefix) const {
      return prefix.length () <= length_ && strncmp (chars (), prefix.chars (), prefix.length ()) == 0;
   }

   bool endsWith (StringRef suffix) const {
      return suffix.length () <= length_ && strncmp (chars () + length_ - suffix.length (), suffix.chars (), suffix.length ()) == 0;
   }

   constexpr bool contains (StringRef rhs) const {
      return find (rhs) != InvalidIndex;
   }

   constexpr size_t find (char pattern, size_t start = 0) const {
      for (size_t i = start; i < length_; ++i) {
         if (chars_[i] == pattern) {
            return i;
         }
      }
      return InvalidIndex;
   }

   constexpr size_t find (StringRef pattern, size_t start = 0) const {
      if (pattern.length () > length_ || start > length_) {
         return InvalidIndex;
      }

      for (size_t i = start; i <= length_ - pattern.length (); ++i) {
         size_t index = 0;

         for (; index < pattern.length () && chars_[index]; ++index) {
            if (chars_[i + index] != pattern[index]) {
               break;
            }
         }

         if (!pattern[index]) {
            return i;
         }
      }
      return InvalidIndex;
   }

   constexpr size_t rfind (char pattern) const {
      for (size_t i = length_; i != 0; i--) {
         if (chars_[i] == pattern) {
            return i;
         }
      }
      return InvalidIndex;
   }

   constexpr size_t rfind (StringRef pattern) const {
      if (pattern.length () > length_) {
         return InvalidIndex;
      }
      bool match = true;

      for (size_t i = length_ - 1; i >= pattern.length (); i--) {
         match = true;

         for (size_t j = pattern.length () - 1; j > 0; j--) {
            if (chars_[i + i] != pattern[j]) {
               match = false;
               break;
            }
         }

         if (match) {
            return i;
         }
      }
      return InvalidIndex;
   }

   constexpr size_t findFirstOf (StringRef pattern, size_t start = 0) const {
      for (size_t i = start; i < length_; ++i) {
         for (size_t j = 0; j < pattern.length (); ++j) {
            if (chars_[i] == pattern[j]) {
               return i;
            }
         }
      }
      return InvalidIndex;
   }

   constexpr size_t findLastOf (StringRef pattern) const {
      for (size_t i = length_ - 1; i > 0; i--) {
         for (size_t j = 0; j < pattern.length (); ++j) {
            if (chars_[i] == pattern[j]) {
               return i;
            }
         }
      }
      return InvalidIndex;
   }
   
   constexpr size_t findFirstNotOf (StringRef pattern, size_t start = 0) const {
      bool different = true;

      for (size_t i = start; i < length_; ++i) {
         different = true;

         for (size_t j = 0; j < pattern.length (); ++j) {
            if (chars_[i] == pattern[j]) {
               different = false;
               break;
            }
         }

         if (different) {
            return i;
         }
      }
      return InvalidIndex;
   }
   
   constexpr size_t findLastNotOf (StringRef pattern) const {
      bool different = true;

      for (size_t i = length_ - 1; i > 0; i--) {
         different = true;

         for (size_t j = 0; j < pattern.length (); ++j) {
            if (chars_[i] == pattern[j]) {
               different = false;
               break;
            }
         }
         if (different) {
            return i;
         }
      }
      return InvalidIndex;
   }
   
   constexpr size_t countChar (char ch) const {
      size_t count = 0;

      for (size_t i = 0, e = length (); i != e; ++i) {
         if (chars_[i] == ch) {
            ++count;
         }
      }
      return count;
   }
 
   constexpr size_t countStr (StringRef pattern) const {
      if (pattern.length () > length_) {
         return 0;
      }
      size_t count = 0;

      for (size_t i = 0, e = length_ - pattern.length () + 1; i != e; ++i) {
         if (substr (i, pattern.length ()) == pattern) {
            ++count;
         }
      }
      return count;
   }

   constexpr StringRef substr (size_t start, size_t count = InvalidIndex) const {
      start = cr::min (start, length_);

      if (count == InvalidIndex) {
         count = length_;
      }
      return { chars () + start, cr::min (count, length () - start) };
   }

   template <typename U = StringRef> constexpr Array <U> split (StringRef delim) const {
      Array <U> tokens;
      size_t prev = 0, pos = 0;

      while ((pos = find (delim, pos)) != InvalidIndex) {
         tokens.push (substr (prev, pos - prev));
         prev = ++pos;
      }
      tokens.push (substr (prev, pos - prev));

      return tokens;
   }

   template <typename U = StringRef> constexpr Array <U> split (size_t maxLength) const {
      Array <U> tokens;

      for (size_t i = 0; i < length (); i += maxLength) {
         tokens.emplace (substr (i, maxLength));
      }
      return tokens;
   }

public:
   constexpr const char *begin () const {
      return chars_;
   }

   constexpr const char *end () const {
      return chars_ + length_;
   }
};

// simple std::string analogue
class String final {
public:
   enum : size_t {
      InvalidIndex = StringRef::InvalidIndex
   };

private:
   UniquePtr <char[]> chars_ {};
   size_t length_ {};
   size_t capacity_ {};

private:
   size_t getGrowFactor (const size_t length) const {
      auto capacity = capacity_ ? capacity_ : cr::max <size_t> (12U, length + 1);

      while (length_ + length > capacity) {
         capacity += capacity * 2 / 3;
      }
      return capacity + (length < 4 ? 8 : length);
   }

   size_t calcLength (const char *str, const size_t length) {
      return length > 0 ? length : (str ? strlen (str) : 0);
   }

   void reset () {
      capacity_ = 0;
      length_ = 0;
   }

public:
   String () = default;
   ~String () = default;

   String (const char *str, size_t length = 0) {
      assign (str, length);
   }

   String (const String &str)  {
      assign (str.chars ());
   }

   String (StringRef str) {
      assign (str.chars (), str.length ());
   }

   String (const char ch) {
      assign (ch);
   }

   String (String &&rhs) noexcept : chars_ (cr::move (rhs.chars_)), length_ (rhs.length_), capacity_ (rhs.capacity_) {
      rhs.reset ();
   }

   String (UniquePtr <char[]> &&rhs, size_t length) noexcept : chars_ (cr::move (rhs)), length_ (length)
   {}

public:
   void resize (const size_t amount) noexcept {
      if (length_ + amount < capacity_) {
         return;
      }
      const size_t factor = getGrowFactor (amount) + length_;

      if (chars_) {
         auto transfer = makeUnique <char[]> (factor);
         memcpy (&transfer[0], &chars_[0], length_);

         chars_ = cr::move (transfer);
      }
      else {
         chars_ = makeUnique <char[]> (factor);
         chars_[0] = kNullChar;
      }
      capacity_ = factor;
   }

   String &assign (const char *str, size_t length = 0) {
      length = calcLength (str, length);

      resize (length);

      if (str) {
         memcpy (&chars_[0], str, length);
      }
      else {
         chars_[0] = kNullChar;
      }

      length_ = length;
      chars_[length_] = kNullChar;
      return *this;
   }

   String &assign (const String &str, size_t length = 0) {
      return assign (str.chars (), length);
   }

   String &assign (const char ch) {
      return assign (&ch, 1);
   }

   String &append (const char *str, size_t length = 0) {
      length = calcLength (str, length);

      resize (length);
      memcpy (chars_.get () + length_, str, length);

      length_ += length;
      chars_[length_] = kNullChar;

      return *this;
   }

   String &append (const String &str, size_t length = 0) {
      return append (str.chars (), length);
   }

   String &append (const char ch) {
      return append (&ch, 1);
   }

   template <typename ...Args> String &assignf (const char *fmt, Args &&...args) {
      const size_t size = fmtwrap.exec (nullptr, 0, fmt, args...);

      SmallArray <char> buffer (size + 1);
      fmtwrap.exec (buffer.data (), size + 1, fmt, cr::forward <Args> (args)...);

      return assign (buffer.data ());
   }

   template <typename ...Args> String &appendf (const char *fmt, Args &&...args) {
      if (empty ()) {
         return assignf (fmt, cr::forward <Args> (args)...);
      }
      const size_t size = fmtwrap.exec (nullptr, 0, fmt, args...) + length ();

      SmallArray <char> buffer (size + 1);
      fmtwrap.exec (buffer.data (), size + 1, fmt, cr::forward <Args> (args)...);

      return append (buffer.data ());
   }

public:
   const char &at (size_t index) const {
      return chars ()[index];
   }

   char &at (size_t index) {
      return const_cast <char *> (chars ())[index];
   }

   const char *chars () const {
      return chars_ ? chars_.get () : "\0";
   }

   size_t length () const {
      return length_;
   }

   size_t capacity () const {
      return capacity_;
   }

   bool empty () const {
      return !length_;
   }

   void clear () {
      assign ("\0");
   }

   StringRef str () const {
      return { chars_.get (), length_ };
   }

public:
   bool insert (size_t index, StringRef str) {
      if (str.empty ()) {
         return false;
      }

      if (index >= length_) {
         append (str.chars (), str.length ());
      }
      else {
         resize (length_ + str.length ());

         for (size_t i = length_; i > index; --i) {
            at (i + str.length () - 1) = at (i - 1);
         }

         for (size_t i = 0; i < str.length (); ++i) {
            at (i + index) = str[i];
         }
         length_ += str.length ();
      }
      return true;
   }

   bool erase (size_t index, size_t count = 1) {
      if (index + count > length_) {
         return false;
      }
      length_ -= count;

      for (size_t i = index; i < length_; ++i) {
         at (i) = at (i + count);
      }
      return true;
   }

   size_t replace (StringRef needle, StringRef to) {
      if (needle.empty () || to.empty ()) {
         return 0;
      }
      size_t replaced = 0, pos = 0;

      while (pos < length ()) {
         pos = find (needle, pos);

         if (pos == InvalidIndex) {
            break;
         }
         erase (pos, needle.length ());
         insert (pos, to);

         pos += to.length ();
         ++replaced;
      }
      return replaced;
   }

   String &lowercase () {
      for (auto &ch : *this) {
         ch = static_cast <char> (::tolower (ch));
      }
      return *this;
   }

   String &uppercase () {
      for (auto &ch : *this) {
         ch = static_cast <char> (::toupper (ch));
      }
      return *this;
   }

   String &ltrim (StringRef characters = "\r\n\t ") {
      size_t begin = length_;

      for (size_t i = 0; i < begin; ++i) {
         if (characters.find (at (i)) == InvalidIndex) {
            begin = i;
            break;
         }
      }
      return *this = substr (begin, length_ - begin);
   }

   String &rtrim (StringRef characters = "\r\n\t ") {
      size_t end = 0;

      for (size_t i = length_; i > 0; --i) {
         if (characters.find (at (i - 1)) == InvalidIndex) {
            end = i;
            break;
         }
      }
      return *this = substr (0, end);
   }

   String &trim (StringRef characters = "\r\n\t ") {
      return ltrim (characters).rtrim (characters);
   }

public:
   uint32_t hash () const {
      return str ().hash ();
   }

   bool contains (StringRef rhs) const {
      return str ().contains (rhs);
   }

   bool startsWith (StringRef prefix) const {
      return str ().startsWith (prefix);
   }

   bool endsWith (StringRef suffix) const {
      return str ().endsWith (suffix);
   }

   size_t find (char pattern, size_t start = 0) const {
      return str ().find (pattern, start);
   }

   size_t find (StringRef pattern, size_t start = 0) const {
      return str ().find (pattern, start);
   }

   size_t rfind (char pattern) const {
      return str ().rfind (pattern);
   }

   size_t rfind (StringRef pattern) const {
      return str ().rfind (pattern);
   }

   size_t findFirstOf (StringRef pattern, size_t start = 0) const {
      return str ().findFirstOf (pattern, start);
   }

   size_t findLastOf (StringRef pattern) const {
      return str ().findLastOf (pattern);
   }

   size_t findFirstNotOf (StringRef pattern, size_t start = 0) const {
      return str ().findFirstNotOf (pattern, start);
   }

   size_t findLastNotOf (StringRef pattern) const {
      return str ().findLastNotOf (pattern);
   }

   size_t countChar (char ch) const {
      return str ().countChar (ch);
   }

   size_t countStr (StringRef pattern) const {
      return str ().countStr (pattern);
   }

   String substr (size_t start, size_t count = InvalidIndex) const {
      return str ().substr (start, count);
   }

   Array <String> split (StringRef delim) const {
      return str ().split <String> (delim);
   }

   Array <String> split (size_t maxLength) const {
      return str ().split <String> (maxLength);
   }

public:
   template <typename U> constexpr U as () const {
      if constexpr (cr::is_same <U, float>::value) {
         return str ().as <float> ();
      }
      else if constexpr (cr::is_same <U, int>::value) {
         return str ().as <int> ();
      }
   }

   // for range-based loops
public:
   char *begin () {
      return const_cast <char *> (chars ());
   }

   char *begin () const {
      return const_cast <char *> (chars ());
   }

   char *end () {
      return begin () + length_;
   }

   char *end () const {
      return begin () + length_;
   }

public:
   String &operator = (String &&rhs) noexcept {
      if (this != &rhs) {
         chars_ = cr::move (rhs.chars_);
         length_ = rhs.length_;
         capacity_ = rhs.capacity_;

         rhs.reset ();
      }
      return *this;
   }

   String &operator = (const String &rhs) {
      return assign (rhs);
   }

   String &operator = (const char *rhs) {
      return assign (rhs);
   }

   String &operator = (char rhs) {
      return assign (rhs);
   }

   String &operator += (const String &rhs) {
      return append (rhs);
   }

   String &operator += (const char *rhs) {
      return append (rhs);
   }

   const char &operator [] (size_t index) const {
      return at (index);
   }

   char &operator [] (size_t index) {
      return at (index);
   }

   friend String operator + (const String &lhs, char rhs) {
      return String (lhs).append (rhs);
   }

   friend String operator + (char lhs, const String &rhs) {
      return String (lhs).append (rhs);
   }

   friend String operator + (const String &lhs, const char *rhs) {
      return String (lhs).append (rhs);
   }

   friend String operator + (const char *lhs, const String &rhs) {
      return String (lhs).append (rhs);
   }

   friend String operator + (const String &lhs, const String &rhs) {
      return String (lhs).append (rhs);
   }

   friend bool operator == (const String &lhs, const String &rhs) {
      return lhs.str ().equals (rhs);
   }

   friend bool operator == (const char *lhs, const String &rhs) {
      return rhs.str ().equals (lhs);
   }

   friend bool operator == (const String &lhs, const char *rhs) {
      return lhs.str ().equals (rhs);
   }

   friend bool operator != (const String &lhs, const String &rhs) {
      return !lhs.str ().equals (rhs);
   }

   friend bool operator != (const char *lhs, const String &rhs) {
      return !rhs.str ().equals (lhs);
   }

   friend bool operator != (const String &lhs, const char *rhs) {
      return !lhs.str ().equals (rhs);
   }

public:
   static String join (const Array <String> &sequence, StringRef delim, const size_t start = 0) {
      if (sequence.empty ()) {
         return "";
      }

      if (sequence.length () == 1) {
         return sequence.at (0);
      }
      String result;

      for (size_t index = start; index < sequence.length (); ++index) {
         if (index != start) {
            result += delim + sequence[index];
         }
         else {
            result += sequence[index];
         }
      }
      return result;
   }
};

// constructor from string to string_ref
inline StringRef::StringRef (const String &str) : chars_ (str.chars ()), length_ (str.length ())
{ }

// wrapping String for snprintf
inline const char *SNPrintfWrap::cast (const String &value) {
   return value.chars ();
}

// wrapping StringRef for snprintf
inline const char *SNPrintfWrap::cast (const StringRef &value) {
   return value.chars ();
}

// simple rotation-string pool for holding temporary data passed to different modules and for formatting
class StringBuffer final : public Singleton <StringBuffer> {
public:
   enum : size_t {
      StaticBufferSize = static_cast <size_t> (1024),
      RotationCount = static_cast <size_t> (16)
   };

private:
   char data_[RotationCount + 1][StaticBufferSize + 1] {};
   size_t rotate_ = 0;

public:
   StringBuffer () = default;
   ~StringBuffer () = default;

public:
   char *chars () noexcept {
      if (++rotate_ >= RotationCount) {
         rotate_ = 0;
      }
      auto result = data_[cr::clamp <size_t> (rotate_, 0, RotationCount)];
      result[0] = kNullChar;

      return result;
   }

   template <typename U, typename ...Args> U *format (const U *fmt, Args &&...args) noexcept {
      auto buffer = chars ();
      fmtwrap.exec (buffer, StaticBufferSize, fmt, args...);

      return buffer;
   }

   template <typename ...Args> String joinPath (Args &&...args) noexcept {
      Array <String> data ({ cr::forward <Args> (args)... });
      return String::join (data, kPathSeparator);
   }

   template <typename U> U *format (const U *fmt) noexcept {
      auto buffer = chars ();
      copy (buffer, fmt, StaticBufferSize);

      return buffer;
   }

   // checks if string is not empty
   bool isEmpty (const char *input) const noexcept {
      if (input == nullptr) {
         return true;
      }
      return *input == kNullChar;
   }

   bool matches (const char *str1, const char *str2) noexcept {
#if defined(CR_WINDOWS)
      return _stricmp (str1, str2) == 0;
#else
      return ::strcasecmp (str1, str2) == 0;
#endif
   }

   template <typename U> U *copy (U *dst, const U *src, size_t len) noexcept {
#if defined(CR_CXX_MSVC)
      strncpy_s (dst, len, src, len - 1);
      return dst;
#else
      return strncpy (dst, src, len);
#endif
   }

   template <typename U> U *concat (U *dst, const U *src, size_t len) noexcept {
#if defined(CR_CXX_MSVC)
      strncat_s (dst, len, src, len - 1);
      return dst;
#else
      return strncat (dst, src, len);
#endif
   }
};

// expose global string pool
CR_EXPOSE_GLOBAL_SINGLETON (StringBuffer, strings);

// some limited utf8 stuff
class Utf8Tools : public Singleton <Utf8Tools> {
private:
   enum : int32_t {
      Utf8MaxChars = 706
   };

private:
   // sample implementation from unicode home page: https://web.archive.org/web/19970105220809/http://www.stonehand.com/unicode/standard/fss-utf.html
   struct Utf8Table {
      int32_t cmask, cval, shift;
      long lmask, lval;

      constexpr Utf8Table (int32_t cmask, int32_t cval, int32_t shift, long lmask, long lval) :
         cmask (cmask), cval (cval), shift (shift), lmask (lmask), lval (lval) {
      }
   };

   struct Utf8CaseTable {
      int32_t from, to;
   };

private:
  static constexpr Utf8CaseTable upperTable_[Utf8MaxChars] = {
      { 0x0061, 0x0041 }, { 0x0062, 0x0042 }, { 0x0063, 0x0043 }, { 0x0064, 0x0044 }, { 0x0065, 0x0045 }, { 0x0066, 0x0046 }, { 0x0067, 0x0047 }, { 0x0068, 0x0048 },
      { 0x0069, 0x0049 }, { 0x006a, 0x004a }, { 0x006b, 0x004b }, { 0x006c, 0x004c }, { 0x006d, 0x004d }, { 0x006e, 0x004e }, { 0x006f, 0x004f }, { 0x0070, 0x0050 },
      { 0x0071, 0x0051 }, { 0x0072, 0x0052 }, { 0x0073, 0x0053 }, { 0x0074, 0x0054 }, { 0x0075, 0x0055 }, { 0x0076, 0x0056 }, { 0x0077, 0x0057 }, { 0x0078, 0x0058 },
      { 0x0079, 0x0059 }, { 0x007a, 0x005a }, { 0x00e0, 0x00c0 }, { 0x00e1, 0x00c1 }, { 0x00e2, 0x00c2 }, { 0x00e3, 0x00c3 }, { 0x00e4, 0x00c4 }, { 0x00e5, 0x00c5 },
      { 0x00e6, 0x00c6 }, { 0x00e7, 0x00c7 }, { 0x00e8, 0x00c8 }, { 0x00e9, 0x00c9 }, { 0x00ea, 0x00ca }, { 0x00eb, 0x00cb }, { 0x00ec, 0x00cc }, { 0x00ed, 0x00cd },
      { 0x00ee, 0x00ce }, { 0x00ef, 0x00cf }, { 0x00f0, 0x00d0 }, { 0x00f1, 0x00d1 }, { 0x00f2, 0x00d2 }, { 0x00f3, 0x00d3 }, { 0x00f4, 0x00d4 }, { 0x00f5, 0x00d5 },
      { 0x00f6, 0x00d6 }, { 0x00f8, 0x00d8 }, { 0x00f9, 0x00d9 }, { 0x00fa, 0x00da }, { 0x00fb, 0x00db }, { 0x00fc, 0x00dc }, { 0x00fd, 0x00dd }, { 0x00fe, 0x00de },
      { 0x00ff, 0x0178 }, { 0x0101, 0x0100 }, { 0x0103, 0x0102 }, { 0x0105, 0x0104 }, { 0x0107, 0x0106 }, { 0x0109, 0x0108 }, { 0x010b, 0x010a }, { 0x010d, 0x010c },
      { 0x010f, 0x010e }, { 0x0111, 0x0110 }, { 0x0113, 0x0112 }, { 0x0115, 0x0114 }, { 0x0117, 0x0116 }, { 0x0119, 0x0118 }, { 0x011b, 0x011a }, { 0x011d, 0x011c },
      { 0x011f, 0x011e }, { 0x0121, 0x0120 }, { 0x0123, 0x0122 }, { 0x0125, 0x0124 }, { 0x0127, 0x0126 }, { 0x0129, 0x0128 }, { 0x012b, 0x012a }, { 0x012d, 0x012c },
      { 0x012f, 0x012e }, { 0x0131, 0x0049 }, { 0x0133, 0x0132 }, { 0x0135, 0x0134 }, { 0x0137, 0x0136 }, { 0x013a, 0x0139 }, { 0x013c, 0x013b }, { 0x013e, 0x013d },
      { 0x0140, 0x013f }, { 0x0142, 0x0141 }, { 0x0144, 0x0143 }, { 0x0146, 0x0145 }, { 0x0148, 0x0147 }, { 0x014b, 0x014a }, { 0x014d, 0x014c }, { 0x014f, 0x014e },
      { 0x0151, 0x0150 }, { 0x0153, 0x0152 }, { 0x0155, 0x0154 }, { 0x0157, 0x0156 }, { 0x0159, 0x0158 }, { 0x015b, 0x015a }, { 0x015d, 0x015c }, { 0x015f, 0x015e },
      { 0x0161, 0x0160 }, { 0x0163, 0x0162 }, { 0x0165, 0x0164 }, { 0x0167, 0x0166 }, { 0x0169, 0x0168 }, { 0x016b, 0x016a }, { 0x016d, 0x016c }, { 0x016f, 0x016e },
      { 0x0171, 0x0170 }, { 0x0173, 0x0172 }, { 0x0175, 0x0174 }, { 0x0177, 0x0176 }, { 0x017a, 0x0179 }, { 0x017c, 0x017b }, { 0x017e, 0x017d }, { 0x0183, 0x0182 },
      { 0x0185, 0x0184 }, { 0x0188, 0x0187 }, { 0x018c, 0x018b }, { 0x0192, 0x0191 }, { 0x0195, 0x01f6 }, { 0x0199, 0x0198 }, { 0x019e, 0x0220 }, { 0x01a1, 0x01a0 },
      { 0x01a3, 0x01a2 }, { 0x01a5, 0x01a4 }, { 0x01a8, 0x01a7 }, { 0x01ad, 0x01ac }, { 0x01b0, 0x01af }, { 0x01b4, 0x01b3 }, { 0x01b6, 0x01b5 }, { 0x01b9, 0x01b8 },
      { 0x01bd, 0x01bc }, { 0x01bf, 0x01f7 }, { 0x01c6, 0x01c4 }, { 0x01c9, 0x01c7 }, { 0x01cc, 0x01ca }, { 0x01ce, 0x01cd }, { 0x01d0, 0x01cf }, { 0x01d2, 0x01d1 },
      { 0x01d4, 0x01d3 }, { 0x01d6, 0x01d5 }, { 0x01d8, 0x01d7 }, { 0x01da, 0x01d9 }, { 0x01dc, 0x01db }, { 0x01dd, 0x018e }, { 0x01df, 0x01de }, { 0x01e1, 0x01e0 },
      { 0x01e3, 0x01e2 }, { 0x01e5, 0x01e4 }, { 0x01e7, 0x01e6 }, { 0x01e9, 0x01e8 }, { 0x01eb, 0x01ea }, { 0x01ed, 0x01ec }, { 0x01ef, 0x01ee }, { 0x01f3, 0x01f1 },
      { 0x01f5, 0x01f4 }, { 0x01f9, 0x01f8 }, { 0x01fb, 0x01fa }, { 0x01fd, 0x01fc }, { 0x01ff, 0x01fe }, { 0x0201, 0x0200 }, { 0x0203, 0x0202 }, { 0x0205, 0x0204 },
      { 0x0207, 0x0206 }, { 0x0209, 0x0208 }, { 0x020b, 0x020a }, { 0x020d, 0x020c }, { 0x020f, 0x020e }, { 0x0211, 0x0210 }, { 0x0213, 0x0212 }, { 0x0215, 0x0214 },
      { 0x0217, 0x0216 }, { 0x0219, 0x0218 }, { 0x021b, 0x021a }, { 0x021d, 0x021c }, { 0x021f, 0x021e }, { 0x0223, 0x0222 }, { 0x0225, 0x0224 }, { 0x0227, 0x0226 },
      { 0x0229, 0x0228 }, { 0x022b, 0x022a }, { 0x022d, 0x022c }, { 0x022f, 0x022e }, { 0x0231, 0x0230 }, { 0x0233, 0x0232 }, { 0x0253, 0x0181 }, { 0x0254, 0x0186 },
      { 0x0256, 0x0189 }, { 0x0257, 0x018a }, { 0x0259, 0x018f }, { 0x025b, 0x0190 }, { 0x0260, 0x0193 }, { 0x0263, 0x0194 }, { 0x0268, 0x0197 }, { 0x0269, 0x0196 },
      { 0x026f, 0x019c }, { 0x0272, 0x019d }, { 0x0275, 0x019f }, { 0x0280, 0x01a6 }, { 0x0283, 0x01a9 }, { 0x0288, 0x01ae }, { 0x028a, 0x01b1 }, { 0x028b, 0x01b2 },
      { 0x0292, 0x01b7 }, { 0x03ac, 0x0386 }, { 0x03ad, 0x0388 }, { 0x03ae, 0x0389 }, { 0x03af, 0x038a }, { 0x03b1, 0x0391 }, { 0x03b2, 0x0392 }, { 0x03b3, 0x0393 },
      { 0x03b4, 0x0394 }, { 0x03b5, 0x0395 }, { 0x03b6, 0x0396 }, { 0x03b7, 0x0397 }, { 0x03b8, 0x0398 }, { 0x03b9, 0x0345 }, { 0x03ba, 0x039a }, { 0x03bb, 0x039b },
      { 0x03bc, 0x00b5 }, { 0x03bd, 0x039d }, { 0x03be, 0x039e }, { 0x03bf, 0x039f }, { 0x03c0, 0x03a0 }, { 0x03c1, 0x03a1 }, { 0x03c3, 0x03a3 }, { 0x03c4, 0x03a4 },
      { 0x03c5, 0x03a5 }, { 0x03c6, 0x03a6 }, { 0x03c7, 0x03a7 }, { 0x03c8, 0x03a8 }, { 0x03c9, 0x03a9 }, { 0x03ca, 0x03aa }, { 0x03cb, 0x03ab }, { 0x03cc, 0x038c },
      { 0x03cd, 0x038e }, { 0x03ce, 0x038f }, { 0x03d9, 0x03d8 }, { 0x03db, 0x03da }, { 0x03dd, 0x03dc }, { 0x03df, 0x03de }, { 0x03e1, 0x03e0 }, { 0x03e3, 0x03e2 },
      { 0x03e5, 0x03e4 }, { 0x03e7, 0x03e6 }, { 0x03e9, 0x03e8 }, { 0x03eb, 0x03ea }, { 0x03ed, 0x03ec }, { 0x03ef, 0x03ee }, { 0x03f2, 0x03f9 }, { 0x03f8, 0x03f7 },
      { 0x03fb, 0x03fa }, { 0x0430, 0x0410 }, { 0x0431, 0x0411 }, { 0x0432, 0x0412 }, { 0x0433, 0x0413 }, { 0x0434, 0x0414 }, { 0x0435, 0x0415 }, { 0x0436, 0x0416 },
      { 0x0437, 0x0417 }, { 0x0438, 0x0418 }, { 0x0439, 0x0419 }, { 0x043a, 0x041a }, { 0x043b, 0x041b }, { 0x043c, 0x041c }, { 0x043d, 0x041d }, { 0x043e, 0x041e },
      { 0x043f, 0x041f }, { 0x0440, 0x0420 }, { 0x0441, 0x0421 }, { 0x0442, 0x0422 }, { 0x0443, 0x0423 }, { 0x0444, 0x0424 }, { 0x0445, 0x0425 }, { 0x0446, 0x0426 },
      { 0x0447, 0x0427 }, { 0x0448, 0x0428 }, { 0x0449, 0x0429 }, { 0x044a, 0x042a }, { 0x044b, 0x042b }, { 0x044c, 0x042c }, { 0x044d, 0x042d }, { 0x044e, 0x042e },
      { 0x044f, 0x042f }, { 0x0450, 0x0400 }, { 0x0451, 0x0401 }, { 0x0452, 0x0402 }, { 0x0453, 0x0403 }, { 0x0454, 0x0404 }, { 0x0455, 0x0405 }, { 0x0456, 0x0406 },
      { 0x0457, 0x0407 }, { 0x0458, 0x0408 }, { 0x0459, 0x0409 }, { 0x045a, 0x040a }, { 0x045b, 0x040b }, { 0x045c, 0x040c }, { 0x045d, 0x040d }, { 0x045e, 0x040e },
      { 0x045f, 0x040f }, { 0x0461, 0x0460 }, { 0x0463, 0x0462 }, { 0x0465, 0x0464 }, { 0x0467, 0x0466 }, { 0x0469, 0x0468 }, { 0x046b, 0x046a }, { 0x046d, 0x046c },
      { 0x046f, 0x046e }, { 0x0471, 0x0470 }, { 0x0473, 0x0472 }, { 0x0475, 0x0474 }, { 0x0477, 0x0476 }, { 0x0479, 0x0478 }, { 0x047b, 0x047a }, { 0x047d, 0x047c },
      { 0x047f, 0x047e }, { 0x0481, 0x0480 }, { 0x048b, 0x048a }, { 0x048d, 0x048c }, { 0x048f, 0x048e }, { 0x0491, 0x0490 }, { 0x0493, 0x0492 }, { 0x0495, 0x0494 },
      { 0x0497, 0x0496 }, { 0x0499, 0x0498 }, { 0x049b, 0x049a }, { 0x049d, 0x049c }, { 0x049f, 0x049e }, { 0x04a1, 0x04a0 }, { 0x04a3, 0x04a2 }, { 0x04a5, 0x04a4 },
      { 0x04a7, 0x04a6 }, { 0x04a9, 0x04a8 }, { 0x04ab, 0x04aa }, { 0x04ad, 0x04ac }, { 0x04af, 0x04ae }, { 0x04b1, 0x04b0 }, { 0x04b3, 0x04b2 }, { 0x04b5, 0x04b4 },
      { 0x04b7, 0x04b6 }, { 0x04b9, 0x04b8 }, { 0x04bb, 0x04ba }, { 0x04bd, 0x04bc }, { 0x04bf, 0x04be }, { 0x04c2, 0x04c1 }, { 0x04c4, 0x04c3 }, { 0x04c6, 0x04c5 },
      { 0x04c8, 0x04c7 }, { 0x04ca, 0x04c9 }, { 0x04cc, 0x04cb }, { 0x04ce, 0x04cd }, { 0x04d1, 0x04d0 }, { 0x04d3, 0x04d2 }, { 0x04d5, 0x04d4 }, { 0x04d7, 0x04d6 },
      { 0x04d9, 0x04d8 }, { 0x04db, 0x04da }, { 0x04dd, 0x04dc }, { 0x04df, 0x04de }, { 0x04e1, 0x04e0 }, { 0x04e3, 0x04e2 }, { 0x04e5, 0x04e4 }, { 0x04e7, 0x04e6 },
      { 0x04e9, 0x04e8 }, { 0x04eb, 0x04ea }, { 0x04ed, 0x04ec }, { 0x04ef, 0x04ee }, { 0x04f1, 0x04f0 }, { 0x04f3, 0x04f2 }, { 0x04f5, 0x04f4 }, { 0x04f9, 0x04f8 },
      { 0x0501, 0x0500 }, { 0x0503, 0x0502 }, { 0x0505, 0x0504 }, { 0x0507, 0x0506 }, { 0x0509, 0x0508 }, { 0x050b, 0x050a }, { 0x050d, 0x050c }, { 0x050f, 0x050e },
      { 0x0561, 0x0531 }, { 0x0562, 0x0532 }, { 0x0563, 0x0533 }, { 0x0564, 0x0534 }, { 0x0565, 0x0535 }, { 0x0566, 0x0536 }, { 0x0567, 0x0537 }, { 0x0568, 0x0538 },
      { 0x0569, 0x0539 }, { 0x056a, 0x053a }, { 0x056b, 0x053b }, { 0x056c, 0x053c }, { 0x056d, 0x053d }, { 0x056e, 0x053e }, { 0x056f, 0x053f }, { 0x0570, 0x0540 },
      { 0x0571, 0x0541 }, { 0x0572, 0x0542 }, { 0x0573, 0x0543 }, { 0x0574, 0x0544 }, { 0x0575, 0x0545 }, { 0x0576, 0x0546 }, { 0x0577, 0x0547 }, { 0x0578, 0x0548 },
      { 0x0579, 0x0549 }, { 0x057a, 0x054a }, { 0x057b, 0x054b }, { 0x057c, 0x054c }, { 0x057d, 0x054d }, { 0x057e, 0x054e }, { 0x057f, 0x054f }, { 0x0580, 0x0550 },
      { 0x0581, 0x0551 }, { 0x0582, 0x0552 }, { 0x0583, 0x0553 }, { 0x0584, 0x0554 }, { 0x0585, 0x0555 }, { 0x0586, 0x0556 }, { 0x1e01, 0x1e00 }, { 0x1e03, 0x1e02 },
      { 0x1e05, 0x1e04 }, { 0x1e07, 0x1e06 }, { 0x1e09, 0x1e08 }, { 0x1e0b, 0x1e0a }, { 0x1e0d, 0x1e0c }, { 0x1e0f, 0x1e0e }, { 0x1e11, 0x1e10 }, { 0x1e13, 0x1e12 },
      { 0x1e15, 0x1e14 }, { 0x1e17, 0x1e16 }, { 0x1e19, 0x1e18 }, { 0x1e1b, 0x1e1a }, { 0x1e1d, 0x1e1c }, { 0x1e1f, 0x1e1e }, { 0x1e21, 0x1e20 }, { 0x1e23, 0x1e22 },
      { 0x1e25, 0x1e24 }, { 0x1e27, 0x1e26 }, { 0x1e29, 0x1e28 }, { 0x1e2b, 0x1e2a }, { 0x1e2d, 0x1e2c }, { 0x1e2f, 0x1e2e }, { 0x1e31, 0x1e30 }, { 0x1e33, 0x1e32 },
      { 0x1e35, 0x1e34 }, { 0x1e37, 0x1e36 }, { 0x1e39, 0x1e38 }, { 0x1e3b, 0x1e3a }, { 0x1e3d, 0x1e3c }, { 0x1e3f, 0x1e3e }, { 0x1e41, 0x1e40 }, { 0x1e43, 0x1e42 },
      { 0x1e45, 0x1e44 }, { 0x1e47, 0x1e46 }, { 0x1e49, 0x1e48 }, { 0x1e4b, 0x1e4a }, { 0x1e4d, 0x1e4c }, { 0x1e4f, 0x1e4e }, { 0x1e51, 0x1e50 }, { 0x1e53, 0x1e52 },
      { 0x1e55, 0x1e54 }, { 0x1e57, 0x1e56 }, { 0x1e59, 0x1e58 }, { 0x1e5b, 0x1e5a }, { 0x1e5d, 0x1e5c }, { 0x1e5f, 0x1e5e }, { 0x1e61, 0x1e60 }, { 0x1e63, 0x1e62 },
      { 0x1e65, 0x1e64 }, { 0x1e67, 0x1e66 }, { 0x1e69, 0x1e68 }, { 0x1e6b, 0x1e6a }, { 0x1e6d, 0x1e6c }, { 0x1e6f, 0x1e6e }, { 0x1e71, 0x1e70 }, { 0x1e73, 0x1e72 },
      { 0x1e75, 0x1e74 }, { 0x1e77, 0x1e76 }, { 0x1e79, 0x1e78 }, { 0x1e7b, 0x1e7a }, { 0x1e7d, 0x1e7c }, { 0x1e7f, 0x1e7e }, { 0x1e81, 0x1e80 }, { 0x1e83, 0x1e82 },
      { 0x1e85, 0x1e84 }, { 0x1e87, 0x1e86 }, { 0x1e89, 0x1e88 }, { 0x1e8b, 0x1e8a }, { 0x1e8d, 0x1e8c }, { 0x1e8f, 0x1e8e }, { 0x1e91, 0x1e90 }, { 0x1e93, 0x1e92 },
      { 0x1e95, 0x1e94 }, { 0x1ea1, 0x1ea0 }, { 0x1ea3, 0x1ea2 }, { 0x1ea5, 0x1ea4 }, { 0x1ea7, 0x1ea6 }, { 0x1ea9, 0x1ea8 }, { 0x1eab, 0x1eaa }, { 0x1ead, 0x1eac },
      { 0x1eaf, 0x1eae }, { 0x1eb1, 0x1eb0 }, { 0x1eb3, 0x1eb2 }, { 0x1eb5, 0x1eb4 }, { 0x1eb7, 0x1eb6 }, { 0x1eb9, 0x1eb8 }, { 0x1ebb, 0x1eba }, { 0x1ebd, 0x1ebc },
      { 0x1ebf, 0x1ebe }, { 0x1ec1, 0x1ec0 }, { 0x1ec3, 0x1ec2 }, { 0x1ec5, 0x1ec4 }, { 0x1ec7, 0x1ec6 }, { 0x1ec9, 0x1ec8 }, { 0x1ecb, 0x1eca }, { 0x1ecd, 0x1ecc },
      { 0x1ecf, 0x1ece }, { 0x1ed1, 0x1ed0 }, { 0x1ed3, 0x1ed2 }, { 0x1ed5, 0x1ed4 }, { 0x1ed7, 0x1ed6 }, { 0x1ed9, 0x1ed8 }, { 0x1edb, 0x1eda }, { 0x1edd, 0x1edc },
      { 0x1edf, 0x1ede }, { 0x1ee1, 0x1ee0 }, { 0x1ee3, 0x1ee2 }, { 0x1ee5, 0x1ee4 }, { 0x1ee7, 0x1ee6 }, { 0x1ee9, 0x1ee8 }, { 0x1eeb, 0x1eea }, { 0x1eed, 0x1eec },
      { 0x1eef, 0x1eee }, { 0x1ef1, 0x1ef0 }, { 0x1ef3, 0x1ef2 }, { 0x1ef5, 0x1ef4 }, { 0x1ef7, 0x1ef6 }, { 0x1ef9, 0x1ef8 }, { 0x1f00, 0x1f08 }, { 0x1f01, 0x1f09 },
      { 0x1f02, 0x1f0a }, { 0x1f03, 0x1f0b }, { 0x1f04, 0x1f0c }, { 0x1f05, 0x1f0d }, { 0x1f06, 0x1f0e }, { 0x1f07, 0x1f0f }, { 0x1f10, 0x1f18 }, { 0x1f11, 0x1f19 },
      { 0x1f12, 0x1f1a }, { 0x1f13, 0x1f1b }, { 0x1f14, 0x1f1c }, { 0x1f15, 0x1f1d }, { 0x1f20, 0x1f28 }, { 0x1f21, 0x1f29 }, { 0x1f22, 0x1f2a }, { 0x1f23, 0x1f2b },
      { 0x1f24, 0x1f2c }, { 0x1f25, 0x1f2d }, { 0x1f26, 0x1f2e }, { 0x1f27, 0x1f2f }, { 0x1f30, 0x1f38 }, { 0x1f31, 0x1f39 }, { 0x1f32, 0x1f3a }, { 0x1f33, 0x1f3b },
      { 0x1f34, 0x1f3c }, { 0x1f35, 0x1f3d }, { 0x1f36, 0x1f3e }, { 0x1f37, 0x1f3f }, { 0x1f40, 0x1f48 }, { 0x1f41, 0x1f49 }, { 0x1f42, 0x1f4a }, { 0x1f43, 0x1f4b },
      { 0x1f44, 0x1f4c }, { 0x1f45, 0x1f4d }, { 0x1f51, 0x1f59 }, { 0x1f53, 0x1f5b }, { 0x1f55, 0x1f5d }, { 0x1f57, 0x1f5f }, { 0x1f60, 0x1f68 }, { 0x1f61, 0x1f69 },
      { 0x1f62, 0x1f6a }, { 0x1f63, 0x1f6b }, { 0x1f64, 0x1f6c }, { 0x1f65, 0x1f6d }, { 0x1f66, 0x1f6e }, { 0x1f67, 0x1f6f }, { 0x1f70, 0x1fba }, { 0x1f71, 0x1fbb },
      { 0x1f72, 0x1fc8 }, { 0x1f73, 0x1fc9 }, { 0x1f74, 0x1fca }, { 0x1f75, 0x1fcb }, { 0x1f76, 0x1fda }, { 0x1f77, 0x1fdb }, { 0x1f78, 0x1ff8 }, { 0x1f79, 0x1ff9 },
      { 0x1f7a, 0x1fea }, { 0x1f7b, 0x1feb }, { 0x1f7c, 0x1ffa }, { 0x1f7d, 0x1ffb }, { 0x1f80, 0x1f88 }, { 0x1f81, 0x1f89 }, { 0x1f82, 0x1f8a }, { 0x1f83, 0x1f8b },
      { 0x1f84, 0x1f8c }, { 0x1f85, 0x1f8d }, { 0x1f86, 0x1f8e }, { 0x1f87, 0x1f8f }, { 0x1f90, 0x1f98 }, { 0x1f91, 0x1f99 }, { 0x1f92, 0x1f9a }, { 0x1f93, 0x1f9b },
      { 0x1f94, 0x1f9c }, { 0x1f95, 0x1f9d }, { 0x1f96, 0x1f9e }, { 0x1f97, 0x1f9f }, { 0x1fa0, 0x1fa8 }, { 0x1fa1, 0x1fa9 }, { 0x1fa2, 0x1faa }, { 0x1fa3, 0x1fab },
      { 0x1fa4, 0x1fac }, { 0x1fa5, 0x1fad }, { 0x1fa6, 0x1fae }, { 0x1fa7, 0x1faf }, { 0x1fb0, 0x1fb8 }, { 0x1fb1, 0x1fb9 }, { 0x1fb3, 0x1fbc }, { 0x1fc3, 0x1fcc },
      { 0x1fd0, 0x1fd8 }, { 0x1fd1, 0x1fd9 }, { 0x1fe0, 0x1fe8 }, { 0x1fe1, 0x1fe9 }, { 0x1fe5, 0x1fec }, { 0x1ff3, 0x1ffc }, { 0x2170, 0x2160 }, { 0x2171, 0x2161 },
      { 0x2172, 0x2162 }, { 0x2173, 0x2163 }, { 0x2174, 0x2164 }, { 0x2175, 0x2165 }, { 0x2176, 0x2166 }, { 0x2177, 0x2167 }, { 0x2178, 0x2168 }, { 0x2179, 0x2169 },
      { 0x217a, 0x216a }, { 0x217b, 0x216b }, { 0x217c, 0x216c }, { 0x217d, 0x216d }, { 0x217e, 0x216e }, { 0x217f, 0x216f }, { 0x24d0, 0x24b6 }, { 0x24d1, 0x24b7 },
      { 0x24d2, 0x24b8 }, { 0x24d3, 0x24b9 }, { 0x24d4, 0x24ba }, { 0x24d5, 0x24bb }, { 0x24d6, 0x24bc }, { 0x24d7, 0x24bd }, { 0x24d8, 0x24be }, { 0x24d9, 0x24bf },
      { 0x24da, 0x24c0 }, { 0x24db, 0x24c1 }, { 0x24dc, 0x24c2 }, { 0x24dd, 0x24c3 }, { 0x24de, 0x24c4 }, { 0x24df, 0x24c5 }, { 0x24e0, 0x24c6 }, { 0x24e1, 0x24c7 },
      { 0x24e2, 0x24c8 }, { 0x24e3, 0x24c9 }, { 0x24e4, 0x24ca }, { 0x24e5, 0x24cb }, { 0x24e6, 0x24cc }, { 0x24e7, 0x24cd }, { 0x24e8, 0x24ce }, { 0x24e9, 0x24cf },
      { 0xff41, 0xff21 }, { 0xff42, 0xff22 }, { 0xff43, 0xff23 }, { 0xff44, 0xff24 }, { 0xff45, 0xff25 }, { 0xff46, 0xff26 }, { 0xff47, 0xff27 }, { 0xff48, 0xff28 },
      { 0xff49, 0xff29 }, { 0xff4a, 0xff2a }, { 0xff4b, 0xff2b }, { 0xff4c, 0xff2c }, { 0xff4d, 0xff2d }, { 0xff4e, 0xff2e }, { 0xff4f, 0xff2f }, { 0xff50, 0xff30 },
      { 0xff51, 0xff31 }, { 0xff52, 0xff32 }, { 0xff53, 0xff33 }, { 0xff54, 0xff34 }, { 0xff55, 0xff35 }, { 0xff56, 0xff36 }, { 0xff57, 0xff37 }, { 0xff58, 0xff38 },
      { 0xff59, 0xff39 }, { 0xff5a, 0xff3a } 
   };

private:
   SmallArray <Utf8Table> utfTable_;

private:
   void buildTable () {
      utfTable_.emplace (0x80, 0x00, 0 * 6, 0x7f, 0); // 1 byte sequence
      utfTable_.emplace (0xe0, 0xc0, 1 * 6, 0x7ff, 0x80); // 2 byte sequence
      utfTable_.emplace (0xf0, 0xe0, 2 * 6, 0xffff, 0x800); // 3 byte sequence
      utfTable_.emplace (0xf8, 0xf0, 3 * 6, 0x1fffff, 0x10000); // 4 byte sequence
      utfTable_.emplace (0xfc, 0xf8, 4 * 6, 0x3ffffff, 0x200000); // 5 byte sequence
      utfTable_.emplace (0xfe, 0xfc, 5 * 6, 0x7fffffff, 0x4000000); // 6 byte sequence
   }

   int32_t multiByteToWideChar (wchar_t *wide, const char *mbs) {
      int32_t len = 0;

      auto ch = *mbs;
      auto lval = static_cast <int> (ch);

      for (const auto &table : utfTable_) {
         len++;

         if ((ch & table.cmask) == table.cval) {
            lval &= table.lmask;

            if (lval < table.lval) {
               return -1;
            }
            *wide = static_cast <wchar_t> (lval);
            return len;
         }
         mbs++;
         auto test = (*mbs ^ 0x80) & 0xff;

         if (test & 0xc0) {
            return -1;
         }
         lval = (lval << 6) | test;
      }
      return -1;
   }

   int32_t wideCharToMultiByte (char *mbs, wchar_t wide) {
      if (!mbs) {
         return 0;
      }
      long lmask = wide;
      int32_t len = 0;

      for (const auto &table : utfTable_) {
         len++;

         if (lmask <= table.lmask) {
            auto ch = table.shift;
            *mbs = static_cast <char> (table.cval | (lmask >> ch));

            while (ch > 0) {
               ch -= 6;
               mbs++;

               *mbs = 0x80 | ((lmask >> ch) & 0x3F);
            }
            return len;
         }
      }
      return -1;
   }

public:
   Utf8Tools () {
      buildTable ();
   }

   ~Utf8Tools () = default;

public:
   wchar_t toUpper (wchar_t ch) {
      int32_t bottom = 0;
      int32_t top = Utf8MaxChars - 1;

      while (bottom <= top) {
         const auto mid = (bottom + top) / 2;
         auto cur = static_cast <wchar_t> (upperTable_[mid].from);

         if (ch == cur) {
            return static_cast <wchar_t> (upperTable_[mid].to);
         }
         if (ch > cur) {
            bottom = mid + 1;
         }
         else {
            top = mid - 1;
         }
      }
      return ch;
   }

   String strToUpper (StringRef in) {
      String result (in);

      auto ptr = const_cast <char *> (result.chars ());
      int32_t len = 0;

      while (*ptr) {
         wchar_t wide = 0;

         multiByteToWideChar (&wide, ptr);
         len += wideCharToMultiByte (ptr, toUpper (wide));

         if (static_cast <size_t> (len) >= result.length ()) {
            break;
         }
      }
      return result.uppercase ();
   }
};

// expose global utf8 tools 
CR_EXPOSE_GLOBAL_SINGLETON (Utf8Tools, utf8tools);


CR_NAMESPACE_END
