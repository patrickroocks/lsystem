#pragma once

#include <QtCore>

namespace util {

/// interface for thread-safe destruction
class Destroyable : public QObject
{
	Q_OBJECT
public:
	Destroyable()
	{
		this->connect(this, &Destroyable::destroySignaled, this, &Destroyable::callDestroy, Qt::BlockingQueuedConnection);
	}

	/// do the thread-critical destroy operation in the overridden function of the using class
	virtual void destroy() {}

signals:
	void destroySignaled();

private slots:
	void callDestroy() { destroy(); }

protected:
	/// call this in the destructor of the using class
	void startDestroy() { emit destroySignaled(); }
};

}
