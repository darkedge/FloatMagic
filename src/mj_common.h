#pragma once
#include <string.h>
#include <stdint.h>
#include "mj_allocator.h"

// Annotation macros

// Variable declaration does not call initialization code
#define MJ_UNINITIALIZED
// Variable declaration calls initialization code
#define MJ_INITIALIZED

#define MJ_COUNTOF(arr) sizeof(arr) / sizeof(arr[0])

// https://www.gingerbill.org/article/2015/08/19/defer-in-cpp/
namespace mj
{
  namespace detail
  {
    template <typename F>
    struct Defer
    {
      F f;
      Defer(F f) : f(f)
      {
      }
      ~Defer()
      {
        f();
      }
    };

    template <typename F>
    Defer<F> defer(F f)
    {
      return Defer<F>(f);
    }
  } // namespace detail
} // namespace mj

#define MJ_CONCAT(x, y)    x##y
#define MJ_XCONCAT(x, y)   MJ_CONCAT(x, y)
#define MJ_DEFER_DETAIL(x) MJ_XCONCAT(x, __COUNTER__)
#define MJ_DEFER(code)     auto MJ_DEFER_DETAIL(_defer_) = mj::detail::defer([&]() { code; })

namespace mj
{
  /// <summary>
  /// Calculates min and max at the same time.
  /// Also checks if the order of a and b was initially correct or not.
  /// </summary>
  /// <typeparam name="T"></typeparam>
  /// <param name="a"></param>
  /// <param name="b"></param>
  /// <param name="outMin">Output min value.</param>
  /// <param name="outMax">Output max value.</param>
  /// <returns>True if a is min and b is max, otherwise false.</returns>
  template <typename T>
  inline constexpr bool minmax(const T& a, const T& b, T& outMin, T& outMax)
  {
    if (a < b)
    {
      outMin = a;
      outMax = b;
      return true;
    }
    else
    {
      outMin = b;
      outMax = a;
      return false;
    }
  }

  template <typename T>
  void swap(T& a, T& b)
  {
    T c = a;
    a   = b;
    b   = c;
  }

  template <typename T>
  class ArrayListView;

  /// <summary>
  /// Reduced functionality mj::ArrayList replacement
  /// </summary>
  template <typename T>
  class ArrayList
  {
  private:
    static constexpr const uint32_t TSize = sizeof(T);

    Allocator* pAllocator;
    T* pData             = nullptr;
    uint32_t numElements = 0;
    uint32_t capacity    = 0;

  public:
    /// <summary>
    /// Does no allocation on construction.
    /// </summary>
    /// <returns></returns>
    ArrayList(Allocator* pAllocator) : pAllocator(pAllocator)
    {
    }

    /// <summary>
    /// ArrayList with an initial capacity.
    /// </summary>
    /// <param name="capacity"></param>
    /// <returns></returns>
    ArrayList(Allocator* pAllocator, uint32_t capacity) : pAllocator(pAllocator), capacity(capacity)
    {
      pData = reinterpret_cast<T*>(this->pAllocator->Allocate(capacity * TSize));
    }

    // Destructor
    ~ArrayList()
    {
      // Destroy elements
      for (auto& t : *this)
      {
        t.~T();
      }

      // Free memory
      this->pAllocator->Free(pData);

      // Invalidate internal state
      pData       = nullptr;
      numElements = 0;
      capacity    = 0;
    }

    /// <summary>
    ///
    /// </summary>
    /// <typeparam name="U"></typeparam>
    /// <returns></returns>
    template <typename U = T>
    ArrayListView<U> ToView()
    {
      return ArrayListView<U>(*this);
    }

    /// <summary>
    /// Reserves additional memory for new objects. No construction is done.
    /// </summary>
    /// <param name="num">Amount of objects</param>
    /// <returns>Pointer to the newly reserved range, or nullptr if there is no more space.</returns>
    T* Reserve(uint32_t num)
    {
      if (num == 0)
      {
        return nullptr;
      }

      if (numElements + num <= capacity)
      {
        T* ptr = pData + numElements;
        numElements += num;
        return ptr;
      }
      else
      {
        if (Expand(numElements + num))
        {
          return Reserve(num);
        }
        else
        {
          return nullptr;
        }
      }
    }

