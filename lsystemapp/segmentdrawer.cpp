#include "segmentdrawer.h"

#include <drawingcollection.h>

namespace lsystem {

void SegmentDrawer::startDraw(const common::ExecResult & execResult, const QSharedPointer<common::AllDrawData> & data)
{
	if (data->meta.maximize) {
		emit drawFrameDone(QSharedPointer<ui::DrawingFrame>::create(execResult, data), data);
	} else {
		emit drawDone(QSharedPointer<ui::Drawing>::create(execResult, data), data);
	}
}

}
