#pragma once

#include <qsigwatcher/structures.h>
#include <qsigwatcher/impl/watcherhandler.h>
#include <util/print.h>
#include <util/destroyable.h>
#include <util/test/utils.h>

namespace sigwatcher::impl {

template<typename DstType, typename SrcType>
DstType * castHelper(SrcType * src)
{
	static_assert(std::is_base_of_v<SrcType, DstType>, "Cast failed: Source type is not a base class of the destination type");
	return dynamic_cast<DstType*>(src);
}

template<typename DstType, typename InnerSrcType>
DstType * castHelper(const QSharedPointer<InnerSrcType> & src)
{
	return castHelper<DstType>(src.data());
}

// base class needed, because pointers on templated classes are difficult
class Watcher : public util::Destroyable
{
	Q_OBJECT

protected:
	friend class SigWatcherHandler;
	virtual WaitingState getWaitingState() const = 0;
	virtual void reset() = 0;
	virtual QString getName() const = 0;

protected:
	static QMutex mutex;
};

template<typename... F>
class SigBaseWatcher : public Watcher
{
public:
	using ArgsTupleOriginal = std::tuple<F...>;
	using ArgsTupleVal = std::tuple<typename std::remove_const_t<typename std::remove_reference_t<F>>...>;
	using ArgsTupleRef = std::tuple<typename std::add_lvalue_reference_t<typename std::remove_const_t<typename std::remove_reference_t<F>>>...>;

	using TestResult = util::test::impl::TestResult;
	using CheckFunction = std::function<bool (F...)>;
	using CheckFunctionExplain = std::function<TestResult (F...)>;
	using PrepareFunction = std::function<void (typename std::add_lvalue_reference_t<typename std::remove_const_t<typename std::remove_reference_t<F>>>...)>;

public:
	SigBaseWatcher(const char * file, int line, const char * varName, const QFlags<WatcherFlags> & flags)
		: watcherVariableName(varName)
		, watcherLocation({file, line})
	{
		setFlags(flags);
	}

	// ----- expections ------

	struct ValueUnderTest
	{
		ValueUnderTest(const ArgsTupleOriginal & values) : values(values) {}
		ArgsTupleVal values;
		QStringList additionalInfos;
	};

	class ExpectionBase
	{
	public:
		virtual ~ExpectionBase() {}
		virtual bool processAndCheck(ValueUnderTest & /*val*/) const { return false; }
		virtual QString toString() const { return QString(); }
	};

	class ExpectValues : public ExpectionBase
	{
	public:
		ExpectValues(ArgsTupleOriginal argsTuple)
			: values(argsTuple)
		{}

		bool processAndCheck(ValueUnderTest & val) const override
		{
			return val.values == values;
		}

		QString toString() const override {	return QString("[values=%1]").arg(util::print(values)); }

	private:
		ArgsTupleVal values;
	};

	template <std::size_t ValuePositionToPick, typename ValueType>
	class ExpectValue : public ExpectionBase
	{
	public:
		ExpectValue(ValueType value) :
			value(value)
		{}

		bool processAndCheck(ValueUnderTest & val) const override
		{
			auto pickedValue = std::get<ValuePositionToPick-1>(val.values);
			if (pickedValue != value) {
				val.additionalInfos << QString("unexpected value in argument %1, got: %2").arg(ValuePositionToPick).arg(util::print(pickedValue));
				return false;
			}
			return true;
		}

		QString toString() const override {	return QString("[value(%1)=%2]").arg(ValuePositionToPick).arg(util::print(value)); }

	private:
		ValueType value;
	};

	template <std::size_t ValuePositionToPick, typename ValueType>
	class ExpectCheckAt : public ExpectionBase
	{
	public:
		ExpectCheckAt(std::function<bool(const ValueType & )> checkFunction) :
			checkFunction(checkFunction)
		{}

		bool processAndCheck(ValueUnderTest & val) const override
		{
			auto pickedValue = std::get<ValuePositionToPick-1>(val.values);
			if (!checkFunction(pickedValue)) {
				val.additionalInfos << QString("compare lambda failed in argument %1, got: %2")
						.arg(ValuePositionToPick).arg(util::print(pickedValue));
				return false;
			}
			return true;
		}