    /// <summary>
    /// Inserts a range of elements at the specified position.
    /// </summary>
    /// <param name="position"></param>
    /// <param name="pSrc"></param>
    /// <param name="num"></param>
    /// <returns>A pointer to the start of the newly inserted elements, or nullptr if there was no space.</returns>
    T* Insert(uint32_t position, const T* pSrc, uint32_t num)
    {
      // Note: These first checks are redundant after we recurse
      if (num == 0)
      {
        return end();
      }
      if (!pSrc)
      {
        return nullptr;
      }
      if (position > this->numElements) // Out of range
      {
        return nullptr;
      }

      if (this->numElements + num <= this->capacity)
      {
        // Number of elements to move right
        uint32_t numMove = this->numElements - position;

        T* ptr = begin() + position;

        // Move elements right
        memmove(end() + num - numMove, ptr, numMove * TSize);

        // Copy new elements into position
        memcpy(ptr, pSrc, num * TSize);

        this->numElements += num;

        return ptr;
      }
      else
      {
        if (Expand(this->numElements + num))
        {
          return Insert(position, pSrc, num);
        }
        else
        {
          return nullptr;
        }
      }
    }

    /// <summary>
    /// Erases a range of elements.
    /// </summary>
    /// <param name="position"></param>
    /// <param name="num"></param>
    /// <returns>
    /// Pointer to the element at the specified position after erasure
    /// (can be the end pointer if all trailing elements were erased)
    /// or nullptr if one or more arguments are out of range
    /// </returns>
    T* Erase(uint32_t position, uint32_t num)
    {
      if (position < this->numElements)
      {
        T* pDst = begin() + position;
        T* pSrc = pDst + num;

        if (pSrc < end())
        {
          // Move elements left
          memmove(pDst, pSrc, (end() - pDst - 1) * TSize);

          this->numElements -= num;

          return pDst;
        }
      }

      return nullptr;
    }

    /// <summary>
    /// Assumes the source does not overlap this ArrayList.
    /// </summary>
    /// <param name="pT"></param>
    /// <param name="length"></param>
    /// <returns></returns>
    bool Assign(const T* pT, uint32_t length)
    {
      if (this->pData)
      {
        this->~ArrayList();
      }

      if (this->Expand(length))
      {
        // TODO MJ: Using memcpy skips copy constructors!
        static_cast<void>(memcpy(this->pData, pT, length * TSize));
        this->numElements = length;
        return true;
      }
      else
      {
        return false;
      }
    }

    bool Add(const T& t)
    {
      if (numElements < capacity)
      {
        T* ptr = pData + numElements;
        *ptr   = t;
        ++numElements;
        return true;
      }
      else
      {
        if (Double())
        {
          return Add(t);
        }
        else
        {
          return nullptr;
        }
      }
    }

    void Clear()
    {
      for (auto& t : *this)
      {
        t.~T();
      }
      this->numElements = 0;
    }

    uint32_t Size() const
    {
      return this->numElements;
    }

    uint32_t Capacity() const
    {
      return this->capacity;
    }

    uint32_t ElemSize() const
    {
      return TSize;
    }

    uint32_t ByteWidth() const
    {
      return this->Size() * this->ElemSize();
    }

    T* Get() const
    {
      return pData;
    }

    T* begin() const
    {
      return pData;
    }

    T* end() const
    {
      return pData + Size();
    }

    T& operator[](uint32_t index)
    {
      // assert(index < numElements);
      return pData[index];
    }

    operator mj::ArrayListView<T>()
    {
      return mj::ArrayListView(pData, numElements);
    }

  private:
    template <typename U>
    friend class ArrayListView;

    bool Double()
    {
      if (this->capacity == 0)
      {
        return Expand(4);
      }
      else
      {
        return Expand(2 * this->capacity);
      }
    }

    bool Expand(uint32_t newCapacity)
    {
      T* ptr = reinterpret_cast<T*>(this->pAllocator->Allocate(newCapacity * ElemSize()));

      if (ptr)
      {
        memcpy(ptr, pData, static_cast<size_t>(numElements) * ElemSize());
        this->pAllocator->Free(pData);
        capacity = newCapacity;
        pData    = ptr;
        return true;
      }
      else
      {
        // pData stays valid
        return false;
      }
    }
  };

