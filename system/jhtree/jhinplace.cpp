/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2023 HPCC Systems®.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */

#include "platform.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __linux__
#include <alloca.h>
#endif
#include <algorithm>

#include "jmisc.hpp"
#include "jset.hpp"
#include "hlzw.h"

#include "ctfile.hpp"
#include "jhinplace.hpp"
#include "jstats.h"

#ifdef _DEBUG
//#define SANITY_CHECK_INPLACE_BUILDER     // painfully expensive consistency check
//#define TRACE_BUILDING
#endif

static constexpr size32_t minRepeatCount = 2;       // minimum number of times a 0x00 or 0x20 is repeated to generate a special opcode
static constexpr size32_t minRepeatXCount = 3;      // minimum number of times a byte is repeated to generate a special opcode
static constexpr size32_t maxQuotedCount = 31 + 256; // maximum number of characters that can be quoted in a row
static constexpr size32_t repeatDelta = (minRepeatCount - 1);
static constexpr size32_t repeatXDelta = (minRepeatXCount - 1);

static constexpr byte SFmanyDuplicates = 0x80;      // Flag to indicate > 287 duplicates - can occur in a key with a vast number of duplicate keyed entries

//Flags for the node size byte
static constexpr byte NSFuncompressedPayload = 0x40;
static constexpr byte NSFscaleFilepos        = 0x80;    // filepositions are scaled by nodesize

constexpr size32_t getMinRepeatCount(byte value)
{
    return (value == 0x00 || value == 0x20) ? minRepeatCount : minRepeatXCount;
}

enum SquashOp : byte
{
    SqZero      = 0x00,           // repeated zeros
    SqSpace     = 0x01,           // repeated spaces
    SqQuote     = 0x02,           // Quoted text
    SqRepeat    = 0x03,           // any character repeated
    SqOption    = 0x04,           // an set of option bytes
    SqSingle    = 0x05,           // a single character - count indicates which of the common values it is.
    SqSingleX   = 0x06,           // A SqSingle followed by a single byte

    //What other options are worth doing?  Best idea so far is...
    //SqDictX   = 0x07,           // A dictionary of common strings, especially useful for trailing payloads.
};

//MORE: What other special values are worth encoding in real life indexes?
//Only 31 characters are used because it uses the general purpose op encoding, where 32 means add another byte
static constexpr byte specialSingleChars[31] = {
    0, 1, 2, 3, 4, 5, 6, 7,                 // 1-8      Useful for small numbers
    ' ', '0', 'F', 'M', 'U',                // 9-13     Single padding and gender
    '1','2','3','4','5','6','7','8','9',    // 14-22    Textual representation of numbers
    'E','R','N',                            // 23-25    common last letters
    'A','B','C','D',                        // 26-29    Remaining hex digits
    ' ', ' '                                // 30-31    Currently unused.
};

/*

How can you store non-leaf nodes that are compressed, don't need decompression to compare, and are efficient to resolve?

The following compression aims to implement that.  It aims to be cache efficient, rather than minimizing the number
of byte comparisons - in particular it scans blocks of bytes rather than binary chopping them.

The compressed format is a sequence of different "operations".  At its simplest it is an alternating tree of string
matches and byte options.  Other operations are used to compress blocks of bytes that must match.  The match keys
are always a fixed length.

Take the following list of names:
(abel, abigail, absalom, barry, barty, bill, brian, briany) (5, 8, 8, 6, 6, 5, 5, 6) = 49bytes

This is an example of the operations for a keyed length of 7 (sp(n) represents n spaces)

opt(2)
    'a' match(1, "b")
        opt(3)
            'e' match(1, "l") sp(3)         (end)
            'i' match(4, "gail")            (end)
            's' match(4, "alom")            (end)
    'b' opt(3)
        'a' match(1, "r")
            opt(2)
                'r' match(1, "y") sp(2)     (end)
                't' match(1, "y") sp(2)     (end)
        'i' match(3, "ill") sp(3)           (end)
        'r' match(3, "ian")
            opt(2)
                ' ' sp(1)                   (end)
                'y' sp(1)                   (end)

The actual representation in memory of options is slightly different - they hold counts of matching rows within each of the branches,
and offsets of the next part of the match tree.


The representation uses several different methods to compress the data
    All arrays of counts or offsets use a number of bytes required for the largest number they are storing.
    All lengths use packed integers
    The end of the matching is implied by reaching the end of the key, rather than stored as a flag
    Store count-2 for options since there must be at least 2 alternatives
    Store count-1 for the counts of the option branches
    Don't store the offset after the last option branch since it is never needed.
    Use a scaled array for the filepositions, and use a bit to indicate scale by the node size.
    Use a special size code (0) for duplicates at the end of the keyed portion

Done:
  * Add code to trace the node cache sizes and the time taken to search branch nodes.
    Use this to compare i) normal ii) new format iii) new format with direct search
  * Ensure that the allocated node result is padded with 3 zero bytes to allow misaligned access.
  * Use leading bit count operations to implement packing/unpacking operations

Next Steps:
  * Implement findGT (inline in terms of a common function). - delay until new format introduced
    Add to unit tests as well as node code.
  * Allow repeated values and matches to the null record to be compressed - could free up an opcode
    if space/zero matching meant "matches null row"

Future potential optimizations
    compare generated code updating offset rather than search in compare code -> is it more efficient?
    Compare in 4byte chunks and use __builtin_bswap32 to reverse bytes when they differ
    Don't save the last count in an options list (it must = next-prev)
    Move the countBytes=0 processing into the caller function, same file filepos packing, and check not zero in extract function
    Allow nibbles for sizes of arrays
    Store offset-1 for the offsets of the option branches
    Could have varing node sizes for lead and non-leaf nodes, but I suspect it wouldn't save much.
    ?Use LZ END to compress payload in leaf nodes?

Different ways of storing the payload:
  * Small fixed size, store as is, offset calculated
  * small compressed/variablesize.  store size as a byte, but accumulate to find the offsets.
  * large compressed.  store offset as a word

NOTES:
    Always guarantee 3 bytes after last item in an array to allow misaligned read access
    Once you finish building subtree the representation will not change.  Can cache the serialized size and only recalculate if the tree changes.
*/


//For the following the efficiency of serialization isn't a big concern, speed of deserialization is.

constexpr unsigned maxRepeatCount = 32 + 255;
void serializeOp(MemoryBuffer & out, SquashOp op, size32_t count)
{
    assertex(count != 0 && count <= maxRepeatCount);
    if (count <= 31)
    {
        out.append((byte)((op << 5) | count));
    }
    else
    {
        out.append((byte)(op << 5));
        out.append((byte)(count - 32));
    }
}

unsigned sizeSerializedOp(size32_t count)
{
    return (count <= 31) ? 1 : 2;
}


unsigned getSpecialSingleCharIndex(byte search)
{
    for (unsigned i=0; i < sizeof(specialSingleChars); i++)
    {
        if (specialSingleChars[i] == search)
            return i+1;
    }
    return 0;
}

//Packing using a top bit to indicate more.  Revisit if it isn't great.  Could alternatively use rtl packed format, but want to avoid the overhead of a call
static unsigned sizePacked(unsigned __int64 value)
{
    unsigned size = 1;
    while (value >= 0x80)
    {
        value >>= 7;
        size++;
    }
    return size;
}


static void serializePacked(MemoryBuffer & out, unsigned __int64  value)
{
    constexpr unsigned maxBytes = 9;
    unsigned index = maxBytes;
    byte result[maxBytes];

    result[--index] = value & 0x7f;
    while (value >= 0x80)
    {
        value >>= 7;
        result[--index] = (value & 0x7f) | 0x80;
    }
    out.append(maxBytes - index, result + index);
}

inline unsigned readPacked32(const byte * & cur)
{
    byte next = *cur++;
    unsigned value = next;
    if (unlikely(next >= 0x80))
    {
        value = value & 0x7f;
        do
        {
            next = *cur++;
            value = (value << 7) | (next & 0x7f);
        } while (next & 0x80);
    }
    return value;
}

inline unsigned __int64 readPacked64(const byte * & cur)
{
    byte next = *cur++;
    unsigned __int64 value = next;
    if (unlikely(next >= 0x80))
    {
        value = value & 0x7f;
        do
        {
            next = *cur++;
            value = (value << 7) | (next & 0x7f);
        } while (next & 0x80);
    }
    return value;
}
//----- special packed format - code is untested

