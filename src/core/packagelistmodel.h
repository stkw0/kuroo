#ifndef PACKAGE_LIST_MODEL
#define PACKAGE_LIST_MODEL

#include <QAbstractItemModel>
#include <QList>

class PackageListItem;

class PackageListModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	PackageListModel(QObject *parent = 0);
	~PackageListModel();

	void setPackages(QList<PackageListItem*>&);

	// Declaration of pure virtual methods to be implemented.
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex& index) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual void sort(int column, Qt::SortOrder order);
	
	// Some virtual methods to reimplement.
	
	// Used for the header.
	virtual QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	bool hasChildren(const QModelIndex& parent = QModelIndex()) const;

	QList<PackageListItem*> packages() const;

private:
	QList<PackageListItem*> m_packages;

	static bool packageLessThan(PackageListItem *p1, PackageListItem *p2);
	static bool packageMoreThan(PackageListItem *p1, PackageListItem *p2);
};

#endif
