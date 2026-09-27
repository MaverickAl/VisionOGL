#pragma once
#include "WcpOglUtility.h"
#include <map>

class VariableTable;

enum ItemType
{
	E_ITEM_TYPE_NONE		= 0,
	E_ITEM_STRING			= (1<<0),
	E_ITEM_INT				= (1<<1),
	E_ITEM_INT_VECTOR		= (1<<2),
	E_ITEM_FLOAT			= (1<<3),
	E_ITEM_FLOAT_VECTOR		= (1<<4),
	E_ITEM_VARIABLETABLE	= (1<<5),
	E_ITEM_TYPE_ALL			= (1<<6)	// Must be last
};


struct VariableInfo
{
	ItemType type;

	union
	{
		const VariableTable*	vt;
		float					f;
		int						i;
		const intVector*		ia;
		const floatVector*		fa;
	};

	string	s; // Kinda breaks the pretty system
};


class VariableTable
{
	friend class Iterator;
public:
	VariableTable(void);
	~VariableTable(void);

	class Iterator
	{
	public:
		Iterator(const VariableTable& table, int itemTypes);
		void operator++();
		void operator--();
		bool operator!=(int value) { return value!=m_pos; }
		void operator=(int value);
		const VariableInfo& Value() { return m_currentInfo; }
	private:
		void UpdateCurrentInfo();

		int						m_pos;
		int						m_itemTypes;
		VariableInfo			m_currentInfo;
		const VariableTable&	m_table;
	};

	bool Load( istream &is, string name, string templateName = string("") );
	bool LoadFromFile( const string &fileName );
	bool Save( ostream &os, uint indentLevel = 0 );

	bool GetString( const string &name, string *out ) const;
	bool GetInt( const string &name, int *out ) const;
	bool GetFloat( const string &name, float *out ) const;
	// Arrays aren't copied so the result is const
	bool GetIntArray( const string &name, const intVector **out ) const;
	bool GetFloatArray( const string &name, const floatVector **out ) const;
	bool GetVector( const string &name, Vector3& out ) const;
	bool GetVariableTable( const string &name, const VariableTable** out ) const;
	bool GetBool(const string &name, bool &out) const;

	bool GetOptionalString( const string &name, string *out, string default = string("") ) const;
	bool GetOptionalInt( const string &name, int *out,  int default = 0) const;
	bool GetOptionalFloat( const string &name, float *out, float default = 0 ) const;
	// Arrays have no default, will return null if not found
	bool GetOptionalIntArray( const string &name, const intVector **out ) const;
	bool GetOptionalFloatArray( const string &name, const floatVector **out ) const;
	// Is actually just loading a float array and asserting its of length 3
	bool GetOptionalVector( const string &name, Vector3 &out, Vector3 default = Vector3(0.0f, 0.0f, 0.0f) ) const;
	bool GetOptionalVariableTable( const string &name, const VariableTable** out ) const;
	bool GetOptionalBool(const string &name, bool &out, bool default = false) const;

	const string &GetName() const { return m_name; }
	bool InheritsFromTable(const string &tableName) const;

	int Begin() const { return 0; }
	int End() const { return (int)m_indices.size()-1; }

private:
	map<string, string>			m_strings;
	map<string, int>			m_ints;
	map<string, intVector*>		m_intArrays;
	map<string, float>			m_floats;
	map<string, floatVector*>	m_floatArrays;
	map<string, VariableTable*> m_variableTables;
	const VariableTable*		m_pParent;
	string						m_name;
	bool						m_isSaving;

	struct ItemIndex
	{
		ItemType	type;
		string		name;
	};

	// Used to iterate, and save variables in order they were loaded
	vector<ItemIndex*>			m_indices;
};


inline bool VariableTable::GetString( const string &name, string *out ) const
{
	assertIf( GetOptionalString(name, out) )
	{
		return true;
	}
	return false;
}

inline bool VariableTable::GetInt( const string &name, int *out ) const
{
	assertIf( GetOptionalInt(name, out) )
	{
		return true;
	}
	return false;
}

inline bool VariableTable::GetFloat( const string &name, float *out ) const
{
	assertIf( GetOptionalFloat(name, out) )
	{
		return true;
	}
	return false;
}

