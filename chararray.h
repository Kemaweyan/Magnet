#ifndef CHARARRAY_H
#define CHARARRAY_H

#include <cstring>
#include <iostream>

class CharArray
{
	public:
		CharArray() : m_array(NULL), m_length(0)
		{
		}

		CharArray(const CharArray& arr);

		~CharArray()
		{
			clear();
		}

		void assign(const char *data, int length);
		void clear();
		void add(const char *data, int length);

		void add(char data)
		{
			add(&data, 1);
		}

		char operator [] (int index)
		{
			return m_array[index];
		}

		char *data() const
		{
			return m_array;
		}

		int length() const
		{
			return m_length;
		}

		friend std::ostream& operator << (std::ostream& out, CharArray& array);
	private:
		char *m_array;
		int m_length;
};

#endif
