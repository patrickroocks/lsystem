#pragma once

#include <common.h>
#include <drawingcollection.h>

namespace lsystem {

// wrapper around Drawing constructor, runs in own thread
class SegmentDrawer : public QObject
{
	Q_OBJECT

public slots:
	void startDraw(const common::ExecResult & execResult, const QSharedPointer<lsystem::common::MetaData> & metaData);

signals:
	void drawDone(const ui::Drawing & drawing, const QSharedPointer<common::MetaData> & metaData);
};

} // namespace lsystem