		QString toString() const override
		{
			return QString("[compare lambda on value(%1)]").arg(ValuePositionToPick);
		}

	private:
		std::function<bool(const ValueType & )> checkFunction;
	};

	template <std::size_t ValuePositionToPick, typename ValueType>
	class ExpectCheckAtExplain : public ExpectionBase
	{
	public:
		ExpectCheckAtExplain(std::function<TestResult(const ValueType & )> checkFunction)
			: checkFunction_(checkFunction)
		{}

		bool processAndCheck(ValueUnderTest & val) const override
		{
			auto pickedValue = std::get<ValuePositionToPick-1>(val.values);

			const TestResult res = checkFunction_(pickedValue);

			if (!res.success) {
				val.additionalInfos << QString("compare lambda failed in argument %1, got: %2\n   details: %3")
						.arg(ValuePositionToPick).arg(util::print(pickedValue)).arg(res.message);
			}
			return res.success;
		}

		QString toString() const override
		{
			return QString("[compare lambda on value(%1)]").arg(ValuePositionToPick);
		}

	private:
		std::function<TestResult(const ValueType & )> checkFunction_;
	};

	class ExpectPrepare : public ExpectionBase
	{
	public:
		ExpectPrepare(PrepareFunction prepareFunction)
			: prepareFunction(prepareFunction)
		{}

		bool processAndCheck(ValueUnderTest & val) const override
		{
			std::apply(prepareFunction, val.values);
			val.additionalInfos << QString("prepared to: %1").arg(util::print(val.values));
			return true;
		}

		QString toString() const override { return "[preparator on all args]" ; }

	private:
		PrepareFunction prepareFunction;
	};

	template <std::size_t ValuePositionToPick, typename ValueType>
	class ExpectPrepareAt : public ExpectionBase
	{
	public:
		ExpectPrepareAt(std::function<void(ValueType)> prepareFunction)
			: prepareFunction(prepareFunction)
		{}

		bool processAndCheck(ValueUnderTest & val) const override
		{
			auto & value = std::get<ValuePositionToPick-1>(val.values);
			prepareFunction(value);
			val.additionalInfos << QString("prepared argument %1 to: %2")
					.arg(ValuePositionToPick).arg(util::print(value));
			return true;
		}

		QString toString() const override { return QString("[preparator on arg %1]").arg(ValuePositionToPick) ; }

	private:
		std::function<void(ValueType)> prepareFunction;
	};

	template<std::size_t ElementPositionToCast, typename CastToType>
	class ExpectCastBase : public ExpectionBase
	{
	public:
		ExpectCastBase(const char * castToTypeName)
			: castToTypeName(castToTypeName)
		{}

		CastToType * doCast(ValueUnderTest & val) const
		{
			CastToType * casted = castHelper<CastToType>(std::get<ElementPositionToCast-1>(val.values));

			if (!casted) {
				val.additionalInfos << QString("cast in argument %1 was not successful, expected '%2', but got '%3'")
						.arg(ElementPositionToCast).arg(castToTypeName).arg(util::print(std::get<ElementPositionToCast-1>(val.values)));
			}

			return casted;
		}

	protected:
		const QString castToTypeName;
	};

	template<std::size_t ElementPositionToCast, typename CastToType>
	class ExpectCastVerify : public ExpectCastBase<ElementPositionToCast, CastToType>
	{
	public:
		using ExpectCastBase<ElementPositionToCast, CastToType>::ExpectCastBase;

		bool processAndCheck(ValueUnderTest & val) const override
		{
			return this->doCast(val);
		}

		QString toString() const override
		{
			return QString("[value(%1) has type <%2>]")
					.arg(ElementPositionToCast).arg(this->castToTypeName);
		}

		private:
			const CastToType value;
	};

	template<std::size_t ElementPositionToCast, typename CastToType>
	class ExpectCastValue : public ExpectCastBase<ElementPositionToCast, CastToType>
	{
	public:
		ExpectCastValue(const char * castToTypeName, CastToType value)
			: ExpectCastBase<ElementPositionToCast, CastToType>(castToTypeName)
			, value(value)
		{}

