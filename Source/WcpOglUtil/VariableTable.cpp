#include "VariableTable.h"
#include "VariableTableMgr.h"
#include "WcpOglPreComp.h"
#include "WcpOglFile.h"
#include <sstream>
#include <string>

VariableTable::VariableTable(void)
{
	m_pParent	= NULL;
	m_isSaving	= false;
}


VariableTable::~VariableTable(void)
{
	wcpogl::DeleteStlMap( m_intArrays );
	wcpogl::DeleteStlMap( m_floatArrays );
	wcpogl::DeleteStlMap( m_variableTables );
	wcpogl::DeleteStlVector( m_indices );
}


VariableTable::Iterator::Iterator(const VariableTable& table, int itemTypes):
m_table(table)
{
	m_itemTypes = itemTypes;

	if( m_itemTypes == E_ITEM_TYPE_ALL )
	{
		// Will set all flags
		m_itemTypes--;
	}

	if (m_table.End() >= m_table.Begin())
	{
		while (!(m_table.m_indices[m_pos]->type & m_itemTypes)
			&& m_pos != m_table.End())
		{
			m_pos++;
		}

		UpdateCurrentInfo();
	}
}

void VariableTable::Iterator::operator++()
{
	while ( !(m_table.m_indices[m_pos]->type & m_itemTypes)
		&& m_pos != m_table.End() )
	{
		m_pos++;
	}

	UpdateCurrentInfo();
}

void VariableTable::Iterator::operator=(int value)
{
	m_pos = value;
	if( value>m_table.End() )
	{
		m_pos = m_table.End();
	}
	
	while ( !(m_table.m_indices[m_pos]->type & m_itemTypes)
		&& m_pos != m_table.End() )
	{
		m_pos++;
	}

	UpdateCurrentInfo();
}

void VariableTable::Iterator::operator--()
{
	if(m_pos>0)
		m_pos--;

	while ( !(m_table.m_indices[m_pos]->type & m_itemTypes)
		&& m_pos > 0 )
	{
		m_pos--;
	}

	UpdateCurrentInfo();
}

void VariableTable::Iterator::UpdateCurrentInfo()
{
	m_currentInfo.type	= m_table.m_indices[m_pos]->type;
	const string &name	= m_table.m_indices[m_pos]->name;

	switch( m_currentInfo.type )
	{
	case E_ITEM_INT:
		m_table.GetInt(name, &m_currentInfo.i); break;
	case E_ITEM_STRING:
		m_table.GetString(name, &m_currentInfo.s); break;
	case E_ITEM_FLOAT:
		m_table.GetFloat(name, &m_currentInfo.f); break;
	case E_ITEM_FLOAT_VECTOR:
		m_table.GetFloatArray(name, &m_currentInfo.fa); break;
	case E_ITEM_INT_VECTOR:
		m_table.GetIntArray(name, &m_currentInfo.ia); break;
	case E_ITEM_VARIABLETABLE:
		m_table.GetVariableTable(name, &m_currentInfo.vt); break;
	default:
		assertMsg(false, "Unrecognized type");
	}
}

bool VariableTable::LoadFromFile( const string &fileName )
{
	assertIf( wcpogl::IsFileReadable(fileName) )
	{
		ifstream fileIn(fileName.c_str());
		return Load( fileIn, fileName );
	}
	else
	{
		return false;
	}
}


