#ifndef CHECKRESULT_H
#define CHECKRESULT_H

namespace icinga
{

struct CheckResult
{
public:
	CheckResult(void);
	CheckResult(const Dictionary::Ptr& dictionary);

	Dictionary::Ptr GetDictionary(void) const;

	void SetStartTime(time_t ts);
	time_t GetStartTime(void) const;

	void SetEndTime(time_t ts);
	time_t GetEndTime(void) const;

	void SetState(ServiceState state);
	ServiceState GetState(void) const;

	void SetOutput(string output);
	string GetOutput(void) const;

	void SetPerformanceData(const Dictionary::Ptr& pd);
	Dictionary::Ptr GetPerformanceData(void) const;

private:
	Dictionary::Ptr m_Data;
};

}

#endif /* CHECKRESULT_H */
