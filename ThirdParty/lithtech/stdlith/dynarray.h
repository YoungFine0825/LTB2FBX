//------------------------------------------------------------------
//
//  FILE      : DynArray.h
//
//  PURPOSE   : Caching dynamic arrays used everywhere.
//
//  CREATED   : 5/1/96
//
//  COPYRIGHT : Microsoft 1996 All Rights Reserved
//
//------------------------------------------------------------------


#ifndef __DYNARRAY_H__
#define __DYNARRAY_H__

#ifndef __MEMORY_H__
#include "memory.h"
#endif

#ifndef __L_ALLOCATOR_H__
#include "l_allocator.h"
#endif

#ifndef __GENLIST_H__
#include "genlist.h"
#endif


// Defines....
#define CHECK_ARRAY_BOUNDS
#define CACHE_DEFAULT_SIZE  0


// Predefined types of arrays.
#define CMoPtrArray     CMoArray<void*>
#define CMoDWordArray   CMoArray<uint32>
#define CMoWordArray    CMoArray<WORD>
#define CMoByteArray    CMoArray<unsigned char>
#define DYNA_TEMPLATE template<class T, class C>

#define BAD_INDEX   ((uint32)-1)

// This can be used if you don't want the extra 4 bytes of caching info in the array.
class NoCache
{
public:
    uint32  GetCacheSize() const        {return 0;}
    void    SetCacheSize(uint32 size)   {}
    uint32  GetWantedCache() const      {return 0;}
    void    SetWantedCache(uint32 size) {}
};


class DefaultCache
{
public:
    uint32  GetCacheSize() const    {return m_CacheSize;}
    void    SetCacheSize(uint32 val)    {m_CacheSize = val;}
    uint32  GetWantedCache() const  {return m_WantedCache;}
    void    SetWantedCache(uint32 val){m_WantedCache = val;}

 
private:        
    uint32  m_CacheSize;
    uint32  m_WantedCache;
};



// This is defined as a separate function because it's called by the virtual
// GenFindElement and we don't want to generate a bunch of template code for
// every type of object we use in a list.
uint32 MoArray_FindElementMemcmp(
    const void *pToFind,
    const void *pArray, 
    uint32 nElements, 
    uint32 elementSize);


template<class T, class C=DefaultCache>
class CMoArray : public GenList<T>
{
    public:

        // Constructors                 
                        CMoArray(CMoArray<T, C> &copyFrom, const T &toAdd)
                        {
                            Clear();
                            Init();
                            CopyArray(copyFrom);
                            Append(toAdd);
                        }

                        CMoArray()
                        { 
                            Clear();
                            Init(); 
                        }
                        
                        CMoArray(uint32 cacheSize)
                        {
                            Clear();
                            Init(0, cacheSize); 
                        }
                    
        // Destructor
                        ~CMoArray() { Term(); }

        
        // Member functions
        LTBOOL          Init(uint32 size = 0, uint32 cacheSize = CACHE_DEFAULT_SIZE);
        void            Term(LAlloc *pAlloc=&g_DefAlloc) { SetSize2(0, pAlloc); }

        // WARNING: use these at your own risk.  If you use CopyPointers, you MUST
        // use TermPointers before modifying the array after that.
        void            CopyPointers(CMoArray<T,C> &other);
        void            TermPointers();

        // Comparison
        LTBOOL          Compare(const CMoArray<T, C> &other);
        
        // Assignment
        
        // You should use CopyArray whenever possible.
        CMoArray<T, C>&     operator=(const CMoArray<T, C> &other);
        CMoArray<T, C>      operator+(const T &toAdd);
        
        
        LTBOOL          CopyGenList(const GenList<T> &theList);
        LTBOOL          CopyArray(const CMoArray<T, C> &other);
        LTBOOL          CopyArray2(const CMoArray<T, C> &other, LAlloc *pAlloc);
    
        LTBOOL          AppendArray(const CMoArray<T, C> &other);

