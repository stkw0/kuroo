#ifndef QUEUE_LIST_DELEGATE_H
#define QUEUE_LIST_DELEGATE_H

#include <QStyledItemDelegate>

class QueueListDelegate : public QStyledItemDelegate
{
public:
	QueueListDelegate(QWidget *parent = 0) : QStyledItemDelegate(parent) {}
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif
