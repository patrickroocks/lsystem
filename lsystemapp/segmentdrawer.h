#ifndef SEGMENTDRAWER_H
#define SEGMENTDRAWER_H

#include <common.h>
#include <drawingcollection.h>

namespace lsystem {

class SegmentDrawer : public QObject
{
	Q_OBJECT

public slots:
	void startDraw(const common::ExecResult & execResult, const QSharedPointer<lsystem::common::MetaData> & metaData);

signals:
	void drawDone(const ui::Drawing & drawing, const QSharedPointer<common::MetaData> & metaData);
};

}

#endif // SEGMENTDRAWER_H
