#ifndef CHECKRESULT_H
#define CHECKRESULT_H

namespace icinga
{

class I2_CIB_API CheckResult : public MessagePart
{
public:
	CheckResult(void);
	CheckResult(const MessagePart& message);

	void SetScheduleStart(time_t ts);
	time_t GetScheduleStart(void) const;

	void SetScheduleEnd(time_t ts);
	time_t GetScheduleEnd(void) const;

	void SetExecutionStart(time_t ts);
	time_t GetExecutionStart(void) const;

	void SetExecutionEnd(time_t ts);
	time_t GetExecutionEnd(void) const;

	void SetState(ServiceState state);
	ServiceState GetState(void) const;

	void SetOutput(string output);
	string GetOutput(void) const;

	void SetPerformanceDataRaw(const string& pd);
	string GetPerformanceDataRaw(void) const;

	void SetPerformanceData(const Dictionary::Ptr& pd);
	Dictionary::Ptr GetPerformanceData(void) const;
};

}

#endif /* CHECKRESULT_H */