        // Find and compare with T::operator==.
        uint32          FindElement(const T x) const;
        
        LTBOOL          Append(const T &toAdd) { return Insert(m_nElements, toAdd); }
        LTBOOL          Insert(uint32 index, const T &toInsert);
        LTBOOL          Insert2(uint32 index, const T &toInsert, LAlloc *pAlloc);
        void            Remove(uint32 index);
        void            Remove2(uint32 index, LAlloc *pAlloc);

        // You can use it like a stack with these...
        LTBOOL          Push(const T &toAdd)  { return Append(toAdd); }
        void            Pop()                   { ASSERT(m_nElements>0); Remove(m_nElements-1); }



        // Accessors
        LTBOOL          IsValid() { return TRUE; }
        
        // Helpers for if you want to wrap around...
        T&              Last()      const   { ASSERT(m_nElements>0); return m_pArray[ m_nElements-1 ]; }
        uint32          LastI()     const   { ASSERT(m_nElements>0); return m_nElements-1; }
        
        T&              Next(uint32 index) const { return m_pArray[ NextI(index) ]; }
        T&              Prev(uint32 index) const { return m_pArray[ PrevI(index) ]; }
        uint32          NextI(uint32 i) const
        {
            #ifdef CHECK_ARRAY_BOUNDS
                ASSERT(m_nElements > 0);
            #endif

            if (i < (m_nElements-1))
                return i+1;
            else
                return 0;
        }

        uint32          PrevI(uint32 i) const
        {
            #ifdef CHECK_ARRAY_BOUNDS
                ASSERT(m_nElements > 0);
            #endif
            
            if (i == 0)
                return m_nElements - 1;
            else
                return i-1;
        }

        // Number of elements
                        operator uint32() const { return (uint32)m_nElements;}

        // Array-like access.
        T&              operator[](const uint32 index) const { return Get(index); }
        
        // Returns false if there isn't enough memory.
        bool SetSize(uint32 newSize);
        LTBOOL          SetSize2(uint32 newSize, LAlloc *pAlloc, bool bQuadWordAlign = false);

        LTBOOL          SetSizeInit(uint32 newSize, T &val);
        LTBOOL          SetSizeInit2(uint32 newSize, T val);
        LTBOOL          SetSizeInit3(uint32 newSize, T &val, LAlloc *pAlloc);
        LTBOOL          SetSizeInit4(uint32 newSize, T val, LAlloc *pAlloc);

 
        // Same as SetSize but preserves the old contents (ie: sizing from 8 to 4 preserves
        // the first 4 and sizing from 4 to 8 preserves the first 4).
        LTBOOL          NiceSetSize(uint32 newSize)                     {return InternalNiceSetSize(newSize, FALSE, &g_DefAlloc);}
        LTBOOL          NiceSetSize2(uint32 newSize, LAlloc *pAlloc)        {return InternalNiceSetSize(newSize, FALSE, pAlloc);}
        
        // Same as NiceSetSize, but uses memcpy instead of operator=.
        LTBOOL          Fast_NiceSetSize(uint32 newSize)                        {return InternalNiceSetSize(newSize, TRUE, &g_DefAlloc);}
        LTBOOL          Fast_NiceSetSize2(uint32 newSize, LAlloc *pAlloc)   {return InternalNiceSetSize(newSize, TRUE, pAlloc);}


        uint32          GetSize() const { return m_nElements; }

        // Sets the cache size
        void            SetCacheSize(uint32 size)
        {
            m_Cache.SetWantedCache(size);
        }

        // Get and set
        T&              Get(uint32 index) const
        {
            #ifdef CHECK_ARRAY_BOUNDS
                ASSERT(index < m_nElements);
            #endif
            return m_pArray[index];
        }

        void            Set(uint32 index, T &data)
        {
            #ifdef CHECK_ARRAY_BOUNDS
                ASSERT(index < m_nElements);
            #endif
            m_pArray[index] = data;
        }

