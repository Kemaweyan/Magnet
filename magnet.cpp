#include "magnet.h"
#include <cstdio>

#ifdef WIN32
#include "sha1.h"
#else
#include <gcrypt.h>
#endif

/** =====             TorrentItem class                 ===== */

int TorrentItem::sha1hash(std::string& str)
{
	if (m_error)
		return m_error;

#ifdef WIN32
	SHA1Context sha;
	SHA1Reset(&sha);

	SHA1Input(&sha, (const unsigned char*) m_text.data(), m_text.length());
	SHA1Result(&sha);

	char out[41];
	sprintf(out, "%08x%08x%08x%08x%08x", sha.Message_Digest[0], sha.Message_Digest[1], sha.Message_Digest[2], sha.Message_Digest[3], sha.Message_Digest[4]);
	str = out;
#else
	int hash_len = gcry_md_get_algo_dlen(GCRY_MD_SHA1);
	unsigned char *hash  = new unsigned char[hash_len];
	char *out = new char[hash_len * 2 + 1];
	char *p = out;

	gcry_md_hash_buffer(GCRY_MD_SHA1, hash, m_text.data(), m_text.length());

	for (int i = 0; i < hash_len; i++, p += 2)
		snprintf(p, 3, "%02x", hash[i]);

	str = out;

	delete [] hash;
	delete [] out;
#endif

	return 0;
}

char TorrentItem::getType()
{
	if (m_text[0] >= '0' && m_text[0] <= '9')
		return 'c';

	return m_text[0];
}

/** =====             TorrentInteger class               ===== */

TorrentInteger::TorrentInteger(const char *text, int length) : TorrentItem(text, length)
{
	char *tmpStr = new char[length + 1];
	memcpy(tmpStr, text, length);
	tmpStr[length] = '\0';

	if (!sscanf(tmpStr, "i%llde", &m_value))
		m_error = TE_PARSING_INT;

	delete [] tmpStr;
}

int TorrentInteger::getStructure(std::string& str, std::string& prefix)
{
	if (m_error)
		return m_error;
	
	str += prefix;

	char tmpStr[21];
	sprintf(tmpStr, "%lld", m_value);
	str += tmpStr;

	return 0;
}

/** =====             TorrentString class               ===== */

TorrentString::TorrentString(const char *text, int length) : TorrentItem(text, length)
{
	int i;
	for (i = 0; i < length; ++i)
		if (text[i] == ':')
			break;
	++i;

	if (i < length)
		m_value.assign(text + i, length - i);
	else
		m_error = TE_PARSING_STRING;
}

int TorrentString::getValue(std::string& str)
{
	if (m_error)
		return m_error;
	
	char *tmpStr = new char[m_value.length() + 1];
	memcpy(tmpStr, m_value.data(), m_value.length());
	tmpStr[m_value.length()] = '\0';

	str = tmpStr;

	delete [] tmpStr;

	return 0;
}

int TorrentString::getStructure(std::string& str, std::string& prefix)
{
	if (m_error)
		return m_error;

	str += prefix;

	int i;
	bool is_ascii = true;
	for (i = 0; i < m_value.length(); ++i)
		if (m_value[i] < 32 || m_value[i] > 126) {
			is_ascii = false;
			break;
		}

	if (is_ascii) {
		str.append(m_value.data(), m_value.length());
		return 0;
	}

	for (i = 0; i < m_value.length(); ++i) {
		char tmpStr[3];
		snprintf(tmpStr, 3, "%02x", m_value[i]);

		str += tmpStr;
	}

	str += " (hex)";

	return 0;
}

/** =====             TorrentList class               ===== */

TorrentList::TorrentList(const char *text, int length) : TorrentItem(text, length)
{
	std::vector <CharArray> items;
	if (m_error = parseText(items))
		return;

	for (int i = 0; i < items.size(); ++i) {
		switch (items[i][0]) {
			case 'i':
				m_items.push_back(new TorrentInteger(items[i].data(), items[i].length()));
				break;
			case 'l':
				m_items.push_back(new TorrentList(items[i].data(), items[i].length()));
				break;
			case 'd':
				m_items.push_back(new TorrentHashTable(items[i].data(), items[i].length()));
				break;
			default:
				m_items.push_back(new TorrentString(items[i].data(), items[i].length()));
				break;
		}

		if (m_error = m_items[i]->errorCode())
			break;
	}
}

