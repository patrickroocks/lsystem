#include "segmentdrawer.h"
#include <drawingcollection.h>

namespace lsystem {

void SegmentDrawer::startDraw(const common::LineSegs & segs, const QSharedPointer<common::MetaData> & metaData)
{
	if (segs.isEmpty()) return;
	emit drawDone(ui::Drawing::fromSegments(segs), metaData);
}

}