inline unsigned getLeadingMask(byte extraBytes) { return (0x100U - (1U << (8-extraBytes))); }
inline unsigned getLeadingValueMask(byte extraBytes) { return (~getLeadingMask(extraBytes)) >> 1; }

static void serializePacked2(MemoryBuffer & out, size32_t value)
{
    //Efficiency of serialization is not the key consideration
    byte mask = 0;
    unsigned size = sizePacked(value);
    constexpr unsigned maxBytes = 9;
    byte result[maxBytes];

    for (unsigned i=1; i < size; i++)
    {
        result[maxBytes - i] = value;
        value >>= 8;
        mask = 0x80 | (mask >> 1);
    }
    unsigned start = maxBytes - size;
    result[start] |= mask;
    out.append(size, result + start);
}

inline unsigned numExtraBytesFromFirst1(byte first)
{
    if (first >= 0xF0)
        if (first >= 0xFC)
            if (first >= 0xFE)
                if (first >= 0xFF)
                    return 8;
                else
                    return 7;
            else
                return 6;
        else
            if (first >= 0xF8)
                return 5;
            else
                return 4;
    else
        if (first >= 0xC0)
            if (first >= 0xE0)
                return 3;
            else
                return 2;
        else
            return (first >> 7);    // (first >= 0x80) ? 1 : 0
}

inline unsigned numExtraBytesFromFirst2(byte first)
{
    //Surely should be faster, but seems slower on AMD. Retest in its actual context
    unsigned value = first;
    return countLeadingUnsetBits(~(value << 24));
}

inline unsigned numExtraBytesFromFirst(byte first)
{
    return numExtraBytesFromFirst1(first);
}

inline unsigned readPacked2(const byte * & cur)
{
    byte first = *cur++;
    unsigned extraBytes = numExtraBytesFromFirst(first);
    unsigned value = first & getLeadingValueMask(extraBytes);
    while (extraBytes--)
    {
        value <<= 8;
        value |= *cur++;
    }
    return value;
}

//-------------------

static unsigned bytesRequired(unsigned __int64 value)
{
    unsigned bytes = 1;
    while (value >= 0x100)
    {
        value >>= 8;
        bytes++;
    }
    return bytes;
}

void serializeBytes(MemoryBuffer & out, unsigned __int64 value, unsigned bytes)
{
    for (unsigned i=0; i < bytes; i++)
    {
        out.append((byte)value);
        value >>= 8;
    }
    assertex(value == 0);
}

unsigned readBytesEntry32(const byte * address, unsigned index, unsigned bytes)
{
    dbgassertex(bytes != 0);

    //Non aligned access, assumes little endian, but avoids a loop
    const byte * addrValue = address + index * bytes;
    unsigned value = *(const unsigned *)addrValue;
    unsigned mask = 0xFFFFFFFF >> ((4 - bytes) * 8);
    return (value & mask);
}

//MORE: Inline would probably speed this up
unsigned readBytesEntry16(const byte * address, unsigned index, unsigned bytes)
{
    dbgassertex(bytes != 0);

    //Use a shift (works for bytes = 1 or 2) to avoid a multiply
    const byte * addrValue = address + (index << (bytes - 1));
    unsigned short value = *(const unsigned short *)addrValue;
    if (bytes == 1)
        value = (byte)value;
    return value;
}

unsigned __int64 readBytesEntry64(const byte * address, unsigned index, unsigned bytes)
{
    dbgassertex(bytes != 0);

    //Non aligned access, assumes little endian, but avoids a loop
    const unsigned __int64 * pValue = (const unsigned __int64 *)(address + index * bytes);
    unsigned __int64 value = *pValue;
    unsigned __int64 mask = U64C(0xFFFFFFFFFFFFFFFF) >> ((8 - bytes) * 8);
    return (value & mask);
}

//=========================================================================================================

class PartialMatchBuilder;
//---------------------------------------------------------------------------------------------------------------------

bool PartialMatch::allNextAreEnd()
{
    ForEachItemIn(i, next)
    {
        if (!next.item(i).isEnd())
            return false;
    }
    return true;
}

void PartialMatch::cacheSizes()
{
    if (!dirty)
        return;

    squash();
    dirty = false;
    if (squashed && squashedData.length())
        size = squashedData.length();
    else
        size = 0;

    maxOffset = 0;
    unsigned numNext = next.ordinality();
    if (numNext)
    {
        if (allNextAreEnd())
        {
            if (numNext <= maxRepeatCount)
            {
                size += sizeSerializedOp(numNext-1);  // count of options
                size += 1;                      // end marker
            }
            else
            {
                //Unusual situation where there are very large numbers of duplicate key entries.
                size += sizeSerializedOp(1);  // count of options
                size += 1;                    // end marker
                size += sizePacked(numNext - maxRepeatCount);
            }

            maxCount = numNext;
        }
        else
        {
            size32_t offset = 0;
            maxCount = 0;
            for (unsigned i=0; i < numNext; i++)
            {
                maxCount += next.item(i).getCount();
                maxOffset = offset;
                offset += next.item(i).getSize();
            }

            size += sizeSerializedOp(numNext-1);  // count of options
            size += 1;                      // count and offset table information
            size += numNext;                // bytes of data

            //Space for the count table - if it is required
            if (maxCount != numNext)
                size += (numNext * bytesRequired(maxCount-1));

            //offset table.
            size += (numNext - 1) * bytesRequired(maxOffset);

            size += offset;                       // embedded tree
        }
    }
    else
        maxCount = 1;
}

bool PartialMatch::combine(size32_t newLen, const byte * newData)
{
    size32_t curLen = data.length();
    const byte * curData = (const byte *)data.toByteArray();
    unsigned compareLen = std::min(newLen, curLen);
    unsigned matchLen;
    for (matchLen = 0; matchLen < compareLen; matchLen++)
    {
        if (curData[matchLen] != newData[matchLen])
            break;
    }

    if (matchLen || isRoot)
    {
        dirty = true;
        if (next.ordinality() == 0)
        {
            next.append(*new PartialMatch(builder, curLen - matchLen, curData + matchLen, rowOffset + matchLen, false));
            next.append(*new PartialMatch(builder, newLen - matchLen, newData + matchLen, rowOffset + matchLen, false));
            data.setLength(matchLen);
            squashed = false;
            return true;
        }

        if (matchLen == curLen)
        {
            //Either add a new option, or combine with the last option
            if (next.tos().combine(newLen - matchLen, newData + matchLen))
                return true;
            next.append(*new PartialMatch(builder, newLen - matchLen, newData + matchLen, rowOffset + matchLen, false));
            return true;
        }

        //Split this node into two
        Owned<PartialMatch> childNode = new PartialMatch(builder, curLen-matchLen, curData + matchLen, rowOffset + matchLen, false);
        next.swapWith(childNode->next);
        next.append(*childNode.getClear());
        next.append(*new PartialMatch(builder, newLen - matchLen, newData + matchLen, rowOffset + matchLen, false));
        data.setLength(matchLen);
        squashed = false;
        return true;
    }
    return false;
}

size32_t PartialMatch::getSize()
{
    cacheSizes();
    return size;
}

size32_t PartialMatch::getCount()
{
    cacheSizes();
    return maxCount;
}

size32_t PartialMatch::getMaxOffset()
{
    cacheSizes();
    return maxOffset;
}

bool PartialMatch::removeLast()
{
    dirty = true;
    if (next.ordinality() == 0)
        return true; // merge with caller
    if (next.tos().removeLast())
    {
        if (next.ordinality() == 2)
        {
            //Remove the second entry, and merge this element with the first
            Linked<PartialMatch> first = &next.item(0);
            next.kill();
            data.append(first->data);
            next.swapWith(first->next);
            squashed = false;
            return false;
        }
        else
        {
            next.pop();
            return false;
        }
    }
    return false;
}

