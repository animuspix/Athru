#pragma once

#include "Typedefs.h"

#include "leakChecker.h"

template <typename keyType, typename valueType> class Map
{
	private:
		twoByteUnsigned ValueToIndex(valueType value)
		{
			twoByteUnsigned index = 0;
			for (twoByteUnsigned indexFinder = 0; indexFinder < count; indexFinder += 1)
			{
				if (values[indexFinder] == value)
				{
					index = indexFinder;
					break;
				}
			}

			return index;
		}

		twoByteUnsigned KeyToIndex(keyType key)
		{
			twoByteUnsigned index = 0;
			for (twoByteUnsigned indexFinder = 0; indexFinder < count; indexFinder += 1)
			{
				if (keys[indexFinder] == key)
				{
					index = indexFinder;
					break;
				}
			}

			return index;
		}

	public:
		Map(twoByteUnsigned length)
		{
			keys = new keyType[length];
			values = new valueType[length];
			count = 0;
		}

		~Map()
		{
			// Delete the key-pointer
			delete keys;

			// Clean-up all values
			for (twoByteUnsigned valuePurger = 0; valuePurger < count; valuePurger += 1)
			{
				delete values[valuePurger];
				values[valuePurger] = nullptr;
			}

			// Delete the value-pointer
			delete values;

			// Send the key and value-pointers to [nullptr]
			keys = nullptr;
			values = nullptr;
		}

		bool Assign(twoByteUnsigned index, keyType key, valueType value)
		{
			if ((index > 0) && (index < count))
			{
				keys[index] = key;
				values[index] = value;
				count += 1;
				return true;
			}

			else
			{
				return false;
			}
		}

		void AssignLast(keyType key, valueType value)
		{
			keys[count] = key;
			values[count] = value;
			count += 1;
		}

		void SetKey(keyType key, valueType value)
		{
			keys[ValueToIndex(value)] = key;
		}

		void SetValue(keyType key, valueType value)
		{
			values[KeyToIndex(key)] = value;
		}

		keyType GetKey(valueType value)
		{
			return keys[ValueToIndex(value)];
		}

		valueType GetValue(keyType key)
		{
			return values[KeyToIndex(key)];
		}

		twoByteUnsigned GetCount()
		{
			return count;
		}

	private:
		keyType* keys;
		valueType* values;
		twoByteUnsigned count;
};