		bool processAndCheck(ValueUnderTest & val) const override
		{
			CastToType * casted = this->doCast(val);
			if (!casted) return false;

			if (*casted != value) {
				val.additionalInfos << QString("unexpected value in argument %1, got: %2").arg(ElementPositionToCast).arg(util::print(casted));
				return false;
			}

			return true;
		}

		QString toString() const override
		{
			return QString("[value(%1)<%2>=%3]")
					.arg(ElementPositionToCast).arg(this->castToTypeName).arg(util::print(value));
		}

		private:
			const CastToType value;
	};

	template<std::size_t ElementPositionToCast, typename CastToType>
	class ExpectCastCheck : public ExpectCastBase<ElementPositionToCast, CastToType>
	{
	public:
		ExpectCastCheck(const char * castToTypeName, std::function<bool(const CastToType & )> checkFunction)
			: ExpectCastBase<ElementPositionToCast, CastToType>(castToTypeName)
			, checkFunction(checkFunction)
		{}

		bool processAndCheck(ValueUnderTest & val) const override
		{
			CastToType * casted = this->doCast(val);
			if (!casted) return false;

			if (!checkFunction(*casted)) {
				val.additionalInfos << QString("compare lambda failed in argument %1, got: %2").arg(ElementPositionToCast).arg(util::print(casted));
				return false;
			}
			return true;
		}

		QString toString() const override
		{
			return QString("[compare lambda on value(%1)<%2>]")
					.arg(ElementPositionToCast).arg(this->castToTypeName);
		}

		private:
			const std::function<bool(const CastToType &)> checkFunction;
	};

	template<std::size_t ElementPositionToCast, typename CastToType>
	class ExpectCastCheckExplain : public ExpectCastBase<ElementPositionToCast, CastToType>
	{
	public:
		ExpectCastCheckExplain(const char * castToTypeName, std::function<TestResult(const CastToType & )> checkFunction)
			: ExpectCastBase<ElementPositionToCast, CastToType>(castToTypeName)
			, checkFunction(checkFunction)
		{}

		bool processAndCheck(ValueUnderTest & val) const override
		{
			CastToType * casted = this->doCast(val);
			if (!casted) return false;

			const TestResult res = checkFunction(*casted);

			if (!res.success) {
				val.additionalInfos << QString("compare lambda in argument %1, got: %2\n   details: %3")
						.arg(ElementPositionToCast).arg(util::print(casted)).arg(res.message);
			}
			return res.success;
		}

		QString toString() const override
		{
			return QString("[compare lambda on value(%1)<%2>]")
					.arg(ElementPositionToCast).arg(this->castToTypeName);
		}

		private:
			const std::function<TestResult(const CastToType &)> checkFunction;
	};


	class ExpectCheck : public ExpectionBase
	{
	public:
		ExpectCheck(CheckFunction compare)
			: checkFunction(compare)
		{}

		bool processAndCheck(ValueUnderTest & val) const override
		{
			return std::apply(checkFunction, val.values);
		}

		QString toString() const override { return QString("[compare lambda]"); }

	private:
		CheckFunction checkFunction;
	};

	class ExpectCheckExplain : public ExpectionBase
	{
	public:
		ExpectCheckExplain(CheckFunctionExplain compare)
			: checkFunction(compare)
		{}

		bool processAndCheck(ValueUnderTest & val) const override
		{
			const TestResult res = std::apply(checkFunction, val.values);

			if (!res.success) {
				val.additionalInfos << QString("compare lambda failed\n   details: %1")
						.arg(res.message);
			}
			return res.success;
		}

		QString toString() const override { return QString("[compare lambda]"); }

	private:
		CheckFunctionExplain checkFunction;
	};

	class ExpectOccurrence : public ExpectionBase
	{
	public:
		ExpectOccurrence() = default;
		bool processAndCheck(ValueUnderTest & /*vals*/) const override { return true; }
	};

	class ExpectionChain
	{
	public:
		ExpectionChain() = default;
		ExpectionChain(const QSharedPointer<ExpectionBase> & expection)
			: expections({expection})
		{}