void PartialMatch::serialize(MemoryBuffer & out)
{
    squash();

    size32_t originalPos = out.length();
    if (squashed && squashedData.length())
        out.append(squashedData);
    else
    {
        unsigned skip = isRoot ? 0 : 1;
        assertex (data.length() <= skip);
    }

    unsigned numNext = next.ordinality();
    if (numNext)
    {
        if (allNextAreEnd())
        {
            builder->numDuplicates++;
            if (numNext <= maxRepeatCount)
            {
                serializeOp(out, SqOption, numNext-1);  // count of options
                out.append((byte)0);
            }
            else
            {
                //Unusual situation where there are very large numbers of duplicate key entries.
                //Set the top bit on the sizeinfo.  The number is currently needed - so could potentially remove!
                serializeOp(out, SqOption, 1);  // count of options
                out.append((byte)SFmanyDuplicates);
                serializePacked(out, numNext - maxRepeatCount);
            }
        }
        else
        {
            byte offsetBytes = bytesRequired(getMaxOffset());
            byte countBytes = bytesRequired(getCount()-1);

            byte sizeInfo = offsetBytes;
            if (getCount() != numNext)
                sizeInfo |= (countBytes << 3);

            serializeOp(out, SqOption, numNext-1);  // count of options
            out.append(sizeInfo);

            for (unsigned iFirst = 0; iFirst < numNext; iFirst++)
            {
                next.item(iFirst).serializeFirst(out);
            }

            //Space for the count table - if it is required
            if (getCount() != numNext)
            {
                unsigned runningCount = 0;
                for (unsigned iCount = 0; iCount < numNext; iCount++)
                {
                    runningCount += next.item(iCount).getCount();
                    serializeBytes(out, runningCount-1, countBytes);
                }
                assertex(getCount() == runningCount);
            }

            unsigned offset = 0;
            for (unsigned iOff=1; iOff < numNext; iOff++)
            {
                offset += next.item(iOff-1).getSize();
                serializeBytes(out, offset, offsetBytes);
            }

            for (unsigned iNext=0; iNext < numNext; iNext++)
                next.item(iNext).serialize(out);
        }
    }

    size32_t newPos = out.length();
    assertex(newPos - originalPos == getSize());
}

unsigned PartialMatch::appendRepeat(size32_t offset, size32_t copyOffset, byte repeatByte, size32_t repeatCount)
{
    unsigned numOps = 0;
    const byte * source = data.bytes();
    size32_t copySize = (offset - copyOffset) - repeatCount;
    if (copySize)
    {
        //First append any non repeated data that comes before the repeat
        bool useSpecial = true;
        bool prevSpecial = false;
        for (unsigned i=0; i < copySize; i++)
        {
            if (getSpecialSingleCharIndex(source[copyOffset+i]) == 0)
            {
                if (!prevSpecial)
                {
                    useSpecial = false;
                    break;
                }
                prevSpecial = false;
            }
            else
                prevSpecial = true;
        }

        //If the copied data can all use "special" characters then use those rather than quoting
        //because it will be shorter.
        if (useSpecial)
        {
            for (unsigned i=0; i < copySize; i++)
            {
                unsigned special = getSpecialSingleCharIndex(source[copyOffset+i]);
                if (i + 1 < copySize)
                {
                    byte nextByte = source[copyOffset+i+1];
                    unsigned nextSpecial = getSpecialSingleCharIndex(nextByte);
                    if (nextSpecial == 0)
                    {
                        serializeOp(squashedData, SqSingleX, special);
                        squashedData.append(nextByte);
                        i++;
                    }
                    else
                        serializeOp(squashedData, SqSingle, special);
                }
                else
                    serializeOp(squashedData, SqSingle, special);
            }
            copySize = 0;
        }

        //Quote if special characters cannot represent.  Not worth worrying if portions could use the special characters.
        while (copySize)
        {
            size32_t chunkSize = std::min(copySize, maxQuotedCount);
            serializeOp(squashedData, SqQuote, chunkSize);
            squashedData.append(chunkSize, source+copyOffset);
            copyOffset += chunkSize;
            copySize -= chunkSize;
            numOps++;
        }
    }
    if (repeatCount)
    {
        switch (repeatByte)
        {
        case 0x00:
            serializeOp(squashedData, SqZero, repeatCount - repeatDelta);
            break;
        case 0x20:
            serializeOp(squashedData, SqSpace, repeatCount - repeatDelta);
            break;
        default:
            serializeOp(squashedData, SqRepeat, repeatCount - repeatXDelta);
            squashedData.append(repeatByte);
            break;
        }
        numOps++;
    }
    return numOps;
}

bool PartialMatch::squash()
{
    if (!squashed)
    {
        squashed = true;
        squashedData.clear();

        //Always squash if you have some text - avoids length calculation elsewhere
        size32_t startOffset = isRoot ? 0 : 1;
        size32_t keyLen = builder->queryKeyLen();
        if (data.length() > startOffset)
        {
            const byte * source = data.bytes();
            size32_t maxOffset = data.length();

            unsigned copyOffset = startOffset;
            unsigned repeatCount = 0;
            byte prevByte = 0;
            const byte * nullRow = queryNullRow();
            for (unsigned offset = startOffset; offset < maxOffset; offset++)
            {
                byte nextByte = source[offset];

                //MORE Add support for compressing against the null row at somepoint - could lead to better compression over
                //field boundaries.
                if (nextByte == prevByte)
                {
                    repeatCount++;
                    if (repeatCount >= maxRepeatCount + repeatDelta)
                    {
                        appendRepeat(offset+1, copyOffset, prevByte, repeatCount);
                        copyOffset = offset+1;
                        repeatCount = 0;
                    }
                }
                else
                {
                    //MORE: Revisit and make this more sophisticated.  Trade off between space and comparison time.
                    //  if space or /0 decrease the threshold by 1.
                    //  if the start of the string then reduce the threshold.
                    //  If no child entries increase the threshold (since it may require a special continuation byte)?
                    if (repeatCount > 3)
                    {
                        appendRepeat(offset, copyOffset, prevByte, repeatCount);
                        copyOffset = offset;
                    }
                    repeatCount = 1;
                    prevByte = nextByte;
                }
            }

            if (repeatCount < getMinRepeatCount(prevByte))
                repeatCount = 0;

            appendRepeat(maxOffset, copyOffset, prevByte, repeatCount);
        }
    }
    return dirty;
}

const byte * PartialMatch::queryNullRow()
{
    return builder->queryNullRow();
}

//MORE: Pass this in...
void PartialMatch::serializeFirst(MemoryBuffer & out)
{
    if (data.length())
        out.append(data.bytes()[0]);
    else if (rowOffset < builder->queryKeyLen())
    {
        assertex(queryNullRow());
        out.append(queryNullRow()[rowOffset]);
    }
    else
        out.append((byte)0);    // will probably be harmless - potentially appending an extra 0 byte to shorter payloads.
}

void PartialMatch::describeSquashed(StringBuffer & out)
{
    const byte * cur = squashedData.bytes();
    const byte * end = cur + squashedData.length();
    while (cur < end)
    {
        byte nextByte = *cur++;
        SquashOp op = (SquashOp)(nextByte >> 5);
        unsigned count = (nextByte & 0x1f);
        if (count == 0)
            count = 32 + *cur++;

        switch (op)
        {
        case SqZero:
            out.appendf("ze(%u) ", count + repeatDelta);
            break;
        case SqSpace:
            out.appendf("sp(%u) ", count + repeatDelta);
            break;
        case SqQuote:
            out.appendf("qu(%u,", count);
            for (unsigned i=0; i < count; i++)
                out.appendf(" %02x", cur[i]);
            out.append(") ");
            cur += count;
            break;
        case SqRepeat:
            out.appendf("re(%u, %02x) ", count + repeatXDelta, *cur);
            cur++;
            break;
        case SqSingle:
            out.appendf("si(%u) ", count);
            break;
        case SqSingleX:
            out.appendf("sx(%u %02x) ", count, *cur);
            cur++;
            break;
        }
    }
}


void PartialMatch::trace(unsigned indent)
{
    StringBuffer squashedText;
    describeSquashed(squashedText);

    StringBuffer clean;
    StringBuffer dataHex;
    for (unsigned i=0; i < data.length(); i++)
    {
        byte next = data.bytes()[i];
        dataHex.appendhex(next, true);
        if (isprint(next))
            clean.append(next);
        else
            clean.append('.');
    }

    StringBuffer text;
    text.appendf("%*s(%s[%s] %u:%u[%u] [%s]", indent, "", clean.str(), dataHex.str(), data.length(), squashedData.length(), getSize(), squashedText.str());
    if (next.ordinality())
    {
        DBGLOG("%s, %u{", text.str(), next.ordinality());
        ForEachItemIn(i, next)
            next.item(i).trace(indent+1);
        DBGLOG("%*s})", indent, "");
    }
    else
        DBGLOG("%s)!", text.str());
}

