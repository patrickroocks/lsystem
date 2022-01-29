#include "segmentdrawer.h"
#include <drawingcollection.h>

namespace lsystem {

void SegmentDrawer::startDraw(const common::ExecResult & execResult, const QSharedPointer<common::MetaData> & metaData)
{
	if (execResult.segments.isEmpty() && (execResult.segmentsLastIter.isEmpty() || !metaData->showLastIter)) {
		// nothing to do
		return;
	}

	emit drawDone(ui::Drawing(execResult, metaData), metaData);
}

}
