#include "segmentdrawer.h"

#include <drawingcollection.h>

namespace lsystem {

void SegmentDrawer::startDraw(const common::ExecResult & execResult, const QSharedPointer<common::AllDrawData> & data)
{
	emit drawDone(QSharedPointer<ui::Drawing>::create(execResult, qSharedPointerCast<common::ConfigAndMeta>(data)), data);
}

}
