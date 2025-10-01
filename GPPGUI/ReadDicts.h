#ifndef READDICTS_h
#define READDICTS_h

#include "NormalDictModel.h"
#include "DictionaryModel.h"
#include <QList>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

class ReadDicts : public QObject
{
	Q_OBJECT

public:
	explicit ReadDicts(QObject* parent = nullptr);
	~ReadDicts() override;

	static QList<DictionaryEntry> readGptDicts(const fs::path& dictPath);
	static QList<DictionaryEntry> readGptDicts(const std::vector<fs::path>& dictPaths);

	static QList<NormalDictEntry> readNormalDicts(const fs::path& dictPath);

	static QString readGptDictsStr(const std::vector<fs::path>& dictPaths);
	static QString readDictsStr(const fs::path& dictPath);


};


#endif