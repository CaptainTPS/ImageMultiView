#ifndef _VIDEO_THREAD_H_
#define _VIDEO_THREAD_H_

#include <qthread.h>
#include <QMutex>

class videoThread : public QThread
{
	Q_OBJECT

public:
	videoThread(QObject *parent = 0);
	~videoThread();

	void startplay();
	void stopplay();

signals:
	void play();

protected:
	void run();

private:
	bool m_abort;
	QMutex mutex;
};


#endif