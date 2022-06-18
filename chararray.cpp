#include "chararray.h"

/** =====             CharArray class                 ===== */

CharArray::CharArray(const CharArray& arr)
{
	m_length = arr.length();
	m_array = new char[m_length];
	memcpy(m_array, arr.data(), m_length);
}

void CharArray::assign(const char *data, int length)
{
	if (m_array)
		delete [] m_array;

	m_length = length;
	m_array = new char[m_length];
	memcpy(m_array, data, m_length);
}

void CharArray::clear()
{
	if (m_array)
		delete [] m_array;

	m_array = NULL;
	m_length = 0;
}

void CharArray::add(const char *data, int length)
{
	if (!m_array) {
		assign(data, length);
		return;
	}

	char *old_array = m_array;

	m_array = new char[m_length + length];
	memcpy(m_array, old_array, m_length);
	memcpy(m_array + m_length, data, length);

	delete [] old_array;
	m_length += length;
}

std::ostream& operator << (std::ostream& out, CharArray& array)
{
	for (int i = 0; i < array.length(); ++i)
		out << array[i];

	return out;
}