//---------------------------------------------------------------------------------------------------------------------

void PartialMatchBuilder::add(size32_t len, const void * data)
{
    if (!root)
        root.set(new PartialMatch(this, len, data, 0, true));
    else
        root->combine(len, (const byte *)data);

#ifdef SANITY_CHECK_INPLACE_BUILDER
    MemoryBuffer out;
    root->serialize(out);
#endif
}

void PartialMatchBuilder::removeLast()
{
    root->removeLast();
}


void PartialMatchBuilder::serialize(MemoryBuffer & out)
{
    root->serialize(out);
}

void PartialMatchBuilder::trace()
{
    if (root)
        root->trace(0);
}

unsigned PartialMatchBuilder::getCount()
{
    return root ? root->getCount() : 0;
}

unsigned PartialMatchBuilder::getSize()
{
    return root ? root->getSize() : 0;
}

//---------------------------------------------------------------------------------------------------------------------

InplaceNodeSearcher::InplaceNodeSearcher(unsigned _count, const byte * data, size32_t _keyLen, const byte * _nullRow)
: nodeData(data), nullRow(_nullRow), count(_count), keyLen(_keyLen)
{
}

void InplaceNodeSearcher::init(unsigned _count, const byte * data, size32_t _keyLen, const byte * _nullRow)
{
    count = _count;
    nodeData = data;
    keyLen = _keyLen;
    nullRow = _nullRow;
}

int InplaceNodeSearcher::compareValueAt(const char * search, unsigned int compareIndex) const
{
    unsigned resultPrev = 0;
    unsigned resultNext = count;
    const byte * finger = nodeData;
    unsigned offset = 0;

    while (offset < keyLen)
    {
        byte next = *finger++;
        SquashOp op = (SquashOp)(next >> 5);
        unsigned count = (next & 0x1f);
        if (count == 0)
            count = 32 + *finger++;

        switch (op)
        {
        case SqQuote:
        {
            unsigned numBytes = count;
            for (unsigned i=0; i < numBytes; i++)
            {
                const byte nextSearch = search[i];
                const byte nextFinger = finger[i];
                if (nextFinger != nextSearch)
                {
                    if (offset + i >= keyLen)
                        return 0;

                    if (nextFinger > nextSearch)
                    {
                        //This entry is larger than the search value => we have a match
                        return -1;
                    }
                    else
                    {
                        //This entry (and all children) are less than the search value
                        //=> the next entry is the match
                        return +1;
                    }
                }
            }
            search += numBytes;
            offset += numBytes;
            finger += numBytes;
            break;
        }
        case SqSingle:
        {
            const byte nextSearch = search[0];
            const byte nextFinger = specialSingleChars[count-1];
            if (nextFinger != nextSearch)
            {
                if (nextFinger > nextSearch)
                    return -1;
                else
                    return +1;
            }
            search += 1;
            offset += 1;
            break;
        }
        case SqSingleX:
        {
            {
                const byte nextSearch = search[0];
                const byte nextFinger = specialSingleChars[count-1];
                if (nextFinger != nextSearch)
                {
                    if (nextFinger > nextSearch)
                        return -1;
                    else
                        return +1;
                }
            }
            {
                const byte nextSearch = search[1];
                const byte nextFinger = *finger++;
                if (nextFinger != nextSearch)
                {
                    if (nextFinger > nextSearch)
                        return -1;
                    else
                        return +1;
                }
            }
            search += 2;
            offset += 2;
            break;
        }
        case SqZero:
        case SqSpace:
        {
            const byte nextFinger = (op == SqZero) ? 0 : ' ';
            unsigned numBytes = count + repeatDelta;
            for (unsigned i=0; i < numBytes; i++)
            {
                const byte nextSearch = search[i];
                if (nextFinger != nextSearch)
                {
                    if (offset + i >= keyLen)
                        return 0;
                    if (nextFinger > nextSearch)
                        return -1;
                    else
                        return +1;
                }
            }
            search += numBytes;
            offset += numBytes;
            break;
        }
        case SqRepeat:
        {
            const byte nextFinger = *finger++;
            unsigned numBytes = count + repeatXDelta;
            for (unsigned i=0; i < numBytes; i++)
            {
                const byte nextSearch = search[i];
                if (nextFinger != nextSearch)
                {
                    if (offset + i >= keyLen)
                        return 0;
                    if (nextFinger > nextSearch)
                        return -1;
                    else
                        return +1;
                }
            }
            search += numBytes;
            offset += numBytes;
            break;
        }
        case SqOption:
        {
            const unsigned numOptions = count+1;
            byte sizeInfo = *finger++;
            byte bytesPerCount = (sizeInfo >> 3) & 7; // could pack other information in....
            byte bytesPerOffset = (sizeInfo & 7);

            //Duplicates can occur on the last byte of the key - if so we have a match
            //MORE: I'm not sure this code can ever be reached - see HPCC-29108
            if (bytesPerOffset == 0)
            {
#ifdef _DEBUG
                unsigned options = numOptions;
                if (sizeInfo & SFmanyDuplicates)
                    options = readPacked32(finger) + maxRepeatCount;
                assertex(resultNext == resultPrev+options);
#endif
                return 0;
            }

            const byte * counts = finger + numOptions; // counts (if present) follow the data
            const byte nextSearch = search[0];
            for (unsigned i=0; i < numOptions; i++)
            {
                const byte nextFinger = finger[i];
                if (nextFinger > nextSearch)
                {
                    //This entry is greater than search => item(i) is the correct entry
                    if (i == 0)
                        return -1;

                    unsigned delta;
                    if (bytesPerCount == 0)
                        delta = i;
                    else
                        delta = readBytesEntry16(counts, i-1, bytesPerCount) + 1;
                    unsigned matchIndex = resultPrev + delta;
                    return (compareIndex >= matchIndex) ? -1 : +1;
                }
                else if (nextFinger < nextSearch)
                {
                    //The entry is < search => keep looping
                }
                else
                {
                    if (bytesPerCount == 0)
                    {
                        resultPrev += i;
                        resultNext = resultPrev+1;
                    }
                    else
                    {
                        //Exact match.  Reduce the range of the match counts using the running counts
                        //stored for each of the options, and continue matching.
                        resultNext = resultPrev + readBytesEntry16(counts, i, bytesPerCount)+1;
                        if (i > 0)
                            resultPrev += readBytesEntry16(counts, i-1, bytesPerCount)+1;
                    }

                    //If the compareIndex is < the lower bound for the match index the search value must be higher
                    if (compareIndex < resultPrev)
                        return +1;

                    //If the compareIndex is >= the upper bound for the match index the search value must be lower
                    if (compareIndex >= resultNext)
                        return -1;

                    const byte * offsets = counts + numOptions * bytesPerCount;
                    const byte * next = offsets + (numOptions-1) * bytesPerOffset;
                    finger = next;
                    if (i > 0)
                        finger += readBytesEntry16(offsets, i-1, bytesPerOffset);
                    search++;
                    offset++;
                    //Use a goto because we can't continue the outer loop from an inner loop
                    goto nextTree;
                }
            }

            //Search is > all elements
            return +1;
        }
        }

    nextTree:
        ;
    }

    return 0;
}

