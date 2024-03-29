#pragma once

#include "valuerestriction.h"

#include <QLineEdit>

class FocusableLineEdit final : public QLineEdit
{
	Q_OBJECT

	Q_PROPERTY(ValueRestriction valueRestriction READ valueRestriction WRITE setValueRestriction)
public:
	Q_ENUM(ValueRestriction);

	explicit FocusableLineEdit(QWidget * parent = Q_NULLPTR);
	ValueRestriction valueRestriction() const { return valueRestriction_; }

public Q_SLOTS:
	void setValueRestriction(const ValueRestriction& valueRestriction) { valueRestriction_ = valueRestriction; }

signals:
	void gotFocus(FocusableLineEdit * self);
	void lostFocus(FocusableLineEdit * self);

protected:
	void focusInEvent(QFocusEvent * event) override;
	void focusOutEvent(QFocusEvent * event) override;

private:
	ValueRestriction valueRestriction_ = ValueRestriction::None;

};