		ExpectionChain operator&(const ExpectionChain & other)
		{
			ExpectionChain rv;
			rv.expections << expections << other.expections;
			return rv;
		}

		QString toString() const { return util::print(expections); }

		bool processAndCheck(ValueUnderTest & val) const
		{
			for (const QSharedPointer<ExpectionBase> & expection : qAsConst(expections)) {
				if (!expection->processAndCheck(val)) return false;
			}
			return true;
		}

	private:
		QList<QSharedPointer<ExpectionBase>> expections;
	};

	// ---------------------- factories for expect & ignore ----------------------

	ExpectionChain makeExpectValues(F... args)
	{
		return QSharedPointer<ExpectionBase>(new ExpectValues(std::forward_as_tuple(args...)));
	}

	template<std::size_t ValuePositionToCast, typename ValueType>
	ExpectionChain makeExpectValue(const ValueType & valueType)
	{
		return QSharedPointer<ExpectionBase>(new ExpectValue<ValuePositionToCast, ValueType>(valueType));
	}

	template<std::size_t ValuePositionToCast, typename CastToType>
	ExpectionChain makeExpectCastValue(const char * castToTypeName, const CastToType & valueType)
	{
		return QSharedPointer<ExpectionBase>(new ExpectCastValue<ValuePositionToCast, CastToType>(castToTypeName, valueType));
	}

	template<std::size_t ValuePositionToCast, typename CastToType>
	ExpectionChain makeExpectCastCheck(const char * castToTypeName, std::function<bool(const CastToType & )> checkFunction)
	{
		return QSharedPointer<ExpectionBase>(new ExpectCastCheck<ValuePositionToCast, CastToType>(castToTypeName, checkFunction));
	}

	template<std::size_t ValuePositionToCast, typename CastToType>
	ExpectionChain makeExpectCastCheck(const char * castToTypeName, std::function<TestResult(const CastToType & )> checkFunction)
	{
		return QSharedPointer<ExpectionBase>(new ExpectCastCheckExplain<ValuePositionToCast, CastToType>(castToTypeName, checkFunction));
	}

	template<std::size_t ValuePositionToCast, typename CastToType>
	ExpectionChain makeExpectCastVerify(const char * castToTypeName)
	{
		return QSharedPointer<ExpectionBase>(new ExpectCastVerify<ValuePositionToCast, CastToType>(castToTypeName));
	}

	ExpectionChain makeExpectOccurrence()
	{
		return QSharedPointer<ExpectionBase>(new ExpectOccurrence());
	}

	ExpectionChain makeExpectCheck(CheckFunction check)
	{
		return QSharedPointer<ExpectionBase>(new ExpectCheck(check));
	}

	ExpectionChain makeExpectCheck(CheckFunctionExplain check)
	{
		return QSharedPointer<ExpectionBase>(new ExpectCheckExplain(check));
	}

	template<std::size_t ValuePosition>
	ExpectionChain makeExpectCheckAt(std::function<bool(std::tuple_element_t<ValuePosition-1, ArgsTupleOriginal>)> checkFunction)
	{
		return QSharedPointer<ExpectionBase>(new ExpectCheckAt<ValuePosition, std::tuple_element_t<ValuePosition-1, ArgsTupleOriginal>>(checkFunction));
	}

	template<std::size_t ValuePosition>
	ExpectionChain makeExpectCheckAt(std::function<TestResult(std::tuple_element_t<ValuePosition-1, ArgsTupleOriginal>)> checkFunction)
	{
		return QSharedPointer<ExpectionBase>(new ExpectCheckAtExplain<ValuePosition, std::tuple_element_t<ValuePosition-1, ArgsTupleOriginal>>(checkFunction));
	}

	void expect(const char * file, int line, const ExpectionChain & expections, const QFlags<ExpectFlags> & expectFlags = ExpectFlags::None)
	{
		QMutexLocker lock(&mutex);

		expectedEntries << ExpectEntry(prepareChain & expections, {file, line}, expectFlags);
	}

