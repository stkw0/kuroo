#include <QApplication>
#include <QThread>
#include <assert.h>

#include "queuelistdelegate.h"
#include "queuelistitem.h"

#define MIN(x,y)	(x) > (y) ? (y) : (x)

void QueueListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	assert(QThread::currentThread() == qApp->thread());
	QueueListItem *item = static_cast<QueueListItem*>(index.internalPointer());
	if (item && index.column() == 5 && item->pretended())
	{
		int progress;
		if (item->duration() > 0)
			progress = MIN((item->steps()/item->duration())*100, 100);
		else
			progress = item->steps();

	 	QStyleOptionProgressBar progressBarOption;
	 	progressBarOption.rect = option.rect;
	 	progressBarOption.minimum = 0;

		if (item->isComplete())
		{
	 		progressBarOption.maximum = 100;
	 		progressBarOption.progress = 100;
	 		progressBarOption.textVisible = true;
		}
		else if (!item->hasStarted())
		{
	 		progressBarOption.maximum = 100;
	 		progressBarOption.progress = 0;
	 		progressBarOption.textVisible = true;
		}
		else
		{
			progressBarOption.maximum = item->duration() > 0 ? 100 : 0;
	 		progressBarOption.progress = progress;
	 		progressBarOption.textVisible = item->duration() > 0 ? true : false;
		}
	 	
		progressBarOption.text = QString::number(progressBarOption.progress) + " %";
		
		QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);

	}
	else
		QStyledItemDelegate::paint(painter, option, index);
}
