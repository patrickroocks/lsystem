#include "segmentdrawer.h"
#include <drawingcollection.h>

namespace lsystem {

void SegmentDrawer::startDraw(const common::ExecResult & execResult, const QSharedPointer<common::MetaData> & metaData)
{
	emit drawDone(ui::Drawing(execResult, metaData), metaData);
}

}
