#include "segmentdrawer.h"

#include <drawingcollection.h>

namespace lsystem {

void SegmentDrawer::startDraw(const common::ExecResult & execResult, const QSharedPointer<common::AllDrawData> & data)
{
	emit drawDone(ui::Drawing(execResult, qSharedPointerCast<common::ConfigAndMeta>(data)), data);
}

}
