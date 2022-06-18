#ifndef MAGNET_H
#define MAGNET_H

#include <string>
#include <vector>
#include "chararray.h"

enum {
	TE_NONE = 0,
	TE_FILE_NOT_FOUND,
	TE_READING_FILE,
	TE_PARSING_INT,
	TE_PARSING_STRING,
	TE_INVALID_FILE_STRUCTURE,
	TE_CANNOT_FIND_CLOSE_TAG,
	TE_UNEXPECTED_END_OF_FILE,
	TE_UNABLE_DETERMINE_STRING_LENGTH
};

class TorrentItem
{
	public:
		TorrentItem(const char *text, int length) : m_error(0)
		{
			m_text.assign(text, length);
		}

		virtual ~TorrentItem()
		{
		}

		int errorCode()
		{
			return m_error;
		}

		char getType();
		int sha1hash(std::string& str);
		virtual int getStructure(std::string& str, std::string& prefix) = 0;
	protected:
		CharArray m_text;
		int m_error;
};

class TorrentInteger : public TorrentItem
{
	public:
		TorrentInteger(const char *text, int length);
		int getStructure(std::string& str, std::string& prefix);

		unsigned long long getValue()
		{
			return m_value;
		}
	private:
		unsigned long long m_value;
};

class TorrentString : public TorrentItem
{
	public:
		TorrentString(const char *text, int length);
		int getValue(std::string& str);
		int getStructure(std::string& str, std::string& prefix);
	private:
		CharArray m_value;
};

class TorrentList : public TorrentItem
{
	public:
		TorrentList(const char *text, int length);
		~TorrentList();
		int getStructure(std::string& str, std::string& prefix);

		TorrentItem* operator [] (int index)
		{
			return at(index);
		}

		TorrentItem* at(int index)
		{
			return m_items[index];
		}

		size_t size()
		{
			return m_items.size();
		}
	protected:
		std::vector <TorrentItem*> m_items;

		int parseText(std::vector <CharArray> & items);
};

class TorrentHashTable : public TorrentList
{
	public:
		TorrentHashTable(const char *text, int length) : TorrentList(text, length)
		{
		}

		int getKey(int index, std::string& str);
		TorrentItem* getItem(const char *key);
		int getStructure(std::string& str, std::string& prefix);

		size_t size()
		{
			return m_items.size() / 2;
		}
};

class TorrentFile
{
	public:
		TorrentFile(const char *fileName);
		~TorrentFile();
		int createMagnet(std::string& str, bool keepAnnounces);
		int getName(std::string& str);

		int getStructure(std::string& str)
		{
			std::string prefix;
			return m_items->getStructure(str, prefix);
		}

		int errorCode()
		{
			return m_error;
		}

		std::string& getFileName()
		{
			return m_filename;
		}
	private:
		TorrentHashTable *m_items;
		int m_error;
		std::string m_filename;
};

#endif
