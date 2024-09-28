#pragma once

#include <common.h>

#include <QImage>
#include <QPainter>

#include <optional>

namespace lsystem::ui {

struct DrawingFrameSummary
{
	QPoint topLeft;
	QPoint botRight;
	QPoint offset;
	common::ConfigSet config;
};

struct DrawingSummary final : public DrawingFrameSummary
{
	qint64 drawingNum = 0;
	int listIndex = 0;
	int segmentsCount = 0;
	int animStep = 0;
};

class DrawingFrame
{
public:
	DrawingFrame(const common::ExecResult & execResult, const QSharedPointer<common::AllDrawData> & configAndMeta);
	DrawingFrameSummary toDrawingFrameSummary();

	QPoint offset;
	common::ConfigSet config;

protected:
	common::MetaData metaData;
	const bool paintLastIter;
	QPoint topLeft;
	QPoint botRight;

private:
	void expandSizeToSegments(const common::LineSegs & segs, double thickness);
	void updateRect(double minX, double minY, double maxX, double maxY);
};

class Drawing final : public DrawingFrame
{
public:
	Drawing(const common::ExecResult & execResult, const QSharedPointer<common::AllDrawData> & metaData);
	void drawToImage(QImage & dstImage, bool isMarked, bool isHighlighted);
	QPoint size() const;
	bool withinArea(const QPoint & pos);
	DrawingSummary toDrawingSummary();

	common::AnimatorResult newAnimationStep(int step, bool relativeStep);
	void drawBasicImage();

public:
	struct InternalMeta
	{
		double opacityFactor = 0;
		double thickness = 0;
		bool antiAliasing = false;
		std::optional<lsystem::common::ColorGradient> colorGradient;
	};

	qint64 num = 0;
	qint64 zIndex = 0;
	int listIndex = 0;
	common::LineSegs segments;
	QVector<QColor> actionColors;
	QImage lastIterImage;
	QImage image;
	InternalMeta mainMeta;
	std::optional<InternalMeta> lastIterMeta;
	bool usesOpacity = false;

	struct AnimState final
	{
		bool inProgress = false;
		int curSeg = 0; // index of the last segment which was painted
	} animState;

private:
	void drawSegments(const common::LineSegs & segs, const InternalMeta & meta);
	void drawSegmentRange(const common::LineSegs & segs, int numStart, int numEnd, const InternalMeta & meta);
};

} // namespace lsystem::ui
