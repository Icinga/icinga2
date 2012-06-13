#ifndef NAGIOSCHECKTASK_H
#define NAGIOSCHECKTASK_H

namespace icinga
{

class NagiosCheckTask : public CheckTask
{
public:
	NagiosCheckTask(const Service& service);

	virtual CheckResult Execute(void) const;

	static CheckTask::Ptr CreateTask(const Service& service);

private:
	string m_Command;
};

}

#endif /* NAGIOSCHECKTASK_H */