//Find the first row that is >= the search row
unsigned InplaceNodeSearcher::findGE(const unsigned len, const byte * search) const
{
    unsigned resultPrev = 0;
    unsigned resultNext = count;
    const byte * finger = nodeData;
    unsigned offset = 0;

    while (offset < keyLen)
    {
        byte next = *finger++;
        SquashOp op = (SquashOp)(next >> 5);
        unsigned count = (next & 0x1f);
        if (count == 0)
            count = 32 + *finger++;

        switch (op)
        {
        case SqQuote:
        {
            unsigned numBytes = count;
            for (unsigned i=0; i < numBytes; i++)
            {
                const byte nextSearch = search[i];
                const byte nextFinger = finger[i];
                if (nextFinger != nextSearch)
                {
                    if (nextFinger > nextSearch)
                    {
                        //This entry is larger than the search value => we have a match
                        return resultPrev;
                    }
                    else
                    {
                        //This entry (and all children) are less than the search value
                        //=> the next entry is the match
                        return resultNext;
                    }
                }
            }
            search += numBytes;
            offset += numBytes;
            finger += numBytes;
            break;
        }
        case SqSingle:
        {
            const byte nextSearch = search[0];
            const byte nextFinger = specialSingleChars[count-1];
            if (nextFinger != nextSearch)
            {
                if (nextFinger > nextSearch)
                    return resultPrev;
                else
                    return resultNext;
            }
            search += 1;
            offset += 1;
            break;
        }
        case SqSingleX:
        {
            {
                const byte nextSearch = search[0];
                const byte nextFinger = specialSingleChars[count-1];
                if (nextFinger != nextSearch)
                {
                    if (nextFinger > nextSearch)
                        return resultPrev;
                    else
                        return resultNext;
                }
            }
            {
                const byte nextSearch = search[1];
                const byte nextFinger = *finger++;
                if (nextFinger != nextSearch)
                {
                    if (nextFinger > nextSearch)
                        return resultPrev;
                    else
                        return resultNext;
                }
            }
            search += 2;
            offset += 2;
            break;
        }
        case SqZero:
        case SqSpace:
        {
            const byte nextFinger = (op == SqZero) ? 0 : ' ';
            unsigned numBytes = count + repeatDelta;
            for (unsigned i=0; i < numBytes; i++)
            {
                const byte nextSearch = search[i];
                if (nextFinger != nextSearch)
                {
                    if (nextFinger > nextSearch)
                        return resultPrev;
                    else
                        return resultNext;
                }
            }
            search += numBytes;
            offset += numBytes;
            break;
        }
        case SqRepeat:
        {
            const byte nextFinger = *finger++;
            unsigned numBytes = count + repeatXDelta;
            for (unsigned i=0; i < numBytes; i++)
            {
                const byte nextSearch = search[i];
                if (nextFinger != nextSearch)
                {
                    if (nextFinger > nextSearch)
                        return resultPrev;
                    else
                        return resultNext;
                }
            }
            search += numBytes;
            offset += numBytes;
            break;
        }
        case SqOption:
        {
            //Read early to give more time to load before they are used
            byte sizeInfo = *finger++;
            const byte nextSearch = search[0];

            const unsigned numOptions = count+1;
            //Could also use fewer bits for the count if there was a use for the other bits.
            byte bytesPerCount = (sizeInfo >> 3) & 7; // could pack other information in....
            byte bytesPerOffset = (sizeInfo & 7);
            dbgassertex(bytesPerCount <= 2);
            dbgassertex(bytesPerOffset <= 2);

            //MORE: Duplicates can occur on the last byte of the key - if so we have a match
            //MORE: This is never executed - it should never be generated....
            dbgassertex(bytesPerOffset != 0);
            #if 0
            if (bytesPerOffset == 0)
            {
                dbgassertex(resultNext == resultPrev+numOptions);
                break;
            }
            #endif

            const byte * counts = finger + numOptions; // counts follow the data
            for (unsigned i=0; i < numOptions; i++)
            {
                const byte nextFinger = finger[i];
                if (likely(nextFinger < nextSearch))
                {
                    //The entry is < search => keep looping
                    continue;
                }

                if (nextFinger > nextSearch)
                {
                    //This entry is greater than search => this is the correct entry
                    if (bytesPerCount == 0)
                        return resultPrev + i;
                    if (i == 0)
                        return resultPrev;
                    return resultPrev + readBytesEntry16(counts, i-1, bytesPerCount) + 1;
                }

                const byte * offsets = counts + numOptions * bytesPerCount;

                //Avoid multiplications..  only works for 1-2 bytes per offset
                //const byte * next = offsets + (numOptions-1) * bytesPerOffset;
                const byte * next = offsets + ((numOptions-1) << (bytesPerOffset -1));

                if (i > 0)
                {
                    next += readBytesEntry16(offsets, i-1, bytesPerOffset);
                    __builtin_prefetch(next);
                }

                if (bytesPerCount == 0)
                {
                    resultPrev += i;
                    resultNext = resultPrev+1;
                }
                else
                {
                    //Exact match.  Reduce the range of the match counts using the running counts
                    //stored for each of the options, and continue matching.
                    resultNext = resultPrev + readBytesEntry16(counts, i, bytesPerCount)+1;
                    if (i > 0)
                        resultPrev += readBytesEntry16(counts, i-1, bytesPerCount)+1;
                }

                finger = next;
                search++;
                offset++;
                //Use a goto because we can't continue the outer loop from an inner loop
                goto nextTree;
            }

            //Did not match any => next value matches
            return resultNext;
        }
        }

    nextTree:
        ;
    }

    return resultPrev;
}

size32_t InplaceNodeSearcher::getValueAt(unsigned int searchIndex, char *key) const
{
    if (searchIndex >= count)
        return 0;

    unsigned resultPrev = 0;
    unsigned resultNext = count;
    const byte * finger = nodeData;
    unsigned offset = 0;

    while (offset < keyLen)
    {
        byte next = *finger++;
        SquashOp op = (SquashOp)(next >> 5);
        unsigned count = (next & 0x1f);
        if (count == 0)
            count = 32 + *finger++;

        switch (op)
        {
        case SqQuote:
        {
            unsigned i=0;
            unsigned numBytes = count;
            for (; i < numBytes; i++)
                key[offset+i] = finger[i];
            offset += numBytes;
            finger += numBytes;
            break;
        }
        case SqSingle:
        {
            key[offset] = specialSingleChars[count-1];
            offset += 1;
            break;
        }
        case SqSingleX:
        {
            key[offset] = specialSingleChars[count-1];
            key[offset+1] = finger[0];
            offset += 2;
            finger++;
            break;
        }
        case SqZero:
        case SqSpace:
        {
            const byte nextFinger = "\x00\x20"[op];
            unsigned numBytes = count + repeatDelta;
            for (unsigned i=0; i < numBytes; i++)
                key[offset+i] = nextFinger;
            offset += numBytes;
            break;
        }
        case SqRepeat:
        {
            const byte nextFinger = *finger++;
            unsigned numBytes = count + repeatXDelta;
            for (unsigned i=0; i < numBytes; i++)
                key[offset+i] = nextFinger;

            offset += numBytes;
            break;
        }
        case SqOption:
        {
            const unsigned numOptions = count+1;
            byte sizeInfo = *finger++;
            byte bytesPerCount = (sizeInfo >> 3) & 7; // could pack other information in....
            byte bytesPerOffset = (sizeInfo & 7);

            //MORE: Duplicates can occur after the last byte of the key - bytesPerOffset is set to 0 if this occurs
            if (bytesPerOffset == 0)
                break;

            const byte * counts = finger + numOptions; // counts follow the data
            unsigned option = 0;
            unsigned countPrev = 0;
            unsigned countNext = 1;
            if (bytesPerCount == 0)
            {
                option = searchIndex - resultPrev;
                countPrev = option;
                countNext = option+1;
            }
            else
            {
                countPrev = 0;
                for (unsigned i=0; i < numOptions; i++)
                {
                    countNext = readBytesEntry16(counts, i, bytesPerCount)+1;
                    if (searchIndex < resultPrev + countNext)
                    {
                        option = i;
                        break;
                    }
                    countPrev = countNext;
                }
            }
            key[offset++] = finger[option];

            resultNext = resultPrev + countNext;
            resultPrev = resultPrev + countPrev;

            const byte * offsets = counts + numOptions * bytesPerCount;
            const byte * next = offsets + (numOptions-1) * bytesPerOffset;
            finger = next;
            if (option > 0)
                finger += readBytesEntry16(offsets, option-1, bytesPerOffset);
            break;
        }
        }
    }

    return offset;
}

int InplaceNodeSearcher::compareValueAtFallback(const char *src, unsigned int index) const
{
    char temp[256] = { 0 };
    getValueAt(index, temp);
    return strcmp(src, temp);
}


//---------------------------------------------------------------------------------------------------------------------

InplaceKeyBuildContext::~InplaceKeyBuildContext()
{
#ifdef TRACE_BUILDING
    DBGLOG("NumDuplicates = %u", numKeyedDuplicates);
#endif

    delete [] nullRow;
}


//---------------------------------------------------------------------------------------------------------------------

CJHInplaceTreeNode::~CJHInplaceTreeNode()
{
    if (ownedPayload)
        free(payload);
}

int CJHInplaceTreeNode::compareValueAt(const char *src, unsigned int index) const
{
    dbgassertex(index < hdr.numKeys);
    return searcher.compareValueAt(src, index);
}

