/*
	Author: Marco Costalba (C) 2005-2006

	Copyright: See COPYING file that comes with this distribution

*/
#ifndef EXCEPTIONMANAGER_H
#define EXCEPTIONMANAGER_H

#include <q3valuevector.h>
#include <qmap.h>
#include <qstring.h>
//Added by qt3to4:
#include <Q3ValueList>

// exception manager sugar macro
#define EM_DECLARE(x)    int x
#define EM_INIT(x,y)     ExceptionManager::em()->init(&x, QString(y))
#define EM_REGISTER(x)   ExceptionManager::em()->add(x)
#define EM_REMOVE(x)     ExceptionManager::em()->remove(x)
#define EM_RAISE(x)      ExceptionManager::em()->raise(x)
#define EM_MATCH(x,y,z)  ExceptionManager::em()->isMatch(x,y,QString(z))
#define EM_DESC(x)       ExceptionManager::em()->desc(x)
#define EM_IS_PENDING(x) ExceptionManager::em()->isPending(x)
#define EM_THROW_PENDING ExceptionManager::em()->throwPending()

#define EM_BEFORE_PROCESS_EVENTS int _region = ExceptionManager::em()->saveThrowableSet()

#define EM_AFTER_PROCESS_EVENTS ExceptionManager::em()->restoreThrowableSet(_region); \
                                ExceptionManager::em()->throwPending()

#define EM_PROCESS_EVENTS do { EM_BEFORE_PROCESS_EVENTS; \
                               qApp->processEvents();    \
                               EM_AFTER_PROCESS_EVENTS;  \
                          } while (false)

#define EM_PROCESS_EVENTS_NO_INPUT do { EM_BEFORE_PROCESS_EVENTS;                          \
                                        qApp->processEvents(QEventLoop::ExcludeUserInput); \
                                        EM_AFTER_PROCESS_EVENTS;                           \
                                   } while (false)

class ExceptionManager { // singleton class
protected:
	ExceptionManager();
	ExceptionManager(const ExceptionManager&);
	ExceptionManager& operator=(const ExceptionManager&);

public:
	static ExceptionManager* em() {
		static ExceptionManager private_em;
		return &private_em;
	}
	void init(int* excpId, const QString& desc);
	void add(int excpId);
	void remove(int excpId);
	void raise(int excpId);
	void throwPending();
	int saveThrowableSet();
	void restoreThrowableSet(int regionId);
	bool isMatch(int value, int excpId, const QString& context);
	const QString desc(int excpId);
	bool isPending(int excpId);

private:
	int excpId;
	int regionId;
	int currentRegionId;

	class Exception {
	public:
		Exception() {}
		Exception(int ex) : excpId(ex), isRaised(false) {}
		int excpId;
		bool isRaised;
	};
	typedef Q3ValueList<Exception> ThrowableSet;
	typedef ThrowableSet::iterator SetIt;
	QMap<int, ThrowableSet> throwableSetMap;
	ThrowableSet totalThrowableSet;
	ThrowableSet regionThrowableSet;
	Q3ValueVector<QString> descriptions;

	SetIt findExcp(ThrowableSet& ts, const SetIt& startIt, int excpId);
	void setRaisedFlag(ThrowableSet& ts, int excpId);
};

#endif