#pragma once

#include "Typedefs.h"

template <typename arrayType> class KnownLengthArray
{
	public:
		KnownLengthArray(twoByteUnsigned length)
		{
			data = new arrayType[length];
			count = 0;
		}

		~KnownLengthArray()
		{
			delete data;
			data = nullptr;
		}

		bool Assign(twoByteUnsigned index, arrayType value)
		{
			if ((index > 0) && (index < count))
			{
				data[index] = value;
				count += 1;
				return true;
			}

			else
			{
				return false;
			}
		}

		void AssignLast(arrayType value)
		{
			if (count > 0)
			{
				data[count - 1] = value;
			}

			else
			{
				data[count] = value;
			}
		}

		arrayType GetValue(twoByteUnsigned index)
		{
			if ((index > 0) && (index < count))
			{
				return data[index];
			}

			else if (index < 0)
			{
				return data[0];
			}

			else
			{
				return data[count - 1];
			}
		}

		twoByteUnsigned GetCount()
		{
			return count;
		}

	private:
		// A pointer containing the actual data
		// within the array
		arrayType* data;

		// The number of usable values in the array
		twoByteUnsigned count;
};