  template <typename T>
  class ArrayListView
  {
  private:
    T* pData                              = nullptr;
    uint32_t numElements                  = 0;
    static constexpr const uint32_t TSize = sizeof(T);

  public:
    template <uint32_t Size>
    ArrayListView(T (&data)[Size]) : pData(data), numElements(Size)
    {
    }

    ArrayListView(T* pData, uint32_t numElements) : pData(pData), numElements(numElements)
    {
    }

    /// <summary>
    /// Creates a type-cast view of an ArrayList.
    /// sizeof(U) should be a proper multiple of TSize.
    /// </summary>
    /// <typeparam name="U"></typeparam>
    /// <param name="c"></param>
    /// <returns></returns>
    template <typename U>
    ArrayListView(ArrayList<U>& c) : pData((T*)c.pData), numElements(c.numElements * sizeof(U) / TSize)
    {
    }

    uint32_t Size() const
    {
      return numElements;
    }

    uint32_t ElemSize() const
    {
      return TSize;
    }

    uint32_t ByteWidth() const
    {
      return Size() * ElemSize();
    }

    T* Get() const
    {
      return pData;
    }

    T* begin() const
    {
      return pData;
    }

    T* end() const
    {
      return pData + Size();
    }

    T& operator[](uint32_t index)
    {
      // assert(index < numElements);
      return pData[index];
    }
  };

  /// <summary>
  /// Read/write stream wrapped around a memory buffer
  /// </summary>
  class MemoryBuffer
  {
  private:
    char* end;
    char* position;

  public:
    MemoryBuffer() : end(nullptr), position(nullptr)
    {
    }

    MemoryBuffer(void* pBegin, void* end) : end(static_cast<char*>(end)), position(static_cast<char*>(pBegin))
    {
    }

    MemoryBuffer(void* pBegin, size_t size)
        : end(static_cast<char*>(pBegin) + size), position(static_cast<char*>(pBegin))
    {
    }

    MemoryBuffer(const MemoryBuffer& other) : end(other.end), position(other.position)
    {
    }

    MemoryBuffer& operator=(const MemoryBuffer& rhs)
    {
      this->end      = rhs.end;
      this->position = rhs.position;
      return *this;
    }

    char* Position()
    {
      return this->position;
    }

    size_t SizeLeft()
    {
      return (this->end - this->position);
    }

    template <typename T>
    MemoryBuffer& operator>>(T& t)
    {
      if (SizeLeft() >= sizeof(T))
      {
        memcpy(&t, this->position, sizeof(T));
        this->position += sizeof(T);
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    template <typename T>
    MemoryBuffer& Write(T& t)
    {
      if (SizeLeft() >= sizeof(T))
      {
        memcpy(this->position, &t, sizeof(T));
        this->position += sizeof(T);
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    MemoryBuffer& Write(void* pData, size_t size)
    {
      if (SizeLeft() >= size)
      {
        memcpy(this->position, pData, size);
        this->position += size;
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    template <typename T>
    MemoryBuffer& Read(T& t)
    {
      if (SizeLeft() >= sizeof(T))
      {
        memcpy(&t, this->position, sizeof(T));
        this->position += sizeof(T);
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    MemoryBuffer& Skip(size_t numBytes)
    {
      if (SizeLeft() >= numBytes)
      {
        this->position += numBytes;
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    template <typename T>
    T* ReserveArrayUnaligned(size_t numElements)
    {
      if (SizeLeft() >= (numElements * sizeof(T)))
      {
        T* t = (T*)this->position;
        this->position += (numElements * sizeof(T));
        return t;
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
        return nullptr;
      }
    }

    template <typename T, typename... Args>
    T* NewUnaligned(Args... args)
    {
      if (SizeLeft() >= sizeof(T))
      {
        T* t = (T*)this->position;
        this->position += sizeof(T);
        return new (t) T(args...);
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
        return nullptr;
      }
    }

    template <typename T>
    T* NewArrayUnaligned(size_t numElements)
    {
      if (SizeLeft() >= (numElements * sizeof(T)))
      {
        T* t = (T*)this->position;
        this->position += (numElements * sizeof(T));
        for (size_t i = 0; i < numElements; i++)
        {
          new (t + i) T();
        }
        return t;
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
        return nullptr;
      }
    }

    bool Good()
    {
      return (this->end && this->position);
    }
  };
} // namespace mj
