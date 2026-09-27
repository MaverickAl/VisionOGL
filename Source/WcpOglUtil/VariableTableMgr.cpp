#include "VariableTableMgr.h"
#include "VariableTable.h"
#include "WcpOglFile.h"
#include <fstream>

VariableTableMgr* VariableTableMgr::m_pInstance = 0;

void VariableTableMgr::LoadTemplates(const string& templateListName)
{
	assert( wcpogl::IsFileReadable(templateListName) );

	ifstream fileIn(templateListName.c_str());
	string fileName;

	// TODO: Parsing to allow comments
	while( fileIn >> fileName )
	{
		LoadFromFile(fileName);
	}
}


bool VariableTableMgr::LoadFromFile(const string& fileName)
{
	assert( wcpogl::IsFileReadable(fileName) );
	if( !wcpogl::IsFileReadable(fileName) )
	{
		return false;
	}
	ifstream		fileIn(fileName.c_str());
	VariableTable*	table;
	string			attribute;

	while( fileIn >> attribute )
	{
		if(attribute == "template")
		{
			// We have a base template
			LoadTemplate(fileIn);
		}
		else
		{
			// TODO: Assert on template with unknown base
			if( wcpogl::StlMapHasKey(m_templates, attribute) )
			{
				// We have a derivative template
				string name;
				fileIn >> name;
				table = new VariableTable();
				table->Load(fileIn, name, attribute);
				m_templates[name] = table;
			}
		}
	}

	return true;
}

bool VariableTableMgr::LoadTemplate(istream &is)
{
	VariableTable*	table = new VariableTable();
	string			tableName;
	// We have a base template
	is >> tableName;
	table->Load(is, tableName);
	m_templates[tableName] = table;
	return true;
}


const VariableTable* VariableTableMgr::GetTemplate(const string &name)
{
	if( wcpogl::StlMapHasKey(m_templates, const_cast<string&>(name)) )
	{
		return m_templates[name];
	}
	else
	{
		return NULL;
	}
}


bool VariableTableMgr::HasTemplate(const string &name)
{
	return wcpogl::StlMapHasKey(m_templates, const_cast<string&>(name));
}

