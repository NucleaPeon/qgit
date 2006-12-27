/*
	Description: file names persistent cache

	Author: Marco Costalba (C) 2005-2006

	Copyright: See COPYING file that comes with this distribution

*/
#include <qfile.h>
#include <qdir.h>
#include <qapplication.h>
#include <qdatastream.h>
#include "cache.h"

using namespace QGit;

bool Cache::save(const QString& gitDir, const RevFileMap& rf,
                 const StrVect& dirs, const StrVect& files) {

	if (gitDir.isEmpty() || rf.isEmpty())
		return false;

	QString path(gitDir + C_DAT_FILE);
	QString tmpPath(path + BAK_EXT);

	QDir dir;
	if (!dir.exists(gitDir)) {
		dbs("Git directory not found, unable to save cache");
		return false;
	}
	QFile f(tmpPath);
	if (!f.open(QIODevice::WriteOnly))
		return false;

	dbs("Saving cache. Please wait...");

	// compress in memory before write to file
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);

	// Write a header with a "magic number" and a version
	stream << (Q_UINT32)C_MAGIC;
	stream << (Q_INT32)C_VERSION;

	stream << (Q_INT32)dirs.count();
	for (int i = 0; i < dirs.count(); ++i)
		stream << dirs[i];

	stream << (Q_INT32)files.count();
	for (int i = 0; i < files.count(); ++i)
		stream << files[i];

	// to achieve a better compression we save the sha's as
	// one very long string instead of feeding the stream with
	// each one. With this trick we gain a 15% size reduction
	// in the final compressed file. The save/load speed is
	// almost the same.
	uint bufSize = rf.count() * 40 + 1000; // a little bit more space then required
	stream << (Q_INT32)bufSize;

	QString buf;
	buf.reserve(bufSize);
	Q3DictIterator<RevFile> it(rf);
	for ( ; it.current(); ++it) {
		if (   it.currentKey() == ZERO_SHA
		    || it.currentKey() == CUSTOM_SHA
		    || it.currentKey().startsWith("A")) // ALL_MERGE_FILES + rev sha
			continue;

		buf.append(it.currentKey());
	}
	stream << buf;

	for (it.toFirst(); it.current(); ++it) {
		if (   it.currentKey() == ZERO_SHA
		    || it.currentKey() == CUSTOM_SHA
		    || it.currentKey().startsWith("A")) // ALL_MERGE_FILES + rev sha
			continue;

		stream << it.current()->names;
		stream << it.current()->dirs;
		stream << it.current()->status;
		stream << it.current()->mergeParent;
	}
	dbs("Compressing data...");
	f.writeBlock(qCompress(data)); // no need to encode with compressed data
	f.close();

	// rename C_DAT_FILE + BAK_EXT -> C_DAT_FILE
	if (dir.exists(path)) {
		if (!dir.remove(path)) {
			dbs("access denied to " + path);
			dir.remove(tmpPath);
			return false;
		}
	}
	dir.rename(tmpPath, path);
	dbs("Done.");
	return true;
}

bool Cache::load(const QString& gitDir, RevFileMap& rf, StrVect& dirs, StrVect& files) {

	// check for cache file
	QString path(gitDir + C_DAT_FILE);
	QFile f(path);
	if (!f.exists())
		return true; // no cache file is not an error

	if (!f.open(QIODevice::ReadOnly))
		return false;

	QDataStream* stream = new QDataStream(qUncompress(f.readAll()));
	Q_UINT32 magic;
	Q_INT32 version;
	Q_INT32 dirsNum, filesNum, bufSize;
	*stream >> magic;
	*stream >> version;
	if (magic != C_MAGIC || version != C_VERSION) {
		f.close();
		delete stream;
		return false;
	}
	// read the data
	*stream >> dirsNum;
	dirs.resize(dirsNum);
	for (int i = 0; i < dirsNum; ++i)
		*stream >> dirs[i];

	*stream >> filesNum;
	files.resize(filesNum);
	for (int i = 0; i < filesNum; ++i)
		*stream >> files[i];

	*stream >> bufSize;
	QString buf;
	buf.reserve(bufSize);
	*stream >> buf;

	uint bufIdx = 0;
	while (!stream->atEnd()) {

		SCRef sha(buf.mid(bufIdx, 40));
		bufIdx += 40;

		rf.insert(sha, new RevFile());
		RevFile* p = rf[sha];
		*stream >> p->names;
		*stream >> p->dirs;
		*stream >> p->status;
		*stream >> p->mergeParent;
	}
	f.close();
	delete stream;
	return true;
}