inline bool VariableTable::GetIntArray( const string &name, const intVector **out ) const
{
	assertIf( GetOptionalIntArray(name, out) )
	{
		return true;
	}
	return false;
}

inline bool VariableTable::GetFloatArray( const string &name, const floatVector **out ) const
{
	assertIf( GetOptionalFloatArray(name, out) )
	{
		return true;
	}
	return false;
}

inline bool VariableTable::GetVector(const string &name, Vector3& out) const
{
	assertIf( GetOptionalVector(name, out) )
	{
		return true;
	}
	return false;
}

inline bool VariableTable::GetVariableTable(const string &name, const VariableTable** out) const
{
	assertIf( GetOptionalVariableTable(name, out) )
	{
		return true;
	}
	return false;
}


inline bool VariableTable::GetBool( const string &name, bool &out ) const
{
	assertIf( GetOptionalBool(name, out) )
	{
		return true;
	}
	return false;
}


inline bool VariableTable::GetOptionalString( const string &name, string *out, string default ) const
{
	string mapVal;
	if( wcpogl::GetMapItemByKey( m_strings, name, &mapVal ) )
	{
		*out = mapVal;
		return true;
	}
	else if( m_pParent )
	{
		return m_pParent->GetOptionalString(name, out, default);
	}
	else
	{
		*out = default;
		return false;
	}
}

inline bool VariableTable::GetOptionalInt( const string &name, int *out, int default ) const
{
	int mapVal;
	if( wcpogl::GetMapItemByKey( m_ints, name, &mapVal ) )
	{
		*out = mapVal;
		return true;
	}
	else if( m_pParent )
	{
		return m_pParent->GetOptionalInt(name, out, default);
	}
	else
	{
		*out = default;
		return false;
	}
}

inline bool VariableTable::GetOptionalFloat( const string &name, float *out, float default ) const
{
	float mapVal;
	if( wcpogl::GetMapItemByKey( m_floats, name, &mapVal ) )
	{
		*out = mapVal;
		return true;
	}
	else if( m_pParent )
	{
		return  m_pParent->GetOptionalFloat(name, out, default);
	}
	else
	{
		*out = default;
		return false;
	}
}


inline bool VariableTable::GetOptionalBool( const string &name, bool &out, bool default ) const
{
	string mapVal;
	if( wcpogl::GetMapItemByKey( m_strings, name, &mapVal ) )
	{
		if( mapVal == "FALSE" )
		{
			out = false;
		}
		else if( mapVal == "TRUE" )
		{
			out = true;
		}
		else
		{
			assert(false);
			return false;
		}
		return true;
	}
	else if( m_pParent )
	{
		return  m_pParent->GetOptionalBool(name, out, default);
	}
	else
	{
		out = default;
		return false;
	}
}

inline bool VariableTable::GetOptionalIntArray( const string &name, const intVector **out ) const
{
	if( wcpogl::GetMapValPointerByKey( m_intArrays, name, out ) )
	{
		return true;
	}
	else if( m_pParent )
	{
		return GetOptionalIntArray( name, out );
	}
	return false;
}

inline bool VariableTable::GetOptionalFloatArray( const string &name, const floatVector **out ) const
{
	if( wcpogl::GetMapValPointerByKey( m_floatArrays, name, out ) )
	{
		return true;
	}
	else if( m_pParent )
	{
		return GetOptionalFloatArray( name, out );
	}
	return false;
}


inline bool VariableTable::GetOptionalVector(const string &name, Vector3 &out, Vector3 default) const
{
	const floatVector* values;
	if( wcpogl::GetMapValPointerByKey( m_floatArrays, name, &values ) )
	{
		if( values->size() == 3 )
		{
			out.SetUp( (*values)[0], (*values)[1], (*values)[2] );
			return true;
		}
	}
	else if( m_pParent )
	{
		return m_pParent->GetOptionalVector(name, out, default);
	}

	out = default;
	return false;
}


inline bool VariableTable::GetOptionalVariableTable(const string &name, const VariableTable** out) const
{
	if( wcpogl::GetMapValPointerByKey( m_variableTables, name, out ) )
	{
		return true;
	}
	else if( m_pParent )
	{
		return GetOptionalVariableTable( name, out );
	}
	return false;
}