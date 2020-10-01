// ======================================================================
//
// StringTable.cpp
// asommers
//
// copyright 2003, sony online entertainment
//
// ======================================================================

#include "FirstSwgSpaceQuestEditor.h"
#include "StringTable.h"

#include "fileInterface/StdioFile.h"
#include "LocalizedStringTableReaderWriter.h"
#include "LocalizedString.h"
#include "UnicodeUtils.h"

// ======================================================================

StringTable::StringTable() :
	m_stringMap()
{
}

// ----------------------------------------------------------------------

StringTable::~StringTable()
{
}

// ----------------------------------------------------------------------

void StringTable::clear()
{
	m_stringMap.clear();
}

// ----------------------------------------------------------------------

int StringTable::getNumberOfStrings() const
{
	return m_stringMap.size();
}

// ----------------------------------------------------------------------

CString const & StringTable::getStringKey(int const index) const
{
	VALIDATE_RANGE_INCLUSIVE_EXCLUSIVE(0, index, getNumberOfStrings());

	StringMap::const_iterator iter = m_stringMap.begin();
	for (int i = 0; i < index; ++i, ++iter)
		;

	return iter->first;
}

// ----------------------------------------------------------------------

CString const & StringTable::getString(CString const & key) const
{
	return const_cast<StringTable &>(*this).m_stringMap[key];
}

// ----------------------------------------------------------------------

bool StringTable::hasStringKey(CString const & key) const
{
	return m_stringMap.find(key) != m_stringMap.end();
}

// ----------------------------------------------------------------------

void StringTable::setString(CString const & key, CString const & value)
{
	m_stringMap[key] = value;
}

// ----------------------------------------------------------------------

void StringTable::removeString(CString const & key)
{
	StringMap::iterator iter = m_stringMap.find(key);
	if (iter != m_stringMap.end())
		m_stringMap.erase(iter);
}

// ----------------------------------------------------------------------

bool StringTable::load(char const * const fileName)
{
	StdioFileFactory fileFactory;
	LocalizedStringTableRW * stringTable = LocalizedStringTableRW::loadRW(fileFactory, std::string(fileName));
	if (stringTable)
	{
		const LocalizedStringTable::Map_t & stringMap = stringTable->getMap();
		LocalizedStringTable::Map_t::const_iterator end = stringMap.end();
		for (LocalizedStringTable::Map_t::const_iterator iter = stringMap.begin(); iter != end; ++iter)
		{	
			LocalizedString const * const value = iter->second;
			unsigned long const stringId = value->getId();
			Unicode::NarrowString const * const key = stringTable->getNameById(stringId);

			setString(key->c_str(), Unicode::wideToNarrow(value->getString()).c_str());
		}

		delete stringTable;

		return true;
	}

	return false;
}

// ----------------------------------------------------------------------

bool StringTable::save(char const * const fileName) const
{
	LocalizedStringTableRW stringTable("temp");

	{
		typedef std::vector<std::pair<std::string, std::string> > StringList2;
		StringList2 stringList;

		{
			StringMap::const_iterator end = m_stringMap.end();
			for (StringMap::const_iterator iter = m_stringMap.begin(); iter != end; ++iter)
				stringList.push_back(std::make_pair(std::string(iter->first), std::string(iter->second)));
		}

		{
			int id = 0;

			StringList2::const_iterator end = stringList.end();
			for (StringList2::const_iterator iter = stringList.begin(); iter != end; ++iter)
			{
				std::string result;
				stringTable.addString(new LocalizedString(++id, 0, Unicode::narrowToWide(iter->second)), iter->first, result);
			}
		}
	}

	StdioFileFactory fileFactory;
	return stringTable.writeRW(fileFactory, std::string(fileName));
}

// ======================================================================
