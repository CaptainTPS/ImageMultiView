#include "videoThread.h"

videoThread::videoThread(QObject *parent)
	: QThread(parent){
	m_abort = false;
}


videoThread::~videoThread()
{
	mutex.lock();
	m_abort = true;
	mutex.unlock();

	wait();
}

void videoThread::startplay(){
	mutex.lock();
	m_abort = false;
	mutex.unlock();
	start();
}

void videoThread::stopplay(){
	mutex.lock();
	m_abort = true;
	mutex.unlock();
}

void videoThread::run(){
	while (!m_abort)
	{
		emit play();
		msleep(100);
	}
}