bool VariableTable::Load(istream& is, string name, string baseTableName)
{
	string			attribute;
	string			checkString;
	int				intVar = 0;
	float			floatVar = 0.0f;
	floatVector* floatVec = 0;
	intVector* intVec = 0;
	ItemType		type = E_ITEM_TYPE_NONE;
	ItemIndex* index = 0;
	VariableTable* varTable = 0;


	if (!baseTableName.empty())
	{
		m_pParent = VariableTableMgr::Instance().GetTemplate(baseTableName);
	}

	// TODO: Assert no replica keys
	while (is >> attribute)
	{
		type = E_ITEM_TYPE_NONE;

		// Dictionaries within files will be terminated by a }
		if (attribute[0] == '}')
		{
			return true;
		}
		if (attribute[0] == '{')
		{
			continue;
		}

		// Get the line, minus comments
		// TODO: Would be useful in its own function
		if (attribute == "template")
		{
			// It's a template, inform the template manager about it, we ignore it
			VariableTableMgr::Instance().LoadTemplate(is);
			continue;
		}
		getline(is, checkString);

		// Obey old comment style and new
		auto backslashPos = checkString.find("\\\\");
		auto slashPos = checkString.find("//");
		auto commentStart = string::npos;

		if (backslashPos != string::npos && slashPos != string::npos)
		{
			commentStart = min(backslashPos, slashPos);
		}
		else if (backslashPos != string::npos)
		{
			commentStart = backslashPos;
		}
		else if (slashPos != string::npos)
		{
			commentStart = slashPos;
		}

		if (commentStart != string::npos)
		{
			checkString = checkString.substr(0, commentStart);
		}
		
		string errorMsg = "Duplicate string value: ";
		errorMsg += checkString;

		stringstream variableSs;
		variableSs << checkString;

		auto openPos = checkString.find('\"', 0);
		if (openPos != string::npos)
		{
			// We're a string! Find the second quotation mark
			type = E_ITEM_STRING;
			auto closePos = checkString.find('\"', openPos + 1);
			assertMsg(closePos != string::npos, "Missing string closing quotation mark");
			checkString = checkString.substr(openPos + 1, closePos - openPos - 1);
			assertMsg(!wcpogl::StlMapHasKey(m_strings, attribute), errorMsg);
			m_strings[attribute] = checkString;
		}
		else
		{
			uint varCount = wcpogl::VarsInString(checkString);
			// Get the number of decimal points (don't forget your decimal points kids)
			uint floatCount = wcpogl::MatchingsCharsInString(checkString, '.');

			if (floatCount > 0)
			{
				assertMsg(floatCount == varCount, "Parsing error of floats");
				if (varCount == 1)
				{
					type = E_ITEM_FLOAT;
					assertMsg(!wcpogl::StlMapHasKey(m_floats, checkString), errorMsg);
					variableSs >> floatVar;
					m_floats[attribute] = floatVar;
				}
				else
				{
					type = E_ITEM_FLOAT_VECTOR;
					assertMsg(!wcpogl::StlMapHasKey(m_floatArrays, checkString), errorMsg);
					floatVec = new floatVector;
					for (uint i = 0; i < varCount; i++)
					{
						variableSs >> floatVar;
						floatVec->push_back(floatVar);
					}
					m_floatArrays[attribute] = floatVec;
				}
			}
			else if (VariableTableMgr::Instance().HasTemplate(attribute))
			{
				type = E_ITEM_VARIABLETABLE;
				string tableName;
				variableSs >> tableName;
				varTable = new VariableTable;
				varTable->Load(is, tableName, attribute);
				m_variableTables[tableName] = varTable;
			}
			else if (varCount == 1)
			{
				type = E_ITEM_INT;
				assertMsg(!wcpogl::StlMapHasKey(m_ints, checkString), errorMsg);
				variableSs >> intVar;
				m_ints[attribute] = intVar;
			}
			else
			{
				type = E_ITEM_INT_VECTOR;
				assertMsg(!wcpogl::StlMapHasKey(m_intArrays, checkString), errorMsg);
				intVec = new intVector;
				for (uint i = 0; i < varCount; i++)
				{
					variableSs >> intVar;
					intVec->push_back(intVar);
				}
				m_intArrays[attribute] = intVec;
			}
		}

		assertIf(type != E_ITEM_TYPE_NONE)
		{
			index = new ItemIndex;
			index->name = attribute;
			index->type = type;
			m_indices.push_back(index);
		}
	}

	return true;
}

bool VariableTable::Save(ostream &os, uint indentLevel)
{
	vector<ItemIndex*>::const_iterator indexIt;

	for( indexIt = m_indices.begin(); indexIt != m_indices.end(); ++indexIt )
	{
		for( uint i=0; i<indentLevel; i++ )
		{
			os << "\t";
		}

		ItemType	type = (*indexIt)->type;
		string		name = (*indexIt)->name;
		// TODO: Check length of name and align variables
		os << name + "\t";

		switch( type )
		{
		case E_ITEM_VARIABLETABLE:
			assertMsg( wcpogl::StlMapHasKey(m_variableTables, name), "Attempt to save a non-existent variable table" );
			m_variableTables[name]->Save(os, indentLevel+1);
			break;
		case E_ITEM_INT:
			assertMsg( wcpogl::StlMapHasKey(m_variableTables, name), "Attempt to save a non-existent integer" );
			os << m_ints[name];
			break;
		case E_ITEM_FLOAT:
			assertMsg( wcpogl::StlMapHasKey(m_variableTables, name), "Attempt to save a non-existent float" );
			os << m_floats[name];
			break;
		case E_ITEM_FLOAT_VECTOR:
			assertMsg( wcpogl::StlMapHasKey(m_variableTables, name), "Attempt to save a non-existent float vector" );
			for( uint i=0; i<m_floatArrays[name]->size(); i++ )
			{
				os << (*m_floatArrays[name])[i] << " ";
			}
			break;
		case E_ITEM_INT_VECTOR:
			assertMsg( wcpogl::StlMapHasKey(m_variableTables, name), "Attempt to save a non-existent integer vector" );
			for( uint i=0; i<m_intArrays[name]->size(); i++ )
			{
				os << (*m_intArrays[name])[i] << " ";
			}
			break;
		case E_ITEM_STRING:
			assertMsg( wcpogl::StlMapHasKey(m_variableTables, name), "Attempt to save a non-existent string value" );
			os << m_strings[name];
			break;
		default:
			assertMsg(false, "Un-recognised variable type");
		}

		os << "\n";
	}

	return true;
}


bool VariableTable::InheritsFromTable(const string &tableName) const
{
	if( m_name==tableName )
	{
		return true;
	}
	else if( m_pParent != NULL )
	{
		return m_pParent->InheritsFromTable(tableName);
	}
	else
	{
		return false;
	}
}