        // Returns a pointer to the internal array..
        T*              GetArray()  { return m_pArray; }
			const T*		GetArray() const	{ return m_pArray; }


// Accessors for MFC compatibility.
public:

        T&              GetAt(uint32 index) const     { return Get(index); }
        void            SetAt(uint32 index, T data)   { Set(index, data); }

        void            RemoveAll()                     { SetSize(0); }
        LTBOOL          Add(const T &toAdd)           { return Insert(m_nElements, toAdd); }

        // Yes, we can be a hash bucket.
        static void     CheckSupportHashBucket() {}


// GenList implementation.
public:

    virtual GenListPos  GenBegin() const
    {
        return GenListPos((uint32)0);
    }

    virtual LTBOOL      GenIsValid(const GenListPos &pos) const
    {
        return pos.m_Index < GetSize();
    }

    virtual T           GenGetNext(GenListPos &pos) const
    {
        T& ret = Get(pos.m_Index);
        ++pos.m_Index;
        return ret;
    }

    virtual T           GenGetAt(GenListPos &pos) const
    {
        return Get(pos.m_Index);
    }

    virtual LTBOOL      GenAppend(T &toAppend)
    {
        return Append(toAppend);
    }

    virtual void        GenRemoveAt(GenListPos pos)
    {
        Remove(pos.m_Index);
    }

    virtual void        GenRemoveAll()
    {
        Term();
    }

    virtual uint32      GenGetSize() const
    {
        return GetSize();
    }

    virtual LTBOOL      GenCopyList(const GenList<T> &theList)
    {
        GenListPos pos;
        uint32 iCurOut;

        if (!SetSize(theList.GenGetSize()))
            return FALSE;

        iCurOut = 0;
        for (pos=theList.GenBegin(); theList.GenIsValid(pos);)
        {
            // BAD bug in theList!
            if (iCurOut >= GetSize())
            {
                Term();
                return FALSE;
            }

            (*this)[iCurOut] = theList.GenGetNext(pos);
            iCurOut++;
        }

        return TRUE;
    }

    virtual LTBOOL      GenAppendList(const GenList<T> &theList)
    {
        GenListPos pos;
        uint32 iCurOut, prevSize;

        
        prevSize = GetSize();
        
        if (!NiceSetSize(prevSize + theList.GenGetSize()))
            return FALSE;

        iCurOut = prevSize;
        for (pos=theList.GenBegin(); theList.GenIsValid(pos);)
        {
            // BAD bug in theList!
            if (iCurOut >= GetSize())
            {
                Term();
                return FALSE;
            }

            (*this)[iCurOut] = theList.GenGetNext(pos);
            iCurOut++;
        }

        return TRUE;
    }

    virtual LTBOOL      GenFindElement(const T &toFind, GenListPos &thePos) const
    {
        thePos.m_Index = MoArray_FindElementMemcmp(
            &toFind,
            m_pArray, 
            m_nElements, 
            sizeof(T));
        
        return thePos.m_Index != BAD_INDEX;
    }

    virtual void        GenSetCacheSize(uint32 size)
    {
        SetCacheSize(size);
    }


private:

        void    _InitArray(uint32 wantedCache);
        void    _DeleteAndDestroyArray(LAlloc *pAlloc, uint32 nElements);
        T*      _AllocateTArray(uint32 nElements, LAlloc *pAlloc, bool bQuadWordAlign = false);

        LTBOOL  InternalNiceSetSize(uint32 newSize, LTBOOL bFast, LAlloc *pAlloc);
        inline uint32   GetNumAllocatedElements()   {return m_nElements + m_Cache.GetCacheSize();}


private:

        void    Clear()
        {
            m_pArray = 0;
            m_nElements = 0;
            m_Cache.SetCacheSize(0);
            m_Cache.SetWantedCache(0);
        }

        // Member variables
        
        T       *m_pArray;
        uint32  m_nElements;

