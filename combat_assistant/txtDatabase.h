/**
 * @file txtDatabase.h
 * @author Daniele Molinari (daniele.molinari6@gmail.com)
 * @date 27/04/14
 * @version 1.0
 */

#ifndef TXTDATABASE_H
#define TXTDATABASE_H

////include
//std
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <algorithm>
//boost
#include <boost/regex.hpp>

class txtDatabase {
public:
	/**
	 * Some typedef for the used types
	 */
	typedef std::map<std::string, int> LabelMapType;
	typedef std::vector<std::string> RecordType;
// 	class RecordType : public std::vector<std::string> {
// 	public:
// 		////typedef
// 		typedef std::vector<std::string> Base;
// 		RecordType(const LabelMapType& labels) : m_labels(labels) {}
// 		Base::reference operator[](std::string label) {
// 			return Base::operator[](m_labels.at(label));
// 		}
// 	private:
// 		const LabelMapType& m_labels;
// 	};
	typedef std::vector<RecordType> DatabaseType;
	
	/**
	 * @param separator the char used to split the row
	 */
	txtDatabase(std::string separator = "\t") : m_separator(separator)
		{}
		
	/**
	 * @brief get the current separator
	 */
	std::string getSeparator() const {
		return m_separator;
	}
	/**
	 * @brief se the current separator
	 */
	void setSeparator(const std::string& separator) {
		m_separator = separator;
	}
	/**
	 * @brief the number of field in each record
	 */
	int getFiledCount() const {
		return m_field_count;
	}
	/**
	 * @brief the number of record stored
	 */
	int getRecordCount() const {
		return m_db.size();
	}
	
	/**
	 * @brief load a new database
	 * @param fname the file name of the database
	 * 
	 * The database must contain a record for each row.
	 * Inside the same record each field must be separated by @p m_separator
	 * 
	 * Moreover the first row must contain a label for each field separated by @p m_separator
	 */
	bool loadDatabase(const char* fname) {
		using namespace std;
		
		ifstream in(fname);
		//sanity check
		if(!in) {
			throw runtime_error("txtDatabase::loadDatabase ERROR: invalid database " + string(fname) + ".");
		}
		
		//read the labels
		m_labels.clear();
		string line, token;
		if(!getline(in, line)) {
			throw runtime_error("txtDatabase::loadDatabase ERROR: while reading the label row.");
		}
		size_t pos;
		int index = 0;
		while((pos = line.find(m_separator)) != string::npos) {
			token = line.substr(0, pos);
			m_labels[token] = index;
			line.erase(0, pos+m_separator.length());
			++index;
		}
		//take the last value
		m_labels[line] = index++;
		//save the number of field
		m_field_count = index;
		
		//read the actual database
		m_db.clear();
		while(getline(in, line)) {
			RecordType record(m_field_count);
			index = 0;
			while((pos = line.find(m_separator)) != string::npos) {
				token = line.substr(0, pos);
				record[index] = token;
				line.erase(0, pos+m_separator.length());
				++index;
			}
			record[index] = line;
			
			//push the record
			m_db.push_back(record);
		}
	}
	
	/**
	 * @brief access the whole database
	 */
	const DatabaseType& getDatabase() const {
		return m_db;
	}
	
	
	/**
	 * @brief return the current labels map
	 */
	LabelMapType getLabelMap() const {
		return m_labels;
	}
	
	/**
	 * @brief return a subset of the current database matching the filtering
	 * @param[in] label the label of the filed to which the filtering is performed
	 * @param[in] index the index of the filed to which the filtering is performed
	 * @param[in] reg the regex to use
	 * @return a newly created subset of the original database
	 */
	DatabaseType filter(std::string label, boost::regex reg) const {
		return filter(m_labels.at(label), reg);
	}
	DatabaseType filter(int index, boost::regex reg) const {
		DatabaseType res;
		
		std::copy_if(m_db.begin(), m_db.end(), std::back_inserter(res), [&](const RecordType& r) {return boost::regex_search(r[index].begin(), r[index].end(), reg);});
		
		return res;
	}
private:
	///DATA
	//separator
	std::string m_separator;
	//the labels of the database
	LabelMapType m_labels;
	//the database
	DatabaseType m_db;
	//the number of field in each record
	int m_field_count;
};


#endif	//TXTDATABASE_H