#pragma once

#include <common.h>
#include <drawingcollection.h>

namespace lsystem {

// wrapper around Drawing constructor, runs in own thread
class SegmentDrawer : public QObject
{
	Q_OBJECT

public slots:
	void startDraw(const common::ExecResult & execResult, const QSharedPointer<lsystem::common::AllDrawData> & data);

signals:
	void drawDone(const QSharedPointer<ui::Drawing> & drawing, const QSharedPointer<common::AllDrawData> & data);
	void drawFrameDone(const QSharedPointer<ui::DrawingFrame> & drawing, const QSharedPointer<common::AllDrawData> & data);
};

} // namespace lsystem