        C       m_Cache;

};


DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::Init(uint32 size, uint32 cacheSize)
{
    Term();
    _InitArray(cacheSize);
    
    return SetSize(size);
}


DYNA_TEMPLATE
void CMoArray<T,C>::CopyPointers(CMoArray<T,C> &other)
{
    Term();
    m_pArray = other.m_pArray;
    m_nElements = other.m_nElements;
    m_Cache = other.m_Cache;
}


DYNA_TEMPLATE
void CMoArray<T,C>::TermPointers()
{
    m_pArray = NULL;
    m_nElements = 0;
    m_Cache.SetCacheSize(0);
    m_Cache.SetWantedCache(0);
}


DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::Compare(const CMoArray<T, C> &other)
{
    uint32  i;

    if (m_nElements != other.m_nElements)
        return FALSE;

    for (i=0; i < m_nElements; i++)
        if (m_pArray[i] != other.m_pArray[i])
            return FALSE;

    return TRUE;
}



template<class T, class C>
CMoArray<T, C>& CMoArray<T, C>::operator=(const CMoArray<T, C> &other)
{
    CopyArray(other);
    return *this;
}


DYNA_TEMPLATE
CMoArray<T, C> CMoArray<T, C>::operator+(const T &toAdd)
{
    return CMoArray<T, C>(*this, toAdd);
}


DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::CopyArray(const CMoArray<T,C> &other)
{
    return CopyArray2(other, &g_DefAlloc);
}


DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::CopyArray2(const CMoArray<T,C> &other, LAlloc *pAlloc)
{
    uint32 i;

    if (m_pArray)
    {
        _DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
    }

    m_nElements = other.m_nElements;
    m_Cache.SetCacheSize(other.m_Cache.GetCacheSize());
    m_Cache.SetWantedCache(other.m_Cache.GetWantedCache());

    if (m_nElements + m_Cache.GetCacheSize() > 0)
    {
        m_pArray = _AllocateTArray(m_nElements + m_Cache.GetCacheSize(), pAlloc);
    }
    else
    {
        m_nElements = 0;
        m_Cache.SetCacheSize(0);
        m_pArray = NULL;
        return TRUE;
    }

    // Could it allocate the array?
    if (!m_pArray)
    {
        m_nElements = 0;
        m_Cache.SetCacheSize(0);
        return FALSE;
    }

    for (i=0; i < m_nElements; i++)
        m_pArray[i] = other.m_pArray[i];

    return TRUE;
}


DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::AppendArray(const CMoArray<T, C> &other)
{
    uint32          i;

    for (i=0; i < other; i++)
        if (!Append(other[i]))
            return FALSE;

    return TRUE;
}


DYNA_TEMPLATE
uint32 CMoArray<T, C>::FindElement(const T x) const
{
    uint32 i, ret = BAD_INDEX;

    for (i=0; i < m_nElements; i++)
    {
        if (m_pArray[i] == x)
        {
            ret = i;
            break;
        }
    }

    return ret;
}



DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::Insert(uint32 index, const T &toInsert)
{
    return Insert2(index, toInsert, &g_DefAlloc);
}



DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::Insert2(uint32 index, const T &toInsert, LAlloc *pAlloc)
{
    T       *pNewArray;
    uint32  newSize, i;

    ASSERT(index <= m_nElements);
    if (index > m_nElements)
        return FALSE;

    // Create a new array (possibly).
    newSize = m_nElements + 1;
    
    //if(newSize >= (m_nElements+m_CacheSize) || m_nElements == 0)
    if (m_Cache.GetCacheSize() == 0)
    {
        pNewArray = _AllocateTArray(newSize + m_Cache.GetWantedCache(), pAlloc);
        if (!pNewArray)
            return FALSE;

        // Copy the old array into the new one, start inserting at index.
        for (i=0; i < index; i++)
            pNewArray[i] = m_pArray[i];

        for (i=index; i < m_nElements; i++)
            pNewArray[i+1] = m_pArray[i];
        
        // Insert the new item into the array
        pNewArray[index] = toInsert;

        // Free the old array and set our pointer to the new one
        if (m_pArray)
        {
            _DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
        }

        m_Cache.SetCacheSize(m_Cache.GetWantedCache());
        m_pArray = pNewArray;
    }
    else
    {
        for (i=m_nElements; i > index; i--)
            m_pArray[i] = m_pArray[i-1];
        
        m_Cache.SetCacheSize(m_Cache.GetCacheSize() - 1);

        m_pArray[index] = toInsert;
    }

    ++m_nElements;      

    return TRUE;
}