unsigned __int64 CJHInplaceTreeNode::getSequence(unsigned int index) const
{
    if (index >= hdr.numKeys) return 0;
    return firstSequence + index;
}

int CJHInplaceTreeNode::locateGE(const char * search, unsigned minIndex) const
{
    if (hdr.numKeys == 0) return 0;

#ifdef TIME_NODE_SEARCH
    CCycleTimer timer;
#endif

    unsigned int match = searcher.findGE(keyCompareLen, (const byte *)search);
    if (match < minIndex)
        match = minIndex;

#ifdef TIME_NODE_SEARCH
    unsigned __int64 elapsed = timer.elapsedCycles();
    if (isBranch())
        branchSearchCycles += elapsed;
    else
        leafSearchCycles += elapsed;
#endif

    return match;
}

// Only used for counts, so performance is less critical - implement fully once everything else is implemented
int CJHInplaceTreeNode::locateGT(const char * search, unsigned minIndex) const
{
    unsigned int a = minIndex;
    int b = getNumKeys();
    // Locate first record greater than src
    while ((int)a<b)
    {
        //MORE: Not sure why the index 'i' is subtly different to the GTE version
        //I suspect no good reason, and may mess up cache locality.
        int i = a+(b+1-a)/2;
        int rc = compareValueAt(search, i-1);
        if (rc>=0)
            a = i;
        else
            b = i-1;
    }
    return a;
}


void CJHInplaceTreeNode::load(CKeyHdr *_keyHdr, const void *rawData, offset_t _fpos, bool needCopy)
{
    CJHSearchNode::load(_keyHdr, rawData, _fpos, needCopy);
    keyCompareLen = keyHdr->getNodeKeyLength();
    keyLen = _keyHdr->getMaxKeyLength();
    if (isBranch())
        keyLen = keyHdr->getNodeKeyLength();

    const byte * nullRow = nullptr; //MORE: This should be implemented
    unsigned numKeys = hdr.numKeys;
    if (numKeys)
    {
        size32_t len = hdr.keyBytes;
        const size32_t padding = 8 - 1; // Ensure that unsigned8 values can be read "legally"
        const byte * data = ((const byte *)rawData) + sizeof(hdr);
        keyBuf = (char *) allocMem(len + padding);
        memcpy(keyBuf, data, len);
        memset(keyBuf+len, 0, padding);
        expandedSize = len;

        //Filepositions are stored as a packed base and an (optional) list of scaled compressed deltas
        data = (const byte *)keyBuf;
        byte sizeMask = *data++;
        if (sizeMask & NSFscaleFilepos)
            scaleFposByNodeSize = true;
        bytesPerPosition = (sizeMask & 0xF);
        firstSequence = readPacked64(data);
        if (isLeaf())
            minPosition = readPacked64(data);
        else
            minPosition = firstSequence;

        positionData = data;

        if (bytesPerPosition != 0)
            data += (bytesPerPosition * (numKeys -1));

        if (isLeaf())
        {
            //Extra position serialized for leaves..
            data += bytesPerPosition;

            if (keyHdr->isVariable())
            {
                payloadOffsets.ensureCapacity(numKeys);
                unsigned offset = 0;
                const unsigned bytesPerPayloadLength = 2;
                for (unsigned i=0; i < numKeys; i++)
                {
                    offset += readBytesEntry32(data, i, bytesPerPayloadLength);
                    payloadOffsets.append(offset);
                }
                data += numKeys * bytesPerPayloadLength;
            }

            if (keyLen != keyCompareLen)
            {
                if (sizeMask & NSFuncompressedPayload)
                {
                    size32_t uncompressedLen = readPacked32(data);
                    payload = (byte *)data;
                    data += uncompressedLen;
                }
                else
                {
                    //Currently the uncompressed data is still in memory (if it was moved to the end it could be thrown away)
                    //However that provides the scope to dynamically throw away the decompressed data
                    size32_t compressedLen = readPacked32(data);
                    if (keyHdr->isRowCompressed())
                    {
                        payload = (byte *)data;
                        rowexp.setown(createRandRDiffExpander());
                        rowexp->init(data, false);
                    }
                    else
                    {
                        Owned<IExpander> exp = createLZWExpander(true);
                        int len=exp->init(data);
                        if (len!=0)
                        {
                            payload = (byte *)malloc(len);
                            exp->expand(payload);
                            ownedPayload = true;
                        }
                    }
                    data += compressedLen;
                    expandedSize += len;
                }
            }
        }

        searcher.init(numKeys, data, keyCompareLen, nullRow);
    }
}