TorrentList::~TorrentList()
{
	for (int i = 0; i < m_items.size(); ++i)
		delete m_items[i];
}

int TorrentList::parseText(std::vector <CharArray> & items)
{
	int pos = 1, len;
	CharArray currentItem;
	std::string lenStr;

	while (pos < m_text.length() - 1) {
		currentItem.clear();

		if (m_text[pos] == 'i' || m_text[pos] == 'l' || m_text[pos] == 'd') {
			int openTags = 0;
			int lastStrEnd = 0;

			do {
				if (pos >= m_text.length())
					return TE_CANNOT_FIND_CLOSE_TAG;

				currentItem.add(m_text[pos]);

				if (m_text[pos] == ':') {
					int posBack = pos;
					lenStr.clear();

					while (m_text[--posBack] >= '0' && m_text[posBack] <= '9' && posBack >= lastStrEnd) {
						char tmpStr[2];
						tmpStr[0] = m_text[posBack];
						tmpStr[1] = '\0';

						lenStr.insert(0, tmpStr);
					}

					if (!lenStr.length())
						return TE_INVALID_FILE_STRUCTURE;

					if (!sscanf(lenStr.c_str(), "%d", &len))
						return TE_UNABLE_DETERMINE_STRING_LENGTH;

					currentItem.add(m_text.data() + pos + 1, len + 1);

					pos += len + 1;
					lastStrEnd = pos;
				}

				if (pos >= m_text.length())
					return TE_UNEXPECTED_END_OF_FILE;

				if (m_text[pos] == 'i' || m_text[pos] == 'l' || m_text[pos] == 'd')
					++openTags;

				if (m_text[pos] == 'e')
					--openTags;

			} while (m_text[pos++] != 'e' || openTags > 0);

		} else if (m_text[pos] >= '0' && m_text[pos] <= '9') {
			lenStr.clear();

			while (m_text[pos] != ':') {
				if (pos >= m_text.length())
					return TE_UNEXPECTED_END_OF_FILE;

				lenStr += m_text[pos];
				currentItem.add(m_text.data() + pos++, 1);
			}

			currentItem.add(m_text.data() + pos++, 1);

			if (!sscanf(lenStr.c_str(), "%d", &len))
				return TE_UNABLE_DETERMINE_STRING_LENGTH;

			currentItem.add(m_text.data() + pos, len);
			pos += len;
		} else 
			return TE_INVALID_FILE_STRUCTURE;
		
		items.push_back(currentItem);

		if (pos >= m_text.length())
			return TE_UNEXPECTED_END_OF_FILE;
	}

	if (m_text[pos] != 'e')
		return TE_INVALID_FILE_STRUCTURE;

	return TE_NONE;
}

int TorrentList::getStructure(std::string& str, std::string& prefix)
{
	if (m_error)
		return m_error;

	std::string new_prefix(prefix);
	new_prefix += "\t";

	str += prefix + "{\n";

	for (int i = 0; i < m_items.size(); ++i) {
		if (m_items[i]->getStructure(str, new_prefix))
			return m_items[i]->errorCode();
		str += "\n";
	}

	str += prefix + "}";

	return 0;
}

/** =====             TorrentHashTable class               ===== */

TorrentItem* TorrentHashTable::getItem(const char *key)
{
	for (int i = 0; i < m_items.size(); i += 2) {
		std::string tmpStr;
		static_cast<TorrentString*>(m_items[i])->getValue(tmpStr);

		if (!tmpStr.compare(key))
			return m_items[i + 1];
	}

	return NULL;
}

int TorrentHashTable::getKey(int index, std::string& str)
{
	if (m_error)
		return m_error;

	TorrentItem *item = m_items[index * 2];

	if (item->getType() != 'c')
		return m_error = TE_INVALID_FILE_STRUCTURE;

	return static_cast<TorrentString*>(item)->getValue(str);
}