DYNA_TEMPLATE
void CMoArray<T, C>::Remove(uint32 index)
{
    Remove2(index, &g_DefAlloc);
}


DYNA_TEMPLATE
void CMoArray<T, C>::Remove2(uint32 index, LAlloc *pAlloc)
{
    uint32 i, newSize, newAllocSize;
    T *pNewArray;
    LTBOOL bSlideDown;
                                            

    ASSERT(index < m_nElements && m_pArray);

    bSlideDown = TRUE;
    if (m_Cache.GetCacheSize() >= (m_Cache.GetWantedCache()*2))
    {
        newSize = m_nElements - 1;
        newAllocSize = newSize + m_Cache.GetWantedCache();
        pNewArray = _AllocateTArray(newAllocSize, pAlloc);

        // Make sure it allocated the array .. if it didn't, just have
        // it slide all the elements down (this guarantees that Remove() 
        // won't fail.)
        if (pNewArray || newAllocSize == 0)
        {
            for (i=0; i < index; i++)
                pNewArray[i] = m_pArray[i];

            for (i=index; i < m_nElements-1; i++)
                pNewArray[i] = m_pArray[i+1];

            _DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
            m_pArray = pNewArray;

            m_Cache.SetCacheSize(m_Cache.GetWantedCache());
            bSlideDown = FALSE;
        }
    }


    if (bSlideDown)
    {
        // Slide them all down one.
        m_Cache.SetCacheSize(m_Cache.GetCacheSize() + 1);
        
        for (i=index; i < m_nElements-1; i++)
            m_pArray[i] = m_pArray[i+1];
    }

    --m_nElements;
}



DYNA_TEMPLATE
bool CMoArray<T, C>::SetSize(uint32 newSize)
{
    return SetSize2(newSize, &g_DefAlloc) != 0;
}


DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::SetSize2(uint32 newSize, LAlloc *pAlloc, bool bQuadWordAlign)
{
    // If they request the current settings, there's no need to change
    if ((newSize == m_nElements) && (newSize != 0))
    {
        return TRUE;
    }

    if (m_pArray)
    {
        _DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
    }

    m_nElements = newSize;
    if (newSize > 0)
    {
        m_pArray = _AllocateTArray(newSize + m_Cache.GetWantedCache(), pAlloc, bQuadWordAlign);
        if (!m_pArray)
        {
            m_nElements = 0;
            m_Cache.SetCacheSize(0);
            return FALSE;
        }
        
        m_Cache.SetCacheSize(m_Cache.GetWantedCache());
    }

    return TRUE;
}


DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::SetSizeInit(uint32 newSize, T &val)
{
    return SetSizeInit3(newSize, val, &g_DefAlloc);
}


DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::SetSizeInit2(uint32 newSize, T val)
{
    return SetSizeInit3(newSize, val, &g_DefAlloc);
}


DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::SetSizeInit3(uint32 newSize, T &val, LAlloc *pAlloc)
{
    uint32 i;

    if (!SetSize2(newSize, pAlloc))
        return FALSE;

    for (i=0; i < GetSize(); i++)
    {
        m_pArray[i] = val;
    }
    
    return TRUE;
}


DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::SetSizeInit4(uint32 newSize, T val, LAlloc *pAlloc)
{
    return SetSizeInit3(newSize, val, pAlloc);
}