	void ignore(const char * file, int line, const ExpectionChain & expections, const QFlags<IgnoreFlags> & ignoreFlags = IgnoreFlags::None)
	{
		QMutexLocker lock(&mutex);

		ignoredEntries << IgnoreEntry(prepareChain & expections, {file, line}, ignoreFlags);
	}

	// ---------------------- factories for prepare ----------------------

	ExpectionChain makePrepare(PrepareFunction prepare)
	{
		return QSharedPointer<ExpectionBase>(new ExpectPrepare(prepare));
	}

	template<std::size_t ValuePosition>
	ExpectionChain makePrepareAt(std::function<void(std::tuple_element_t<ValuePosition-1, ArgsTupleRef>)> prepare)
	{
		return QSharedPointer<ExpectionBase>(new ExpectPrepareAt<ValuePosition, std::tuple_element_t<ValuePosition-1, ArgsTupleRef>>(prepare));
	}

	ExpectionChain makeNoPrepare() { return {}; }

	void setPrepare(const ExpectionChain & newPrepareChain)
	{
		prepareChain = newPrepareChain;
	}

	// ---------------------- flags ----------------------

	void setFlags(const QFlags<WatcherFlags> & newFlags)
	{
		QMutexLocker lock(&mutex);
		flags = newFlags;
	}

	void setFlag(WatcherFlags flag, bool enabled)
	{
		QMutexLocker lock(&mutex);
		flags.setFlag(flag, enabled);
	}

protected:

	WaitingState getWaitingState() const override
	{
		QMutexLocker lock(&mutex);
		WaitingState rv;

		rv.isWaitingForExpected = !expectedEntries.isEmpty();
		rv.isWaitingForUnexpected = flags.testFlag(WatcherFlags::WaitForUnexpected);

		for (const ExpectEntry & entry : expectedEntries) rv.waitingList << entry.expectOutput;
		rv.location = watcherLocation;

		return rv;
	}

	void reset() override
	{
		QMutexLocker lock(&mutex);

		// remove ignores supposed to be only for one round
		QMutableListIterator it(ignoredEntries);
		while (it.hasNext()) {
			it.next();

			if (!it.value().flags.testFlag(IgnoreFlags::KeepForAllChecks)) it.remove();
		}
	}

	QString getName() const override { return watcherVariableName; }


protected:
	void sigReceived(F...  vals)
	{
		const QString failStr = handleSigReceived(std::forward<F>(vals)...);
		if (!failStr.isNull()) SigWatcherHandler::instance().finallyFail(watcherLocation, failStr);
	}

private:

	struct ExpectInfo {
		ExpectOutput expectOutput;
		QStringList additionalInfo;
	};

