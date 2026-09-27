#pragma once
#include <windows.h>
#include "WcpOglUtility.h"

class VariableTable;

class VariableTableMgr
{
public:
	VariableTableMgr(void) {}
	~VariableTableMgr(void) {}
	void LoadTemplates(const string& templateListName);
	const VariableTable* GetTemplate(const string &name);
	bool HasTemplate(const string &name);
	bool LoadTemplate(istream &is);

	static VariableTableMgr&	Instance()
	{
		if (m_pInstance == 0)
		{
			m_pInstance = new VariableTableMgr;
		}
		return *m_pInstance;
	}

	void CleanUp() { delete m_pInstance; m_pInstance = 0; }

	static VariableTableMgr*		m_pInstance;
private:
	bool LoadFromFile(const string& fileName);

	std::map<string, VariableTable*>	m_templates;
};