DYNA_TEMPLATE
LTBOOL CMoArray<T, C>::InternalNiceSetSize(uint32 newSize, LTBOOL bFast, LAlloc *pAlloc)
{
    T *pNewArray;
    uint32 i, nToCopy;

    // Trivial reject..
    if (newSize < m_nElements)
    {
        m_Cache.SetCacheSize(m_Cache.GetCacheSize() + (m_nElements - newSize));
        m_nElements = newSize;
        return TRUE;
    }
    else if (newSize > m_nElements && (m_nElements + m_Cache.GetCacheSize()) >= newSize)
    {
        m_Cache.SetCacheSize(m_Cache.GetCacheSize() - (newSize - m_nElements));
        m_nElements = newSize;
        return TRUE;
    }
    else if (newSize == m_nElements)
    {
        // uhhh ok..
        return TRUE;
    }

    pNewArray = _AllocateTArray(newSize + m_Cache.GetWantedCache(), pAlloc);
    if (!pNewArray)
        return FALSE;

    nToCopy = m_nElements;
    if (nToCopy > newSize)
        nToCopy = newSize;

    // Copy as many elements as we can.
    if (bFast)
    {
        memcpy(pNewArray, m_pArray, sizeof(T)*nToCopy);
    }
    else
    {
        for (i=0; i < nToCopy; i++)
        {
            pNewArray[i] = m_pArray[i];
        }
    }
    
    // Get rid of the old array and point at the new one.
    _DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
    m_pArray = pNewArray;
    m_nElements = newSize;
    m_Cache.SetCacheSize(m_Cache.GetWantedCache());

    return TRUE;
}


DYNA_TEMPLATE
void CMoArray<T, C>::_InitArray(uint32 wantedCache)
{
    m_pArray = NULL;
    m_nElements = 0;

    m_Cache.SetWantedCache(wantedCache);
    m_Cache.SetCacheSize(0);
}



DYNA_TEMPLATE
T *CMoArray<T, C>::_AllocateTArray(uint32 nElements, LAlloc *pAlloc, bool bQuadWordAlign)
{
    T *tPtr = LNew_Array(pAlloc, T, nElements, bQuadWordAlign);

    return tPtr;
}



DYNA_TEMPLATE
void CMoArray<T, C>::_DeleteAndDestroyArray(LAlloc *pAlloc, uint32 nElements)
{
    if (m_pArray)
    {
        LDelete_Array(pAlloc, m_pArray, nElements);
        m_pArray = NULL;
    }

    m_Cache.SetCacheSize(0);
}


template<class T, class C, class ToAlloc>
LTBOOL AllocateArray2(CMoArray<T, C> &theArray, ToAlloc *pToAlloc, LAlloc *pAllocator)
{
    uint32 i;

    for (i=0; i < theArray; i++)
    {
        theArray[i] = LNew(pAllocator, ToAlloc);
        if (!theArray[i])
            return FALSE;
    }

    return TRUE;
}


template<class T, class C, class ToAlloc>
LTBOOL AllocateArray(CMoArray<T, C> &theArray, ToAlloc *pToAlloc)
{
    return AllocateArray2(theArray, pToAlloc, &g_DefAlloc);
}


template<class T, class V>
void SetArray(T &theArray, V val)
{
    uint32 i;

    for (i=0; i < theArray; i++)
        theArray[i] = val;
}


template<class T>
void DeleteAndClearArray2(T &theArray, LAlloc *pAlloc)
{
    uint32 i;

    for (i=0; i < theArray.GetSize(); i++)
    {
        if (theArray[i])
        {
            LDelete(pAlloc, theArray[i]);
        }
    }

    theArray.SetSize2(0, pAlloc);
}

template<class T>
void DeleteAndClearArray(T &theArray, LAlloc *pAlloc=&g_DefAlloc)
{
    DeleteAndClearArray2(theArray, pAlloc);
}


#endif 