int TorrentHashTable::getStructure(std::string& str, std::string& prefix)
{
	if (m_error)
		return m_error;

	std::string new_prefix(prefix), tmpStr;
	new_prefix += "\t";

	str += prefix + "{\n";

	for (int i = 0; i < m_items.size(); i += 2) {
		TorrentItem *item = m_items[i];

		if (item->getType() != 'c')
			return m_error = TE_INVALID_FILE_STRUCTURE;

		static_cast<TorrentString*>(item)->getValue(tmpStr);

		if (!tmpStr.compare("pieces")) {
			str += new_prefix;
			str += "pieces: [sha1 hashes]\n";
			continue;
		}
		tmpStr.clear();

		if (item->getStructure(str, new_prefix))
			return m_items[i]->errorCode();

		if (m_items[i + 1]->getType() == 'd' || m_items[i + 1]->getType() == 'l') {
			str += ":\n";
			if (m_items[i + 1]->getStructure(str, new_prefix))
				return m_items[i]->errorCode();
		} else {
			str += ": ";
			if (m_items[i + 1]->getStructure(str, tmpStr))
				return m_items[i]->errorCode();
		}

		str += "\n";
	}

	str += prefix + "}";

	return 0;
}

/** =====             TorrentFile class               ===== */

TorrentFile::TorrentFile(const char *fileName) : m_items(NULL), m_error(0)
{
	char *buffer;
	FILE *pFile = fopen(fileName, "rb");
	m_filename = fileName;

	if (!pFile) {
		m_error = TE_FILE_NOT_FOUND;
		return;
	}

	fseek(pFile, 0, SEEK_END);
	long lSize = ftell(pFile);
	rewind(pFile);

	buffer = new char[lSize];
	if (fread(buffer, 1, lSize, pFile) != lSize) {
		m_error = TE_READING_FILE;
	
		delete [] buffer;
		fclose(pFile);

		return;
	}

	m_items = new TorrentHashTable(buffer, lSize);
	
	m_error = m_items->errorCode();

	delete [] buffer;
	fclose(pFile);
}

TorrentFile::~TorrentFile()
{
	if (m_items)
		delete m_items;
}

int TorrentFile::createMagnet(std::string& str, bool keepAnnounces = false)
{
	if (m_error)
		return m_error;

	std::string info_hash, name;

	TorrentItem *item = m_items->getItem("info");
	if (!item || item->sha1hash(info_hash))
		return m_error = TE_INVALID_FILE_STRUCTURE;

	if (getName(name))
		return m_error = TE_INVALID_FILE_STRUCTURE;

	str = "magnet:?xt=urn:btih:";
	str += info_hash;
	str += "&dn=";
	str += name;

	if (keepAnnounces) {
		std::string announce;
		TorrentItem *item = m_items->getItem("announce");

		if (!item || item->getType() != 'c')
			return 0;

		static_cast<TorrentString*>(item)->getValue(announce);

		str += "&tr=";
		str += announce;

		item = m_items->getItem("announce-list");

		if (!item || item->getType() != 'l')
			return 0;

		for (int i = 0; i < static_cast<TorrentList*>(item)->size(); ++i) {
			TorrentItem *itemFromList = static_cast<TorrentList*>(item)->at(i);
			
			if (!itemFromList || itemFromList->getType() != 'l')
				continue;

			itemFromList = static_cast<TorrentList*>(itemFromList)->at(0);

			if (!itemFromList || itemFromList->getType() != 'c')
				continue;

			std::string announceFromList;
			static_cast<TorrentString*>(itemFromList)->getValue(announceFromList);

			if (announce.compare(announceFromList)) {
				str += "&tr=";
				str += announceFromList;	
			}
		}
	}

	return 0;
}

int TorrentFile::getName(std::string& str)
{
	if (m_error)
		return m_error;

	TorrentItem *item = m_items->getItem("info");

	if (!item || item->getType() != 'd')
		return m_error = TE_INVALID_FILE_STRUCTURE;

	item = static_cast<TorrentHashTable*>(item)->getItem("name");

	if (!item || item->getType() != 'c')
		return m_error = TE_INVALID_FILE_STRUCTURE;

	static_cast<TorrentString*>(item)->getValue(str);

	return 0;
}