	QString doCheck(const ArgsTupleVal & arrived)
	{
		// has do be done before, because preparators will modify shared data
		const QString arrivedStr = util::print(arrived);

		auto expectedFound = [&](const ExpectEntry & entry) {
			if (entry.flags.testFlag(ExpectFlags::IgnoreDuplicated)) ignoredEntries << IgnoreEntry(entry.expections, entry.location, IgnoreFlags::None);
		};

		auto genFailMessage = [&](const QString & infoText, const QList<ExpectInfo> & expectInfos) -> QString {
			const SigWatcherHandler & handler = SigWatcherHandler::instance();
			QString rv = QString("unexpected signal call (%1): %2%3\n\n   ") // arrived is in brackets! (tuple)
					.arg(infoText).arg(watcherVariableName).arg(arrivedStr);

			if (handler.checkLocation.getLine() > 0) {
				rv += QString("%1 here:").arg(handler.checkLocationPassed ? "after having checked" : "while checking")
				   + handler.checkLocation.getQtCreatorStr();
			} else {
				rv += "before first check";
			}

			rv += "\n";
			int count = 0;
			for (const ExpectInfo & expectInfo : expectInfos) {
				rv += "\n   ";
				if (expectInfos.size() == 1) rv += "exclusive expectation:";
				else rv += QString("tried expectation (%1):").arg(++count);
				rv += expectInfo.expectOutput.getStrWithLoc() + "\n";
				if (!expectInfo.additionalInfo.isEmpty()) {
					rv += "   additional infos: \n";
					for (const QString & additionalInfo : expectInfo.additionalInfo) {
						rv += "   " + additionalInfo + "\n";
					}
				}
			}

			rv += "\n   signal:";
			return rv;
		};

		if (!flags.testFlag(WatcherFlags::IgnoreUnexpected) && expectedEntries.isEmpty()) {
			return genFailMessage("nothing expected", {});
		}

		if (flags.testFlag(WatcherFlags::OrderSensitive) && !expectedEntries.isEmpty()) {
			ValueUnderTest val(arrived);
			const ExpectEntry & expectEntry = expectedEntries.first();
			if (!expectEntry.expections.processAndCheck(val)) {
				QList<ExpectInfo> expectInfos;
				expectInfos << ExpectInfo{ expectEntry.expectOutput, val.additionalInfos };
				return genFailMessage("1 expection checked", expectInfos);
			}

			expectedFound(expectEntry);
			expectedEntries.removeFirst();
		} else {
			QList<ExpectInfo> expectInfos;
			QMutableListIterator<ExpectEntry> it(expectedEntries);
			while (it.hasNext()) {
				it.next();
				ValueUnderTest val(arrived);
				ExpectEntry & expectEntry = it.value();
				if (expectEntry.expections.processAndCheck(val)) {
					expectedFound(it.value());
					it.remove();
					return QString();
				} else if (!flags.testFlag(WatcherFlags::IgnoreUnexpected)) {
					expectInfos << ExpectInfo{expectEntry.expectOutput, val.additionalInfos};
				}
			}
			if (!flags.testFlag(WatcherFlags::IgnoreUnexpected)) {
				return genFailMessage(QString("%1 expectation%2 checked").arg(expectedEntries.size())
											.arg(expectedEntries.size() > 1 ? "s" : ""),
									  expectInfos);
			}
		}

		return QString();
	}

	// returns failString on Fail, NULL-String on not-fail
	QString handleSigReceived(F...  vals)
	{
		QMutexLocker lock(&mutex);

		ArgsTupleVal arrived = std::make_tuple(vals...);

		auto ignoreArrived = [&](QList<IgnoreEntry> & ignores, bool alreadyMatched) -> bool {
			QMutableListIterator<IgnoreEntry> it(ignores);
			while (it.hasNext()) {
				it.next();
				const IgnoreEntry & entry = it.value();

				if (alreadyMatched != entry.flags.testFlag(IgnoreFlags::IgnoreIfNotMatched)) continue;

				ValueUnderTest value(arrived);
				if (entry.expections.processAndCheck(value)) {
					if (entry.flags.testFlag(IgnoreFlags::OnlyOnce)) it.remove();

					return true;
				}
			}

			return false;
		};

		// ignore before any check
		if (ignoreArrived(ignoredEntries, false)) return QString();

		QString result = doCheck(arrived);

		// ignore if not matched
		if (!result.isNull() && ignoreArrived(ignoredEntries, true)) return QString();

		return result;
	}

private:
	struct BaseEntry {
		BaseEntry(const ExpectionChain & expections, const Location & place)
			: expections(expections)
			, location(place)
			, expectOutput(location, expections.toString())
		{}

		ExpectionChain expections;
		Location location;
		ExpectOutput expectOutput;
	};

	struct ExpectEntry : public BaseEntry {
		ExpectEntry(const ExpectionChain & expections, const Location & place,
				QFlags<ExpectFlags> flags)
			: BaseEntry(expections, place)
			, flags(flags)
		{}
		QFlags<ExpectFlags> flags;
	};

	struct IgnoreEntry : public BaseEntry {
		IgnoreEntry(const ExpectionChain & expections, const Location & place,
				QFlags<IgnoreFlags> flags)
			: BaseEntry(expections, place)
			, flags(flags)
		{}
		QFlags<IgnoreFlags> flags;
	};

private:
	QList<ExpectEntry> expectedEntries;
	QList<IgnoreEntry> ignoredEntries;
	const QString watcherVariableName;
	ExpectionChain prepareChain;
	Location watcherLocation;
	QFlags<WatcherFlags> flags;

};

}