bool CJHInplaceTreeNode::getKeyAt(unsigned int index, char *dst) const
{
    if (index >= hdr.numKeys) return false;
    if (dst)
        searcher.getValueAt(index, dst);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

offset_t CJHInplaceBranchNode::getFPosAt(unsigned int index) const
{
    if (index >= hdr.numKeys) return 0;

    offset_t delta = 0;
    if ((bytesPerPosition > 0) && (index != 0))
        delta = readBytesEntry64(positionData, index-1, bytesPerPosition);
    else
        delta = index;

    if (scaleFposByNodeSize)
        delta *= getNodeDiskSize();

    return minPosition + delta;
}


bool CJHInplaceBranchNode::fetchPayload(unsigned int num, char *dest) const
{
    throwUnexpected();
}

size32_t CJHInplaceBranchNode::getSizeAt(unsigned int index) const
{
    throwUnexpected();
}

//---------------------------------------------------------------------------------------------------------------------


offset_t CJHInplaceLeafNode::getFPosAt(unsigned int index) const
{
    //Required for the TLK!
    offset_t filePosition = minPosition;
    if (bytesPerPosition > 0)
        filePosition += readBytesEntry64(positionData, index, bytesPerPosition);
    return filePosition;
}

bool CJHInplaceLeafNode::fetchPayload(unsigned int index, char *dst) const
{
    if (index >= hdr.numKeys) return false;
    if (dst)
    {
        unsigned len = keyCompareLen;
        if (rowexp)
        {
            rowexp->expandRow(dst+len,index,0,keyLen-keyCompareLen);
            len = keyLen;
        }
        else
        {
            if (payloadOffsets.ordinality())
            {
                size32_t offset = 0;
                if (index)
                    offset = payloadOffsets.item(index-1);
                size32_t endOffset = payloadOffsets.item(index);
                size32_t copyLen = endOffset - offset;

                if (copyLen)
                {
                    memcpy(dst + len, payload + offset, copyLen);
                    len += copyLen;
                }
            }
            else
            {
                //Fixed size payload...
                unsigned payloadSize = keyLen - keyCompareLen;
                if (likely(payloadSize))
                {
                    memcpy(dst + len, payload + index * payloadSize, payloadSize);
                    len += payloadSize;
                }
            }
        }
        if (keyHdr->hasSpecialFileposition())
        {
            offset_t filePosition = minPosition;
            if (bytesPerPosition > 0)
                filePosition += readBytesEntry64(positionData, index, bytesPerPosition);
            _cpyrev8(dst+len, &filePosition);
        }
    }
    return true;
}


size32_t CJHInplaceLeafNode::getSizeAt(unsigned int index) const
{
    unsigned rowLen = keyLen;
    if (keyHdr->isVariable())
    {
        size32_t offset = 0;
        if (index)
            offset = payloadOffsets.item(index-1);
        size32_t endOffset = payloadOffsets.item(index);
        rowLen = keyCompareLen + (endOffset - offset);
    }

    if (keyHdr->hasSpecialFileposition())
        return rowLen + sizeof(offset_t);
    else
        return rowLen;
}

//=========================================================================================================

CInplaceWriteNode::CInplaceWriteNode(offset_t _fpos, CKeyHdr *_keyHdr, bool isLeafNode)
: CWriteNode(_fpos, _keyHdr, isLeafNode)
{
    hdr.compressionType = InplaceCompression;
    keyCompareLen = keyHdr->getNodeKeyLength();
    lastKeyValue.allocate(keyCompareLen);
    lastSequence = 0;
}

void CInplaceWriteNode::saveLastKey(const void *data, size32_t size, unsigned __int64 sequence)
{
    memcpy(lastKeyValue.bufferBase(), data, keyCompareLen);
    lastSequence = sequence;
}


//---------------------------------------------------------------------------------------------------------

CInplaceBranchWriteNode::CInplaceBranchWriteNode(offset_t _fpos, CKeyHdr *_keyHdr, InplaceKeyBuildContext & _ctx)
: CInplaceWriteNode(_fpos, _keyHdr, false), ctx(_ctx), builder(keyCompareLen, _ctx.nullRow, false)
{
    nodeSize = _keyHdr->getNodeSize();
}

bool CInplaceBranchWriteNode::add(offset_t pos, const void * _data, size32_t size, unsigned __int64 sequence)
{
    if (0xffff == hdr.numKeys)
        return false;

    const byte * data = (const byte *)_data;
    unsigned oldSize = getDataSize();
    builder.add(size, data);

    if (positions.ordinality())
        assertex(positions.tos() <= pos);
    positions.append(pos);
    bool savedScaleFposByNodeSize = scaleFposByNodeSize;
    if (scaleFposByNodeSize && ((pos % nodeSize) != 0))
        scaleFposByNodeSize = false;

    if (getDataSize() > maxBytes)
    {
        builder.removeLast();
        positions.pop();
        scaleFposByNodeSize = savedScaleFposByNodeSize;

        unsigned nowSize = getDataSize();
        assertex(oldSize == nowSize);
#ifdef TRACE_BUILDING
        DBGLOG("---- branch ----");
        builder.trace();
#endif
        return false;
    }

    saveLastKey(data, size, sequence);
    hdr.numKeys++;
    return true;
}

unsigned CInplaceBranchWriteNode::getDataSize()
{
    if (positions.ordinality() == 0)
        return 0;

    //MORE: Cache this, and calculate the incremental increase in size

    //Compress the filepositions by
    //a) storing them as deltas from the first
    //b) scaling by nodeSize if possible.
    //c) storing in the minimum number of bytes possible.
    //d) Don't store the deltas if all the deltas are 1 (common for indexes for leaf nodes)
    unsigned __int64 firstPosition = positions.item(0);
    unsigned __int64 maxDeltaPosition = positions.tos() - firstPosition;
    if (scaleFposByNodeSize)
        maxDeltaPosition /= nodeSize;
    unsigned bytesPerPosition = 0;
    if (maxDeltaPosition + 1 != positions.ordinality())
        bytesPerPosition = bytesRequired(maxDeltaPosition);

    unsigned posSize = 1 + sizePacked(firstPosition) + bytesPerPosition * (positions.ordinality() - 1);
    return posSize + builder.getSize();
}

void CInplaceBranchWriteNode::write(IFileIOStream *out, CRC32 *crc)
{
    hdr.keyBytes = getDataSize();

    MemoryBuffer data;
    data.setBuffer(maxBytes, keyPtr, false);
    data.setWritePos(0);

    if (positions.ordinality())
    {
        //Pack these by scaling and reducing the number of bytes
        unsigned __int64 firstPosition = positions.item(0);
        unsigned __int64 maxPosition = positions.tos() - firstPosition;
        if (scaleFposByNodeSize)
            maxPosition /= nodeSize;

        unsigned bytesPerPosition = 0;
        if (maxPosition + 1 != positions.ordinality())
            bytesPerPosition = bytesRequired(maxPosition);

        byte sizeMask = (byte)bytesPerPosition | (scaleFposByNodeSize ? NSFscaleFilepos : 0);
        data.append(sizeMask);
        serializePacked(data, firstPosition);

        if (bytesPerPosition != 0)
        {
            for (unsigned i=1; i < positions.ordinality(); i++)
            {
                unsigned __int64 delta = positions.item(i) - firstPosition;
                if (scaleFposByNodeSize)
                    delta /= nodeSize;
                serializeBytes(data, delta, bytesPerPosition);
            }
        }

        builder.serialize(data);
        ctx.numKeyedDuplicates += builder.numDuplicates;

        assertex(data.length() == getDataSize());
    }

    CWriteNode::write(out, crc);
}


//=========================================================================================================

CInplaceLeafWriteNode::CInplaceLeafWriteNode(offset_t _fpos, CKeyHdr *_keyHdr, InplaceKeyBuildContext & _ctx)
: CInplaceWriteNode(_fpos, _keyHdr, true), ctx(_ctx), builder(keyCompareLen, _ctx.nullRow, false)
{
    nodeSize = _keyHdr->getNodeSize();
    keyLen = keyHdr->getMaxKeyLength();
    isVariable = keyHdr->isVariable();
    rowCompression = (keyHdr->getKeyType()&HTREE_QUICK_COMPRESSED_KEY)==HTREE_QUICK_COMPRESSED_KEY;

    uncompressed.clear();
    compressed.allocate(nodeSize);
}

bool CInplaceLeafWriteNode::add(offset_t pos, const void * _data, size32_t size, unsigned __int64 sequence)
{
    if (0xffff == hdr.numKeys)
        return false;

    if (0 == hdr.numKeys)
    {
        firstSequence = sequence;
        if (keyLen != keyCompareLen)
            lzwcomp.open(compressed.mem(), nodeSize, isVariable, rowCompression);
    }

    __uint64 savedMinPosition = minPosition;
    __uint64 savedMaxPosition = maxPosition;
    const byte * data = (const byte *)_data;
    unsigned oldSize = getDataSize();
    builder.add(keyCompareLen, data);

    if (positions.ordinality())
    {
        if (pos < minPosition)
            minPosition = pos;
        if (pos > maxPosition)
            maxPosition = pos;
    }
    else
    {
        minPosition = pos;
        maxPosition = pos;
    }
    positions.append(pos);

    const char * extraData = (const char *)(data + keyCompareLen);
    size32_t extraSize = size - keyCompareLen;

    //Save the uncompressed version in case it is smaller than the compressed version...
    //but only up to a certain size - to prevent very large compression ratios creating giant buffers.
    const size32_t uncompressedLimit = 0x2000;
    const size32_t prevUncompressedLen = uncompressed.length();
    if (uncompressed.length() < uncompressedLimit)
        uncompressed.append(extraSize, extraData);
    if (isVariable)
        payloadLengths.append(extraSize);

    size32_t required = getDataSize();
    //The compression algorithm may append some extra data when the block is complete
    //so ensure that that safety margin is subtracted from the space available
    size32_t safetyMargin = (keyLen != keyCompareLen) ? 16 : 0;

    bool hasSpace = (required <= maxBytes);
    if (hasSpace && (keyLen != keyCompareLen))
    {
        size32_t oldLzwSize = lzwcomp.buflen();
        size32_t lzwSpace = maxBytes - (required - oldLzwSize);
        if (!lzwcomp.adjustLimit(lzwSpace) || !lzwcomp.write(extraData, extraSize))
        {
            //Attempting to write data may have flushed some bytes into the lzw output buffer.
            oldSize += (lzwcomp.buflen() - oldLzwSize);
            hasSpace = false;
        }
    }

    if (!hasSpace)
    {
        //Reverse all the operations that have just been performed
        if (isVariable)
            payloadLengths.pop();
        uncompressed.setLength(prevUncompressedLen);

        builder.removeLast();
        positions.pop();
        minPosition = savedMinPosition;
        maxPosition = savedMaxPosition;
        unsigned nowSize = getDataSize();
        assertex(oldSize == nowSize);
#ifdef TRACE_BUILDING
        DBGLOG("---- leaf ----");
        builder.trace();
#endif
        return false;
    }

    saveLastKey(data, size, sequence);
    hdr.numKeys++;
    return true;
}

unsigned CInplaceLeafWriteNode::getDataSize()
{
    if (positions.ordinality() == 0)
        return 0;

    //MORE: Cache this, and calculate the incremental increase in size

    //Compress the filepositions by
    //a) storing them as deltas from the first
    //b) scaling by nodeSize if possible.
    //c) storing in the minimum number of bytes possible.
    unsigned bytesPerPosition = 0;
    if (minPosition != maxPosition)
        bytesPerPosition = bytesRequired(maxPosition-minPosition);

    unsigned posSize = 1 + sizePacked(firstSequence) + sizePacked(minPosition) + bytesPerPosition * positions.ordinality();
    unsigned offsetSize = payloadLengths.ordinality() * 2;  // MORE: Could probably compress
    unsigned payloadSize = 0;
    if (keyLen != keyCompareLen)
    {
        size32_t payloadLen = lzwcomp.buflen();
        payloadSize = sizePacked(payloadLen) + payloadLen;
    }
    return posSize + offsetSize + payloadSize + builder.getSize();
}

void CInplaceLeafWriteNode::write(IFileIOStream *out, CRC32 *crc)
{
    if (keyLen != keyCompareLen)
        lzwcomp.close();

    hdr.keyBytes = getDataSize();

    MemoryBuffer data;
    data.setBuffer(maxBytes, keyPtr, false);
    data.setWritePos(0);

    if (positions.ordinality())
    {
        //Pack these by scaling and reducing the number of bytes
        unsigned bytesPerPosition = 0;
        if (minPosition != maxPosition)
            bytesPerPosition = bytesRequired(maxPosition-minPosition);

        bool useUncompressed = false; // Implement in HPCC-19115
        byte sizeMask = (byte)bytesPerPosition | (useUncompressed ? NSFuncompressedPayload : 0);
        data.append(sizeMask);
        serializePacked(data, firstSequence);
        serializePacked(data, minPosition);
        if (bytesPerPosition != 0)
        {
            for (unsigned i=0; i < positions.ordinality(); i++)
            {
                unsigned __int64 delta = positions.item(i) - minPosition;
                serializeBytes(data, delta, bytesPerPosition);
            }
        }

        if (isVariable)
        {
            //MORE: These may well benefit from packing....
            ForEachItemIn(i, payloadLengths)
                data.append((unsigned short)payloadLengths.item(i));
        }

        if (keyLen != keyCompareLen)
        {
            if (useUncompressed)
            {
                if (isVariable)
                    serializePacked(data, uncompressed.length());
                data.append(uncompressed.length(), uncompressed.bufferBase());
            }
            else
            {
                size32_t payloadLen = lzwcomp.buflen();
                serializePacked(data, payloadLen);
                data.append(payloadLen, compressed.get());
            }
        }

        builder.serialize(data);
        ctx.numKeyedDuplicates += builder.numDuplicates;

        assertex(data.length() == getDataSize());
    }

    CWriteNode::write(out, crc);
}


//=========================================================================================================


InplaceIndexCompressor::InplaceIndexCompressor(size32_t keyedSize, IHThorIndexWriteArg * helper, const char * _compressionName)
: compressionName(_compressionName)
{
    IOutputMetaData * format = helper->queryDiskRecordSize();
    if (format)
    {
        //Create a representation of the null row - potentially used for the new compression algorithm
        byte * nullRow = new byte[keyedSize];
        RtlStaticRowBuilder rowBuilder(nullRow, keyedSize);

        auto & meta = format->queryRecordAccessor(true);
        size32_t offset = 0;
        for (unsigned idx = 0; idx < meta.getNumFields() && offset < keyedSize; idx++)
        {
            const RtlFieldInfo *field = meta.queryField(idx);
            offset = field->type->buildNull(rowBuilder, offset, field);
        }
        ctx.nullRow = nullRow;
    }
}

#ifdef _USE_CPPUNIT
#include "unittests.hpp"
#include "eclrtl.hpp"

class TestInplaceNodeSearcher : public InplaceNodeSearcher
{
public:
    TestInplaceNodeSearcher(unsigned _count, const byte * data, size32_t _keyLen, const byte * _nullRow) : InplaceNodeSearcher(_count,  data, _keyLen, _nullRow)
    {
    }

    void doFind(const char * search)
    {
        unsigned match = findGE(strlen(search), (const byte *)search);
        printf("('%s': %u) ", search, match);
    }

    void find(const char * search)
    {
        StringBuffer text;
        doFind(search);
        doFind(text.clear().append(strlen(search)-1, search));
        doFind(text.clear().append(strlen(search)-1, search).append('a'));
        doFind(text.clear().append(strlen(search)-1, search).append('z'));
        doFind(text.clear().append(search).append(' '));
        doFind(text.clear().append(search).append('\t'));
        doFind(text.clear().append(search).append('z'));
        printf("\n");
    }
};


static int normalizeCompare(int x)
{
    return (x < 0) ? -1 : (x > 0) ? 1 : 0;
}
class InplaceIndexTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( InplaceIndexTest  );
        //CPPUNIT_TEST(testBytesFromFirstTiming);
        CPPUNIT_TEST(testSearching);
    CPPUNIT_TEST_SUITE_END();

    void testBytesFromFirstTiming()
    {
        for (unsigned i=0; i <= 0xff; i++)
            assertex(numExtraBytesFromFirst1(i) == numExtraBytesFromFirst2(i));
        unsigned total = 0;
        {
            CCycleTimer timer;
            for (unsigned i=0; i < 0xffffff; i++)
                total += numExtraBytesFromFirst1(i);
            printf("%llu\n", timer.elapsedNs());
        }
        {
            CCycleTimer timer;
            for (unsigned i2=0; i2 < 0xffffff; i2++)
                total += numExtraBytesFromFirst2(i2);
            printf("%llu\n", timer.elapsedNs());
        }
        printf("%u\n", total);
    }

    void testSearching()
    {
        const size32_t keyLen = 8;
        bool optimizeTrailing = false;  // remove trailing bytes that match the null row.
        const byte * nullRow = (const byte *)"                                   ";
        PartialMatchBuilder builder(keyLen, nullRow, optimizeTrailing);

        const char * entries[] = {
            "abel    ",
            "abigail ",
            "absalom ",
            "barry   ",
            "barty   ",
            "bill    ",
            "brian   ",
            "briany  ",
            "charlie ",
            "charlie ",
            "chhhhhhs",
            "georgina",
            "georgina",
            "georginb",
            "jim     ",
            "jimmy   ",
            "john    ",
        };
        unsigned numEntries = _elements_in(entries);
        for (unsigned i=0; i < numEntries; i++)
        {
            const char * entry = entries[i];
            builder.add(strlen(entry), entry);
        }

        MemoryBuffer buffer;
        builder.serialize(buffer);
        DBGLOG("Raw size = %u", keyLen * numEntries);
        DBGLOG("Serialized size = %u", buffer.length());
        builder.trace();

        TestInplaceNodeSearcher searcher(builder.getCount(), buffer.bytes(), keyLen, nullRow);

        for (unsigned i=0; i < numEntries; i++)
        {
            const char * entry = entries[i];
            StringBuffer s;
            find(entry, [&searcher,&s,numEntries](const char * search) {
                unsigned match = searcher.findGE(strlen(search), (const byte *)search);
                s.appendf("('%s': %u) ", search, match);
                if (match > 0  && !(searcher.compareValueAt(search, match-1) >= 0))
                {
                    s.append("<");
                    //assertex(searcher.compareValueAt(search, match-1) >= 0);
                }
                if (match < numEntries && !(searcher.compareValueAt(search, match) <= 0))
                {
                    s.append(">");
                    //assertex(searcher.compareValueAt(search, match) <= 0);
                }
            });
            DBGLOG("%s", s.str());
        }

        for (unsigned i=0; i < numEntries; i++)
        {
            char result[256] = {0};
            const char * entry = entries[i];

            if (!searcher.getValueAt(i, result))
                printf("%u: getValue() failed\n", i);
            else if (!streq(entry, result))
                printf("%u: '%s', '%s'\n", i, entry, result);

            auto callback = [numEntries, &entries, &searcher](const char * search)
            {
                for (unsigned j= 0; j < numEntries; j++)
                {
                    int expected = normalizeCompare(strcmp(search, entries[j]));
                    int actual = normalizeCompare(searcher.compareValueAt(search, j));
                    if (expected != actual)
                        printf("compareValueAt('%s', %u)=%d, expected %d\n", search, j, actual, expected);
                }
            };
            find(entry, callback);
        }

        exit(0);
    }

    void find(const char * search, std::function<void(const char *)> callback)
    {
        callback(search);

        unsigned searchLen = strlen(search);
        unsigned trimLen = rtlTrimStrLen(searchLen, search);
        StringBuffer text;
        text.clear().append(search).setCharAt(trimLen-1, ' '); callback(text);
        text.clear().append(search).setCharAt(trimLen-1, 'a'); callback(text);
        text.clear().append(search).setCharAt(trimLen-1, 'z'); callback(text);
        if (searchLen != trimLen)
        {
            text.clear().append(search).setCharAt(trimLen, '\t'); callback(text);
            text.clear().append(search).setCharAt(trimLen, 'a'); callback(text);
            text.clear().append(search).setCharAt(trimLen, 'z'); callback(text);
        }
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( InplaceIndexTest );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( InplaceIndexTest, "InplaceIndexTest" );

#endif
