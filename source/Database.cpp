#include "vimvsPCH.h"
#include "Database.h"
#include "Logging.h"
#include "Utils.h"

namespace cz
{

Database::Database()
{
}

bool Database::open(const std::string& dbfname)
{
	bool createDb = !isExistingFile(dbfname);
	if (createDb)
		CZ_LOG(logDefault, Log, "Creating database '%s'", dbfname.c_str());
	CZ_CHECK(m_sqdb.open(dbfname.c_str(), createDb));

	SqStmt optimize;
	optimize.init(m_sqdb, "PRAGMA synchronous = OFF");
	CZ_CHECK(optimize.exec());

	//
	// Create necessary tables
	//
	if (createDb)
	{
		SqStmt sql;
		sql.init(m_sqdb, " \
			CREATE TABLE files ( \
				id            INTEGER PRIMARY KEY, \
				fullpath      VARCHAR COLLATE NOCASE, \
				name          VARCHAR COLLATE NOCASE, \
				prjName       VARCHAR, \
				prjFile       VARCHAR, \
				configuration VARCHAR, \
				defines       VARCHAR, \
				includes      VARCHAR \
			); \
		");

		CZ_CHECK(sql.exec());
	}

	CZ_CHECK(m_sqlGetFile.init(m_sqdb, "SELECT * FROM files WHERE id=?"));
	CZ_CHECK(m_sqlGetWithBasename.init(m_sqdb, "SELECT * FROM files WHERE name=?"));
	CZ_CHECK(m_sqlAddFile.init(m_sqdb, "INSERT OR REPLACE INTO files(id,fullpath,name,prjName,prjFile,configuration,defines,includes) VALUES(?,?,?,?,?,?,?,?)"));

	return true;
}

bool Database::getFile(SourceFile& out)
{
	auto fid = out.id;
	if (!fid)
	{
		CZ_CHECK(out.fullpath.size());
		fid = hash(tolower(out.fullpath));
	}

	CZ_CHECK(m_sqlGetFile.bindInt64(1, fid));

	bool found = false;
	m_sqlGetFile.exec<int64_t, const char*, const char*, const char*, const char*, const char*, const char*, const char*>(
		[&](int64_t id, const char* fullpath, const char* name, const char* prjName, const char* prjFile, const char* configuration, const char* defines, const char* includes)
	{
		CZ_ASSERT(fid == id);
		out.id = id;
		out.fullpath = fullpath;
		out.name = name;
		out.prjName = prjName;
		out.prjFile = prjFile;
		out.configuration = configuration;
		out.defines = defines;
		out.includes = includes;
		found = true;
		return true;
	});

	return found;
}

std::vector<SourceFile> Database::getWithBasename(const std::string& filename)
{
	std::vector<SourceFile> res;

	CZ_CHECK(m_sqlGetWithBasename.bindText(1, filename));

	m_sqlGetWithBasename.exec<int64_t, const char*, const char*, const char*, const char*, const char*, const char*, const char*>(
		[&](int64_t id, const char* fullpath, const char* name, const char* prjName, const char* prjFile, const char* configuration, const char* defines, const char* includes)
	{
		SourceFile out;
		out.id = id;
		out.fullpath = fullpath;
		out.name = name;
		out.prjName = prjName;
		out.prjFile = prjFile;
		out.configuration = configuration;
		out.defines = defines;
		out.includes = includes;
		res.push_back(std::move(out));
		return true;
	});

	return res;
}

void Database::addFile(
	const std::string& fullpath,
	const std::string& prjName, const std::string& prjFile,
	const std::string& defines,
	const std::string& includes,
	bool insertOrReplace)
{
	SourceFile src;
	src.id = hash(tolower(fullpath));

	// If this file was added as part of this vimvs session, then nothing to do
	// This avoid all the repeated "Adding file ..." logs for header files
	if (!m_inserted.insert(src.id).second)
		return;

	if (insertOrReplace || !getFile(src))
	{
		auto basename = splitFolderAndFile(fullpath).second;
		CZ_LOG(logDefault, Log, "Adding file %s to database: id=%llu, fullpath=\"%s\", prj=%s|\"%s\", %s|%s",
			basename.c_str(), src.id, fullpath.c_str(), prjName.c_str(), prjFile.c_str(),
			defines.c_str() , includes.c_str());
		CZ_CHECK(m_sqlAddFile.bindInt64(1, src.id));
		CZ_CHECK(m_sqlAddFile.bindText(2, fullpath));
		CZ_CHECK(m_sqlAddFile.bindText(3, basename));
		CZ_CHECK(m_sqlAddFile.bindText(4, prjName));
		CZ_CHECK(m_sqlAddFile.bindText(5, prjFile));
		CZ_CHECK(m_sqlAddFile.bindText(6, "")); // configuration
		CZ_CHECK(m_sqlAddFile.bindText(7, defines));
		CZ_CHECK(m_sqlAddFile.bindText(8, includes));
		CZ_CHECK(m_sqlAddFile.exec());
	}
}

SourceFile Database::getFile(const std::string& filename)
{
	SourceFile s;
	s.fullpath = filename;
	getFile(s);
	return s;
}

}

